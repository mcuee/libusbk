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
#define EP_ADDRESS				0x81

// Gobals:
KUSB_DRIVER_API		Usb;
DATA_COUNTER_STATS	Dcs;

DWORD __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE usbHandle = NULL;
	KOVL_POOL_HANDLE ovlPool = NULL;
	KOVL_HANDLE ovlItem = NULL;
	DWORD errorCode = ERROR_SUCCESS;
	ULONG policyLength, pipeTimeout, newPipeTimeout;
	UCHAR myBuffer[1024];
	ULONG transferredLength, transferCount;
	BOOL success;
	int pass = 0;


	/*!
	Find the test device. Uses "vid=hhhh pid=hhhh" arguments supplied on the
	command line. (default is: vid=04D8 pid=FA2E)
	*/
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	/*!
	This example will use the dynamic driver api so that it can be used
	with all supported drivers.
	*/
	LibK_LoadDriverAPI(&Usb, deviceInfo->DriverID);

	/*!
	Initialize the device. This creates the physical usb handle.
	*/
	if (!Usb.Init(&usbHandle, deviceInfo))
	{
		errorCode = GetLastError();
		printf("Usb.Init failed. (0x%08X)\n", errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

	newPipeTimeout = 10;

	/*
	Gets the pipe transfer timeout policy value, sets it, and gets it a second time.
	*/
	do
	{
		pipeTimeout = 0;
		printf("[%02Xh] Getting Pipe-Timeout..\n", EP_ADDRESS);

		policyLength = sizeof(ULONG);
		success = Usb.GetPipePolicy(usbHandle, EP_ADDRESS, PIPE_TRANSFER_TIMEOUT, &policyLength, &pipeTimeout);
		if (!success)
		{
			errorCode = GetLastError();
			printf("Usb.GetPipePolicy failed. (0x%08X)\n", errorCode);
			goto Done;
		}
		printf("[%02Xh] Pipe-Timeout is %u\n", EP_ADDRESS, pipeTimeout);
		if (pass == 1) break;

		printf("[%02Xh] Setting Pipe-Timeout to %u..\n", EP_ADDRESS, newPipeTimeout);
		policyLength = sizeof(ULONG);
		success = Usb.SetPipePolicy(usbHandle, EP_ADDRESS, PIPE_TRANSFER_TIMEOUT, policyLength, &newPipeTimeout);
		if (!success)
		{
			errorCode = GetLastError();
			printf("Usb.SetPipePolicy failed. (0x%08X)\n", errorCode);
			goto Done;
		}

	}
	while(++pass < 2);

	if (newPipeTimeout != pipeTimeout)
	{
		errorCode = ERROR_UNIDENTIFIED_ERROR;
		printf("Pipe-Timeout did not change.\n");
		goto Done;
	}

	/*!
	Initialize a new OvlK pool handle.
	*/
	OvlK_Init(&ovlPool, usbHandle, 1, 0);

	/*
	Submit transfers and estimate the compeletion timeout duration.
	*/
	pipeTimeout = 1000;
	transferCount = 0;
	while((transferCount++ <= 3))
	{
		if (!OvlK_Acquire(&ovlItem, ovlPool)) break;

		printf("[%02Xh] Transfer-Begin #%u..\n", EP_ADDRESS, transferCount);
		if (USB_ENDPOINT_DIRECTION_IN(EP_ADDRESS))
			Usb.ReadPipe(usbHandle, EP_ADDRESS, myBuffer, sizeof(myBuffer), NULL, ovlItem);
		else
			Usb.WritePipe(usbHandle, EP_ADDRESS, myBuffer, sizeof(myBuffer), NULL, ovlItem);

		mDcs_Init(&Dcs);

		if (OvlK_WaitAndRelease(ovlItem, 1000, &transferredLength))
		{
			errorCode = ERROR_MORE_DATA;
			continue;
		}
		else
		{
			errorCode = GetLastError();

			/*!
			The OvlK module uses two different error codes to indicate a cancel I/O condition.
			*/
			if (errorCode == ERROR_CANCELLED || errorCode == ERROR_OPERATION_ABORTED)
			{
				errorCode = ERROR_SUCCESS;
				if (pipeTimeout != newPipeTimeout)
				{
					pipeTimeout = newPipeTimeout;
				}
				else
				{
					mDcs_MarkStop(&Dcs, 0);
					printf("[%02Xh] Transfer-Duration: %.2f ms\n", EP_ADDRESS, Dcs.Duration * 1000);
				}
			}
			else
			{
				printf("[%02Xh] Unexpected-Error: %08Xh!\n", EP_ADDRESS, errorCode);
				break;
			}
		}
	}

Done:
	/*!
	Close the usb handle.
	*/
	if (usbHandle) Usb.Free(usbHandle);
	/*!
	Free the device list.
	*/
	LstK_Free(deviceList);

	return errorCode;
}
/*
Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
Device opened successfully!
[81h] Getting Pipe-Timeout..
[81h] Pipe-Timeout is 0
[81h] Setting Pipe-Timeout to 10..
[81h] Getting Pipe-Timeout..
[81h] Pipe-Timeout is 10
[81h] Transfer-Begin #1..
[81h] Transfer-Begin #2..
[81h] Transfer-Duration: 9.66 ms
[81h] Transfer-Begin #3..
[81h] Transfer-Duration: 8.94 ms
[81h] Transfer-Begin #4..
[81h] Transfer-Duration: 9.24 ms
*/
