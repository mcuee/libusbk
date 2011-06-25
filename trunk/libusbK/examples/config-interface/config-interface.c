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
#define INTF_NUMBER			0x00
#define ALT_SETTING_NUMBER	0x00
#define EP_READ				0x81
#define EP_WRITE			0x01

DWORD __cdecl main(int argc, char* argv[])
{
	PKLST_HANDLE deviceList = NULL;
	PKLST_DEV_INFO deviceInfo = NULL;
	LIBUSBK_INTERFACE_HANDLE handle = NULL;
	DWORD errorCode = ERROR_SUCCESS;
	WINUSB_PIPE_INFORMATION readInfo, writeInfo, pipeInfo;
	USB_INTERFACE_DESCRIPTOR interfaceInfo;
	UCHAR pipeIndex = 0;

	// Find the test device.  Uses "vid/pid=hhhh" arguments supplied
	// on the command line. (default is: vid=04D8 pid=FA2E)
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	// Open the device
	if (!UsbK_Open(deviceInfo, &handle))
	{
		errorCode = GetLastError();
		printf("Open device failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

	/*
	Claim the interface. You can use the claim/release interface functions
	cooperatively in libusbK application to ensure only one application is
	usng the interface at a time.
	*/
	if (!UsbK_ClaimInterface(handle, INTF_NUMBER, FALSE))
	{
		errorCode = GetLastError();
		if (errorCode == ERROR_NO_MORE_ITEMS)
			printf("Failed claiming interface. Interface number %02Xh does not exists.\n", INTF_NUMBER);
		else
			printf("Failed claiming interface. Win32Error=%u (0x%08X)\n", errorCode, errorCode);

		goto Done;
	}

	/*
	Get the interface descriptor for the specified alternate settings number.
	*/
	if (!UsbK_QueryInterfaceSettings(handle, ALT_SETTING_NUMBER, &interfaceInfo))
	{
		errorCode = GetLastError();
		if (errorCode == ERROR_NO_MORE_ITEMS)
			printf("Failed querying alt interface. Alt Setting number %02Xh does not exists.\n", ALT_SETTING_NUMBER);
		else
			printf("Failed querying alt interface. Win32Error=%u (0x%08X)\n", errorCode, errorCode);

		goto Done;
	}
	printf("Interface/alt setting number %02Xh/%02Xh found!\n",
	       interfaceInfo.bInterfaceNumber, interfaceInfo.bAlternateSetting);

	/*
	Get the pipe information for the specified read & write pipe IDs.
	*/
	memset(&readInfo, 0, sizeof(readInfo));
	memset(&writeInfo, 0, sizeof(writeInfo));
	while (UsbK_QueryPipe(handle, ALT_SETTING_NUMBER, pipeIndex++, &pipeInfo))
	{
		if (pipeInfo.PipeId == EP_READ)
			memcpy(&readInfo, &pipeInfo, sizeof(readInfo));
		else if (pipeInfo.PipeId == EP_WRITE)
			memcpy(&writeInfo, &pipeInfo, sizeof(writeInfo));
	}

	if (readInfo.PipeId == 0)
	{
		printf("Read pipe %02Xh not found.\n", EP_READ);
		goto Done;
	}
	else
		printf("Read pipe %02Xh found!\n", readInfo.PipeId);

	if (writeInfo.PipeId == 0)
	{
		printf("Write pipe %02Xh not found.\n", EP_WRITE);
		goto Done;
	}
	else
		printf("Write pipe %02Xh found!\n", writeInfo.PipeId);

	/*
	Set the alternate setting number.  This is only required if the device supports
	alternate settings, but it is a standard request and all usb devices must
	support it to be compliant with usb specs.
	*/
	if (!UsbK_SetCurrentAlternateSetting(handle, interfaceInfo.bAlternateSetting))
	{
		errorCode = GetLastError();
		printf("Failed setting alt interface. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}
	else
	{
		printf("Alternate setting %02Xh selected!\n", interfaceInfo.bAlternateSetting);
	}

	/*
	.. The device is open, configured and ready for use.
	*/

Done:
	// Close the device handle
	// if handle is invalid (NULL), has no effect
	UsbK_Close(handle);

	// Free the device list
	// if deviceList is invalid (NULL), has no effect
	LstK_Free(&deviceList);

	return errorCode;
}

/*!
Console Output:
  Looking for device vid/pid 04D8/FA2E..
  Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
  Device opened successfully!
  Interface/alt setting number 00h/00h found!
  Read pipe 81h found!
  Write pipe 01h found!
  Alternate setting 00h selected!
*/
