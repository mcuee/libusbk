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
#define ASYNC_PENDING_IO_COUNT	3
#define ASYNC_TIMEOUT_MS		1000

// Globals:
KUSB_DRIVER_API Usb;

typedef struct _MY_IO_REQUEST
{
	KOVL_HANDLE Ovl;
	DWORD Index;
	DWORD ErrorCode;
	DWORD BufferSize;
	UCHAR Buffer[4096];
	DWORD TransferLength;

	struct _MY_IO_REQUEST* prev;
	struct _MY_IO_REQUEST* next;
} MY_IO_REQUEST, *PMY_IO_REQUEST;

DWORD __cdecl main(int argc, char* argv[])
{
	DWORD errorCode = ERROR_SUCCESS;
	BOOL success;

	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE usbHandle = NULL;
	KOVL_POOL_HANDLE ovlPool = NULL;
	MY_IO_REQUEST ovlArray[ASYNC_PENDING_IO_COUNT];
	PMY_IO_REQUEST requestList = NULL;
	PMY_IO_REQUEST myRequest;
	DWORD ovlIndex;
	BM_TEST_TYPE testType = USB_ENDPOINT_DIRECTION_IN(EP_ADDRESS) ? BM_TEST_TYPE_READ : BM_TEST_TYPE_WRITE;
	ULONG totalLength = 0;

	/*
	Find the test device. Uses "vid=hhhh pid=hhhh" arguments supplied on the
	command line. (default is: vid=04D8 pid=FA2E)
	*/
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	/*
	This example will use the dynamic driver api so that it can be used
	with all supported drivers.
	*/
	LibK_LoadDriverAPI(&Usb, deviceInfo->DriverID);

	/*
	Initialize the device. This creates the physical usb handle.
	*/
	if (!Usb.Init(&usbHandle, deviceInfo))
	{
		errorCode = GetLastError();
		printf("Init device failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

	/*
	Configure the benchmark test device to accept/send data.
	*/
	success = Bench_Configure(usbHandle, BM_COMMAND_SET_TEST, 0, &Usb, &testType);
	if (!success) printf("Bench_Configure failed.\n");

	/*
	Initialize a new OvlK pool handle.
	*/
	OvlK_Init(&ovlPool, usbHandle, ASYNC_PENDING_IO_COUNT, 0);

	memset(ovlArray, 0, sizeof(ovlArray));
	for(ovlIndex = 0; ovlIndex < ASYNC_PENDING_IO_COUNT; ovlIndex++)
	{
		// Get an overlapped handle from the pool initialized above.
		if (!OvlK_Acquire(&ovlArray[ovlIndex].Ovl, ovlPool))
		{
			errorCode = GetLastError();
			printf("OvlK_Acquire failed. ErrorCode: %08Xh\n",  errorCode);
			goto Done;
		}

		ovlArray[ovlIndex].Index		= ovlIndex;
		ovlArray[ovlIndex].BufferSize	= sizeof(ovlArray[ovlIndex].Buffer);

		DL_APPEND(requestList, &ovlArray[ovlIndex]);
	}

	/*
	Submit ASYNC_PENDING_IO_COUNT number of I/O request.
	*/
	DL_FOREACH(requestList, myRequest)
	{
		// This examples works the same for reading and writing.  The endpoint address is used to
		// determine which.
		if (USB_ENDPOINT_DIRECTION_IN(EP_ADDRESS))
			Usb.ReadPipe(usbHandle, EP_ADDRESS, myRequest->Buffer, myRequest->BufferSize, NULL, myRequest->Ovl);
		else
			Usb.WritePipe(usbHandle, EP_ADDRESS, myRequest->Buffer, myRequest->BufferSize, NULL, myRequest->Ovl);

		// On success, all pipe transfer functions using overlapped I/O return false and set last
		// error to ERROR_IO_PENDING.
		myRequest->ErrorCode = GetLastError();
		if (myRequest->ErrorCode != ERROR_IO_PENDING)
		{
			errorCode = myRequest->ErrorCode;
			printf("Failed submitting transfer #%u for %u bytes.\n", myRequest->Index, myRequest->BufferSize);
			break;
		}


		printf("Transfer #%u submitted for %u bytes.\n", myRequest->Index, myRequest->BufferSize);
	}

	/*
	Complete all I/O request submitted above.
	*/
	DL_FOREACH(requestList, myRequest)
	{
		if (myRequest->ErrorCode == ERROR_IO_PENDING)
		{
			printf("Waiting %u ms for transfer #%u to complete..\n", ASYNC_TIMEOUT_MS, myRequest->Index);
			success = OvlK_WaitOrCancel(myRequest->Ovl, ASYNC_TIMEOUT_MS, &myRequest->TransferLength);
			if (!success)
			{
				errorCode = myRequest->ErrorCode = GetLastError();
				printf("Transfer #%u did not complete. ErrorCode=%08Xh\n", myRequest->Index, myRequest->ErrorCode);
			}
			else
			{
				myRequest->ErrorCode = ERROR_SUCCESS;
				totalLength += myRequest->TransferLength;
				printf("Transfer #%u completed with %u bytes.\n", myRequest->Index, myRequest->TransferLength);
			}
		}
	}

	if (errorCode == ERROR_SUCCESS)
	{
		printf("Transferred %u bytes successfully.\n", totalLength, errorCode);
	}
	else
	{
		printf("Transferred %u bytes. ErrorCode=%08Xh\n", totalLength, errorCode);
	}

Done:

	/*
	Close the usb handle.
	*/
	if (usbHandle) Usb.Free(usbHandle);
	/*
	Free the device list.
	*/
	LstK_Free(deviceList);
	/*
	Free and release all resources assigned to this pool.
	*/
	OvlK_Free(ovlPool);
	return errorCode;
}
/*!
Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
Device opened successfully!
Transfer #0 submitted for 4096 bytes.
Transfer #1 submitted for 4096 bytes.
Transfer #2 submitted for 4096 bytes.
Transfer #0 completed with 4096 bytes.
Transfer #1 completed with 4096 bytes.
Transfer #2 completed with 4096 bytes.
Transferred 12288 bytes in 3 transfers. errorCode=00000000h
*/
