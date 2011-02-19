/* libusb-win32 WDF, Generic KMDF Windows USB Driver
 * Copyright (c) 2010-2011 Travis Robinson <libusbdotnet@gmail.com>
 * Copyright (c) 2002-2005 Stephan Meyer <ste_meyer@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __DRV_INTERFACE_H__
#define __DRV_INTERFACE_H__

#include "drv_private.h"

PINTERFACE_CONTEXT Interface_GetContextByNumber(
    __in PDEVICE_CONTEXT deviceContext,
    __in UCHAR interfaceNumber);

PINTERFACE_CONTEXT Interface_GetContextByIndex(
    __in PDEVICE_CONTEXT deviceContext,
    __in UCHAR interfaceIndex);

NTSTATUS Interface_Start(
    __in PDEVICE_CONTEXT deviceContext,
    __in PINTERFACE_CONTEXT interfaceContext);

NTSTATUS Interface_InitContext(
    __in PDEVICE_CONTEXT deviceContext,
    __in PINTERFACE_CONTEXT interfaceContext);

NTSTATUS Interface_SetAltSetting(
    __in  PDEVICE_CONTEXT deviceContext,
    __in  PREQUEST_CONTEXT requestContext);

NTSTATUS Interface_GetAltSetting(
    __in PDEVICE_CONTEXT deviceContext,
    __in PREQUEST_CONTEXT requestContext,
    __out PUCHAR altNumberOrIndex);

NTSTATUS Interface_Claim(
    __in PDEVICE_CONTEXT deviceContext,
    __in PREQUEST_CONTEXT requestContext,
    __in PFILE_OBJECT fileObject);

NTSTATUS Interface_Release(
    __in PDEVICE_CONTEXT deviceContext,
    __in PREQUEST_CONTEXT requestContext,
    __in PFILE_OBJECT fileObject);

NTSTATUS Interface_ReleaseAll(
    __in  PDEVICE_CONTEXT deviceContext,
    __in  PFILE_OBJECT fileObject);

NTSTATUS Interface_QuerySettings(__in  PDEVICE_CONTEXT deviceContext,
                                 __in UCHAR intfIndex,
                                 __in UCHAR altIndex,
                                 __out PUSB_INTERFACE_DESCRIPTOR descriptor);

FORCEINLINE NTSTATUS GetInterfaceAltSettingIndexFromRequest(
    __in PREQUEST_CONTEXT requestContext,
    __in PINTERFACE_CONTEXT interfaceContext,
    __out_opt PUCHAR altSettingIndex,
    __out_opt PUCHAR altSettingCount,
    __out_opt PUSB_INTERFACE_DESCRIPTOR altSettingInterfaceDescriptor)
{
	UCHAR altCount = WdfUsbInterfaceGetNumSettings(interfaceContext->Interface);
	UCHAR altIndex = BYTE_MAX;
	USB_INTERFACE_DESCRIPTOR interfaceDescriptor = {0};

	if (requestContext->IoControlRequest.intf.useAltSettingIndex)
	{
		if (requestContext->IoControlRequest.intf.altsettingIndex >= altCount)
		{
			USBERR("alternate interface index %u does not exist\n",
			       requestContext->IoControlRequest.intf.altsettingIndex);
			return STATUS_INVALID_PARAMETER;
		}
		altIndex = requestContext->IoControlRequest.intf.altsettingIndex;
		WdfUsbInterfaceGetDescriptor(interfaceContext->Interface, altIndex, &interfaceDescriptor);

	}
	else
	{
		UCHAR altPos;
		for (altPos = 0; altPos < altCount; altPos++)
		{
			WdfUsbInterfaceGetDescriptor(interfaceContext->Interface, altPos, &interfaceDescriptor);
			if (interfaceDescriptor.bAlternateSetting == requestContext->IoControlRequest.intf.altsetting_number)
			{
				altIndex = altPos;
				break;
			}
		}
	}

	if (altIndex == BYTE_MAX)
	{
		USBERR("alternate interface number %u does not exist\n",
		       requestContext->IoControlRequest.intf.altsetting_number);
		return STATUS_INVALID_PARAMETER;
	}

	if (altSettingIndex)
		*altSettingIndex = altIndex;

	if (altSettingCount)
		*altSettingCount = altCount;

	if (altSettingInterfaceDescriptor)
		RtlCopyMemory(altSettingInterfaceDescriptor, &interfaceDescriptor, sizeof(*altSettingInterfaceDescriptor));

	return STATUS_SUCCESS;
}

FORCEINLINE NTSTATUS GetInterfaceContextFromRequest(
    __in  PDEVICE_CONTEXT deviceContext,
    __in  PREQUEST_CONTEXT requestContext,
    __out PINTERFACE_CONTEXT* interfaceContext)
{
	if (!deviceContext->UsbConfigurationDescriptor)
	{
		USBERR("device is not configured\n");
		return STATUS_INVALID_DEVICE_STATE;
	}

	if (requestContext->IoControlRequest.intf.useInterfaceIndex)
	{
		UCHAR intfIndex = (UCHAR)requestContext->IoControlRequest.intf.interfaceIndex;
		*interfaceContext = Interface_GetContextByIndex(deviceContext, intfIndex);
		if (!(*interfaceContext))
		{
			USBERR("unable to get interface index %u\n", intfIndex);
			return STATUS_INVALID_PARAMETER;
		}
	}
	else
	{
		UCHAR intfNumber = (UCHAR)requestContext->IoControlRequest.intf.interface_number;
		*interfaceContext = Interface_GetContextByNumber(deviceContext, intfNumber);
		if (!(*interfaceContext))
		{
			USBERR("unable to get interface number %u\n", intfNumber);
			return STATUS_INVALID_PARAMETER;
		}
	}

	return (*interfaceContext) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

#endif