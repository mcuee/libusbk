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

#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LUsbW_EvtIoRead)
#pragma alloc_text(PAGE, LUsbW_EvtIoWrite)
#pragma alloc_text(PAGE, GetPipeContextFromName)
#endif

#define GET_IN_BUFFER(MinimumBufferSize, BufferRef, LengthRef, ErrorText) \
	status = WdfRequestRetrieveInputBuffer(Request, MinimumBufferSize, BufferRef, LengthRef); \
	if(!NT_SUCCESS(status)) \
	{ \
		USBERR("%s WdfRequestRetrieveInputBuffer failed status=%Xh\n", ErrorText, status); \
		break; \
	}

NTSTATUS GetTransferMdl(WDFREQUEST Request, WDF_REQUEST_TYPE RequestType, PMDL* requestMdl)
{
	if(RequestType == WdfRequestTypeRead || RequestType == WdfRequestTypeDeviceControl)
	{
		return WdfRequestRetrieveOutputWdmMdl(Request, requestMdl);
	}
	else
	{
		return WdfRequestRetrieveInputWdmMdl(Request, requestMdl);
	}
}

VOID LUsbW_EvtIoRead(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length)
/*++

Routine Description:

    Called by the framework when it receives Read requests.

Arguments:

    Queue - Default queue handle
    Request - Handle to the read/write request
    Lenght - Length of the data buffer associated with the request.
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:


--*/
{
	PFILE_CONTEXT           fileContext = NULL;
	NTSTATUS				status = STATUS_SUCCESS;

	PAGED_CODE();

	//
	// Get the pipe associate with this request.
	//
	fileContext = GetFileContext(WdfRequestGetFileObject(Request));

	if (!fileContext || !fileContext->PipeContext || !fileContext->PipeContext->Pipe || !fileContext->PipeContext->IsValid)
	{
		USBERR0("Invalid pipe context or handle.\n");
		status = STATUS_INVALID_PARAMETER;
		goto Error;
	}

	AddRequestToPipeQueue(Queue, Request, (ULONG)Length, fileContext->PipeContext);
	return;

Error:
	WdfRequestCompleteWithInformation(Request, status, 0);

	return;
}

VOID
LUsbW_EvtIoWrite(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length
)
/*++

Routine Description:

    Called by the framework when it receives Write requests.

Arguments:

    Queue - Default queue handle
    Request - Handle to the read/write request
    Lenght - Length of the data buffer associated with the request.
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:


--*/
{
	PFILE_CONTEXT           fileContext = NULL;
	NTSTATUS				status = STATUS_SUCCESS;

	PAGED_CODE();

	//
	// Get the pipe associate with this request.
	//
	fileContext = GetFileContext(WdfRequestGetFileObject(Request));

	if (!fileContext || !fileContext->PipeContext || !fileContext->PipeContext->Pipe || !fileContext->PipeContext->IsValid)
	{
		USBERR0("Invalid pipe context or handle.\n");
		status = STATUS_INVALID_PARAMETER;
		goto Error;
	}

	AddRequestToPipeQueue(Queue, Request, (ULONG)Length, fileContext->PipeContext);
	return;

Error:
	WdfRequestCompleteWithInformation(Request, status, 0);

	return;
}

PPIPE_CONTEXT GetPipeContextFromName(IN PDEVICE_CONTEXT DeviceContext,
                                     IN PUNICODE_STRING FileName)
/*++

Routine Description:

    This routine will pass the string pipe name and
    fetch the pipe handle.

Arguments:

    DeviceContext - pointer to Device Context

    FileName - string pipe name

Return Value:

    The device extension maintains a pipe context for
    the pipes on 82930 board.

--*/
{
	INT					nameLength, index;
	ULONG				uval;
	ULONG				umultiplier;
	PPIPE_CONTEXT		pipeContext = NULL;
	PAGED_CODE();

	nameLength = (INT)(FileName->Length / sizeof(WCHAR));
	index = nameLength - 1;
	while(index > 3)
	{
		if ( ((FileName->Buffer[index] >= (WCHAR)'0') && (FileName->Buffer[index] <= (WCHAR)'9')) ||
		        ((FileName->Buffer[index] >= (WCHAR)'A') && (FileName->Buffer[index] <= (WCHAR)'F')))
		{
			break;
		}
		index--;
	}

	if (index <= 3) goto Error;

	uval = 0;
	umultiplier = 1;

	while(index > 3)
	{
		if ((FileName->Buffer[index] >= (WCHAR)'0') && (FileName->Buffer[index] <= (WCHAR)'9'))
		{
			uval += (umultiplier * (ULONG) (FileName->Buffer[index] - (WCHAR) '0'));
		}
		else if ((FileName->Buffer[index] >= (WCHAR)'A') && (FileName->Buffer[index] <= (WCHAR)'F'))
		{
			uval += (umultiplier * (ULONG) (FileName->Buffer[index] - ((WCHAR)'A') + 10));
		}
		else
		{
			break;
		}

		umultiplier<<=1;
		index--;
	}

	// do some extra validation; the format is PIPE_XX where XX is the endpoint address, not the index.
	if ((index > 3) && FileName->Buffer[index] == (WCHAR)'_' &&
	        (FileName->Buffer[index - 1] == (WCHAR)'E' || FileName->Buffer[index - 1] == (WCHAR)'e') &&
	        (FileName->Buffer[index - 2] == (WCHAR)'P' || FileName->Buffer[index - 2] == (WCHAR)'p') &&
	        (FileName->Buffer[index - 3] == (WCHAR)'I' || FileName->Buffer[index - 3] == (WCHAR)'i') &&
	        (FileName->Buffer[index - 4] == (WCHAR)'P' || FileName->Buffer[index - 4] == (WCHAR)'p'))
	{

		UCHAR pipeID = (UCHAR)uval;
		pipeContext = &GetContextByPipeID(DeviceContext, pipeID);
	}

Error:
	if (pipeContext == NULL)
	{
		USBWRN("invalid pipe filename=%wZ\n",FileName->Buffer);
	}
	return pipeContext;
}

VOID AddRequestToPipeQueue(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN ULONG            Length,
    IN PPIPE_CONTEXT	PipeContext)
/*++

Routine Description:

	Forwards the request to the endpoints IO queue.


Arguments:

    Queue			- The queue is which the request currently resides.
    Request			- The request to queue.
    Length			- Length of transfer buffer.
    RequestType		- Request type.
    PipeContext		- Pipe context to forward this request to.

Return Value:

    NT status value

--*/
{
	ULONG                   totalLength;
	ULONG                   packetSize;
	NTSTATUS                status = STATUS_SUCCESS;
	PDEVICE_CONTEXT         deviceContext;

	USBMSG0("begins\n");

	//
	// initialize vars
	//
	totalLength = Length;

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

	if((WdfUsbPipeTypeBulk != PipeContext->PipeInformation.PipeType) &&
	        (WdfUsbPipeTypeInterrupt != PipeContext->PipeInformation.PipeType) &&
	        (WdfUsbPipeTypeIsochronous != PipeContext->PipeInformation.PipeType))
	{
		USBERR0("Incorrect pipe type.\n");
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto Exit;

	}

	//
	// each packet can hold this much info
	//
	packetSize = PipeContext->PipeInformation.MaximumPacketSize;

	if(packetSize == 0)
	{
		USBERR0("Invalid parameter. MaximumPacketSize=0\n");
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	//
	// atleast packet worth of data to be transferred.
	//
	if(totalLength < packetSize)
	{
		if (PipeContext->PipeInformation.PipeType == WdfUsbPipeTypeIsochronous)
		{
			USBERR("Total transfer length is less than MaximumPacketSize.\n");
			status = STATUS_INVALID_PARAMETER;
			goto Exit;

		}
		else
		{
			USBWRN("Total transfer length is less than MaximumPacketSize.\n");
		}
	}

	if (!PipeContext->IsValid)
	{
		USBERR("Invalid pipe context.\n");
		status = STATUS_INVALID_DEVICE_STATE;
		goto Exit;

	}
	//
	// Now forward the requests to their own queues . These queues have Queue Level synch enabled
	// This is neccesssary to make sure that the cancel routine and dispatch routines are properly synched to
	// handle subrequests
	//
	status=WdfRequestForwardToIoQueue(Request, PipeContext->Queue);
	if(!NT_SUCCESS(status))
	{
		USBERR("WdfRequestForwardToIoQueue failed. status=%Xh\n", status);
	}
	else
	{
		return;
	}

Exit:
	WdfRequestCompleteWithInformation(Request, status, 0);
	return;
}

VOID LUsbW_EvtReadFromPipeQueue(IN WDFQUEUE         Queue,
                                IN WDFREQUEST       Request,
                                IN size_t           Length)
{
	NTSTATUS				status = STATUS_SUCCESS;
	PDEVICE_CONTEXT         deviceContext;
	PFILE_CONTEXT			fileContext = NULL;
	PMDL					requestMdl;

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	fileContext = GetFileContext(WdfRequestGetFileObject(Request));

	USBDBG("begins");

	if (!fileContext || !fileContext->PipeContext || !fileContext->PipeContext->Pipe || !fileContext->PipeContext->IsValid)
	{
		USBERR0("Invalid file or pipe context.\n");
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto Error;

	}

	status = GetTransferMdl(Request, WdfRequestTypeRead, &requestMdl);
	if (!NT_SUCCESS(status))
	{
		USBERR("GetTransferMdl failed. status=%Xh\n", status);
		status = STATUS_INVALID_PARAMETER;
		goto Error;
	}

	if (fileContext->PipeContext->PipeInformation.PipeType == WdfUsbPipeTypeIsochronous)
	{
		// This is an isochronous IN pipe.
		if(IsHighSpeedDevice(deviceContext))
		{

			// This is an isochronous IN pipe on a high speed device.
			TransferIsoHS(Queue,
			              Request,
			              (ULONG)Length,
			              WdfRequestTypeRead,
			              requestMdl,
			              fileContext->PipeContext);
		}
		else
		{

			// This is an isochronous IN pipe on a full speed device.
			TransferIsoFS(Queue,
			              Request,
			              (ULONG)Length,
			              WdfRequestTypeRead,
			              requestMdl,
			              fileContext->PipeContext);
		}
	}
	else
	{
		// This is bulk/interrupt IN pipe.
		TransferBulk(Queue,
		             Request,
		             (ULONG)Length,
		             WdfRequestTypeRead,
		             requestMdl,
		             fileContext->PipeContext);
	}

	return;

Error:
	WdfRequestCompleteWithInformation(Request, status, 0);

}

VOID LUsbW_EvtWriteFromPipeQueue(IN WDFQUEUE         Queue,
                                 IN WDFREQUEST       Request,
                                 IN size_t           Length)
{
	NTSTATUS				status = STATUS_SUCCESS;
	PDEVICE_CONTEXT         deviceContext;
	PFILE_CONTEXT			fileContext = NULL;
	PMDL					requestMdl;

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	fileContext = GetFileContext(WdfRequestGetFileObject(Request));

	USBDBG("begins");

	if (!fileContext || !fileContext->PipeContext || !fileContext->PipeContext->Pipe || !fileContext->PipeContext->IsValid)
	{
		USBERR0("Invalid file or pipe context.\n");
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto Error;

	}

	status = GetTransferMdl(Request, WdfRequestTypeWrite, &requestMdl);
	if (!NT_SUCCESS(status))
	{
		USBERR("GetTransferMdl failed. status=%Xh\n", status);
		status = STATUS_INVALID_PARAMETER;
		goto Error;
	}

	if (fileContext->PipeContext->PipeInformation.PipeType == WdfUsbPipeTypeIsochronous)
	{
		// This is an isochronous OUT pipe.
		if(IsHighSpeedDevice(deviceContext))
		{
			// This is an isochronous OUT pipe on a high speed device.
			TransferIsoHS(Queue,
			              Request,
			              (ULONG)Length,
			              WdfRequestTypeWrite,
			              requestMdl,
			              fileContext->PipeContext);
		}
		else
		{

			// This is an isochronous OUT pipe on a full speed device.
			TransferIsoFS(Queue,
			              Request,
			              (ULONG)Length,
			              WdfRequestTypeWrite,
			              requestMdl,
			              fileContext->PipeContext);
		}
	}
	else
	{
		// This is a bulk/interrupt OUT pipe.
		TransferBulk(Queue,
		             Request,
		             (ULONG)Length,
		             WdfRequestTypeWrite,
		             requestMdl,
		             fileContext->PipeContext);
	}

	return;

Error:
	WdfRequestCompleteWithInformation(Request, status, 0);

}

VOID LUsbW_EvtIoDeviceControlFromPipeQueue(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t     OutputBufferLength,
    IN size_t     InputBufferLength,
    IN ULONG      IoControlCode)
{
	NTSTATUS				status;
	size_t					InputBufferLen;
	PPIPE_CONTEXT			pipeContext;
	PMDL					requestMdl;
	libusb_request*			request;
	PDEVICE_CONTEXT			pDevContext;

	UNREFERENCED_PARAMETER(InputBufferLength);

	pDevContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

	USBDBG("begins");

	switch(IoControlCode)
	{
	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "interrupt_or_bulk_read");

		status = GetTransferMdl(Request, WdfRequestTypeDeviceControl, &requestMdl);
		if (!NT_SUCCESS(status))
		{
			USBERR0("interrupt_or_bulk_read: unable to get transfer buffer\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		pipeContext = &GetContextByPipeID(pDevContext, (UCHAR)request->endpoint.endpoint);
		if (!pipeContext || !pipeContext->IsValid)
		{
			USBERR0("interrupt_or_bulk_read: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		TransferBulk(Queue, Request, (ULONG)OutputBufferLength, WdfRequestTypeRead, requestMdl, pipeContext);
		return;

	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "interrupt_or_bulk_write");

		status = GetTransferMdl(Request, WdfRequestTypeDeviceControl, &requestMdl);
		if (!NT_SUCCESS(status))
		{
			USBERR0("interrupt_or_bulk_write: unable to get transfer buffer\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		pipeContext = &GetContextByPipeID(pDevContext, (UCHAR)request->endpoint.endpoint);
		if (!pipeContext || !pipeContext->IsValid)
		{
			USBERR0("interrupt_or_bulk_write: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		TransferBulk(Queue, Request, (ULONG)OutputBufferLength, WdfRequestTypeWrite, requestMdl, pipeContext);
		return;
	case LIBUSB_IOCTL_ISOCHRONOUS_WRITE:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "isochronous_write");

		status = GetTransferMdl(Request, WdfRequestTypeDeviceControl, &requestMdl);
		if (!NT_SUCCESS(status))
		{
			USBERR0("isochronous_write: unable to get transfer buffer\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		pipeContext = &GetContextByPipeID(pDevContext, (UCHAR)request->endpoint.endpoint);
		if (!pipeContext || !pipeContext->IsValid)
		{
			USBERR0("isochronous_write: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		if (IsHighSpeedDevice(pDevContext))
		{
			TransferIsoHS(Queue, Request, (ULONG)OutputBufferLength, WdfRequestTypeWrite, requestMdl, pipeContext);
		}
		else
		{
			TransferIsoFS(Queue, Request, (ULONG)OutputBufferLength, WdfRequestTypeWrite, requestMdl, pipeContext);
		}
		return;
	case LIBUSB_IOCTL_ISOCHRONOUS_READ:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "isochronous_read");

		status = GetTransferMdl(Request, WdfRequestTypeDeviceControl, &requestMdl);
		if (!NT_SUCCESS(status))
		{
			USBERR0("isochronous_read: unable to get transfer buffer\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		pipeContext = &GetContextByPipeID(pDevContext, (UCHAR)request->endpoint.endpoint);
		if (!pipeContext || !pipeContext->IsValid)
		{
			USBERR0("isochronous_read: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		if (IsHighSpeedDevice(pDevContext))
		{
			TransferIsoHS(Queue, Request, (ULONG)OutputBufferLength, WdfRequestTypeRead, requestMdl, pipeContext);
		}
		else
		{
			TransferIsoFS(Queue, Request, (ULONG)OutputBufferLength, WdfRequestTypeRead, requestMdl, pipeContext);
		}
		return;
	default:
		USBERR("Invalid IoControlCode %Xh\n", IoControlCode);
		status = STATUS_INVALID_PARAMETER;
		break;

	}
	WdfRequestCompleteWithInformation(Request, status, 0);

	return;

}