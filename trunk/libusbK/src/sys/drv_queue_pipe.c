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

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#include "drv_common.h"

#if (defined(ALLOC_PRAGMA) && defined(PAGING_ENABLED))
//#pragma alloc_text(PAGE, PipeQueue_OnIoControl)
//#pragma alloc_text(PAGE, PipeQueue_OnRead)
//#pragma alloc_text(PAGE, PipeQueue_OnWrite)
#endif

VOID PipeQueue_OnIoControl(__in WDFQUEUE Queue,
						   __in WDFREQUEST Request,
						   __in size_t OutputBufferLength,
						   __in size_t InputBufferLength,
						   __in ULONG IoControlCode)
{
	NTSTATUS status;
	ULONG length = 0;
	PREQUEST_CONTEXT requestContext = GetRequestContext(Request);
	PQUEUE_CONTEXT queueContext = NULL;

	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status)) goto Done;

	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context");
		goto Done;
	}

	switch(IoControlCode)
	{
	case LIBUSB_IOCTL_ISOCHRONOUS_READ:
	case LIBUSB_IOCTL_ISOCHRONOUS_WRITE:
	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE:
	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ:

		switch (queueContext->Info.PipeType)
		{
		case WdfUsbPipeTypeIsochronous:

			if (USB_ENDPOINT_DIRECTION_IN(queueContext->Info.EndpointAddress))
			{
				XferIsoRead(Queue, Request);
				return;
			}
			XferIsoWrite(Queue, Request);
			return;

		case WdfUsbPipeTypeBulk:
		case WdfUsbPipeTypeInterrupt:
			if(requestContext->Policies.RawIO)
			{
				if (USB_ENDPOINT_DIRECTION_IN(queueContext->Info.EndpointAddress))
					Xfer_ReadBulkRaw(Queue, Request);
				else
					Xfer_WriteBulkRaw(Queue, Request);
			}
			else
			{
				if (USB_ENDPOINT_DIRECTION_IN(queueContext->Info.EndpointAddress))
					Xfer_ReadBulk(Queue, Request);
				else
					Xfer_WriteBulk(Queue, Request);
			}
			return;

		default:
			status = STATUS_INVALID_PARAMETER;
			USBERRN("Invalid PipeType=%s\n", GetPipeTypeString(queueContext->Info.PipeType));
			break;
		}

	case LIBUSBK_IOCTL_ISOEX_READ:
	case LIBUSBK_IOCTL_ISOEX_WRITE:
		if (queueContext->Info.PipeType == WdfUsbPipeTypeIsochronous)
		{
			XferIsoEx(Queue, Request);
			return;
		}
		status = STATUS_INVALID_PARAMETER;
		USBERRN("Invalid PipeType=%s\n", GetPipeTypeString(queueContext->Info.PipeType));
		break;

		/*
		case LIBUSBK_IOCTL_AUTOISOEX_READ:
		case LIBUSBK_IOCTL_AUTOISOEX_WRITE:
			if (queueContext->Info.PipeType == WdfUsbPipeTypeIsochronous)
			{
				XferAutoIsoEx(Queue, Request);
				return;
			}
			status = STATUS_INVALID_PARAMETER;
			USBERRN("Invalid PipeType=%s\n", GetPipeTypeString(queueContext->Info.PipeType));
			break;
		*/

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
VOID PipeQueue_OnRead(__in WDFQUEUE Queue,
					  __in WDFREQUEST Request,
					  __in size_t OutputBufferLength)
{
	NTSTATUS				status = STATUS_SUCCESS;
	PDEVICE_CONTEXT         deviceContext;
	PREQUEST_CONTEXT		requestContext;
	PQUEUE_CONTEXT queueContext = NULL;

	UNREFERENCED_PARAMETER(OutputBufferLength);

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	requestContext = GetRequestContext(Request);

	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status)) goto Done;

	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context");
		goto Done;
	}

	if (!queueContext->PipeHandle)
	{
		USBERR("null pipe handle\n");
		status = STATUS_INVALID_HANDLE;
		goto Done;
	}

	switch(queueContext->Info.PipeType)
	{
	case WdfUsbPipeTypeIsochronous:
		if (USB_ENDPOINT_DIRECTION_IN(queueContext->Info.EndpointAddress))
		{
			XferIsoRead(Queue, Request);
			return;
		}
		break;

	case WdfUsbPipeTypeBulk:
	case WdfUsbPipeTypeInterrupt:
		if (!USB_ENDPOINT_DIRECTION_IN(queueContext->Info.EndpointAddress))
		{
			status = STATUS_INVALID_DEVICE_REQUEST;
			USBERRN("Cannot read from an OUT pipe.");
			goto Done;
		}
		if(requestContext->Policies.RawIO)
			Xfer_ReadBulkRaw(Queue, Request);
		else
			Xfer_ReadBulk(Queue, Request);

		return;
	}

	status = STATUS_INVALID_DEVICE_REQUEST;
	USBERRN("PipeID=%02Xh Invalid request", queueContext->Info.EndpointAddress);

Done:
	WdfRequestCompleteWithInformation(Request, status, 0);

}

VOID PipeQueue_OnWrite(__in WDFQUEUE Queue,
					   __in WDFREQUEST Request,
					   __in size_t InputBufferLength)
{
	NTSTATUS				status = STATUS_SUCCESS;
	PDEVICE_CONTEXT         deviceContext;
	PREQUEST_CONTEXT		requestContext;
	PQUEUE_CONTEXT queueContext = NULL;

	UNREFERENCED_PARAMETER(InputBufferLength);

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	requestContext = GetRequestContext(Request);

	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status)) goto Done;

	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context");
		goto Done;
	}

	if (!queueContext->PipeHandle)
	{
		USBERR("null pipe handle\n");
		status = STATUS_INVALID_HANDLE;
		goto Done;
	}

	switch(queueContext->Info.PipeType)
	{
	case WdfUsbPipeTypeIsochronous:
		if (USB_ENDPOINT_DIRECTION_OUT(queueContext->Info.EndpointAddress))
		{
			XferIsoWrite(Queue, Request);
			return;
		}
		break;

	case WdfUsbPipeTypeBulk:
	case WdfUsbPipeTypeInterrupt:
		if (USB_ENDPOINT_DIRECTION_IN(queueContext->Info.EndpointAddress))
		{
			status = STATUS_INVALID_DEVICE_REQUEST;
			USBERRN("Cannot write to an IN pipe.");
			goto Done;
		}
		if(requestContext->Policies.RawIO)
			Xfer_WriteBulkRaw(Queue, Request);
		else
			Xfer_WriteBulk(Queue, Request);
	}

	status = STATUS_INVALID_DEVICE_REQUEST;
	USBERRN("PipeID=%02Xh Invalid request", queueContext->Info.EndpointAddress);

Done:
	WdfRequestCompleteWithInformation(Request, status, 0);

}

/* EvtIoStop
Invoked for every inflight pipe write request.  The callback executes when:
1) libusbK will stop the queue because of queue policy changes.
2) The system requests stand-by
3) The device is removed
*/
VOID Queue_OnStop(
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request,
    __in ULONG ActionFlags)
{
	PQUEUE_CONTEXT queueContext;
	PREQUEST_CONTEXT requestContext;

	if (Request == NULL || Queue == NULL)
	{
		USBERRN("Invalid wdf object.");
		return;
	}
	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		USBERRN("Invalid queue context.");
		return;
	}
	if ((requestContext = GetRequestContext(Request)) == NULL)
	{
		USBERRN("Invalid request context.");
		return;
	}

	// None of the libusbK transfer functions set EvtRequestCancel, hence this should never happen.
	if (ActionFlags & WdfRequestStopRequestCancelable)
	{
		USBERRN("WdfRequestStopRequestCancelable! pipeID=%02Xh", queueContext->Info.EndpointAddress);
		WdfVerifierDbgBreakPoint();
		return;
	}

	if (ActionFlags & WdfRequestStopActionSuspend)
	{

		USBDBGN("StopAcknowledge for ActionSuspend. pipeID=%02Xh request=%p timeout=%d", 
			queueContext->Info.EndpointAddress, Request, requestContext->Timeout);

		WdfRequestStopAcknowledge(Request, FALSE);
	}
	else if(ActionFlags & WdfRequestStopActionPurge)
	{
		USBDBGN("CancelSentRequest for ActionPurge. pipeID=%02Xh request=%p timeout=%d", 
				queueContext->Info.EndpointAddress, Request, requestContext->Timeout);

		WdfRequestCancelSentRequest(Request);
	}
}

/* EvtIoResume
Invoked for pipe write requests which were "StopAcknowledge" in EvtIoStop.
This callback exucutes after the device has re-entered D0(working).
*/
VOID Queue_OnResume(WDFQUEUE Queue, WDFREQUEST Request)
{
	PQUEUE_CONTEXT queueContext;
	PREQUEST_CONTEXT requestContext;
	NTSTATUS status = STATUS_SUCCESS;

	if (Request == NULL || Queue == NULL)
	{
		USBERRN("Invalid wdf object.");
		return;
	}
	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		USBERRN("Invalid queue context.");
		return;
	}
	if ((requestContext = GetRequestContext(Request)) == NULL)
	{
		USBERRN("Invalid request context.");
		return;
	}

	mXfer_HandlePipeResetScenarios(status, queueContext, requestContext);
}
