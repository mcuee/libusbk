/*! \file drv_request.h
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
	(requestContext->RequestType==WdfRequestTypeWrite) ? BmRequestHostToDevice : BmRequestDeviceToHost,						\
	requestContext->IoControlRequest.descriptor.recipient,																	\
	setDescriptor ? USB_REQUEST_SET_DESCRIPTOR : USB_REQUEST_GET_DESCRIPTOR,												\
	(USHORT)((requestContext->IoControlRequest.descriptor.type << 8) | requestContext->IoControlRequest.descriptor.index),	\
	(USHORT)requestContext->IoControlRequest.descriptor.language_id)

#define FormatVendorRequestAsControlTransfer(requestContext)																\
	WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(																				\
	(PWDF_USB_CONTROL_SETUP_PACKET)&requestContext->IoControlRequest.control,												\
	(requestContext->RequestType==WdfRequestTypeWrite) ? BmRequestHostToDevice : BmRequestDeviceToHost,						\
	(WDF_USB_BMREQUEST_RECIPIENT)requestContext->IoControlRequest.vendor.recipient,											\
	(BYTE)requestContext->IoControlRequest.vendor.request,																	\
	(USHORT)requestContext->IoControlRequest.vendor.value,																	\
	(USHORT)requestContext->IoControlRequest.vendor.index)

#define FormatFeatureRequestAsControlTransfer(requestContext,SetFeature)													\
	WDF_USB_CONTROL_SETUP_PACKET_INIT_FEATURE(																				\
		(PWDF_USB_CONTROL_SETUP_PACKET)&requestContext->IoControlRequest.control,											\
		(WDF_USB_BMREQUEST_RECIPIENT) libusbRequest->feature.recipient,														\
		(USHORT)libusbRequest->feature.feature,																				\
		(USHORT) libusbRequest->feature.index,																				\
		SetFeature)

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

VOID Request_PreIoInitialize(__in WDFDEVICE Device,
                             __in WDFREQUEST Request);
#endif