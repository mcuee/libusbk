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

// Example configuration:
#define EP_TX					0x01
#define EP_RX					0x81
#define MAX_TRANSFER_SIZE		4096
#define MAX_PENDING_IO				3
#define MAX_PENDING_TRANSFERS_RX	64
#define MAX_PENDING_TRANSFERS_TX	3
#define STREAM_TIMEOUT_RX			1000
#define STREAM_TIMEOUT_TX			-1

#define UPDATE_STATS_INTERVAL_SECS		(1.0)

// Gobals:
KUSB_DRIVER_API		Usb;
DATA_COUNTER_STATS	Dcs;

UCHAR gRxBuffer[MAX_TRANSFER_SIZE];
UCHAR gTxBuffer[MAX_TRANSFER_SIZE];

UINT gRxBufferLen = sizeof(gRxBuffer);
UINT gTxBufferLen = sizeof(gTxBuffer);

VOID FillTxBufferForWrite(void);
VOID ProcessRxBufferFromRead(void);

DWORD __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE usbHandle = NULL;
	KSTM_HANDLE streamRx = NULL;
	KSTM_HANDLE streamTx = NULL;
	DWORD errorCode = ERROR_SUCCESS;
	DWORD totalTransferLength = 0;
	BOOL success;
	BM_TEST_TYPE testType = BM_TEST_TYPE_LOOP;

	/*
	Find the test device. Uses "vid=hhhh pid=hhhh" arguments supplied on the
	command line. (default is: vid=04D8 pid=FA2E)
	*/
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	LibK_LoadDriverAPI(&Usb, deviceInfo->DriverID);

	/*
	Initialize the device. This creates the physical usb handle.
	*/
	if (!Usb.Init(&usbHandle, deviceInfo))
	{
		errorCode = GetLastError();
		printf("Usb.Init failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

	/*
	Configure the benchmark test test type.
	*/
	success = Bench_Configure(usbHandle, BM_COMMAND_SET_TEST, 0, &Usb, &testType);

	/*
	Initialize a new IN stream handle.
	*/
	success = StmK_Init(
	              &streamRx,
	              usbHandle,
	              EP_RX,
	              MAX_TRANSFER_SIZE,
	              MAX_PENDING_TRANSFERS_RX,
	              MAX_PENDING_IO,
	              NULL,
	              ((STREAM_TIMEOUT_RX) == -1) ? KSTM_FLAG_NONE : KSTM_FLAG_USE_TIMEOUT | (STREAM_TIMEOUT_RX & KSTM_FLAG_TIMEOUT_MASK));
	if (!success)
	{
		errorCode = GetLastError();
		printf("StmK_Init failed. ErrorCode: %08Xh\n", errorCode);
		goto Done;
	}

	/*
	Initialize a new OUT stream handle.
	*/
	success = StmK_Init(
	              &streamTx,
	              usbHandle,
	              EP_TX,
	              MAX_TRANSFER_SIZE,
	              MAX_PENDING_TRANSFERS_TX,
	              MAX_PENDING_IO,
	              NULL,
	              ((STREAM_TIMEOUT_TX) == -1) ? KSTM_FLAG_NONE : KSTM_FLAG_USE_TIMEOUT | (STREAM_TIMEOUT_TX & KSTM_FLAG_TIMEOUT_MASK));
	if (!success)
	{
		errorCode = GetLastError();
		printf("StmK_Init failed. ErrorCode: %08Xh\n", errorCode);
		goto Done;
	}
	/*
	Start the stream.
	*/
	if (success) success = StmK_Start(streamTx);
	if (success) success = StmK_Start(streamRx);

	if (!success)
	{
		errorCode = GetLastError();
		printf("StmK_Start failed. ErrorCode: %08Xh\n", errorCode);
		goto Done;
	}
	printf("[Start Stream] successful!\n");


	while(_kbhit()) _getch();

	// Init stat tracker.
	mDcs_Init(&Dcs);

	// Prime gTxBuffer with the first batch of data.
	FillTxBufferForWrite();
	do
	{
		UINT length;
		while((success = StmK_Write(streamTx, gTxBuffer, 0, gTxBufferLen, &length)))
		{
			// Fill gTxBuffer with the next batch of data.
			FillTxBufferForWrite();
		}
		if (GetLastError() != ERROR_NO_MORE_ITEMS)
		{
			errorCode = GetLastError();
			printf("StmK_Write failed. ErrorCode: %08Xh\n", errorCode);
			break;
		}

		success = StmK_Read(streamRx, gRxBuffer, 0, sizeof(gRxBuffer), &length);
		if (!success)
		{
			errorCode = GetLastError();
			printf("StmK_Read failed. ErrorCode: %08Xh\n", errorCode);
			break;
		}

		// Update gRxBufferLen with the actual bytes transferred.
		gRxBufferLen = length;

		// Process the data received from EP_RX residing in gRxBuffer
		ProcessRxBufferFromRead();

		totalTransferLength += length;
		mDcs_MarkStop(&Dcs, length);
		if (Dcs.Duration >= UPDATE_STATS_INTERVAL_SECS)
		{
			printf("Bytes transferred:%d total:%d BPS:%.2f\n", (int)Dcs.TotalBytes, totalTransferLength, Dcs.Bps);
			Dcs.Start.QuadPart = Dcs.Stop.QuadPart;
			Dcs.TotalBytes = 0;
		}

	}
	while(!_kbhit());

	/*
	Stop the streams.
	*/
	StmK_Stop(streamTx, 0);
	StmK_Stop(streamRx, 0);
	printf("[Stop Stream] successful!\n");

Done:
	// Free the stream handles.
	if (streamTx) StmK_Free(streamTx);
	if (streamRx) StmK_Free(streamRx);

	// Free the usb handle.
	UsbK_Free(usbHandle);

	// Free the device list.
	LstK_Free(deviceList);

	return errorCode;
}

VOID FillTxBufferForWrite(void)
{
	/* TODO USER:
	1) Fill the tx buffer (gTxBuffer) with data to be sent to EP_TX.
	2) Update gTxBufferLen accordingly.
	*/
	gTxBufferLen = sizeof(gTxBuffer);
}

VOID ProcessRxBufferFromRead(void)
{
	/* TODO USER:
	Process the rx buffer (gRxBuffer) with data received from EP_RX.
	Use gRxBufferLen to determine the actual number of bytes received.
	*/
}

/*!
Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (7&40FF847&0&0000): Benchmark One (Interface 0) - Microchip Technology, Inc.
Device opened successfully!
[Start Stream] successful!
Bytes transferred:196608 total:196608 BPS:192696.90
Bytes transferred:196608 total:393216 BPS:192851.08
Bytes transferred:196608 total:589824 BPS:193082.45
Bytes transferred:196608 total:786432 BPS:192764.73
Bytes transferred:192512 total:978944 BPS:192420.36
Bytes transferred:196608 total:1175552 BPS:192895.71
Bytes transferred:196608 total:1372160 BPS:192936.98
[Stop Stream] successful!
*/
