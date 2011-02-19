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
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG length = 0;
	PUCHAR inputBuffer, outputBuffer;
	libusb_request* libusbRequest;
	PDEVICE_CONTEXT deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	PREQUEST_CONTEXT requestContext = GetRequestContext(Request);

	UNREFERENCED_PARAMETER(InputBufferLength);

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
				Xfer_IsoHS(Queue, Request, (ULONG)OutputBufferLength);
			else
				Xfer_IsoFS(Queue, Request);
			return;

		case WdfUsbPipeTypeBulk:
		case WdfUsbPipeTypeInterrupt:

			Xfer(Queue, Request);
			return;

		}

		USBERR("invalid pipeType=%u\n", requestContext->PipeContext->PipeInformation.PipeType);
		status = STATUS_INVALID_PARAMETER;
		break;

	case LIBUSB_IOCTL_GET_DESCRIPTOR:
	case LIBUSB_IOCTL_CONTROL_READ:
	case LIBUSB_IOCTL_CONTROL_WRITE:

		Xfer_Control(Queue, Request);
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
		USBERR("Invalid IoControlCode %Xh\n", IoControlCode);
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	WdfRequestCompleteWithInformation(Request, status, length);
	return;

}
VOID NONPAGABLE PipeQueue_OnRead(__in WDFQUEUE Queue,
                                 __in WDFREQUEST Request,
                                 __in size_t Length)
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
			Xfer_IsoHS(Queue, Request, (ULONG)Length);
		else
			Xfer_IsoFS(Queue, Request);
	}
	else // bulk/interrupt pipe.
		Xfer(Queue, Request);

	return;

Done:
	WdfRequestCompleteWithInformation(Request, status, 0);

}

VOID NONPAGABLE PipeQueue_OnWrite(__in WDFQUEUE Queue,
                                  __in WDFREQUEST Request,
                                  __in size_t Length)
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
			Xfer_IsoHS(Queue, Request, (ULONG)Length);
		else
			Xfer_IsoFS(Queue, Request);
	}
	else // bulk/interrupt pipe.
		Xfer(Queue, Request);

	return;

Done:
	WdfRequestCompleteWithInformation(Request, status, 0);

}
