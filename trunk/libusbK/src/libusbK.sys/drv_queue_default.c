/*!********************************************************************
libusbK - WDF USB driver.
Copyright (C) 2011 All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen     (xiaofanc@gmail.com)

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
#pragma alloc_text(PAGE, DefaultQueue_OnIoControl)
#pragma alloc_text(PAGE, DefaultQueue_OnRead)
#pragma alloc_text(PAGE, DefaultQueue_OnWrite)
#endif


#define GET_OUT_BUFFER(MinimumBufferSize, BufferRef, LengthRef, ErrorText) \
	status = WdfRequestRetrieveOutputBuffer(Request, MinimumBufferSize, BufferRef, LengthRef); \
	if(!NT_SUCCESS(status)) \
	{ \
		USBERR("%s WdfRequestRetrieveOutputBuffer failed status=%Xh\n", ErrorText, status); \
		break; \
	}

#define GET_OUT_MEMORY(ErrorText) \
	status = WdfRequestRetrieveOutputMemory(Request, &memory); \
	if(!NT_SUCCESS(status)) \
	{ \
		USBERR("%s: WdfRequestRetrieveOutputMemory failed status=%Xh\n", ErrorText, status); \
		break; \
	}

#define GET_IN_MEMORY(ErrorText) \
	status = WdfRequestRetrieveInputMemory(Request, &memory); \
	if(!NT_SUCCESS(status)) \
	{ \
		USBERR("%s: WdfRequestRetrieveInputMemory failed status=%Xh\n", ErrorText, status); \
		break; \
	}

#define SetKInterfaceOutput(OutputBufferPtr, InterfaceContextPtr)															\
if (OutputBufferPtr)																										\
{																															\
	((libusb_request*)OutputBufferPtr)->intf.interface_number=InterfaceContextPtr->InterfaceDescriptor.bInterfaceNumber;	\
	((libusb_request*)OutputBufferPtr)->intf.interface_index=InterfaceContextPtr->InterfaceIndex;							\
	((libusb_request*)OutputBufferPtr)->intf.altsetting_number=InterfaceContextPtr->InterfaceDescriptor.bAlternateSetting;	\
	((libusb_request*)OutputBufferPtr)->intf.altsetting_index=InterfaceContextPtr->SettingIndex;								\
}

// DefaultQueue_OnIoControl
// This event is called when the framework receives IRP_MJ_DEVICE_CONTROL requests from the system.
//
// All DeviceIoControl Requests will come here first; we can either proccess them or forward them to
// other queues for proccessing.
//
VOID DefaultQueue_OnIoControl(__in WDFQUEUE Queue,
                              __in WDFREQUEST Request,
                              __in size_t OutputBufferLength,
                              __in size_t InputBufferLength,
                              __in ULONG IoControlCode)
{
	WDFDEVICE				device;
	NTSTATUS				status;
	PDEVICE_CONTEXT			deviceContext;
	PFILE_CONTEXT			pFileContext;
	ULONG					length = 0;
	libusb_request*			libusbRequest;
	WDF_MEMORY_DESCRIPTOR	memoryDescriptor;
	WDFMEMORY				memory;
	PUCHAR					outputBuffer;
	PUCHAR					inputBuffer;
	size_t					outputBufferLen, inputBufferLen;
	PFILE_OBJECT			wdmFileObject;
	PREQUEST_CONTEXT		requestContext;
	PINTERFACE_CONTEXT		interfaceContext;

	UNREFERENCED_PARAMETER(pFileContext);

	PAGED_CODE();

	//
	// initialize variables
	//
	device = WdfIoQueueGetDevice(Queue);
	deviceContext = GetDeviceContext(device);
	wdmFileObject = WdfFileObjectWdmGetFileObject(WdfRequestGetFileObject(Request));
	requestContext = GetRequestContext(Request);

	USBDEV("outLength=%u inLength=%u IoFunc=%03Xh\n",
	       OutputBufferLength, InputBufferLength, FUNCTION_FROM_CTL_CODE(IoControlCode));

	if (!deviceContext || !requestContext || !wdmFileObject)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("invalid device or request context\n");
		WdfRequestCompleteWithInformation(Request, status, 0);
		return;
	}
	// All io control request must send the libusb_request struct.
	status = WdfRequestRetrieveInputBuffer(Request, sizeof(libusb_request), &libusbRequest, &inputBufferLen);
	if(!NT_SUCCESS(status))
	{
		USBERR("invalid libusb_request size\n");
		WdfRequestCompleteWithInformation(Request, status, 0);
		return;
	}

	// Stores the libusb_request in the WdfRequest context space.
	// If this is a DIRECT transfer, gets the transfer MDL. If there isn't one, OriginalTransferMDL is set to NULL.
	// If GetTransferMdl() fails for any other reason than STATUS_BUFFER_TOO_SMALL, the operation fails.
	status = Request_InitContext(requestContext,
	                             Request,
	                             libusbRequest,
	                             WdfRequestTypeDeviceControl,
	                             IoControlCode,
	                             (ULONG)OutputBufferLength,
	                             (ULONG)InputBufferLength);

	if(!NT_SUCCESS(status))
	{
		USBERR("Request_InitContext failed. status=%Xh\n", status);
		WdfRequestCompleteWithInformation(Request, status, 0);
		return;
	}

	switch(IoControlCode)
	{
	case LIBUSB_IOCTL_ISOCHRONOUS_READ:
	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ:
		ForwardToPipeQueue(Request, deviceContext, requestContext, libusbRequest, WdfRequestTypeRead);
		return;

	case LIBUSB_IOCTL_ISOCHRONOUS_WRITE:
	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE:
		ForwardToPipeQueue(Request, deviceContext, requestContext, libusbRequest, WdfRequestTypeWrite);
		return;

	case LIBUSB_IOCTL_CONTROL_WRITE:

		requestContext->PipeContext = GetPipeContextByID(deviceContext, 0);
		if (!requestContext->PipeContext || !requestContext->PipeContext->IsValid)
		{
			USBERR("control_write: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		requestContext->RequestType = WdfRequestTypeWrite;
		ForwardToQueue(Request, requestContext);
		return;

	case LIBUSB_IOCTL_CONTROL_READ:

		requestContext->PipeContext = GetPipeContextByID(deviceContext, 0);
		if (!requestContext->PipeContext || !requestContext->PipeContext->IsValid)
		{
			USBERR("control_read: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		requestContext->RequestType = WdfRequestTypeRead;
		ForwardToQueue(Request, requestContext);
		return;

	case LIBUSB_IOCTL_GET_VERSION:

		GET_OUT_BUFFER(sizeof(libusb_request), &libusbRequest, &outputBufferLen, "get_version");

		length = sizeof(libusb_request);

		libusbRequest->version.major = VERSION_MAJOR;
		libusbRequest->version.minor = VERSION_MINOR;
		libusbRequest->version.micro = VERSION_MICRO;
		libusbRequest->version.nano  = VERSION_NANO;
		libusbRequest->version.mod_value = 1;

		status = STATUS_SUCCESS;

		break;

	case LIBUSB_IOCTL_GET_DESCRIPTOR:

		requestContext->PipeContext = GetPipeContextByID(deviceContext, 0);
		if (!requestContext->PipeContext || !requestContext->PipeContext->IsValid)
		{
			USBERR("control_read: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		requestContext->RequestType = WdfRequestTypeRead;
		FormatDescriptorRequestAsControlTransfer(requestContext, FALSE);
		ForwardToQueue(Request, requestContext);

		return;

	case LIBUSB_IOCTL_SET_DESCRIPTOR:

		requestContext->PipeContext = GetPipeContextByID(deviceContext, 0);
		if (!requestContext->PipeContext || !requestContext->PipeContext->IsValid)
		{
			USBERR("set_feature: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		// !IMPORTANT! This length must be reset to the total input buffer length
		// for these legacy BUFFERED ioctl codes that are HostToDevice before they
		// ae sent to the control pipe queue.
		requestContext->Length		= (ULONG)InputBufferLength;
		requestContext->RequestType = WdfRequestTypeWrite;
		//////////////////////////////////////////////////////////////////////////

		FormatDescriptorRequestAsControlTransfer(requestContext, TRUE);
		ForwardToQueue(Request, requestContext);

		return;

	case LIBUSB_IOCTL_SET_CONFIGURATION:

		if ((deviceContext->UsbConfigurationDescriptor) &&
		        (INT)deviceContext->UsbConfigurationDescriptor->bConfigurationValue == (INT)libusbRequest->configuration.configuration)
		{
			status = STATUS_SUCCESS;
		}
		else if ((deviceContext->UsbConfigurationDescriptor) &&
		         (INT)libusbRequest->configuration.configuration == -1)
		{
			status = STATUS_SUCCESS;
		}
		else
		{
			status = STATUS_NOT_IMPLEMENTED;
		}

		break;

	case LIBUSB_IOCTL_GET_CACHED_CONFIGURATION:
	case LIBUSB_IOCTL_GET_CONFIGURATION:

		GET_OUT_BUFFER(1, &outputBuffer, &outputBufferLen, "get_configuration");

		if (deviceContext->UsbConfigurationDescriptor)
			*((UCHAR*)outputBuffer) = deviceContext->UsbConfigurationDescriptor->bConfigurationValue;
		else
			*((UCHAR*)outputBuffer) = 0;

		length = 1;
		status = STATUS_SUCCESS;

		break;

		/////////////////////////////////////////////////////////////////////
		// libusbK interface claim/release/set/get IOCTLs
		/////////////////////////////////////////////////////////////////////
	case LIBUSBK_IOCTL_RELEASE_ALL_INTERFACES:
		status = Interface_ReleaseAll(deviceContext, wdmFileObject);
		break;

	case LIBUSBK_IOCTL_CLAIM_INTERFACE:
		GET_OUT_BUFFER(sizeof(libusb_request), &outputBuffer, &outputBufferLen, "claim_interface");
		interfaceContext = NULL;
		status = Interface_Claim(deviceContext, requestContext, wdmFileObject, &interfaceContext);
		if (NT_SUCCESS(status) && interfaceContext)
		{
			SetKInterfaceOutput(outputBuffer, interfaceContext);
			length = sizeof(libusb_request);
		}
		break;

	case LIBUSBK_IOCTL_RELEASE_INTERFACE:
		GET_OUT_BUFFER(sizeof(libusb_request), &outputBuffer, &outputBufferLen, "release_interface");
		interfaceContext = NULL;
		status = Interface_Release(deviceContext, requestContext, wdmFileObject, &interfaceContext);
		if (NT_SUCCESS(status) && interfaceContext)
		{
			SetKInterfaceOutput(outputBuffer, interfaceContext);
			length = sizeof(libusb_request);
		}
		break;

	case LIBUSBK_IOCTL_SET_INTERFACE:
		GET_OUT_BUFFER(sizeof(libusb_request), &outputBuffer, &outputBufferLen, "set_interface");
		interfaceContext = NULL;

		status = Interface_SetAltSetting(deviceContext, requestContext, &interfaceContext);
		if (NT_SUCCESS(status) && interfaceContext)
		{
			SetKInterfaceOutput(outputBuffer, interfaceContext);
			length = sizeof(libusb_request);
		}
		break;

	case LIBUSBK_IOCTL_GET_INTERFACE:
		GET_OUT_BUFFER(sizeof(libusb_request), &outputBuffer, &outputBufferLen, "get_interface");
		interfaceContext = NULL;

		status = Interface_GetAltSetting(deviceContext, requestContext, &interfaceContext);
		if (NT_SUCCESS(status) && interfaceContext)
		{
			SetKInterfaceOutput(outputBuffer, interfaceContext);
			length = sizeof(libusb_request);
		}
		break;
		/////////////////////////////////////////////////////////////////////

	case LIBUSB_IOCTL_CLAIM_INTERFACE:
		requestContext->IoControlRequest.intf.intf_use_index = 0;
		requestContext->IoControlRequest.intf.altf_use_index = 0;
		status = Interface_Claim(deviceContext, requestContext, wdmFileObject, &interfaceContext);
		break;

	case LIBUSB_IOCTL_RELEASE_INTERFACE:
		requestContext->IoControlRequest.intf.altf_use_index = 0;
		requestContext->IoControlRequest.intf.intf_use_index = 0;
		status = Interface_Release(deviceContext, requestContext, wdmFileObject, &interfaceContext);
		break;

	case LIBUSB_IOCTL_SET_INTERFACE:
		requestContext->IoControlRequest.intf.altf_use_index = 0;
		requestContext->IoControlRequest.intf.intf_use_index = 0;
		status = Interface_SetAltSetting(deviceContext, requestContext, &interfaceContext);
		break;

	case LIBUSB_IOCTL_GET_INTERFACE:
		GET_OUT_BUFFER(1, &outputBuffer, &outputBufferLen, "get_interface");
		requestContext->IoControlRequest.intf.altf_use_index = 0;
		requestContext->IoControlRequest.intf.intf_use_index = 0;
		status = Interface_GetAltSetting(deviceContext, requestContext, &interfaceContext);
		if (NT_SUCCESS(status) && interfaceContext)
		{
			*outputBuffer = interfaceContext->InterfaceDescriptor.bAlternateSetting;
			length = 1;
		}
		break;

	case LIBUSB_IOCTL_SET_FEATURE:

		requestContext->PipeContext = GetPipeContextByID(deviceContext, 0);
		if (!requestContext->PipeContext || !requestContext->PipeContext->IsValid)
		{
			USBERR("set_feature: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		// !IMPORTANT! This length must be reset to the total input buffer length
		// for these legacy BUFFERED ioctl codes that are HostToDevice before they
		// ae sent to the control pipe queue.
		requestContext->Length		= (ULONG)InputBufferLength;
		requestContext->RequestType = WdfRequestTypeWrite;
		//////////////////////////////////////////////////////////////////////////

		FormatFeatureRequestAsControlTransfer(requestContext, TRUE);
		ForwardToQueue(Request, requestContext);

		return;

	case LIBUSB_IOCTL_CLEAR_FEATURE:

		requestContext->PipeContext = GetPipeContextByID(deviceContext, 0);
		if (!requestContext->PipeContext || !requestContext->PipeContext->IsValid)
		{
			USBERR("clear_feature: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		// !IMPORTANT! This length must be reset to the total input buffer length
		// for these legacy BUFFERED ioctl codes that are HostToDevice before they
		// ae sent to the control pipe queue.
		requestContext->Length		= (ULONG)InputBufferLength;
		requestContext->RequestType = WdfRequestTypeWrite;
		//////////////////////////////////////////////////////////////////////////

		FormatFeatureRequestAsControlTransfer(requestContext, FALSE);
		ForwardToQueue(Request, requestContext);

		return;

	case LIBUSB_IOCTL_VENDOR_READ:

		requestContext->PipeContext = GetPipeContextByID(deviceContext, 0);
		if (!requestContext->PipeContext || !requestContext->PipeContext->IsValid)
		{
			USBERR("vendor_read: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		requestContext->RequestType = WdfRequestTypeRead;

		FormatVendorRequestAsControlTransfer(requestContext);
		ForwardToQueue(Request, requestContext);

		return;

	case LIBUSB_IOCTL_VENDOR_WRITE:

		requestContext->PipeContext = GetPipeContextByID(deviceContext, 0);
		if (!requestContext->PipeContext || !requestContext->PipeContext->IsValid)
		{
			USBERR("vendor_write: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		// !IMPORTANT! This length must be reset to the total input buffer length
		// for these legacy BUFFERED ioctl codes that are HostToDevice before they
		// ae sent to the control pipe queue.
		requestContext->Length		= (ULONG)InputBufferLength;
		requestContext->RequestType = WdfRequestTypeWrite;
		//////////////////////////////////////////////////////////////////////////

		FormatVendorRequestAsControlTransfer(requestContext);
		ForwardToQueue(Request, requestContext);

		return;

	case LIBUSB_IOCTL_GET_STATUS:

		if (OutputBufferLength < 2)
		{
			USBERR("get_status: invalid output buffer\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		GET_OUT_MEMORY("get_status");
		WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&memoryDescriptor, memory, NULL);

		status = Request_Status(deviceContext,
		                        &memoryDescriptor,
		                        (WDF_USB_BMREQUEST_RECIPIENT)libusbRequest->status.recipient,
		                        (USHORT)libusbRequest->status.index,
		                        libusbRequest->timeout,
		                        &length);

		break;

	case LIBUSB_IOCTL_RESET_ENDPOINT:

		status = Pipe_Reset(deviceContext, (UCHAR)libusbRequest->endpoint.endpoint);
		break;

	case LIBUSB_IOCTL_ABORT_ENDPOINT:

		status = Pipe_Abort(deviceContext, (UCHAR)libusbRequest->endpoint.endpoint);
		break;

	case LIBUSB_IOCTL_RESET_DEVICE:

		status = Device_Reset(device);
		break;

	case LIBUSB_IOCTL_SET_PIPE_POLICY:

		requestContext->PipeContext = GetPipeContextByID(deviceContext, (UCHAR)libusbRequest->pipe_policy.pipe_id);
		if (!requestContext->PipeContext || !requestContext->PipeContext->IsValid || !requestContext->PipeContext->Queue)
		{
			USBERR("SetPipePolicy: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		status = WdfRequestForwardToIoQueue(Request, requestContext->PipeContext->Queue);
		if(!NT_SUCCESS(status))
		{
			USBERR("WdfRequestForwardToIoQueue failed. status=%Xh\n", status);
			break;
		}
		return;

	case LIBUSB_IOCTL_GET_PIPE_POLICY:

		requestContext->PipeContext = GetPipeContextByID(deviceContext, (UCHAR)libusbRequest->pipe_policy.pipe_id);
		if (!requestContext->PipeContext || !requestContext->PipeContext->IsValid || !requestContext->PipeContext->Queue)
		{
			USBERR("GetPipePolicy: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		status = WdfRequestForwardToIoQueue(Request, requestContext->PipeContext->Queue);
		if(!NT_SUCCESS(status))
		{
			USBERR("WdfRequestForwardToIoQueue failed. status=%Xh\n", status);
			break;
		}
		return;


	case LIBUSB_IOCTL_SET_POWER_POLICY:
		inputBufferLen -= sizeof(libusb_request);
		inputBuffer = GetRequestDataPtr(libusbRequest);

		USBDBG("SetPowerPolicy: policyType=%02Xh valueLength=%u\n",
		       (UCHAR)libusbRequest->power_policy.policy_type, inputBufferLen);

		status = Policy_SetPower(
		             deviceContext,
		             libusbRequest->power_policy.policy_type,
		             inputBuffer,
		             (ULONG)inputBufferLen);

		break;

	case LIBUSB_IOCTL_GET_POWER_POLICY:

		GET_OUT_BUFFER(1, &outputBuffer, &outputBufferLen, "get_power_policy");

		length = (ULONG)outputBufferLen;

		USBDBG("GetPowerPolicy: policyType=%02Xh valueLength=%u\n",
		       (UCHAR)libusbRequest->power_policy.policy_type, outputBufferLen);

		status = Policy_GetPower(
		             deviceContext,
		             libusbRequest->power_policy.policy_type,
		             outputBuffer,
		             &length);

		if (!NT_SUCCESS(status))
		{
			length = 0;
		}
		break;

	case LIBUSB_IOCTL_QUERY_DEVICE_INFORMATION:

		GET_OUT_BUFFER(1, &outputBuffer, &outputBufferLen, "query_device_information");
		length = (ULONG)outputBufferLen;

		USBDBG("QueryDeviceInformation: informationType=%02Xh valueLength=%u\n",
		       (UCHAR)libusbRequest->query_device.information_type, outputBufferLen);

		status = Policy_GetDevice(
		             deviceContext,
		             libusbRequest->query_device.information_type,
		             outputBuffer,
		             &length);

		if (!NT_SUCCESS(status))
		{
			length = 0;
		}
		break;

	case LIBUSB_IOCTL_FLUSH_PIPE:

		USBDBG("FlushPipe: pipeID=%02Xh\n",
		       (UCHAR)libusbRequest->endpoint.endpoint);

		requestContext->PipeContext = GetPipeContextByID(deviceContext, (UCHAR)libusbRequest->endpoint.endpoint);
		if (!requestContext->PipeContext->IsValid || !requestContext->PipeContext->Pipe)
		{
			USBERR("pipe %02Xh not found\n", libusbRequest->endpoint.endpoint);
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		if (NT_SUCCESS((status = Pipe_Stop(requestContext->PipeContext, WdfIoTargetCancelSentIo))))
			status = Pipe_Start(deviceContext, requestContext->PipeContext);

		break;

	default :

		USBERR("unknown IoControlCode %Xh (function=%04Xh)\n", IoControlCode, FUNCTION_FROM_CTL_CODE(IoControlCode));
		status = STATUS_INVALID_DEVICE_REQUEST;
		WdfRequestCompleteWithInformation(Request, status, 0);

		return;
	}


	if (!NT_SUCCESS(status))
	{
		USBDBG("status=%Xh\n", status);
	}

	WdfRequestCompleteWithInformation(Request, status, length);

	return;
}

// Called by the framework when it receives Read requests. (ReadFile)
VOID DefaultQueue_OnRead(__in WDFQUEUE Queue,
                         __in WDFREQUEST Request,
                         __in size_t Length)
{
	PFILE_CONTEXT           fileContext = NULL;
	PREQUEST_CONTEXT        requestContext = NULL;
	NTSTATUS				status = STATUS_SUCCESS;

	PAGED_CODE();

	UNREFERENCED_PARAMETER(Queue); //the queue it came from.

	fileContext = GetFileContext(WdfRequestGetFileObject(Request));
	requestContext = GetRequestContext(Request);

	if (!requestContext || !fileContext || !fileContext->PipeContext || !fileContext->PipeContext->Pipe || !fileContext->PipeContext->IsValid)
	{
		USBERR("invalid pipe context or handle\n");
		status = STATUS_INVALID_PARAMETER;
		goto Done;
	}

	requestContext->PipeContext = fileContext->PipeContext;
	requestContext->RequestType = WdfRequestTypeRead;

	switch(requestContext->PipeContext->PipeInformation.PipeType)
	{
	case WdfUsbPipeTypeIsochronous:
		requestContext->IoControlCode = LIBUSB_IOCTL_ISOCHRONOUS_READ;
		break;
	case WdfUsbPipeTypeInterrupt:
	case WdfUsbPipeTypeBulk:
		requestContext->IoControlCode = LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ;
		break;
	default:
		USBERR("invalid PipeInformation.PipeType (%u)\n",
		       requestContext->PipeContext->PipeInformation.PipeType);
		status = STATUS_INVALID_PARAMETER;
		goto Done;

	}

	// Stores the libusb_request in the WdfRequest context space.
	// If this is a DIRECT transfer, gets the transfer MDL. If there isn't one, OriginalTransferMDL is set to NULL.
	// If GetTransferMdl() fails for any other reason than STATUS_BUFFER_TOO_SMALL, the operation fails.
	status = Request_InitContext(requestContext, Request, NULL, WdfRequestTypeRead, 0, (ULONG)Length, 0);
	if(!NT_SUCCESS(status))
	{
		USBERR("Request_InitContext failed. status=%Xh\n", status);
		goto Done;
	}

	if (!USB_ENDPOINT_DIRECTION_IN(GetRequestPipeID(requestContext)))
	{
		USBERR("pipeID=%02Xh cannot read from an outgoing pipe\n",
		       GetRequestPipeID(requestContext));
		status = STATUS_INVALID_PARAMETER;
		goto Done;
	}

	// For read/read request there is no libusb_request so it is 'ForwardToQueue' and
	// not ForwardToPipeQueue here.
	ForwardToQueue(Request, requestContext);
	return;

Done:
	WdfRequestCompleteWithInformation(Request, status, 0);
	return;
}

// Called by the framework when it receives Write requests.
VOID DefaultQueue_OnWrite(__in WDFQUEUE Queue,
                          __in WDFREQUEST Request,
                          __in size_t Length)
{
	PFILE_CONTEXT           fileContext = NULL;
	PREQUEST_CONTEXT        requestContext = NULL;
	NTSTATUS				status = STATUS_SUCCESS;

	PAGED_CODE();

	UNREFERENCED_PARAMETER(Queue); //the queue it came from.

	fileContext = GetFileContext(WdfRequestGetFileObject(Request));
	requestContext = GetRequestContext(Request);

	if (!requestContext || !fileContext || !fileContext->PipeContext || !fileContext->PipeContext->Pipe || !fileContext->PipeContext->IsValid)
	{
		USBERR("invalid pipe context or handle\n");
		status = STATUS_INVALID_PARAMETER;
		goto Done;
	}

	requestContext->RequestType = WdfRequestTypeWrite;
	requestContext->PipeContext = fileContext->PipeContext;

	switch(requestContext->PipeContext->PipeInformation.PipeType)
	{
	case WdfUsbPipeTypeIsochronous:
		requestContext->IoControlCode = LIBUSB_IOCTL_ISOCHRONOUS_WRITE;
		break;
	case WdfUsbPipeTypeInterrupt:
	case WdfUsbPipeTypeBulk:
		requestContext->IoControlCode = LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE;
		break;
	default:
		USBERR("invalid PipeInformation.PipeType (%u)\n", requestContext->PipeContext->PipeInformation.PipeType);
		status = STATUS_INVALID_PARAMETER;
		goto Done;

	}

	status = Request_InitContext(requestContext, Request, NULL, WdfRequestTypeWrite, 0, 0, (ULONG)Length);
	if(!NT_SUCCESS(status))
	{
		USBERR("Request_InitContext failed. status=%Xh\n", status);
		goto Done;
	}

	if (USB_ENDPOINT_DIRECTION_IN(GetRequestPipeID(requestContext)))
	{
		USBERR("pipeID=%02Xh cannot write from an incoming pipe\n",
		       GetRequestPipeID(requestContext));
		status = STATUS_INVALID_PARAMETER;
		goto Done;
	}

	// For read/read request there is no libusb_request so it is 'ForwardToQueue' and
	// not ForwardToPipeQueue here.
	ForwardToQueue(Request, requestContext);
	return;

Done:
	WdfRequestCompleteWithInformation(Request, status, 0);
	return;
}
