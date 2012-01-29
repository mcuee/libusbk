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

// Globals:
KUSB_DRIVER_API Usb;

DWORD __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;			// device list handle (the list of device infos)
	KLST_DEVINFO_HANDLE deviceInfo = NULL;	// device info handle (the device list element)
	KUSB_HANDLE usbHandle = NULL;				// device interface usbHandle (the opened USB device)
	DWORD errorCode	= ERROR_SUCCESS;
	BOOL success;
	USB_DEVICE_DESCRIPTOR deviceDescriptor;
	UCHAR vendorBuffer[8 + 1];
	KUSB_SETUP_PACKET setupPacket;

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
	Use a standard control transfer to get the device descriptor. (DeviceToHost)
	*/

	// Setup packets are always 8 bytes (64 bits)
	*((__int64*)&setupPacket) = 0;

	// Fill the setup packet.
	setupPacket.BmRequest.Dir		= BMREQUEST_DIR_DEVICE_TO_HOST;
	setupPacket.BmRequest.Type		= BMREQUEST_TYPE_STANDARD;
	setupPacket.BmRequest.Recipient = BMREQUEST_RECIPIENT_DEVICE;
	setupPacket.Value				= USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(USB_DESCRIPTOR_TYPE_DEVICE, 0);
	setupPacket.Request				= USB_REQUEST_GET_DESCRIPTOR;
	setupPacket.Length				= sizeof(deviceDescriptor);
	success = UsbK_ControlTransfer(usbHandle, *((WINUSB_SETUP_PACKET*)&setupPacket), (PUCHAR)&deviceDescriptor, sizeof(deviceDescriptor), NULL, NULL);
	if (!success)
	{
		errorCode = GetLastError();
		printf("UsbK_ControlTransfer failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;

	}

	/*
	Initialize the 8 byte benchmark vendor buffer
	*/
	memset(vendorBuffer, 0, sizeof(vendorBuffer));
	vendorBuffer[0] = 'A';
	vendorBuffer[1] = 'B';
	vendorBuffer[2] = 'C';

	/*
	Use a vendor control transfer to set the benchmark vendor buffer. (HostToDevice)
	*/
	setupPacket.BmRequest.Dir		= BMREQUEST_DIR_HOST_TO_DEVICE;
	setupPacket.BmRequest.Type		= BMREQUEST_TYPE_VENDOR;
	setupPacket.BmRequest.Recipient = BMREQUEST_RECIPIENT_DEVICE;
	setupPacket.Request				= BM_COMMAND_SET_VBUF;
	setupPacket.Length				= sizeof(vendorBuffer) - 1;
	success = UsbK_ControlTransfer(usbHandle, *((WINUSB_SETUP_PACKET*)&setupPacket), vendorBuffer, sizeof(vendorBuffer) - 1, NULL, NULL);
	if (!success)
	{
		errorCode = GetLastError();
		printf("UsbK_ControlTransfer failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;

	}

	/*
	Print the value assigned to the benchmark vendor buffer.
	*/
	printf("vendorBuffer: %s=", vendorBuffer);
	memset(vendorBuffer, 0, sizeof(vendorBuffer));

	/*
	Use a vendor control transfer to get the benchmark vendor buffer. (DeviceToHost)
	*/

	// Setup packets are always 8 bytes (64 bits)
	*((__int64*)&setupPacket) = 0;
	setupPacket.BmRequest.Dir		= BMREQUEST_DIR_DEVICE_TO_HOST;
	setupPacket.BmRequest.Type		= BMREQUEST_TYPE_VENDOR;
	setupPacket.BmRequest.Recipient = BMREQUEST_RECIPIENT_DEVICE;
	setupPacket.Request				= BM_COMMAND_GET_VBUF;
	setupPacket.Length				= sizeof(vendorBuffer) - 1;
	success = UsbK_ControlTransfer(usbHandle, *((WINUSB_SETUP_PACKET*)&setupPacket), vendorBuffer, sizeof(vendorBuffer) - 1, NULL, NULL);
	if (!success)
	{
		errorCode = GetLastError();
		printf("UsbK_ControlTransfer failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;

	}

	/*
	Print the value stored in the benchmark vendor buffer.
	*/
	printf("%s\n", vendorBuffer);


Done:
	/*!
	Close the usb handle. If usbHandle is invalid (NULL), has no effect.
	*/
	if (usbHandle) Usb.Free(usbHandle);
	/*!
	Free the device list. If deviceList is invalid (NULL), has no effect.
	*/
	LstK_Free(deviceList);

	return errorCode;
}
/*!
Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
Device opened successfully!
vendorBuffer: ABC=ABC
*/
