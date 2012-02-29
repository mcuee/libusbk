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

typedef struct _DESCRIPTOR_ITERATOR
{
	LONG	Remaining;

	union
	{
		PUCHAR							Bytes;

		PUSB_COMMON_DESCRIPTOR			Common;
		PUSB_CONFIGURATION_DESCRIPTOR	Config;
		PUSB_INTERFACE_DESCRIPTOR		Interface;
		PUSB_ENDPOINT_DESCRIPTOR		Endpoint;
	} Ptr;
} DESCRIPTOR_ITERATOR, *PDESCRIPTOR_ITERATOR;

/*
Initializes a DESCRIPTOR_ITERATOR structure.  Upon success, The iterator will point to the config descriptor itself.
*/
BOOL InitDescriptorIterator(PDESCRIPTOR_ITERATOR descIterator, BYTE* configDescriptor, DWORD lengthTransferred);
/*
Advances a DESCRIPTOR_ITERATOR structure.  Upon success, The iterator will point to the next descriptor.
*/
BOOL NextDescriptor(PDESCRIPTOR_ITERATOR descIterator);

/*
Console display routines.
*/
void PrintConfigDescriptor(PUSB_CONFIGURATION_DESCRIPTOR desc);
void PrintInterfaceDescriptor(PUSB_INTERFACE_DESCRIPTOR desc);
void PrintEndpointDescriptor(PUSB_ENDPOINT_DESCRIPTOR desc);

// Globals:
KUSB_DRIVER_API Usb;

BOOL InitDescriptorIterator(PDESCRIPTOR_ITERATOR descIterator, BYTE* configDescriptor, DWORD lengthTransferred)
{
	memset(descIterator, 0, sizeof(descIterator));
	descIterator->Ptr.Bytes		= configDescriptor;
	descIterator->Remaining		= descIterator->Ptr.Config->wTotalLength;

	if (lengthTransferred > sizeof(USB_CONFIGURATION_DESCRIPTOR) && lengthTransferred >= descIterator->Ptr.Config->wTotalLength)
	{
		if ( descIterator->Ptr.Config->wTotalLength >= sizeof(USB_CONFIGURATION_DESCRIPTOR) + sizeof(USB_INTERFACE_DESCRIPTOR))
			return TRUE;
	}

	SetLastError(ERROR_BAD_LENGTH);
	descIterator->Remaining = 0;
	return FALSE;
}

BOOL NextDescriptor(PDESCRIPTOR_ITERATOR descIterator)
{
	if (descIterator->Remaining >= sizeof(USB_COMMON_DESCRIPTOR) && descIterator->Remaining >= descIterator->Ptr.Common->bLength)
	{
		descIterator->Remaining -= descIterator->Ptr.Common->bLength;
		if (descIterator->Remaining >= sizeof(USB_COMMON_DESCRIPTOR))
		{
			descIterator->Ptr.Bytes += descIterator->Ptr.Common->bLength;
			return TRUE;
		}
	}
	descIterator->Remaining = 0;
	SetLastError(ERROR_NO_MORE_ITEMS);
	return FALSE;
}

DWORD __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;			// device list handle (the list of device infos)
	KLST_DEVINFO_HANDLE deviceInfo = NULL;	// device info handle (the device list element)
	KUSB_HANDLE usbHandle = NULL;				// device interface usbHandle (the opened USB device)
	DWORD errorCode = ERROR_SUCCESS;
	WINUSB_SETUP_PACKET Pkt;
	KUSB_SETUP_PACKET* kPkt = (KUSB_SETUP_PACKET*)&Pkt;
	BYTE configDescriptorBuffer[4096];
	DWORD lengthTransferred;
	DESCRIPTOR_ITERATOR descIterator;

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

	memset(&Pkt, 0, sizeof(Pkt));
	kPkt->BmRequest.Dir = BMREQUEST_DIR_DEVICE_TO_HOST;
	kPkt->Request = USB_REQUEST_GET_DESCRIPTOR;
	kPkt->ValueHi = USB_DESCRIPTOR_TYPE_CONFIGURATION;
	kPkt->ValueLo = 0; // Index
	kPkt->Length = (USHORT)sizeof(configDescriptorBuffer);

	/*
	Fetch the devices complete config descriptor.  This will send an I/O request directly to the device.
	*/
	if (!Usb.ControlTransfer(usbHandle, Pkt, configDescriptorBuffer, sizeof(configDescriptorBuffer), &lengthTransferred, NULL))
	{
		errorCode = GetLastError();
		printf("ControlTransfer failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}

	/*
	Initialize the descriptor iterator.
	*/
	if (!InitDescriptorIterator(&descIterator, configDescriptorBuffer, lengthTransferred))
	{
		errorCode = GetLastError();
		printf("InitDescriptorIterator failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}

	/*
	Initially, the iterator will point to the config descriptor.
	*/
	PrintConfigDescriptor(descIterator.Ptr.Config);

	/*
	Continue advancing the iterator while it returns true.
	*/
	while(NextDescriptor(&descIterator))
	{
		switch (descIterator.Ptr.Common->bDescriptorType)
		{
		case USB_DESCRIPTOR_TYPE_INTERFACE:
			PrintInterfaceDescriptor(descIterator.Ptr.Interface);
			break;
		case USB_DESCRIPTOR_TYPE_ENDPOINT:
			PrintEndpointDescriptor(descIterator.Ptr.Endpoint);
			break;

		}
	}


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


void PrintConfigDescriptor(PUSB_CONFIGURATION_DESCRIPTOR desc)
{
	printf("[Config Descriptor]\n");

	printf("  bLength             :%d\n", desc->bLength);
	printf("  bDescriptorType     :0x%02X\n", desc->bDescriptorType);

	printf("  wTotalLength        :%d\n", desc->wTotalLength);
	printf("  bNumInterfaces      :%d\n", desc->bNumInterfaces);
	printf("  bConfigurationValue :%d\n", desc->bConfigurationValue);
	printf("  iConfiguration      :%d\n", desc->iConfiguration);
	printf("  bmAttributes        :0x%02X\n", desc->bmAttributes);
	printf("  MaxPower            :%d\n", desc->MaxPower);

}

void PrintInterfaceDescriptor(PUSB_INTERFACE_DESCRIPTOR desc)
{
	printf("  [Interface Descriptor]\n");
	printf("    bLength            :%d\n", desc->bLength);
	printf("    bDescriptorType    :0x%02X\n", desc->bDescriptorType);

	printf("    bInterfaceNumber   :%d\n", desc->bInterfaceNumber);
	printf("    bAlternateSetting  :%d\n", desc->bAlternateSetting);
	printf("    bNumEndpoints      :%d\n", desc->bNumEndpoints);
	printf("    bInterfaceClass    :0x%02X\n", desc->bInterfaceClass);
	printf("    bInterfaceSubClass :0x%02X\n", desc->bInterfaceSubClass);
	printf("    bInterfaceProtocol :0x%02X\n", desc->bInterfaceProtocol);
	printf("    iInterface         :%d\n", desc->iInterface);
}

void PrintEndpointDescriptor(PUSB_ENDPOINT_DESCRIPTOR desc)
{
	printf("    [Endpoint Descriptor]\n");
	printf("      bLength          :%d\n", desc->bLength);
	printf("      bDescriptorType  :0x%02X\n", desc->bDescriptorType);

	printf("      bEndpointAddress :%d\n", desc->bEndpointAddress);
	printf("      bmAttributes     :%d\n", desc->bmAttributes);
	printf("      wMaxPacketSize   :%d\n", desc->wMaxPacketSize);
	printf("      bInterval        :0x%02X\n", desc->bInterval);
}
