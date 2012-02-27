/*!********************************************************************
libusbK - WDF USB driver.
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

#include "drv_common.h"

#if (defined(ALLOC_PRAGMA) && defined(PAGING_ENABLED))
#pragma alloc_text(PAGE, Request_Feature)
#pragma alloc_text(PAGE, Request_Vendor)
#pragma alloc_text(PAGE, Request_Status)
#pragma alloc_text(PAGE, Request_Descriptor)
#endif

NTSTATUS Request_Feature(__in PDEVICE_CONTEXT deviceContext,
                         __in WDF_USB_BMREQUEST_RECIPIENT recipient,
                         __in USHORT featureIndex,
                         __in USHORT feature,
                         __in BOOLEAN setFeature,
                         __in INT timeout)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;
	WDF_REQUEST_SEND_OPTIONS  syncReqOptions;

	PAGED_CODE();

	if (timeout > LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT || !timeout)
	{
		timeout = LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT;
	}

	USBMSG("[%s] recipient=%04d featureIndex=%04d feature=%04d timeout=%d\n",
	       setFeature ? "Set" : "Clear", recipient, featureIndex, feature, timeout);

	WDF_USB_CONTROL_SETUP_PACKET_INIT_FEATURE(&controlSetupPacket, recipient, feature, featureIndex, setFeature);

	WDF_REQUEST_SEND_OPTIONS_INIT(&syncReqOptions, 0);
	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&syncReqOptions, WDF_REL_TIMEOUT_IN_MS(timeout));

	status = WdfUsbTargetDeviceSendControlTransferSynchronously(
	             deviceContext->WdfUsbTargetDevice,
	             WDF_NO_HANDLE,
	             &syncReqOptions,
	             &controlSetupPacket,
	             NULL,
	             NULL);

	if (!NT_SUCCESS(status))
	{
		USBERR("failed status=%Xh\n", status);
	}
	return status;
}

NTSTATUS Request_Vendor(__in PDEVICE_CONTEXT deviceContext,
                        __in PWDF_MEMORY_DESCRIPTOR memoryDescriptor,
                        __in WDF_USB_BMREQUEST_DIRECTION direction,
                        __in WDF_USB_BMREQUEST_RECIPIENT recipient,
                        __in BYTE request,
                        __in USHORT value,
                        __in USHORT index,
                        __in INT timeout,
                        __out PULONG transferred)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;
	WDF_REQUEST_SEND_OPTIONS  syncReqOptions;

	PAGED_CODE();

	if (timeout > LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT || !timeout)
	{
		timeout = LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT;
	}

	USBMSG("direction=%u recipient=%04d request=%04d value=%04d index=%04d timeout=%d\n",
	       direction, recipient, request, value, index, timeout);

	WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(
	    &controlSetupPacket,
	    direction,
	    recipient,
	    request,
	    value,
	    index);

	WDF_REQUEST_SEND_OPTIONS_INIT(&syncReqOptions, 0);
	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&syncReqOptions, WDF_REL_TIMEOUT_IN_MS(timeout));

	status = WdfUsbTargetDeviceSendControlTransferSynchronously(
	             deviceContext->WdfUsbTargetDevice,
	             WDF_NO_HANDLE,
	             &syncReqOptions,
	             &controlSetupPacket,
	             memoryDescriptor,
	             transferred);

	if (!NT_SUCCESS(status))
	{
		USBERR("failed status=%Xh\n", status);
	}
	return status;
}

NTSTATUS Request_Status(__in PDEVICE_CONTEXT deviceContext,
                        __in PWDF_MEMORY_DESCRIPTOR memoryDescriptor,
                        __in WDF_USB_BMREQUEST_RECIPIENT recipient,
                        __in USHORT index,
                        __in INT timeout,
                        __out PULONG transferred)
{
	NTSTATUS						status = STATUS_SUCCESS;
	WDF_USB_CONTROL_SETUP_PACKET	controlSetupPacket;
	WDF_REQUEST_SEND_OPTIONS		syncReqOptions;

	PAGED_CODE();

	if (timeout > LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT || !timeout)
	{
		timeout = LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT;
	}

	USBMSG("recipient=%04d index=%04d timeout=%d\n", recipient, index, timeout);

	WDF_USB_CONTROL_SETUP_PACKET_INIT_GET_STATUS(&controlSetupPacket, recipient, index);

	WDF_REQUEST_SEND_OPTIONS_INIT(&syncReqOptions, 0);
	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&syncReqOptions, WDF_REL_TIMEOUT_IN_MS(timeout));

	status = WdfUsbTargetDeviceSendControlTransferSynchronously(
	             deviceContext->WdfUsbTargetDevice,
	             WDF_NO_HANDLE,
	             &syncReqOptions,
	             &controlSetupPacket,
	             memoryDescriptor,
	             transferred);

	if (!NT_SUCCESS(status))
	{
		USBERR("failed status=%Xh\n", status);
	}
	return status;
}

NTSTATUS Request_Descriptor(__in PDEVICE_CONTEXT deviceContext,
                            __in PWDF_MEMORY_DESCRIPTOR memoryDescriptor,
                            __in BOOLEAN setDescriptor,
                            __in WDF_USB_BMREQUEST_RECIPIENT recipient,
                            __in UCHAR descriptorType,
                            __in UCHAR descriptorIndex,
                            __in USHORT langID,
                            __out PULONG descriptorLength,
                            __in INT timeout)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;
	WDF_REQUEST_SEND_OPTIONS  syncReqOptions;

	PAGED_CODE();

	if (timeout > LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT || !timeout)
	{
		timeout = LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT;
	}

	USBMSG("[%s] recipient=%04d type=%04d index=%04d langID=%04d timeout=%d\n",
	       setDescriptor ? "Set" : "Get", recipient, descriptorType, descriptorIndex, langID, timeout);

	WDF_USB_CONTROL_SETUP_PACKET_INIT(
	    &controlSetupPacket,
	    BmRequestDeviceToHost,
	    recipient,
	    setDescriptor ? USB_REQUEST_SET_DESCRIPTOR : USB_REQUEST_GET_DESCRIPTOR,
	    (USHORT)((descriptorType << 8) | descriptorIndex),
	    (USHORT)langID);


	WDF_REQUEST_SEND_OPTIONS_INIT(&syncReqOptions, 0);
	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&syncReqOptions, WDF_REL_TIMEOUT_IN_MS(timeout));

	status = WdfUsbTargetDeviceSendControlTransferSynchronously(
	             deviceContext->WdfUsbTargetDevice,
	             WDF_NO_HANDLE,
	             &syncReqOptions,
	             &controlSetupPacket,
	             memoryDescriptor,
	             descriptorLength);

	if (!NT_SUCCESS(status))
	{
		USBERR("failed status=%Xh\n", status);
	}
	return status;
}
/*
NTSTATUS Request_InitContext(__inout PREQUEST_CONTEXT requestContext,
                             __in WDFREQUEST request,
                             __in_opt libusb_request* libusbRequest,
                             __in_opt WDF_REQUEST_TYPE actualRequestType,
                             __in_opt ULONG ioControlCode,
                             __in_opt ULONG outputBufferLength,
                             __in_opt ULONG inputBufferLength)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(request);

	// store the actual libusb_request in the WdfRequest context space
	if (libusbRequest)
	{
		RtlCopyMemory(&requestContext->IoControlRequest, libusbRequest, sizeof(libusb_request));
	}
	if (actualRequestType)
		requestContext->ActualRequestType = actualRequestType;

	if (ioControlCode)
		requestContext->IoControlCode = ioControlCode;

	switch(actualRequestType)
	{
	case WdfRequestTypeDeviceControl:
	case WdfRequestTypeRead:
		requestContext->Length = outputBufferLength;
		break;
	case WdfRequestTypeWrite:
		requestContext->Length = inputBufferLength;
		break;
	}

	return status;
}
*/

VOID Request_PreIoInitialize(__in WDFDEVICE Device,
                             __in WDFREQUEST Request)
{
	NTSTATUS  status = STATUS_SUCCESS;
	WDF_REQUEST_PARAMETERS  params;
	PREQUEST_CONTEXT requestContext = NULL;
	libusb_request* libusbRequest;
	size_t libusbRequestSize;

	// get the request parameters
	WDF_REQUEST_PARAMETERS_INIT(&params);
	WdfRequestGetParameters(Request, &params);

	// If this is not a DeviceIoControl then there is nothing to do.
	if (params.Type != WdfRequestTypeDeviceControl) goto Done;

	// get the request context
	requestContext = GetRequestContext(Request);
	if (!requestContext)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("NULL request context status=%08Xh\n", status);
		goto Done;
	}

	// All io control request must send the libusb_request struct.
	status = WdfRequestRetrieveInputBuffer(Request, sizeof(libusb_request), &libusbRequest, &libusbRequestSize);
	if(!NT_SUCCESS(status))
	{
		USBERR("WdfRequestRetrieveInputBuffer failed. status=%08Xh\n", status);
		goto Done;
	}

	requestContext->InputBuffer = libusbRequest;
	requestContext->InputBufferLength = (ULONG)libusbRequestSize;

	RtlCopyMemory(&requestContext->IoControlRequest, libusbRequest, sizeof(requestContext->IoControlRequest));

	switch(params.Parameters.DeviceIoControl.IoControlCode)
	{
	case LIBUSBK_IOCTL_ISOEX_READ:
	case LIBUSBK_IOCTL_ISOEX_WRITE:
		// Lock the iso context for write. This allows the iso information to be updated upon completion.
		status = WdfRequestProbeAndLockUserBufferForWrite(Request, libusbRequest->IsoEx.IsoContext, libusbRequest->IsoEx.IsoContextSize, &requestContext->IsoEx.ContextMemory);
		if(!NT_SUCCESS(status))
		{
			USBERR("WdfRequestProbeAndLockUserBufferForWrite failed. status=%08Xh\n", status);
			goto Done;
		}
		break;
	}

Done:
	if(NT_SUCCESS(status))
	{
		status = WdfDeviceEnqueueRequest(Device, Request);
		if(NT_SUCCESS(status)) return;

		USBERR("WdfDeviceEnqueueRequest failed. status=%08Xh\n", status);
	}

	WdfRequestComplete(Request, status);
	return;
}
