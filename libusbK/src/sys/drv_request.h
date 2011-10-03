/*! \file drv_request.h
*/

#ifndef __DRV_REQUEST_H__
#define __DRV_REQUEST_H__

#include "drv_private.h"

#define VALIDATE_REQUEST_CONTEXT(RequestContext, NtStatusVar)										\
	if (!RequestContext)																			\
		{ NtStatusVar = STATUS_INVALID_DEVICE_REQUEST; USBERRN("Invalid Request Context"); }		\
	else if (!RequestContext->QueueContext)															\
		{ NtStatusVar = STATUS_PIPE_BROKEN; USBERRN("Invalid Queue Context."); }					\
	else																							\
		NtStatusVar = STATUS_SUCCESS


#define GetRequestDataPtr(LibusbRequestPtr) (&(((PUCHAR)LibusbRequestPtr)[sizeof(libusb_request)]))

#define FormatDescriptorRequestAsControlTransfer(mRequestContext, mLibUsbReqPtr, mIsSetDescriptor) 					\
	WDF_USB_CONTROL_SETUP_PACKET_INIT(																				\
	(PWDF_USB_CONTROL_SETUP_PACKET)&((mRequestContext)->IoControlRequest.control), 									\
	(mIsSetDescriptor) ? BmRequestHostToDevice : BmRequestDeviceToHost,   											\
	(WDF_USB_BMREQUEST_RECIPIENT)(mLibUsbReqPtr)->descriptor.recipient,   											\
	(mIsSetDescriptor) ? USB_REQUEST_SET_DESCRIPTOR : USB_REQUEST_GET_DESCRIPTOR, 									\
	(USHORT)(((((mLibUsbReqPtr)->descriptor.type) << 8) & 0xFF00) | (((mLibUsbReqPtr)->descriptor.index) & 0xFF)),	\
	(USHORT)((mLibUsbReqPtr)->descriptor.language_id))																\
 
#define FormatVendorRequestAsControlTransfer(mRequestContext, mLibUsbReqPtr, mIsVendorWrite)   						\
	WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR( 																		\
	(PWDF_USB_CONTROL_SETUP_PACKET)&((mRequestContext)->IoControlRequest.control), 									\
	(mIsVendorWrite) ? BmRequestHostToDevice : BmRequestDeviceToHost, 												\
	(WDF_USB_BMREQUEST_RECIPIENT)(mLibUsbReqPtr)->vendor.recipient,   												\
	(BYTE)  (mLibUsbReqPtr)->vendor.request,																		\
	(USHORT)(mLibUsbReqPtr)->vendor.value,																			\
	(USHORT)(mLibUsbReqPtr)->vendor.index)																			\
 
#define FormatFeatureRequestAsControlTransfer(mRequestContext,mLibUsbReqPtr,mIsSetFeature) 							\
	WDF_USB_CONTROL_SETUP_PACKET_INIT_FEATURE(																		\
	(PWDF_USB_CONTROL_SETUP_PACKET)&((mRequestContext)->IoControlRequest.control), 									\
		(WDF_USB_BMREQUEST_RECIPIENT) (mLibUsbReqPtr)->feature.recipient,   										\
		(USHORT) (mLibUsbReqPtr)->feature.feature,  																\
		(USHORT) (mLibUsbReqPtr)->feature.index,																	\
		mIsSetFeature)

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