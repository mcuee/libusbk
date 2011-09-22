/*!
#
# Copyright (c) 2011 Travis Robinson <libusbdotnet@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS LEE ROBINSON
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
*/
#include "examples.h"

// Example configuration:
#define EP_READ						0x81

#define MAX_OUTSTANDING_TRANSFERS	8
#define MAX_TRANSFERS_TOTAL			24
#define ISO_PACKETS_PER_TRANSFER	128

typedef struct _MY_ISO_BUFFER_EL
{
	PUCHAR			DataBuffer;
	KOVL_HANDLE		OvlHandle;

	KISO_CONTEXT*	IsoContext;
	KISO_PACKET*	IsoPackets;

	struct _MY_ISO_BUFFER_EL* prev;
	struct _MY_ISO_BUFFER_EL* next;

} MY_ISO_BUFFER_EL, *PMY_ISO_BUFFER_EL;

typedef struct _MY_ISO_XFERS
{
	KOVL_POOL_HANDLE	OvlPool;
	PMY_ISO_BUFFER_EL	BufferList;

	ULONG				DataBufferSize;

	PMY_ISO_BUFFER_EL	Outstanding;
	PMY_ISO_BUFFER_EL	Completed;

	ULONG				SubmittedCount;
	ULONG				CompletedCount;

	ULONG				FrameNumber;

	ULONG LastStartFrame;

} MY_ISO_XFERS, *PMY_ISO_XFERS;

// Globals:
USB_INTERFACE_DESCRIPTOR	gInterfaceDescriptor;
WINUSB_PIPE_INFORMATION		gPipeInfo;
UCHAR						gAltsettingNumber;
MY_ISO_XFERS				gXfers;
DATA_COUNTER_STATS	Dcs;
FILE* gOutFile = NULL;

/*
Reports isochronous packet information.
*/
VOID IsoXferComplete(PMY_ISO_XFERS myXfers, PMY_ISO_BUFFER_EL myBufferEL, ULONG transferLength)
{
	int packetPos;
	mDcs_MarkStop(&Dcs, transferLength);

	if (!myXfers->LastStartFrame)
	{
		fprintf(gOutFile, "#%u: StartFrame=%08Xh TransferLength=%u BPS-average:%.2f\n",
		        myXfers->CompletedCount, myBufferEL->IsoContext->StartFrame, transferLength, Dcs.Bps);
	}
	else
	{
		ULONG lastFrameCount = myBufferEL->IsoContext->StartFrame - myXfers->LastStartFrame;
		fprintf(gOutFile, "#%u: StartFrame=%08Xh LastFrameCount=%u TransferLength=%u BPS-average:%.2f\n",
		        myXfers->CompletedCount, myBufferEL->IsoContext->StartFrame, lastFrameCount, transferLength, Dcs.Bps);
	}

	for (packetPos = 0; packetPos < myBufferEL->IsoContext->NumberOfPackets; packetPos++)
	{
		KISO_PACKET isoPacket = myBufferEL->IsoPackets[packetPos];
		if (isoPacket.Length > 1)
		{
			UCHAR firstPacketByte = myBufferEL->DataBuffer[isoPacket.Offset];
			UCHAR secondPacketByte = myBufferEL->DataBuffer[isoPacket.Offset + 1];
			fprintf(gOutFile, "  #%04u: Length=%u %02Xh %02Xh\n", packetPos, isoPacket.Length, firstPacketByte, secondPacketByte);
		}
		else
		{
			fprintf(gOutFile, "  #%04u: Empty Packet\n", packetPos);
		}
	}

	myXfers->CompletedCount++;
	myXfers->LastStartFrame = myBufferEL->IsoContext->StartFrame;
}

VOID SetNextFrameNumber(PMY_ISO_XFERS myXfers, PMY_ISO_BUFFER_EL myBufferEL)
{
	myBufferEL->IsoContext->StartFrame = myXfers->FrameNumber;
	myXfers->FrameNumber = myXfers->FrameNumber + (8);
}

DWORD __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE usbHandle = NULL;
	DWORD errorCode = ERROR_SUCCESS;
	BM_TEST_TYPE testType;
	UCHAR interfaceIndex;
	BOOL success;

	memset(&gXfers, 0, sizeof(gXfers));

	// Create a new log file.
	gOutFile = fopen("xfer-iso-read.txt", "w");

	/*
	Find the test device. Uses "vid=hhhh pid=hhhh" arguments supplied on the
	command line. (default is: vid=04D8 pid=FA2E)
	*/
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	/*
	Initialize the device. This creates the physical usb handle.
	*/
	if (!UsbK_Init(&usbHandle, deviceInfo))
	{
		errorCode = GetLastError();
		printf("UsbK_Init failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

	/*
	Select interface by pipe id and get descriptors.
	*/
	interfaceIndex = (UCHAR) - 1;
	while(UsbK_SelectInterface(usbHandle, ++interfaceIndex, TRUE))
	{
		memset(&gInterfaceDescriptor, 0, sizeof(gInterfaceDescriptor));
		memset(&gPipeInfo, 0, sizeof(gPipeInfo));
		gAltsettingNumber = (UCHAR) - 1;
		while(UsbK_QueryInterfaceSettings(usbHandle, ++gAltsettingNumber, &gInterfaceDescriptor))
		{
			UCHAR pipeIndex = (UCHAR) - 1;
			while(UsbK_QueryPipe(usbHandle, gAltsettingNumber, ++pipeIndex, &gPipeInfo))
			{
				if (gPipeInfo.PipeId == EP_READ &&
				        gPipeInfo.PipeType == UsbdPipeTypeIsochronous &&
				        gPipeInfo.MaximumPacketSize)
					break;

				memset(&gPipeInfo, 0, sizeof(gPipeInfo));
			}
			if (gPipeInfo.PipeId) break;
			memset(&gInterfaceDescriptor, 0, sizeof(gInterfaceDescriptor));
		}
	}

	if (!gPipeInfo.PipeId)
	{
		printf("Pipe not found.\n");
		goto Done;
	}

	/*
	Set the desired alternate setting.
	*/
	success = UsbK_SetAltInterface(
	              usbHandle,
	              gInterfaceDescriptor.bInterfaceNumber,
	              FALSE,
	              gInterfaceDescriptor.bAlternateSetting);

	if (!success)
	{
		printf("UsbK_SetAltInterface failed.\n");
		goto Done;
	}

	// Configure the benchmark test device to send data.
	testType	= USB_ENDPOINT_DIRECTION_IN(gPipeInfo.PipeId) ? BM_TEST_TYPE_READ : BM_TEST_TYPE_WRITE;
	success		= Bench_Configure(usbHandle, BM_COMMAND_SET_TEST, 0, NULL, &testType);
	if (!success)
	{
		errorCode = GetLastError();
		printf("Bench_Configure failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}
	printf("Device hardware fully prepared..\n");

	/*
	Allocate the iso buffer resources.
	*/
	do
	{
		int pos;
		gXfers.DataBufferSize = ISO_PACKETS_PER_TRANSFER * gPipeInfo.MaximumPacketSize;

		success = OvlK_Init(&gXfers.OvlPool, usbHandle, MAX_OUTSTANDING_TRANSFERS, KOVL_POOL_FLAG_NONE);
		for (pos = 0; pos < MAX_OUTSTANDING_TRANSFERS; pos++)
		{
			PMY_ISO_BUFFER_EL bufferEL = malloc(sizeof(MY_ISO_BUFFER_EL));
			memset(bufferEL, 0, sizeof(*bufferEL));

			bufferEL->DataBuffer = malloc(gXfers.DataBufferSize);

			IsoK_Init(&bufferEL->IsoContext, ISO_PACKETS_PER_TRANSFER, 0);
			IsoK_SetPackets(bufferEL->IsoContext, gPipeInfo.MaximumPacketSize);

			//bufferEL->IsoContext->Flags = KISO_FLAG_NO_START_ASAP;

			bufferEL->IsoPackets = bufferEL->IsoContext->IsoPackets;
			OvlK_Acquire(&bufferEL->OvlHandle, gXfers.OvlPool);

			DL_APPEND(gXfers.BufferList, bufferEL);
			DL_APPEND(gXfers.Completed, bufferEL);
		}
	}
	while(0);

	/*
	Reset the pipe.
	*/
	UsbK_ResetPipe(usbHandle, (UCHAR)gPipeInfo.PipeId);

	/*
	Set a start frame (not used) see KISO_FLAG_NO_START_ASAP.
	*/
	UsbK_GetCurrentFrameNumber(usbHandle, &gXfers.FrameNumber);
	gXfers.FrameNumber += ISO_PACKETS_PER_TRANSFER * 2;
	gXfers.FrameNumber -= gXfers.FrameNumber % ISO_PACKETS_PER_TRANSFER;

	mDcs_Init(&Dcs);

	/*
	Start reading until an error occurs or MAX_TRANSFERS_TOTAL is reached.
	*/
	do
	{
		PMY_ISO_BUFFER_EL nextXfer;
		ULONG transferred;

		while(errorCode == ERROR_SUCCESS && gXfers.Completed && gXfers.SubmittedCount < MAX_TRANSFERS_TOTAL)
		{
			nextXfer = gXfers.Completed;
			DL_DELETE(gXfers.Completed, nextXfer);
			DL_APPEND(gXfers.Outstanding, nextXfer);

			OvlK_ReUse(nextXfer->OvlHandle);

			SetNextFrameNumber(&gXfers, nextXfer);

			success = UsbK_IsoReadPipe(
			              usbHandle,
			              gPipeInfo.PipeId,
			              nextXfer->DataBuffer,
			              gXfers.DataBufferSize,
			              nextXfer->OvlHandle,
			              nextXfer->IsoContext);

			errorCode = GetLastError();
			if (errorCode != ERROR_IO_PENDING)
			{
				printf("UsbK_IsoReadPipe failed. ErrorCode: %08Xh\n",  errorCode);
				goto Done;
			}
			gXfers.SubmittedCount++;
			errorCode = ERROR_SUCCESS;
		}

		nextXfer = gXfers.Outstanding;
		if (!nextXfer)
		{
			printf("Done!\n");
			goto Done;
		}

		success = OvlK_Wait(nextXfer->OvlHandle, 1000, KOVL_WAIT_FLAG_NONE, &transferred);
		if (!success)
		{
			errorCode = GetLastError();
			printf("OvlK_Wait failed. ErrorCode: %08Xh\n",  errorCode);
			goto Done;
		}
		DL_DELETE(gXfers.Outstanding, nextXfer);
		DL_APPEND(gXfers.Completed, nextXfer);

		IsoXferComplete(&gXfers, nextXfer, transferred);

	}
	while(errorCode == ERROR_SUCCESS);

Done:

	/*
	Cancel all transfers left outstanding.
	*/
	while(gXfers.Outstanding)
	{
		PMY_ISO_BUFFER_EL nextBufferEL = gXfers.Outstanding;
		ULONG transferred;

		OvlK_WaitOrCancel(nextBufferEL->OvlHandle, 0, &transferred);
		DL_DELETE(gXfers.Outstanding, nextBufferEL);
	}

	/*
	Free the iso buffer resources.
	*/
	while(gXfers.BufferList)
	{
		PMY_ISO_BUFFER_EL nextBufferEL = gXfers.BufferList;
		DL_DELETE(gXfers.BufferList, nextBufferEL);

		OvlK_Release(nextBufferEL->OvlHandle);
		IsoK_Free(nextBufferEL->IsoContext);
		free(nextBufferEL->DataBuffer);
		free(nextBufferEL);
	}

	// Free the overlapped pool.
	OvlK_Free(gXfers.OvlPool);

	// Close the device handle.
	UsbK_Free(usbHandle);

	// Free the device list.
	LstK_Free(deviceList);

	// Close the log file.
	if (gOutFile)
	{
		fflush(gOutFile);
		fclose(gOutFile);
		gOutFile = NULL;
	}
	return errorCode;
}

