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
#define SYNC_TRANSFER_COUNT		3

// Globals:
KUSB_DRIVER_API Usb;

DWORD __cdecl main(int argc, char* argv[])
{
	DWORD errorCode = ERROR_SUCCESS;
	BOOL success;

	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE usbHandle = NULL;
	UCHAR myBuffer[4096];
	BM_TEST_TYPE testType = USB_ENDPOINT_DIRECTION_IN(EP_ADDRESS) ? BM_TEST_TYPE_READ : BM_TEST_TYPE_WRITE;
	ULONG totalLength = 0;
	ULONG transferredLength;
	ULONG transferIndex;

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

	success = Bench_Configure(usbHandle, BM_COMMAND_SET_TEST, 0, &Usb, &testType);
	if (!success) printf("Bench_Configure failed.\n");

	/*
	Submit and complete SYNC_TRANSFER_COUNT number of transfers.
	*/
	transferIndex = (DWORD) - 1;
	while (++transferIndex < SYNC_TRANSFER_COUNT)
	{
		// This examples works the same fo reading and writing.  The endpoint address is used to
		// determine which.
		if (USB_ENDPOINT_DIRECTION_IN(EP_ADDRESS))
			success = Usb.ReadPipe(usbHandle, EP_ADDRESS, myBuffer, sizeof(myBuffer), &transferredLength, NULL);
		else
			success = Usb.WritePipe(usbHandle, EP_ADDRESS, myBuffer, sizeof(myBuffer), &transferredLength, NULL);

		if (!success)
		{
			errorCode = GetLastError();
			break;
		}

		totalLength += transferredLength;
		printf("Transfer #%u completed with %u bytes.\n", transferIndex, transferredLength);
	}

	printf("Transferred %u bytes in %u transfers. errorCode=%08Xh\n", totalLength, transferIndex, errorCode);

Done:

	/*
	Close the usb handle.
	*/
	if (usbHandle) Usb.Free(usbHandle);
	/*
	Free the device list.
	*/
	LstK_Free(deviceList);

	return errorCode;
}
/*!
Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
Device opened successfully!
Transfer #0 completed with 4096 bytes.
Transfer #1 completed with 4096 bytes.
Transfer #2 completed with 4096 bytes.
Transferred 12288 bytes in 3 transfers. errorCode=00000000h
*/
