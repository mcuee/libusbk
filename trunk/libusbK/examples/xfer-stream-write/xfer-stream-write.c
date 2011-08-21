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
#define EP_TX					0x01
#define MAX_TRANSFER_SIZE		4096
#define MAX_PENDING_TRANSFERS	128
#define MAX_PENDING_IO			3

// Gobals:
KUSB_DRIVER_API		Usb;
DATA_COUNTER_STATS	Dcs;

DWORD __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;			// device list handle (the list of device infos)
	KLST_DEVINFO_HANDLE deviceInfo = NULL;	// device info handle (the device list element)
	KUSB_HANDLE usbHandle = NULL;			// device interface handle (the opened USB device)
	KSTM_HANDLE streamHandle = NULL;
	DWORD errorCode = ERROR_SUCCESS;
	DWORD transferLength = 0;
	BOOL success;
	UCHAR myBuffer[MAX_TRANSFER_SIZE];
	BM_TEST_TYPE testType = BM_TEST_TYPE_WRITE;

	/*!
	Find the test device. Uses "vid=hhhh pid=hhhh" arguments supplied on the
	command line. (default is: vid=04D8 pid=FA2E)
	*/
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	LibK_LoadDriverAPI(&Usb, deviceInfo->DrvId);

	/*!
	Open the device. This creates the physical USB device handle.
	*/
	if (!Usb.Init(&usbHandle, deviceInfo))
	{
		errorCode = GetLastError();
		printf("Open device failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

	/*
	Configure the benchmark test device for read mode. (DeviceToHost)
	*/
	success = Bench_Configure(usbHandle, BM_COMMAND_SET_TEST, 0, &Usb, &testType);
	if (!success) goto Done;

	/*
	Initialize a new stream handle.
	*/
	success = StmK_Init(
	              &streamHandle,
	              usbHandle,
	              EP_TX,
	              MAX_TRANSFER_SIZE,
	              MAX_PENDING_TRANSFERS,
	              MAX_PENDING_IO,
	              NULL,
	              KSTM_FLAG_NONE);

	printf("[Init  Stream] result = %08Xh\n", success ? 0 : GetLastError());
	if (!success) goto Done;

	/*
	Start the stream.
	*/
	printf("[Start Stream] result = %08Xh\n", ((success = StmK_Start(streamHandle))) ? 0 : GetLastError());
	if (!success) goto Done;

	mDcs_Init(&Dcs);

	while(_kbhit()) _getch();
	do
	{
		ULONG length;
		success = StmK_Write(streamHandle, myBuffer, 0, sizeof(myBuffer), &length);
		if (success)
		{
			transferLength += length;
		}
		else
		{
			// If the return result is ERROR_NO_MORE_ITEMS then there is no more data
			// to read.  Other errors indicate a problem.
			if (GetLastError() != ERROR_NO_MORE_ITEMS)
			{
				break;
			}
			if (transferLength >= sizeof(myBuffer) * 8)
			{
				mDcs_MarkStop(&Dcs, transferLength);
				printf("Transferred bytes:%d BPS:%.2f\n", transferLength, Dcs.Bps);
				transferLength = 0;
			}
			if ((Dcs.TotalBytes >= (sizeof(myBuffer) * 128)))
			{
				if (transferLength)
				{
					mDcs_MarkStop(&Dcs, transferLength);
					transferLength = 0;
				}

				// Stop the stream and then re-start it.
				// This is done here to excercise the API only.
				printf("[Stop  Stream] result = %08Xh\n", ((success = StmK_Stop(streamHandle, 0))) ? 0 : GetLastError());
				if (!success) goto Done;

				printf("[StreamResult] Total bytes:%I64d BPS:%.2f\n", Dcs.TotalBytes, Dcs.Bps);

				printf("[Start Stream] result = %08Xh\n", ((success = StmK_Start(streamHandle))) ? 0 : GetLastError());
				if (!success) goto Done;

				mDcs_Init(&Dcs);
			}
			else
			{
				// ..Emulate some work..
				Sleep(100);
			}
		}

	}
	while(!_kbhit());


	printf("[Stop  Stream] result = %08Xh\n", ((success = StmK_Stop(streamHandle, 0))) ? 0 : GetLastError());
	StmK_Free(streamHandle);

Done:
	/*!
	Close the device handle. If handle is invalid (NULL), has no effect.
	*/
	UsbK_Free(usbHandle);
	/*!
	Free the device list. If deviceList is invalid (NULL), has no effect.
	*/
	LstK_Free(deviceList);

	return errorCode;
}
