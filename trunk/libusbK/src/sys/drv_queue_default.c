/*!********************************************************************
libusbK - WDF USB driver.
Copyright (C) 2011 All Rights Reserved.
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

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

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
	((libusb_request*)OutputBufferPtr)->intf.altsetting_index=InterfaceContextPtr->SettingIndex;							\
}

#define mRequest_InitAndForwardToQueue(mStatus,mRequestType, mActualRequestType, mIoControlCode, mRequestLength, mRequest, mDeviceContext, mRequestContext, mPipeID, mErrorAction) do { 	\
	PPIPE_CONTEXT tempPipeCtx = GetPipeContextByID(mDeviceContext, mPipeID);  						\
	if (!tempPipeCtx || !tempPipeCtx->IsValid || !tempPipeCtx->Queue) 								\
	{ 																								\
		mStatus = STATUS_INVALID_PARAMETER;   														\
		USBERRN("Invalid pipe context."); 															\
		mErrorAction; 																				\
	} 																								\
   																									\
	if (tempPipeCtx->IsQueueDirty)																	\
	{																								\
		mStatus = Pipe_RefreshQueue(mDeviceContext, tempPipeCtx);									\
		if(!NT_SUCCESS(mStatus))  																	\
		{ 																							\
			USBERRN("Pipe_RefreshQueue failed. Status=%08Xh", mStatus);  							\
			mErrorAction; 																			\
		}																							\
	}																								\
	mRequestContext->QueueContext = GetQueueContext(tempPipeCtx->Queue);  							\
	if (!mRequestContext->QueueContext)   															\
	{ 																								\
		mStatus = STATUS_INVALID_PARAMETER;   														\
		USBERRN("Invalid pipe context."); 															\
		mErrorAction; 																				\
	} 																								\
  																									\
	mRequestContext->RequestType	= mRequestType;   												\
	mRequestContext->Length			= (ULONG)mRequestLength;   										\
  																									\
	if (mActualRequestType)   																		\
		mRequestContext->ActualRequestType	= mActualRequestType;   								\
  																									\
	if (mIoControlCode)   																			\
		mRequestContext->IoControlCode = mIoControlCode;  											\
  																									\
	mRequestContext->Policies.values = tempPipeCtx->Policies.values;  								\
  																									\
	mRequestContext->Timeout = 	tempPipeCtx->TimeoutPolicy;											\
	if (mRequestContext->IoControlRequest.timeout)  												\
		mRequestContext->Timeout = mRequestContext->IoControlRequest.timeout; 						\
  																									\
	mStatus = WdfRequestForwardToIoQueue(mRequest, tempPipeCtx->Queue);   							\
	if(!NT_SUCCESS(mStatus))  																		\
	{ 																								\
		USBERRN("WdfRequestForwardToIoQueue failed. Status=%08Xh", mStatus);  						\
		mErrorAction; 																				\
	} 																								\
}while(0)


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
	NTSTATUS				status;
	WDFDEVICE				device;
	PDEVICE_CONTEXT			deviceContext;
	PREQUEST_CONTEXT		requestContext;
	PINTERFACE_CONTEXT		interfaceContext;
	PFILE_OBJECT			wdmFileObject;
	PPIPE_CONTEXT			pipeContext;
	PFILE_CONTEXT			pFileContext;
	ULONG					length = 0;
	WDF_MEMORY_DESCRIPTOR	memoryDescriptor;
	WDFMEMORY				memory;
	PUCHAR					outputBuffer;
	PUCHAR					inputBuffer;
	size_t					outputBufferLen, inputBufferLen;
	libusb_request*			libusbRequest = NULL;

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
	// InputBuffer is assigned in Request_PreIoInitialize.
	libusbRequest = (libusb_request*)requestContext->InputBuffer;
	inputBufferLen = (size_t)requestContext->InputBufferLength;

	switch(IoControlCode)
	{
	case LIBUSB_IOCTL_ISOCHRONOUS_READ:
	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ:
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeRead,
		                               WdfRequestTypeDeviceControl, IoControlCode, OutputBufferLength, Request, deviceContext, requestContext,
		                               ((UCHAR)libusbRequest->endpoint.endpoint), break);
		return;

	case LIBUSBK_IOCTL_ISOEX_READ:
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeRead,
		                               WdfRequestTypeDeviceControl, IoControlCode, OutputBufferLength, Request, deviceContext, requestContext,
		                               libusbRequest->IsoEx.PipeID, break);
		return;

	case LIBUSBK_IOCTL_AUTOISOEX_READ:
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeRead,
		                               WdfRequestTypeDeviceControl, IoControlCode, OutputBufferLength, Request, deviceContext, requestContext,
		                               libusbRequest->AutoIsoEx.PipeID, break);
		return;

	case LIBUSB_IOCTL_ISOCHRONOUS_WRITE:
	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE:
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeWrite,
		                               WdfRequestTypeDeviceControl, IoControlCode, OutputBufferLength, Request, deviceContext, requestContext,
		                               ((UCHAR)libusbRequest->endpoint.endpoint), break);
		return;

	case LIBUSBK_IOCTL_ISOEX_WRITE:
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeWrite,
		                               WdfRequestTypeDeviceControl, IoControlCode, OutputBufferLength, Request, deviceContext, requestContext,
		                               libusbRequest->IsoEx.PipeID, break);
		return;

	case LIBUSBK_IOCTL_AUTOISOEX_WRITE:
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeWrite,
		                               WdfRequestTypeDeviceControl, IoControlCode, OutputBufferLength, Request, deviceContext, requestContext,
		                               libusbRequest->AutoIsoEx.PipeID, break);
		return;

	case LIBUSB_IOCTL_CONTROL_WRITE:
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeWrite,
		                               WdfRequestTypeDeviceControl, IoControlCode, OutputBufferLength, Request, deviceContext, requestContext,
		                               0, break);

		return;

	case LIBUSB_IOCTL_CONTROL_READ:
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeRead,
		                               WdfRequestTypeDeviceControl, IoControlCode, OutputBufferLength, Request, deviceContext, requestContext,
		                               0, break);
		return;

	case LIBUSB_IOCTL_GET_DESCRIPTOR:
		FormatDescriptorRequestAsControlTransfer(requestContext, libusbRequest, FALSE);
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeRead,
		                               WdfRequestTypeDeviceControl, IoControlCode, OutputBufferLength, Request, deviceContext, requestContext,
		                               0, break);
		return;

	case LIBUSB_IOCTL_VENDOR_READ:

		FormatVendorRequestAsControlTransfer(requestContext, libusbRequest, FALSE);
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeRead,
		                               WdfRequestTypeDeviceControl, IoControlCode, OutputBufferLength, Request, deviceContext, requestContext,
		                               0, break);
		return;
	case LIBUSB_IOCTL_SET_DESCRIPTOR:
		requestContext->RequestType = WdfRequestTypeWrite;

		// !IMPORTANT! This length must be reset to the total input buffer length
		// for these legacy BUFFERED ioctl codes that are HostToDevice before they
		// ae sent to the control pipe queue.
		//requestContext->Length		= (ULONG)InputBufferLength;
		//////////////////////////////////////////////////////////////////////////

		FormatDescriptorRequestAsControlTransfer(requestContext, libusbRequest, TRUE);
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeWrite,
		                               WdfRequestTypeDeviceControl, IoControlCode, InputBufferLength, Request, deviceContext, requestContext,
		                               0, break);
		return;

	case LIBUSB_IOCTL_SET_FEATURE:
		// !IMPORTANT! This length must be reset to the total input buffer length
		// for these legacy BUFFERED ioctl codes that are HostToDevice before they
		// ae sent to the control pipe queue.
		// requestContext->Length		= (ULONG)InputBufferLength;
		// requestContext->RequestType = WdfRequestTypeWrite;
		//////////////////////////////////////////////////////////////////////////

		FormatFeatureRequestAsControlTransfer(requestContext, libusbRequest, TRUE);
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeWrite,
		                               WdfRequestTypeDeviceControl, IoControlCode, ((ULONG)InputBufferLength), Request, deviceContext, requestContext,
		                               0, break);
		return;

	case LIBUSB_IOCTL_CLEAR_FEATURE:

		// !IMPORTANT! This length must be reset to the total input buffer length
		// for these legacy BUFFERED ioctl codes that are HostToDevice before they
		// ae sent to the control pipe queue.
		// requestContext->Length		= (ULONG)InputBufferLength;
		// requestContext->RequestType = WdfRequestTypeWrite;
		//////////////////////////////////////////////////////////////////////////

		FormatFeatureRequestAsControlTransfer(requestContext, libusbRequest, FALSE);
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeWrite,
		                               WdfRequestTypeDeviceControl, IoControlCode, ((ULONG)InputBufferLength), Request, deviceContext, requestContext,
		                               0, break);
		return;

	case LIBUSB_IOCTL_VENDOR_WRITE:

		// !IMPORTANT! This length must be reset to the total input buffer length
		// for these legacy BUFFERED ioctl codes that are HostToDevice before they
		// ae sent to the control pipe queue.
		// requestContext->Length		= (ULONG)InputBufferLength;
		// requestContext->RequestType = WdfRequestTypeWrite;
		//////////////////////////////////////////////////////////////////////////

		FormatVendorRequestAsControlTransfer(requestContext, libusbRequest, TRUE);
		mRequest_InitAndForwardToQueue(status, WdfRequestTypeWrite,
		                               WdfRequestTypeDeviceControl, IoControlCode, InputBufferLength, Request, deviceContext, requestContext,
		                               0, break);
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

	case LIBUSB_IOCTL_SET_CONFIGURATION:

		if ((deviceContext->UsbConfigurationDescriptor) && (INT)deviceContext->UsbConfigurationDescriptor->bConfigurationValue == (INT)libusbRequest->configuration.configuration)
			status = STATUS_SUCCESS;
		else if ((deviceContext->UsbConfigurationDescriptor) && (INT)libusbRequest->configuration.configuration == -1)
			status = STATUS_SUCCESS;
		else
			status = STATUS_NOT_IMPLEMENTED;

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


	case LIBUSB_IOCTL_GET_PIPE_POLICY:
		status = WdfRequestRetrieveOutputBuffer(Request, 1, &outputBuffer, &OutputBufferLength);
		if(!NT_SUCCESS(status))
		{
			USBERR("GetPipePolicy: WdfRequestRetrieveOutputBuffer failed status=%Xh\n", status);
			break;
		}

		length = (ULONG)OutputBufferLength;

		USBDBG("GetPipePolicy: pipeID=%02Xh policyType=%02Xh valueLength=%u\n",
		       (UCHAR)libusbRequest->pipe_policy.pipe_id,
		       libusbRequest->pipe_policy.policy_type,
		       OutputBufferLength);

		status = Policy_GetPipe(deviceContext,
		                        (UCHAR)libusbRequest->pipe_policy.pipe_id,
		                        libusbRequest->pipe_policy.policy_type,
		                        outputBuffer,
		                        &length);

		if (!NT_SUCCESS(status))
			length = 0;

		break;

	case LIBUSB_IOCTL_SET_PIPE_POLICY:

		pipeContext = GetPipeContextByID(deviceContext, (UCHAR)libusbRequest->pipe_policy.pipe_id);
		if (!pipeContext || !pipeContext->IsValid || !pipeContext->Queue)
		{
			USBERR("SetPipePolicy: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		InputBufferLength -= sizeof(libusb_request);
		inputBuffer = GetRequestDataPtr(libusbRequest);

		USBDBG("SetPipePolicy: pipeID=%02Xh policyType=%02Xh valueLength=%u\n",
		       (UCHAR)libusbRequest->pipe_policy.pipe_id,
		       libusbRequest->pipe_policy.policy_type,
		       InputBufferLength);

		status = Policy_SetPipe(deviceContext,
		                        (UCHAR)libusbRequest->pipe_policy.pipe_id,
		                        libusbRequest->pipe_policy.policy_type,
		                        inputBuffer,
		                        (ULONG)InputBufferLength);
		break;

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

		USBDBG("FlushPipe: pipeID=%02Xh\n", (UCHAR)libusbRequest->endpoint.endpoint);

		pipeContext = GetPipeContextByID(deviceContext, (UCHAR)libusbRequest->endpoint.endpoint);
		if (!pipeContext->IsValid || !pipeContext->Pipe)
		{
			USBERR("pipe %02Xh not found\n", libusbRequest->endpoint.endpoint);
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		status = Pipe_Stop(pipeContext, WdfIoTargetCancelSentIo, TRUE);
		if (!NT_SUCCESS(status))
		{
			USBERRN("Pipe_Stop failed. PipeID=%02Xh Status=%08Xh", libusbRequest->endpoint.endpoint, status);
			break;
		}

		if (pipeContext->Queue && pipeContext->PipeInformation.EndpointAddress & 0xF)
		{
			PQUEUE_CONTEXT queueContext;

			WdfObjectAcquireLock(pipeContext->Queue);

			queueContext = GetQueueContext(pipeContext->Queue);
			if (queueContext)
			{
				// This is the allow partial read over-run buffer.
				queueContext->OverOfs.BufferLength = 0;
				queueContext->OverOfs.BufferOffset = 0;
			}

			WdfObjectReleaseLock(pipeContext->Queue);
		}

		status = Pipe_Start(deviceContext, pipeContext);
		if (!NT_SUCCESS(status))
		{
			USBERRN("Pipe_Start failed. PipeID=%02Xh Status=%08Xh", libusbRequest->endpoint.endpoint, status);
			break;
		}

		break;
	case LIBUSBK_IOCTL_GET_CURRENTFRAME_NUMBER:
		GET_OUT_BUFFER(sizeof(ULONG), &outputBuffer, &outputBufferLen, "get_currentframe");
		status = WdfUsbTargetDeviceRetrieveCurrentFrameNumber(deviceContext->WdfUsbTargetDevice, (PULONG)outputBuffer);
		if (NT_SUCCESS(status))
		{
			length = sizeof(ULONG);
		}

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
	DWORD					ioControlCode = LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ;
	PDEVICE_CONTEXT			deviceContext;

	PAGED_CODE();

	deviceContext	= GetDeviceContext(WdfIoQueueGetDevice(Queue));
	fileContext		= GetFileContext(WdfRequestGetFileObject(Request));
	requestContext	= GetRequestContext(Request);

	if (!deviceContext || !requestContext || !fileContext)
	{
		USBERRN("Invalid file context.");
		status = STATUS_INVALID_PARAMETER;
		goto Done;
	}

	if (!(fileContext->PipeID & 0x0F) || !(fileContext->PipeID & 0x80) || fileContext->PipeType < WdfUsbPipeTypeIsochronous || fileContext->PipeType > WdfUsbPipeTypeInterrupt)
	{
		USBERRN("Invalid PipeType PipeID=%02Xh PipeType=%s", fileContext->PipeID, GetPipeTypeString(fileContext->PipeType));
		status = STATUS_INVALID_PARAMETER;
		goto Done;
	}

	if (fileContext->PipeType == WdfUsbPipeTypeIsochronous)
		ioControlCode = LIBUSB_IOCTL_ISOCHRONOUS_READ;

	mRequest_InitAndForwardToQueue(status, WdfRequestTypeRead, WdfRequestTypeRead, ioControlCode, Length, Request, deviceContext, requestContext, fileContext->PipeID, goto Done);
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
	DWORD					ioControlCode = LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ;
	PDEVICE_CONTEXT			deviceContext;

	PAGED_CODE();

	deviceContext	= GetDeviceContext(WdfIoQueueGetDevice(Queue));
	fileContext		= GetFileContext(WdfRequestGetFileObject(Request));
	requestContext	= GetRequestContext(Request);

	if (!requestContext || !fileContext)
	{
		USBERRN("Invalid file context.");
		status = STATUS_INVALID_PARAMETER;
		goto Done;
	}

	if (!(fileContext->PipeID & 0x0F) || (fileContext->PipeID & 0x80) || fileContext->PipeType < WdfUsbPipeTypeIsochronous || fileContext->PipeType > WdfUsbPipeTypeInterrupt)
	{
		USBERRN("Invalid PipeType PipeID=%02Xh PipeType=%s", fileContext->PipeID, GetPipeTypeString(fileContext->PipeType));
		status = STATUS_INVALID_PARAMETER;
		goto Done;
	}

	if (fileContext->PipeType == WdfUsbPipeTypeIsochronous)
		ioControlCode = LIBUSB_IOCTL_ISOCHRONOUS_READ;

	mRequest_InitAndForwardToQueue(status, WdfRequestTypeWrite, WdfRequestTypeWrite, ioControlCode, Length, Request, deviceContext, requestContext, fileContext->PipeID, goto Done);
	return;

Done:
	WdfRequestCompleteWithInformation(Request, status, 0);
	return;
}
