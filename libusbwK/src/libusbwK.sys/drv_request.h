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

#ifndef __DRV_REQUEST_H__
#define __DRV_REQUEST_H__

#include "drv_private.h"

#define GetRequestPipeID(RequestContext) (RequestContext->PipeContext->PipeInformation.EndpointAddress)

#define VALIDATE_REQUEST_CONTEXT(RequestContext, NtStatusVar)										\
	if (!RequestContext)																			\
		{ NtStatusVar = STATUS_INVALID_DEVICE_REQUEST; USBERR("invalid request context\n"); }		\
	else if (!RequestContext->PipeContext)															\
		{ NtStatusVar = STATUS_PIPE_BROKEN; USBERR("invalid pipe context\n"); }						\
	else if (!RequestContext->PipeContext->IsValid)													\
		{ NtStatusVar = STATUS_PIPE_BUSY; USBERR("busy or non-existent pipe\n"); }					\
	else																							\
		NtStatusVar = STATUS_SUCCESS


#define GetRequestDataPtr(LibusbRequestPtr) (&(((PUCHAR)LibusbRequestPtr)[sizeof(libusb_request)]))

#define FormatDescriptorRequestAsControlTransfer(requestContext, setDescriptor)												\
	WDF_USB_CONTROL_SETUP_PACKET_INIT(																						\
	(PWDF_USB_CONTROL_SETUP_PACKET)&requestContext->IoControlRequest.control,												\
	BmRequestDeviceToHost,																									\
	requestContext->IoControlRequest.descriptor.recipient,																	\
	setDescriptor ? USB_REQUEST_SET_DESCRIPTOR : USB_REQUEST_GET_DESCRIPTOR,												\
	(USHORT)((requestContext->IoControlRequest.descriptor.type << 8) | requestContext->IoControlRequest.descriptor.index),	\
	(USHORT)requestContext->IoControlRequest.descriptor.language_id)

NTSTATUS Request_Descriptor(__in PDEVICE_CONTEXT deviceContext,
                            __in PWDF_MEMORY_DESCRIPTOR memoryDescriptor,
                            __in BOOLEAN setDescriptor,
                            __in WDF_USB_BMREQUEST_RECIPIENT recipient,
                            __in UCHAR descriptorType,
                            __in UCHAR descriptorIndex,
                            __in USHORT langID,
                            __out PULONG descriptorLength,
                            __in INT timeout);

NTSTATUS Request_Status(__in PDEVICE_CONTEXT deviceContext,
                        __in PWDF_MEMORY_DESCRIPTOR memoryDescriptor,
                        __in WDF_USB_BMREQUEST_RECIPIENT recipient,
                        __in USHORT index,
                        __in INT timeout,
                        __out PULONG transferred);

NTSTATUS Request_InitContext(__inout PREQUEST_CONTEXT requestContext,
                             __in WDFREQUEST request,
                             __in_opt libusb_request* libusbRequest,
                             __in_opt WDF_REQUEST_TYPE actualRequestType,
                             __in_opt ULONG ioControlCode,
                             __in_opt ULONG outputBufferLength,
                             __in_opt ULONG inputBufferLength);

NTSTATUS Request_Feature(__in PDEVICE_CONTEXT deviceContext,
                         __in WDF_USB_BMREQUEST_RECIPIENT recipient,
                         __in USHORT featureIndex,
                         __in USHORT feature,
                         __in BOOLEAN setFeature,
                         __in INT timeout);

NTSTATUS Request_Vendor(__in PDEVICE_CONTEXT deviceContext,
                        __in PWDF_MEMORY_DESCRIPTOR memoryDescriptor,
                        __in WDF_USB_BMREQUEST_DIRECTION direction,
                        __in WDF_USB_BMREQUEST_RECIPIENT recipient,
                        __in BYTE request,
                        __in USHORT value,
                        __in USHORT index,
                        __in INT timeout,
                        __out PULONG transferred);

#endif