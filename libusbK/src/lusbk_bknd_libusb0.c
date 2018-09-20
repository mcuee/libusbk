/*!********************************************************************
libusbK - Multi-driver USB library.
Copyright (C) 2012 Travis Lee Robinson. All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Lee Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen         (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/

#include "lusbk_private.h"
#include "lusbk_handles.h"
#include "lusbk_stack_collection.h"
#include "lusbk_bknd_libusb0.h"

/*
 * Standard requests
 */

#define USB_TYPE_STANDARD		(0x00 << 5)
#define USB_TYPE_CLASS			(0x01 << 5)
#define USB_TYPE_VENDOR			(0x02 << 5)

/*
 * Various libusb API related stuff
 */

#define USB_ENDPOINT_IN			0x80
#define USB_ENDPOINT_OUT		0x00

#define LIBUSB_DEFAULT_TIMEOUT	1000

KUSB_EXP BOOL KUSB_API LUsb0_ControlTransfer(
	_in KUSB_HANDLE InterfaceHandle,
	_in WINUSB_SETUP_PACKET SetupPacket,
	_refopt PUCHAR Buffer,
	_in UINT BufferLength,
	_outopt PUINT LengthTransferred,
	_inopt LPOVERLAPPED Overlapped)
{
	BOOL success;
	int ret;
	PKUSB_HANDLE_INTERNAL handle;

	UNUSED(Overlapped);

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	ret = usb_control_msg(Dev_Handle(), SetupPacket.RequestType, SetupPacket.Request, SetupPacket.Value, SetupPacket.Index, Buffer, BufferLength, LIBUSB_DEFAULT_TIMEOUT);

	if (ret >= 0)
	{
		*LengthTransferred = ret;
		success = TRUE;
	}
	else
	{
		success = FALSE;
	}

	PoolHandle_Dec_UsbK(handle);

	return success;
}

KUSB_EXP BOOL KUSB_API LUsb0_SetConfiguration(
	_in KUSB_HANDLE InterfaceHandle,
	_in UCHAR ConfigurationNumber)
{
	PKUSB_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_UsbK(InterfaceHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	if (!usb_set_configuration((HANDLE*)handle, ConfigurationNumber))
	{
		USBERRN("failed setting configuration #%d", ConfigurationNumber);
		goto Error;
	}

	// rebuild the interface list.
	success = UsbStack_Rebuild(handle, k_Init_Config);
	ErrorNoSet(!success, Error, "->UsbStack_Rebuild");

	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

// See libusb-win32 windows.c:217-254
int usb_set_configuration(HANDLE *dev, int configuration)
{
	libusb_request req;

	if (!dev)
	{
		USBERR("device not open\n");
		return FALSE;
	}

	req.configuration.configuration = configuration;
	req.timeout = LIBUSB_DEFAULT_TIMEOUT;

	if (!_usb_io_sync(dev, LIBUSB_IOCTL_SET_CONFIGURATION,
		&req, sizeof(libusb_request), NULL, 0, NULL))
	{
		USBERR("could not set config %d: ", configuration);
		return FALSE;
	}

	return TRUE;
}

// See libusb-win32 windows.c:671-815
int usb_control_msg(HANDLE *dev, int requesttype, int request,
	int value, int index, PUCHAR bytes, int size, int timeout)
{
	int read = 0;
	libusb_request req;
	void *out = &req;
	int out_size = sizeof(libusb_request);
	void *in = bytes;
	int in_size = size;
	int code;

	if (!dev)
	{
		USBERR("device not open\n");
		return -EINVAL;
	}

	req.timeout = timeout;

	/* windows doesn't support generic control messages, so it needs to be */
	/* split up */
	switch (requesttype & (0x03 << 5))
	{
	case USB_TYPE_STANDARD:
		switch (request)
		{
		case USB_REQUEST_GET_STATUS:
			req.status.recipient = requesttype & 0x1F;
			req.status.index = index;
			code = LIBUSB_IOCTL_GET_STATUS;
			break;

		case USB_REQUEST_CLEAR_FEATURE:
			req.feature.recipient = requesttype & 0x1F;
			req.feature.feature = value;
			req.feature.index = index;
			code = LIBUSB_IOCTL_CLEAR_FEATURE;
			break;

		case USB_REQUEST_SET_FEATURE:
			req.feature.recipient = requesttype & 0x1F;
			req.feature.feature = value;
			req.feature.index = index;
			code = LIBUSB_IOCTL_SET_FEATURE;
			break;

		case USB_REQUEST_GET_DESCRIPTOR:
			req.descriptor.recipient = requesttype & 0x1F;
			req.descriptor.type = (value >> 8) & 0xFF;
			req.descriptor.index = value & 0xFF;
			req.descriptor.language_id = index;
			code = LIBUSB_IOCTL_GET_DESCRIPTOR;
			break;

		case USB_REQUEST_SET_DESCRIPTOR:
			req.descriptor.recipient = requesttype & 0x1F;
			req.descriptor.type = (value >> 8) & 0xFF;
			req.descriptor.index = value & 0xFF;
			req.descriptor.language_id = index;
			code = LIBUSB_IOCTL_SET_DESCRIPTOR;
			break;

		case USB_REQUEST_GET_CONFIGURATION:
			code = LIBUSB_IOCTL_GET_CONFIGURATION;
			break;

		case USB_REQUEST_SET_CONFIGURATION:
			req.configuration.configuration = value;
			code = LIBUSB_IOCTL_SET_CONFIGURATION;
			break;

		case USB_REQUEST_GET_INTERFACE:
			req.intf.interface_number = index;
			code = LIBUSB_IOCTL_GET_INTERFACE;
			break;

		case USB_REQUEST_SET_INTERFACE:
			req.intf.interface_number = index;
			req.intf.altsetting_number = value;
			code = LIBUSB_IOCTL_SET_INTERFACE;
			break;

		default:
			USBERR("invalid request 0x%x", request);
			return -EINVAL;
		}
		break;

	case USB_TYPE_VENDOR:
	case USB_TYPE_CLASS:

		req.vendor.type = (requesttype >> 5) & 0x03;
		req.vendor.recipient = requesttype & 0x1F;
		req.vendor.request = request;
		req.vendor.value = value;
		req.vendor.index = index;

		if (requesttype & 0x80)
			code = LIBUSB_IOCTL_VENDOR_READ;
		else
			code = LIBUSB_IOCTL_VENDOR_WRITE;
		break;

	default:
		USBERR("invalid or unsupported request type: %x",
			requesttype);
		return -EINVAL;
	}

	/* out request? */
	if (!(requesttype & USB_ENDPOINT_IN))
	{
		out = malloc(sizeof(libusb_request) + size);
		if (!out)
		{
			USBERR("memory allocation failed\n");
			return -ENOMEM;
		}

		memcpy(out, &req, sizeof(libusb_request));
		memcpy((char *)out + sizeof(libusb_request), bytes, size);
		out_size = sizeof(libusb_request) + size;
		in = NULL;
		in_size = 0;
	}

	if (!_usb_io_sync(dev, code, out, out_size, in, in_size, &read))
	{
		USBERR("sending control message failed");
		if (!(requesttype & USB_ENDPOINT_IN))
		{
			free(out);
		}
		return -1;
	}

	/* out request? */
	if (!(requesttype & USB_ENDPOINT_IN))
	{
		free(out);
		return size;
	}
	else
		return read;
}

int _usb_io_sync(HANDLE dev, unsigned int code, void *out, int out_size,
	void *in, int in_size, int *ret)
{
	OVERLAPPED ol;
	DWORD _ret;

	memset(&ol, 0, sizeof(ol));

	if (ret)
		*ret = 0;

	ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!ol.hEvent)
		return FALSE;

	if (!DeviceIoControl(dev, code, out, out_size, in, in_size, NULL, &ol))
	{
		if (GetLastError() != ERROR_IO_PENDING)
		{
			CloseHandle(ol.hEvent);
			return FALSE;
		}
	}

	if (GetOverlappedResult(dev, &ol, &_ret, TRUE))
	{
		if (ret)
			*ret = (int)_ret;
		CloseHandle(ol.hEvent);
		return TRUE;
	}

	CloseHandle(ol.hEvent);
	return FALSE;
}