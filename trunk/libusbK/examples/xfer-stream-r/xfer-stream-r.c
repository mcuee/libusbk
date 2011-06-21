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
#define EP_RX					0x81
#define EP_TX					0x02
#define EP_PACKET_SIZE			64
#define ISO_PACKETS_PER_XFER	32

#define ISO_CALC_CONTEXT_SIZE(mNumOfIsoPackets) (sizeof(KUSB_ISO_CONTEXT)+(sizeof(KUSB_ISO_PACKET)*(mNumOfIsoPackets)))
#define ISO_CALC_DATABUFFER_SIZE(mNumOfIsoPackets, mIsoPacketSize) (mNumOfIsoPackets*mIsoPacketSize)

// Custom vendor requests that must be implemented in the benchmark firmware.
// Test selection can be bypassed with the "notestselect" argument.
//
typedef enum _BENCHMARK_DEVICE_COMMAND
{
    SET_TEST = 0x0E,
    GET_TEST = 0x0F,
} BENCHMARK_DEVICE_COMMAND, *PBENCHMARK_DEVICE_COMMAND;

// Tests supported by the official benchmark firmware.
//
typedef enum _BENCHMARK_DEVICE_TEST_TYPE
{
    TestTypeNone	= 0x00,
    TestTypeRead	= 0x01,
    TestTypeWrite	= 0x02,
    TestTypeLoop	= TestTypeRead | TestTypeWrite,
} BENCHMARK_DEVICE_TEST_TYPE, *PBENCHMARK_DEVICE_TEST_TYPE;

BOOL Bench_Configure(__in LIBUSBK_INTERFACE_HANDLE handle,
                     __in BENCHMARK_DEVICE_COMMAND command,
                     __in UCHAR intf,
                     __deref_inout PBENCHMARK_DEVICE_TEST_TYPE testType)
{
	UCHAR buffer[1];
	DWORD transferred = 0;
	WINUSB_SETUP_PACKET Pkt;
	PKUSB_SETUP_PACKET defPkt = (PKUSB_SETUP_PACKET)&Pkt;

	memset(&Pkt, 0, sizeof(Pkt));
	defPkt->bmRequestType.BM.Dir = BMREQUEST_DEVICE_TO_HOST;
	defPkt->bmRequestType.BM.Type = BMREQUEST_VENDOR;
	defPkt->bRequest = (UCHAR)command;
	defPkt->wValue.W = (UCHAR) * testType;
	defPkt->wIndex.W = intf;
	defPkt->wLength = 1;

	if (UsbK_ControlTransfer(handle, Pkt, buffer, 1, &transferred, NULL))
	{
		if (transferred)
			return TRUE;
	}

	return FALSE;
}

VOID IsoInitPackets(PKUSB_ISO_CONTEXT isoCtx, ULONG PacketSize)
{
	ULONG isoPacket;
	ULONG nextOffset = 0;
	for (isoPacket = 0; isoPacket < isoCtx->NumberOfPackets; isoPacket++)
	{
		isoCtx->IsoPackets[isoPacket].Offset = nextOffset;
		nextOffset += PacketSize;
	}
}

DWORD __cdecl main(int argc, char* argv[])
{
	PKUSB_DEV_LIST deviceList = NULL;
	PKUSB_DEV_INFO deviceInfo = NULL;
	LIBUSBK_INTERFACE_HANDLE handle = NULL;
	DWORD errorCode = ERROR_SUCCESS;

	BOOL success;
	UCHAR dataBuffer[ISO_CALC_DATABUFFER_SIZE(ISO_PACKETS_PER_XFER, EP_PACKET_SIZE)];
	ULONG transferred = 0;
	PKUSB_ISO_CONTEXT isoCtx = NULL;
	POVERLAPPED_K ovlkHandle = NULL;
	BENCHMARK_DEVICE_TEST_TYPE testType = TestTypeRead;
	ULONG posPacket;
	ULONG currentFrameNumber;

	/*!
	Find the test device. Uses "vid=hhhh pid=hhhh" arguments supplied on the
	command line. (default is: vid=04D8 pid=FA2E)
	*/
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	/*!
	Open the device. This creates the physical USB device handle.
	*/
	if (!UsbK_Open(deviceInfo, &handle))
	{
		errorCode = GetLastError();
		printf("Open device failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

	success = Bench_Configure(handle, SET_TEST, 0, &testType);
	if (!success)
	{
		errorCode = GetLastError();
		printf("Bench_Configure failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}

	isoCtx = malloc(ISO_CALC_CONTEXT_SIZE(ISO_PACKETS_PER_XFER));
	memset(isoCtx, 0, ISO_CALC_CONTEXT_SIZE(ISO_PACKETS_PER_XFER));
	isoCtx->NumberOfPackets = ISO_PACKETS_PER_XFER;

	if (testType == TestTypeRead)
		isoCtx->PipeID = EP_RX;
	else
		isoCtx->PipeID = EP_TX;

	IsoInitPackets(isoCtx, EP_PACKET_SIZE);

	ovlkHandle = OvlK_Acquire(NULL);
	UsbK_ResetPipe(handle, isoCtx->PipeID);

	success = UsbK_GetCurrentFrameNumber(handle, &currentFrameNumber);
	if (!success)
	{
		errorCode = GetLastError();
		printf("UsbK_GetCurrentFrameNumber failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}

	isoCtx->StartFrame = currentFrameNumber + 8 + (currentFrameNumber % 8);
	if (testType == TestTypeRead)
		UsbK_IsoReadPipe(handle, isoCtx, dataBuffer, sizeof(dataBuffer), ovlkHandle);
	else
		UsbK_IsoWritePipe(handle, isoCtx, dataBuffer, sizeof(dataBuffer), ovlkHandle);

	success = OvlK_WaitAndRelease(ovlkHandle, 1000, &transferred);
	if (!success)
	{
		errorCode = GetLastError();
		printf("IsoReadPipe failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}

	printf("ISO StartFrame=%08Xh ErrorCount=%u TransferCounter=%u Transferred=%u\n",
	       isoCtx->StartFrame, isoCtx->ErrorCount, isoCtx->TransferCounter, transferred);

	for (posPacket = 0; posPacket < isoCtx->NumberOfPackets; posPacket++)
	{
		PKUSB_ISO_PACKET isoPacket = &isoCtx->IsoPackets[posPacket];
		ULONG posData = isoPacket->Offset;
		ULONG posDataMax =  posPacket + 1 < isoCtx->NumberOfPackets ? isoCtx->IsoPackets[posPacket + 1].Offset : sizeof(dataBuffer);

		printf("  IsoPacket[%d] Length=%u Status=%08Xh\n", posPacket, isoPacket->Length, isoPacket->Status);
		printf("  Data:");

		for (; posData < posDataMax; posData++)
		{
			printf("%02Xh ", dataBuffer[posData]);
			if ((posData & 0xF) == 0xF) printf("\n       ");
		}
		printf("\n");
		if ((posData & 0xF) != 0xF) printf("\n");
	}

Done:
	/*!
	Close the device handle. If handle is invalid (NULL), has no effect.
	*/
	UsbK_Close(handle);
	/*!
	Free the device list. If deviceList is invalid (NULL), has no effect.
	*/
	LstK_Free(&deviceList);
	/*!
	Free the iso context.
	*/
	if (isoCtx)
		free(isoCtx);

	return errorCode;
}
