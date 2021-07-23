/*!
#
# Copyright (c) 2012 Travis Robinson <libusbdotnet@gmail.com>
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

#define MAX_OUTSTANDING_TRANSFERS	3
#define MAX_TRANSFERS_TOTAL			64
#define ISO_PACKETS_PER_TRANSFER	(8)

KUSB_DRIVER_API Usb;

// UINT_MAX for first interface found.
// NOTE: use "intf=" on the command line to specify a specific one. EG: intf=1
UINT gIntfIndex = UINT_MAX;

// UINT_MAX for first alt interface found.
// NOTE: use "altf=" on the command line to specify a specific one. EG: altf=2
UINT gAltfSetting = UINT_MAX;

// UINT_MAX for first ep found. (read or write)
// 0x80 for first IN (read)  endpoint found
// 0x00 for first out (write) endpoint found
// NOTE: use "ep=" on the command line to specify a specific one. EG: ep=81
UINT gEpID = UINT_MAX;

typedef struct _MY_ISO_BUFFER_EL
{
	PUCHAR			TransferBuffer;
	KOVL_HANDLE		OvlHandle;

	KISOCH_HANDLE	IsoHandle;
	UINT FrameNumber;

	struct _MY_ISO_BUFFER_EL* prev;
	struct _MY_ISO_BUFFER_EL* next;
} MY_ISO_BUFFER_EL, * PMY_ISO_BUFFER_EL;

typedef struct _MY_ISO_XFERS
{
	KOVL_POOL_HANDLE	OvlPool;

	ULONG				TransferBufferSize;

	PMY_ISO_BUFFER_EL	Outstanding;
	PMY_ISO_BUFFER_EL	Completed;

	ULONG				SubmittedCount;
	ULONG				CompletedCount;

	UINT				FrameNumber;

	ULONG LastStartFrame;

} MY_ISO_XFERS, * PMY_ISO_XFERS;

// Globals:
USB_INTERFACE_DESCRIPTOR	gInterfaceDescriptor;
WINUSB_PIPE_INFORMATION_EX	gPipeInfo;
MY_ISO_XFERS				gXfers;
DATA_COUNTER_STATS	Dcs;
FILE* gOutFile = NULL;

int write_output(FILE* const stream, const char* fmt, ...)
{
	int len;
	va_list args;
	va_start(args, fmt);
	if (stream)
	{
		len = vfprintf(stream, fmt, args);
	}
	else
	{
		len = vprintf(fmt, args);
	}
	va_end(args);
	return len;
}

/*
Reports isochronous packet information.
*/
VOID IsoXferComplete(PMY_ISO_XFERS myXfers, PMY_ISO_BUFFER_EL myBufferEL, ULONG transferLength)
{
	int packetPos;
	UINT goodPackets, badPackets;
	char packetCounters[ISO_PACKETS_PER_TRANSFER * 3 + 1];
	goodPackets = 0;
	badPackets = 0;

	if (gPipeInfo.PipeId & 0x80)
	{
		transferLength = 0;
		for (packetPos = 0; packetPos < ISO_PACKETS_PER_TRANSFER; packetPos++)
		{
			UINT offset, length, status;
			IsochK_GetPacket(myBufferEL->IsoHandle, packetPos, &offset, &length, &status);
			sprintf(&packetCounters[packetPos * 3], "%02X ", myBufferEL->TransferBuffer[offset + 1]);
			if (status == 0 && length != 0)
			{
				goodPackets++;
				transferLength += length;
			}
			else
			{
				badPackets++;
			}
		}
	}
	else
	{
		transferLength = gXfers.TransferBufferSize;
	}
	Dcs.TotalBytes = 0;
	mDcs_MarkStop(&Dcs, transferLength);

	if (gPipeInfo.PipeId & 0x80)
	{
		if (myXfers->LastStartFrame != 0)
		{
			write_output(gOutFile,
				"#%u: StartFrame=%08Xh TransferLength=%u GoodPackets=%u, BadPackets=%u, BPS-average:%.2f\n\t%s\n",
				myXfers->CompletedCount, myBufferEL->FrameNumber, transferLength, goodPackets, badPackets, Dcs.Bps, packetCounters);
		}
		else
		{
			write_output(gOutFile,
				"#%u: StartFrame=%08Xh TransferLength=%u GoodPackets=%u, BadPackets=%u, BPS-average:unknown\n\t%s\n",
				myXfers->CompletedCount, myBufferEL->FrameNumber, transferLength, goodPackets, badPackets, packetCounters);
		}
	}
	else
	{
		write_output(gOutFile,
			"#%u: StartFrame=%08Xh TransferLength=%u\n",
			myXfers->CompletedCount, myBufferEL->FrameNumber, transferLength);
	}

	Dcs.Start = Dcs.Stop;
	myXfers->CompletedCount++;
	myXfers->LastStartFrame = myBufferEL->FrameNumber;
}

int __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE usbHandle = NULL;
	DWORD errorCode = ERROR_SUCCESS;
	BM_TEST_TYPE testType;
	UCHAR interfaceIndex;
	BOOL success;
	UINT deviceSpeedLength;
	BYTE deviceSpeed;
	KISOCH_PACKET_INFORMATION isoPacketInformation;
	PMY_ISO_BUFFER_EL nextBufferEL;
	PMY_ISO_BUFFER_EL nextBufferELTemp;
	ULONG transferred;
	BYTE bTemp;
	char logFilename[128];
	UINT logFilenameLength;

	memset(&gXfers, 0, sizeof(gXfers));
	memset(&gInterfaceDescriptor, 0, sizeof(gInterfaceDescriptor));
	memset(&gPipeInfo, 0, sizeof(gPipeInfo));

	/*
	Find the test device. Uses "vid=hhhh pid=hhhh" arguments supplied on the
	command line. (default is: vid=04D8 pid=FA2E)
	*/
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	// use settings from command line if specified
	Examples_GetArgVal(argc, argv, "intf=", &gIntfIndex, FALSE);
	Examples_GetArgVal(argc, argv, "altf=", &gAltfSetting, TRUE);
	Examples_GetArgVal(argc, argv, "ep=", &gEpID, TRUE);
	logFilenameLength = sizeof(logFilename) - 1;
	if (Examples_GetArgStr(argc, argv, "logfile", logFilename, &logFilenameLength))
		gOutFile = fopen(logFilename, "w");

	if (!LibK_LoadDriverAPI(&Usb, deviceInfo->DriverID))
	{
		errorCode = GetLastError();
		printf("LibK_LoadDriverAPI failed. ErrorCode: %08lXh\n", errorCode);
		goto Done;
	}

	if (!LibK_IsFunctionSupported(&Usb, KUSB_FNID_IsochReadPipe))
	{
		errorCode = ERROR_NOT_SUPPORTED;
		printf("DriverID %i does not support isochronous transfers using 'Isoch' functions. ErrorCode: %08lXh\n", Usb.Info.DriverID, errorCode);
		goto Done;
	}

	/*
	Initialize the device. This creates the physical usb handle.
	*/
	if (!Usb.Init(&usbHandle, deviceInfo))
	{
		errorCode = GetLastError();
		printf("Usb.Init failed. ErrorCode: %08lXh\n", errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

	/*
	Select interface by pipe id and get descriptors.
	*/
	interfaceIndex = (UCHAR)-1;
	memset(&gInterfaceDescriptor, 0, sizeof(gInterfaceDescriptor));
	memset(&gPipeInfo, 0, sizeof(gPipeInfo));
	while (Usb.SelectInterface(usbHandle, ++interfaceIndex, TRUE))
	{
		UCHAR gAltsettingIndex = (UCHAR)-1;
		if (gIntfIndex != UINT_MAX && gIntfIndex != interfaceIndex) continue;

		memset(&gInterfaceDescriptor, 0, sizeof(gInterfaceDescriptor));
		memset(&gPipeInfo, 0, sizeof(gPipeInfo));
		while (Usb.QueryInterfaceSettings(usbHandle, ++gAltsettingIndex, &gInterfaceDescriptor))
		{
			if (gAltfSetting == UINT_MAX || gAltfSetting == gInterfaceDescriptor.bAlternateSetting)
			{
				UCHAR pipeIndex = (UCHAR)-1;
				while (Usb.QueryPipeEx(usbHandle, gAltsettingIndex, ++pipeIndex, &gPipeInfo))
				{
					if (gPipeInfo.PipeType == UsbdPipeTypeIsochronous && gPipeInfo.MaximumPacketSize)
					{
						// use first endpoint
						if (gEpID == UINT_MAX)
							break;

						if ((gEpID == 0) && (gPipeInfo.PipeId & 0x80) == 0)
						{
							// using first write endpoint
							break;
						}
						if ((gEpID == 0x80) && (gPipeInfo.PipeId & 0x80) == 0x80)
						{
							// using first read endpoint
							break;
						}
						if (gEpID == gPipeInfo.PipeId)
						{
							// using a user specified read or write endpoint
							break;
						}
					}
				}
				if (gPipeInfo.PipeId) break;
				memset(&gPipeInfo, 0, sizeof(gPipeInfo));
			}
			memset(&gInterfaceDescriptor, 0, sizeof(gInterfaceDescriptor));
		}
	}

	if (!gPipeInfo.PipeId)
	{
		printf("Pipe not found.\n");
		goto Done;
	}

	if (!gPipeInfo.MaximumBytesPerInterval)
	{
		printf("Pipe does not have MaximumBytesPerInterval.\n");
		goto Done;

	}
	/*
	Set the desired alternate setting.
	*/
	success = Usb.SetAltInterface(
		usbHandle,
		gInterfaceDescriptor.bInterfaceNumber,
		FALSE,
		gInterfaceDescriptor.bAlternateSetting);

	if (!success)
	{
		printf("Usb.SetAltInterface failed.\n");
		goto Done;
	}

	// get the device speed
	deviceSpeedLength = 1;
	success = Usb.QueryDeviceInformation(usbHandle, DEVICE_SPEED, &deviceSpeedLength, &deviceSpeed);
	if (!success)
	{
		printf("Usb.QueryDeviceInformation failed.\n");
		goto Done;
	}

	// make sure ISO_ALWAYS_START_ASAP is disabled
	bTemp = FALSE;
	Usb.SetPipePolicy(usbHandle, gPipeInfo.PipeId, ISO_ALWAYS_START_ASAP, 1, &bTemp);

	// get the iso packet information to help with frame calculations.
	success = IsochK_CalcPacketInformation(deviceSpeed >= 3, &gPipeInfo, &isoPacketInformation);
	if (!success)
	{
		printf("IsochK_CalcPacketInformation failed.\n");
		goto Done;
	}

	// Configure the benchmark test device to send data.
	testType = USB_ENDPOINT_DIRECTION_IN(gPipeInfo.PipeId) ? BM_TEST_TYPE_READ : BM_TEST_TYPE_WRITE;
	success = Bench_Configure(usbHandle, BM_COMMAND_SET_TEST, 0, &Usb, &testType);
	if (!success)
	{
		errorCode = GetLastError();
		printf("Bench_Configure failed. ErrorCode: %08lXh\n", errorCode);
		goto Done;
	}

	printf("Device hardware fully prepared..\n");

	write_output(gOutFile, "XFER-ISO TEST\n", deviceInfo->DeviceID);
	write_output(gOutFile, "----------------------------------------------------------\n");
	write_output(gOutFile, "Device ID         : %s\n", deviceInfo->DeviceID);
	write_output(gOutFile, "Device Name       : %s\n", deviceInfo->DeviceDesc);
	write_output(gOutFile, "Interface         : %u\n", gInterfaceDescriptor.bInterfaceNumber);
	write_output(gOutFile, "Alt Setting       : %u\n", gInterfaceDescriptor.bAlternateSetting);
	write_output(gOutFile, "Pipe ID           : 0x%02X (%s)\n", gPipeInfo.PipeId, (gPipeInfo.PipeId & 0x80) ? "Read" : "Write");
	write_output(gOutFile, "Bytes Per Interval: %u\n", gPipeInfo.MaximumBytesPerInterval);
	write_output(gOutFile, "Interval Period   : %uus\n", isoPacketInformation.PollingPeriodMicroseconds);
	write_output(gOutFile, "\n");
	/*
	Allocate the iso buffer resources.
	*/
	do
	{
		int pos;
		gXfers.TransferBufferSize = ISO_PACKETS_PER_TRANSFER * gPipeInfo.MaximumBytesPerInterval;

		success = OvlK_Init(&gXfers.OvlPool, usbHandle, MAX_OUTSTANDING_TRANSFERS, KOVL_POOL_FLAG_NONE);
		if (!success)
		{
			errorCode = GetLastError();
			printf("OvlK_Init Failed.\n");
			goto Done;
		}

		for (pos = 0; pos < MAX_OUTSTANDING_TRANSFERS; pos++)
		{
			nextBufferEL = malloc(sizeof(MY_ISO_BUFFER_EL));
			if (nextBufferEL != NULL)
			{
				memset(nextBufferEL, 0, sizeof(nextBufferEL[0]));

				nextBufferEL->TransferBuffer = malloc(gXfers.TransferBufferSize);

				if (!IsochK_Init(&nextBufferEL->IsoHandle, usbHandle, gPipeInfo.PipeId, ISO_PACKETS_PER_TRANSFER, nextBufferEL->TransferBuffer, gXfers.TransferBufferSize))
				{
					errorCode = GetLastError();
					printf("IsochK_Init Failed.\n");
					goto Done;
				}
				if (!IsochK_SetPacketOffsets(nextBufferEL->IsoHandle, gPipeInfo.MaximumBytesPerInterval))
				{
					errorCode = GetLastError();
					printf("IsochK_SetPacketOffsets Failed.\n");
					goto Done;

				}

				if (!OvlK_Acquire(&nextBufferEL->OvlHandle, gXfers.OvlPool))
				{
					errorCode = GetLastError();
					printf("OvlK_Acquire Failed.\n");
					goto Done;

				}

				DL_APPEND(gXfers.Completed, nextBufferEL);
			}
		}
	} while (0);

	//Usb.ResetPipe(usbHandle, gPipeInfo.PipeId);

	/*
	Set a start frame. Add 5ms of startup delay.
	*/
	Usb.GetCurrentFrameNumber(usbHandle, &gXfers.FrameNumber);

	// Give plenty of time to queue up all of our transfers BEFORE the bus starts consuming them
	// Note that this is also the startup delay in milliseconds. IE: (6*2)=12ms
	gXfers.FrameNumber += (MAX_OUTSTANDING_TRANSFERS * 2);

	mDcs_Init(&Dcs);

	/*
	Start reading until an error occurs or MAX_TRANSFERS_TOTAL is reached.
	*/
	do
	{

		while (errorCode == ERROR_SUCCESS && gXfers.Completed && gXfers.SubmittedCount < MAX_TRANSFERS_TOTAL)
		{
			nextBufferEL = gXfers.Completed;
			DL_DELETE(gXfers.Completed, nextBufferEL);
			DL_APPEND(gXfers.Outstanding, nextBufferEL);

			OvlK_ReUse(nextBufferEL->OvlHandle);

			nextBufferEL->FrameNumber = gXfers.FrameNumber;
			if (gPipeInfo.PipeId & 0x80)
				Usb.IsochReadPipe(nextBufferEL->IsoHandle, gXfers.TransferBufferSize, &gXfers.FrameNumber, ISO_PACKETS_PER_TRANSFER, nextBufferEL->OvlHandle);
			else
				Usb.IsochWritePipe(nextBufferEL->IsoHandle, gXfers.TransferBufferSize, &gXfers.FrameNumber, ISO_PACKETS_PER_TRANSFER, nextBufferEL->OvlHandle);

			errorCode = GetLastError();
			if (errorCode != ERROR_IO_PENDING)
			{
				printf("Usb.IsochReadPipe failed. ErrorCode: %08lXh\n", errorCode);
				goto Done;
			}
			gXfers.SubmittedCount++;
			errorCode = ERROR_SUCCESS;
		}

		nextBufferEL = gXfers.Outstanding;
		if (!nextBufferEL)
		{
			printf("Done!\n");
			goto Done;
		}

		success = Usb.GetOverlappedResult(usbHandle, nextBufferEL->OvlHandle, &transferred, TRUE);
		//success = OvlK_Wait(nextBufferEL->OvlHandle, 1000, KOVL_WAIT_FLAG_NONE, (PUINT)&transferred);
		if (!success)
		{
			errorCode = GetLastError();
			printf("OvlK_Wait failed. ErrorCode: %08lXh\n", errorCode);
			goto Done;
		}
		DL_DELETE(gXfers.Outstanding, nextBufferEL);
		DL_APPEND(gXfers.Completed, nextBufferEL);

		IsoXferComplete(&gXfers, nextBufferEL, transferred);

	} while (errorCode == ERROR_SUCCESS);

Done:


	/*
	Cancel all transfers left outstanding.
	*/
	DL_FOREACH_SAFE(gXfers.Outstanding, nextBufferEL, nextBufferELTemp)
	{
		DL_DELETE(gXfers.Outstanding, nextBufferEL);
		DL_APPEND(gXfers.Completed, nextBufferEL);
		OvlK_WaitOrCancel(nextBufferEL->OvlHandle, 0, (PUINT)&transferred);
	}

	/*
	Free the iso buffer resources.
	*/
	DL_FOREACH_SAFE(gXfers.Completed, nextBufferEL, nextBufferELTemp)
	{
		DL_DELETE(gXfers.Completed, nextBufferEL);
		OvlK_Release(nextBufferEL->OvlHandle);
		IsochK_Free(nextBufferEL->IsoHandle);
		free(nextBufferEL->TransferBuffer);
		free(nextBufferEL);
	}

	// Free the overlapped pool.
	OvlK_Free(gXfers.OvlPool);

	// return the device interface back to alt setting #0
	Usb.SetAltInterface(usbHandle, gInterfaceDescriptor.bInterfaceNumber, TRUE, 0);

	// Close the device handle.
	Usb.Free(usbHandle);

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

