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
//#pragma alloc_text(PAGE, PipeQueue_OnIoControl)
//#pragma alloc_text(PAGE, PipeQueue_OnRead)
//#pragma alloc_text(PAGE, PipeQueue_OnWrite)
#endif

VOID NONPAGABLE PipeQueue_OnIoControl(__in WDFQUEUE Queue,
                                      __in WDFREQUEST Request,
                                      __in size_t OutputBufferLength,
                                      __in size_t InputBufferLength,
                                      __in ULONG IoControlCode)
{
	NTSTATUS status;
	ULONG length = 0;
	PUCHAR inputBuffer, outputBuffer;
	libusb_request* libusbRequest;
	PDEVICE_CONTEXT deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	PREQUEST_CONTEXT requestContext = GetRequestContext(Request);

	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status))
		goto Done;

	switch(IoControlCode)
	{
	case LIBUSB_IOCTL_ISOCHRONOUS_READ:
	case LIBUSB_IOCTL_ISOCHRONOUS_WRITE:
	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE:
	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ:

		switch (requestContext->PipeContext->PipeInformation.PipeType)
		{
		case WdfUsbPipeTypeIsochronous:

			if (IsHighSpeedDevice(deviceContext))
			{
				XferIsoHS(Queue, Request, (ULONG)OutputBufferLength);
			}
			else
			{
				XferIsoFS(Queue, Request, InputBufferLength, OutputBufferLength);
			}

			return;

		case WdfUsbPipeTypeBulk:
		case WdfUsbPipeTypeInterrupt:

			Xfer(Queue, Request, InputBufferLength, OutputBufferLength);

			return;

		}
		status = STATUS_INVALID_PARAMETER;
		USBERR("invalid pipeType=%u\n", requestContext->PipeContext->PipeInformation.PipeType);
		break;

	case LIBUSB_IOCTL_SET_FEATURE:
	case LIBUSB_IOCTL_CLEAR_FEATURE:
	case LIBUSB_IOCTL_GET_DESCRIPTOR:
	case LIBUSB_IOCTL_SET_DESCRIPTOR:
	case LIBUSB_IOCTL_VENDOR_WRITE:
	case LIBUSB_IOCTL_VENDOR_READ:
	case LIBUSB_IOCTL_CONTROL_READ:
	case LIBUSB_IOCTL_CONTROL_WRITE:

		XferCtrl(Queue, Request, InputBufferLength, OutputBufferLength);
		return;

	case LIBUSB_IOCTL_GET_PIPE_POLICY:

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(libusb_request), &libusbRequest, &InputBufferLength);
		if(!NT_SUCCESS(status))
		{
			USBERR("GetPipePolicy: WdfRequestRetrieveInputBuffer failed status=%Xh\n", status);
			break;
		}

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

		status = WdfRequestRetrieveInputBuffer(Request, sizeof(libusb_request), &libusbRequest, &InputBufferLength);
		if(!NT_SUCCESS(status))
		{
			USBERR("SetPipePolicy: WdfRequestRetrieveInputBuffer failed status=%Xh\n", status);
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

	default:

		USBERR("unknown IoControlCode %Xh (function=%04Xh)\n", IoControlCode, FUNCTION_FROM_CTL_CODE(IoControlCode));
		status = STATUS_INVALID_DEVICE_REQUEST;
		WdfRequestCompleteWithInformation(Request, status, 0);

		return;
	}

Done:
	WdfRequestCompleteWithInformation(Request, status, length);
	return;

}
VOID NONPAGABLE PipeQueue_OnRead(__in WDFQUEUE Queue,
                                 __in WDFREQUEST Request,
                                 __in size_t OutputBufferLength)
{
	NTSTATUS				status = STATUS_SUCCESS;
	PDEVICE_CONTEXT         deviceContext;
	PREQUEST_CONTEXT		requestContext;

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	requestContext = GetRequestContext(Request);

	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status))
		goto Done;

	if (!requestContext->PipeContext->Pipe)
	{
		USBERR("null pipe handle\n");
		status = STATUS_INVALID_HANDLE;
		goto Done;
	}

	if (requestContext->PipeContext->PipeInformation.PipeType == WdfUsbPipeTypeIsochronous)
	{
		if(IsHighSpeedDevice(deviceContext))
			XferIsoHS(Queue, Request, (ULONG)OutputBufferLength);
		else
			XferIsoFS(Queue, Request, SIZE_T_MAX, OutputBufferLength);
	}
	else // bulk/interrupt pipe.
		Xfer(Queue, Request, SIZE_T_MAX, OutputBufferLength);

	return;

Done:
	WdfRequestCompleteWithInformation(Request, status, 0);

}

VOID NONPAGABLE PipeQueue_OnWrite(__in WDFQUEUE Queue,
                                  __in WDFREQUEST Request,
                                  __in size_t InputBufferLength)
{
	NTSTATUS				status = STATUS_SUCCESS;
	PDEVICE_CONTEXT         deviceContext;
	PREQUEST_CONTEXT		requestContext;

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	requestContext = GetRequestContext(Request);

	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status))
		goto Done;

	if (!requestContext->PipeContext->Pipe)
	{
		USBERR("null pipe handle\n");
		status = STATUS_INVALID_HANDLE;
		goto Done;
	}

	if (requestContext->PipeContext->PipeInformation.PipeType == WdfUsbPipeTypeIsochronous)
	{
		if(IsHighSpeedDevice(deviceContext))
			XferIsoHS(Queue, Request, (ULONG)InputBufferLength);
		else
			XferIsoFS(Queue, Request, InputBufferLength, SIZE_T_MAX);
	}
	else // bulk/interrupt pipe.
		Xfer(Queue, Request, InputBufferLength, SIZE_T_MAX);

	return;

Done:
	WdfRequestCompleteWithInformation(Request, status, 0);

}
