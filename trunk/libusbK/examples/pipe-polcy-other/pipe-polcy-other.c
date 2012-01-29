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
#define EP_ADDRESS				0x02
#define EP_PACKET_SIZE			512
#define SYNC_TRANSFER_COUNT		128
#define SYNC_TRANSFER_SIZE		((EP_PACKET_SIZE*4096))

#define USE_IGNORE_SHORT_PACKETS
#define USE_RAW_IO
#define USE_SHORT_PACKET_TERMINATE

// Globals:
KUSB_DRIVER_API Usb;
UCHAR myBuffer[SYNC_TRANSFER_SIZE];

DWORD __cdecl main(int argc, char* argv[])
{
	DWORD errorCode = ERROR_SUCCESS;
	BOOL success;

	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE usbHandle = NULL;
	BM_TEST_TYPE testType = USB_ENDPOINT_DIRECTION_IN(EP_ADDRESS) ? BM_TEST_TYPE_READ : BM_TEST_TYPE_WRITE;
	ULONG totalLength = 0;
	ULONG transferredLength;
	ULONG transferIndex;
	ULONG polLength, polValue;

	BYTE polIgnoreShortPackets = 1;
	BYTE polRawIO = 0;
	BYTE polShortPacketTerminate = 1;

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

	polLength = 4;
	success = Usb.GetPipePolicy(usbHandle, EP_ADDRESS, MAXIMUM_TRANSFER_SIZE, &polLength, &polValue);
	if (!success)
	{
		errorCode = GetLastError();
		printf("SetPipePolicy failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}

	printf("MAXIMUM_TRANSFER_SIZE=%u\n", polValue);

#ifdef USE_IGNORE_SHORT_PACKETS
	if (USB_ENDPOINT_DIRECTION_IN(EP_ADDRESS))
	{
		success = Usb.SetPipePolicy(usbHandle, EP_ADDRESS, IGNORE_SHORT_PACKETS, 1, &polIgnoreShortPackets);
		if (!success)
		{
			errorCode = GetLastError();
			printf("SetPipePolicy failed. ErrorCode: %08Xh\n",  errorCode);
			goto Done;
		}
	}
#endif

#ifdef USE_RAW_IO
	success = Usb.SetPipePolicy(usbHandle, EP_ADDRESS, RAW_IO, 1, &polRawIO);
	if (!success)
	{
		errorCode = GetLastError();
		printf("SetPipePolicy failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}
#endif

#ifdef USE_SHORT_PACKET_TERMINATE
	if (USB_ENDPOINT_DIRECTION_OUT(EP_ADDRESS))
	{
		success = Usb.SetPipePolicy(usbHandle, EP_ADDRESS, SHORT_PACKET_TERMINATE, 1, &polShortPacketTerminate);
		if (!success)
		{
			errorCode = GetLastError();
			printf("SetPipePolicy failed. ErrorCode: %08Xh\n",  errorCode);
			goto Done;
		}
	}
#endif

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

