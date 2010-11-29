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

#endif

EVT_WDF_WORKITEM ReadWriteWorkItem;


#if !defined(BUFFERED_READ_WRITE) // if doing DIRECT_IO

VOID TransferBulk(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN ULONG            Length,
    IN WDF_REQUEST_TYPE RequestType,
    IN PMDL				TransferMDL,
    IN PPIPE_CONTEXT	PipeContext
)
/*++

Routine Description:

    This callback is invoked when the framework received  WdfRequestTypeRead or
    WdfRequestTypeWrite request. This read/write is performed in stages of
    MAX_TRANSFER_SIZE. Once a stage of transfer is complete, then the
    request is circulated again, until the requested length of transfer is
    performed.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.

    Request - Handle to a framework request object. This one represents
              the WdfRequestTypeRead/WdfRequestTypeWrite IRP received by the framework.

    Length - Length of the input/output buffer.

Return Value:

   VOID

--*/
{
	PMDL                    newMdl = NULL;
	PURB                    urb = NULL;
	WDFMEMORY               urbMemory;
	ULONG                   totalLength = Length;
	ULONG                   stageLength = 0;
	ULONG                   urbFlags = 0;
	NTSTATUS                status;
	ULONG_PTR               virtualAddress = 0;
	PREQUEST_CONTEXT        rwContext = NULL;
	WDF_OBJECT_ATTRIBUTES   objectAttribs;
	USBD_PIPE_HANDLE        usbdPipeHandle;
	PDEVICE_CONTEXT         deviceContext;

	USBMSG0("begins\n");

	//
	// First validate input parameters.
	//
	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

	if (totalLength > deviceContext->MaximumTransferSize)
	{
		USBERR0("Transfer length > MaximumTransferSize\n");
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	if ((RequestType != WdfRequestTypeRead) &&
	        (RequestType != WdfRequestTypeWrite))
	{
		USBERR0("RequestType has to be either Read or Write\n");
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	if (!PipeContext)
	{
		USBERR0("Invalid pipe context\n");
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	if((WdfUsbPipeTypeBulk != PipeContext->PipeInformation.PipeType) &&
	        (WdfUsbPipeTypeInterrupt != PipeContext->PipeInformation.PipeType))
	{
		USBERR0("Usbd pipe type is not bulk or interrupt\n");
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto Exit;

	}

	rwContext = GetRequestContext(Request);

	if(RequestType == WdfRequestTypeRead)
	{

		urbFlags |= USBD_TRANSFER_DIRECTION_IN;
		rwContext->Read = TRUE;
		USBMSG0("Read operation\n");

	}
	else
	{
		urbFlags |= USBD_TRANSFER_DIRECTION_OUT;
		rwContext->Read = FALSE;
		USBMSG0("Write operation\n");
	}

	urbFlags |= USBD_SHORT_TRANSFER_OK;

	virtualAddress = (ULONG_PTR) MmGetMdlVirtualAddress(TransferMDL);

	//
	// the transfer request is for totalLength.
	// we can perform a max of MAX_TRANSFER_SIZE
	// in each stage.
	//
	if (totalLength > MAX_TRANSFER_SIZE)
	{
		stageLength = MAX_TRANSFER_SIZE;
	}
	else
	{
		stageLength = totalLength;
	}

	newMdl = IoAllocateMdl((PVOID) virtualAddress,
	                       totalLength,
	                       FALSE,
	                       FALSE,
	                       NULL);

	if (newMdl == NULL)
	{
		USBERR0("Failed to alloc mem for mdl\n");
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

	//
	// map the portion of user-buffer described by an mdl to another mdl
	//
	IoBuildPartialMdl(TransferMDL,
	                  newMdl,
	                  (PVOID) virtualAddress,
	                  stageLength);

	WDF_OBJECT_ATTRIBUTES_INIT(&objectAttribs);
	objectAttribs.ParentObject = Request;

	status = WdfMemoryCreate(&objectAttribs,
	                         NonPagedPool,
	                         POOL_TAG,
	                         sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
	                         &urbMemory,
	                         (PVOID*) &urb);

	if (!NT_SUCCESS(status))
	{
		USBERR0("Failed to alloc mem for urb\n");
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

	usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(PipeContext->Pipe);

	UsbBuildInterruptOrBulkTransferRequest(urb,
	                                       sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
	                                       usbdPipeHandle,
	                                       NULL,
	                                       newMdl,
	                                       stageLength,
	                                       urbFlags,
	                                       NULL);

	status = WdfUsbTargetPipeFormatRequestForUrb(PipeContext->Pipe, Request, urbMemory, NULL  );
	if (!NT_SUCCESS(status))
	{
		USBERR0("Failed to format requset for urb.\n");
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

	WdfRequestSetCompletionRoutine(Request, ReadWriteCompletion, NULL);

	//
	// set REQUEST_CONTEXT  parameters.
	//
	rwContext->UrbMemory			= urbMemory;
	rwContext->Mdl					= newMdl;
	rwContext->Length				= totalLength;
	rwContext->TotalTransferred		= 0;
	rwContext->VirtualAddress		= virtualAddress;
	rwContext->OriginalTransferMDL	= TransferMDL;
	rwContext->PipeContext			= PipeContext;

	if ((totalLength - stageLength) > 0)
	{
		//
		// This large transfer will be broken into smaller sub-request.
		// Temporarily stop this queue, otherwise request behind this one will
		// proccess before the sub requests.
		//
		USBMSG("Stopping %s queue.\n",
		       (RequestType == WdfRequestTypeRead) ? "read":"write");
		WdfIoQueueStop(PipeContext->Queue, NULL, NULL);
	}

	if (!WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(PipeContext->Pipe), WDF_NO_SEND_OPTIONS))
	{
		status = WdfRequestGetStatus(Request);
		ASSERT(!NT_SUCCESS(status));
	}
	else
	{

	}
Exit:
	if (!NT_SUCCESS(status))
	{
		if ((totalLength - stageLength) > 0)
		{
			// if the queue was stopped, restart it
			USBMSG("Starting %s queue.\n",
			       (RequestType == WdfRequestTypeRead) ? "read":"write");
			WdfIoQueueStart(PipeContext->Queue);
		}
		WdfRequestCompleteWithInformation(Request, status, 0);

		if (newMdl != NULL)
		{
			IoFreeMdl(newMdl);
		}
	}

	USBMSG0("ends\n");

	return;
}


VOID
ReadWriteCompletion(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
)
/*++

Routine Description:

    This is the completion routine for reads/writes
    If the irp completes with success, we check if we
    need to recirculate this irp for another stage of
    transfer.

Arguments:

    Context - Driver supplied context
    Device - Device handle
    Request - Request handle
    Params - request completion params

Return Value:
    None

--*/
{
	WDFUSBPIPE              pipe;
	ULONG                   stageLength;
	NTSTATUS               status;
	PREQUEST_CONTEXT        rwContext;
	PURB                    urb;
	PCHAR                   operation;
	ULONG                   transferred;
	ULONG					remaining = 0;
	WDFQUEUE				queue;

	UNREFERENCED_PARAMETER(Context);

	rwContext = GetRequestContext(Request);
	if (rwContext->Read)
	{
		operation = "Read";
	}
	else
	{
		operation = "Write";
	}

	pipe = (WDFUSBPIPE) Target;
	queue = rwContext->PipeContext->Queue;
	status = CompletionParams->IoStatus.Status;

	if (!NT_SUCCESS(status))
	{
		//
		// Queue a workitem to reset the pipe because the completion could be
		// running at DISPATCH_LEVEL.
		//
		//QueuePassiveLevelCallback(WdfIoTargetGetDevice(Target), pipe);
		goto End;
	}

	urb = (PURB) WdfMemoryGetBuffer(rwContext->UrbMemory, NULL);
	transferred = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
	remaining = rwContext->Length - transferred;

	rwContext->TotalTransferred += transferred;
	rwContext->VirtualAddress += transferred;
	rwContext->Length -= transferred;

	//
	// if there is nothing left to transfer or
	// we transferred less than what was requested and the SHORT_TRANSFER_OK flag is set
	//
	if (remaining == 0 ||
	        (transferred < MAX_TRANSFER_SIZE && urb->UrbBulkOrInterruptTransfer.TransferFlags & USBD_SHORT_TRANSFER_OK))
	{
		//
		// complete the request
		//
		WdfRequestSetInformation(Request, rwContext->TotalTransferred);
		goto End;
	}

	//
	// Start another transfer
	//
	USBMSG("stage next %s transfer...\n", operation);

	if (remaining > MAX_TRANSFER_SIZE)
	{
		stageLength = MAX_TRANSFER_SIZE;
	}
	else
	{
		stageLength = remaining;
	}


	//
	// Following call is required to free any mapping made on the partial MDL
	// and reset internal MDL state.
	//
	MmPrepareMdlForReuse(rwContext->Mdl);

	IoBuildPartialMdl(rwContext->OriginalTransferMDL,
	                  rwContext->Mdl,
	                  (PVOID) rwContext->VirtualAddress,
	                  stageLength);

	//
	// reinitialize the urb
	//
	urb->UrbBulkOrInterruptTransfer.TransferBufferLength = stageLength;


	//
	// Format the request to send a URB to a USB pipe.
	//
	status = WdfUsbTargetPipeFormatRequestForUrb(pipe,
	         Request,
	         rwContext->UrbMemory,
	         NULL);
	if (!NT_SUCCESS(status))
	{
		USBERR0("Failed to format requset for urb\n");
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto End;
	}

	WdfRequestSetCompletionRoutine(Request, ReadWriteCompletion, NULL);

	//
	// Send the request asynchronously.
	//
	if (!WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS))
	{
		USBERR("WdfRequestSend for %s failed\n", operation);
		status = WdfRequestGetStatus(Request);
		goto End;
	}

	if ((remaining - stageLength) == 0)
	{
		USBMSG("restarting %s queue.\n", operation);
		WdfIoQueueStart(queue);
//		if (WDF_IO_QUEUE_STOPPED(WdfIoQueueGetState(queue, NULL, NULL)))
//		{
//		}
	}
	//
	// Else when the request completes, this completion routine will be
	// called again.
	//
	return;

End:
	//
	// We are here because the request failed or some other call failed.
	// Dump the request context, complete the request and return.
	//
	DbgPrintRWContext(rwContext);

	IoFreeMdl(rwContext->Mdl);

	USBMSG("%s request completed. status=%Xh\n",
	       operation, status);

	if (queue)
	{
		if (WDF_IO_QUEUE_STOPPED(WdfIoQueueGetState(queue, NULL, NULL)))
		{
			USBMSG("restarting %s queue.\n", operation);
			WdfIoQueueStart(queue);
		}
	}

	WdfRequestComplete(Request, status);

	return;
}

#else

VOID
TransferBulk(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN ULONG            Length,
    IN WDF_REQUEST_TYPE RequestType
)
/*++

Routine Description:

    This callback is invoked when the framework received  WdfRequestTypeRead or
    RP_MJ_WRITE request. This read/write is performed in stages of
    MAX_TRANSFER_SIZE. Once a stage of transfer is complete, then the
    request is circulated again, until the requested length of transfer is
    performed.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.

    Request - Handle to a framework request object. This one represents
              the WdfRequestTypeRead/WdfRequestTypeWrite IRP received by the framework.

    Length - Length of the input/output buffer.

Return Value:

   VOID

--*/
{
	size_t                totalLength = Length;
	size_t                stageLength = 0;
	NTSTATUS              status;
	PVOID                 virtualAddress = 0;
	PREQUEST_CONTEXT      rwContext = NULL;
	PFILE_CONTEXT         fileContext = NULL;
	WDFUSBPIPE            pipe;
	WDFMEMORY             reqMemory;
	WDFMEMORY_OFFSET      offset;
	WDF_OBJECT_ATTRIBUTES objectAttribs;
	PDEVICE_CONTEXT       deviceContext;

	USBMSG0("begins\n");

	//
	// First validate input parameters.
	//
	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

	if (totalLength > deviceContext->MaximumTransferSize)
	{
		USBERR0("Transfer length > circular buffer\n");
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	if ((RequestType != WdfRequestTypeRead) &&
	        (RequestType != WdfRequestTypeWrite))
	{
		USBERR0("RequestType has to be either Read or Write\n");
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	//
	// Get the pipe associate with this request.
	//
	fileContext = GetFileContext(WdfRequestGetFileObject(Request));
	pipe = fileContext->Pipe;

	rwContext = GetRequestContext(Request);

	if(RequestType == WdfRequestTypeRead)
	{
		status = WdfRequestRetrieveOutputBuffer(Request, Length, &virtualAddress, &totalLength);
		rwContext->Read = TRUE;

	}
	else     //Write
	{

		status = WdfRequestRetrieveInputBuffer(Request, Length, &virtualAddress, &totalLength);
		rwContext->Read = FALSE;
	}

	if(!NT_SUCCESS(status))
	{
		USBERR0("WdfRequestRetrieveInputBuffer failed\n");
		goto Exit;
	}

	//
	// If the totalLength exceeds MAX_TRANSFER_SIZE, we will break
	// that into multiple transfer of size no more than MAX_TRANSFER_SIZE
	// in each stage.
	//
	if (totalLength > MAX_TRANSFER_SIZE)
	{
		stageLength = MAX_TRANSFER_SIZE;
	}
	else
	{
		stageLength = totalLength;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&objectAttribs);
	objectAttribs.ParentObject = Request;
	status = WdfMemoryCreatePreallocated(&objectAttribs,
	                                     virtualAddress,
	                                     totalLength,
	                                     &reqMemory);
	if(!NT_SUCCESS(status))
	{
		USBERR0("WdfMemoryCreatePreallocated failed\n");
		goto Exit;
	}

	offset.BufferOffset = 0;
	offset.BufferLength = stageLength;

	//
	// The framework format call validates to make sure that you are reading or
	// writing to the right pipe type, sets the appropriate transfer flags,
	// creates an URB and initializes the request.
	//
	if(RequestType == WdfRequestTypeRead)
	{

		USBMSG0("Read operation\n");
		status = WdfUsbTargetPipeFormatRequestForRead(pipe,
		         Request,
		         reqMemory,
		         &offset);
	}
	else
	{

		USBMSG0("Write operation\n");
		status = WdfUsbTargetPipeFormatRequestForWrite(pipe,
		         Request,
		         reqMemory,
		         &offset);
	}

	if (!NT_SUCCESS(status))
	{
		USBERR("WdfUsbTargetPipeFormatRequest failed 0x%x\n", status));
		goto Exit;
	}

	WdfRequestSetCompletionRoutine(
	    Request,
	    ReadWriteCompletion,
	    NULL);
	//
	// set REQUEST_CONTEXT parameters.
	//
	rwContext->Length  = totalLength - stageLength;
	rwContext->TotalTransferred = 0;

	//
	// Send the request asynchronously.
	//
	if (WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS) == FALSE)
	{
		USBERR0("WdfRequestSend failed\n");
		status = WdfRequestGetStatus(Request);
		goto Exit;
	}


Exit:
	if (!NT_SUCCESS(status))
	{
		WdfRequestCompleteWithInformation(Request, status, 0);
	}

	USBMSG0("ends\n");

	return;
}

// TODO: Use mouser sample trick to avoid stack recursion

VOID
ReadWriteCompletion(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
)
/*++

Routine Description:

    This is the completion routine for reads/writes
    If the irp completes with success, we check if we
    need to recirculate this irp for another stage of
    transfer.

Arguments:

    Context - Driver supplied context
    Device - Device handle
    Request - Request handle
    Params - request completion params

Return Value:
    None

--*/
{
	WDFUSBPIPE              pipe;
	ULONG                   stageLength;
	NTSTATUS               status;
	PREQUEST_CONTEXT        rwContext;
	ULONG                   bytesReadWritten;
	WDFMEMORY_OFFSET       offset;
	PCHAR                   operation;
	PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;

	UNREFERENCED_PARAMETER(Context);
	rwContext = GetRequestContext(Request);

	usbCompletionParams = CompletionParams->Parameters.Usb.Completion;
	rwContext = GetRequestContext(Request);

	if (rwContext->Read)
	{
		operation = "Read";
		bytesReadWritten =  usbCompletionParams->Parameters.PipeRead.Length;
	}
	else
	{
		operation = "Write";
		bytesReadWritten =  usbCompletionParams->Parameters.PipeWrite.Length;
	}

	pipe = (WDFUSBPIPE) Target;
	status = CompletionParams->IoStatus.Status;

	if (!NT_SUCCESS(status))
	{
		//
		// Queue a workitem to reset the pipe because the completion could be
		// running at DISPATCH_LEVEL.
		// TODO: preallocate per pipe workitem to avoid allocation failure.
		QueuePassiveLevelCallback(WdfIoTargetGetDevice(Target), pipe);
		goto End;
	}

	rwContext->TotalTransferred += bytesReadWritten;

	//
	// If there is anything left to transfer.
	//
	if (rwContext->Length == 0)
	{
		//
		// this is the last transfer
		//
		WdfRequestSetInformation(Request, rwContext->TotalTransferred);
		goto End;
	}

	//
	// Start another transfer
	//
	USBMSG("Stage next %s transfer...\n", operation));

	if (rwContext->Length > MAX_TRANSFER_SIZE)
{
	stageLength = MAX_TRANSFER_SIZE;
}
else
{
	stageLength = rwContext->Length;
}

offset.BufferOffset = rwContext->TotalTransferred;
                      offset.BufferLength = stageLength;

                              rwContext->Length -= stageLength;

                                      if (rwContext->Read)
{

	status = WdfUsbTargetPipeFormatRequestForRead(
	             pipe,
	             Request,
	             usbCompletionParams->Parameters.PipeRead.Buffer,
	             &offset);

	}
	else
	{

		status = WdfUsbTargetPipeFormatRequestForWrite(
		             pipe,
		             Request,
		             usbCompletionParams->Parameters.PipeWrite.Buffer,
		             &offset);

	}

	if (!NT_SUCCESS(status))
	{
		USBERR("WdfUsbTargetPipeFormat%sRequest failed 0x%x\n",
		       operation, status));
		goto End;
	}

	WdfRequestSetCompletionRoutine(
	    Request,
	    ReadWriteCompletion,
	    NULL);

	//
	// Send the request asynchronously.
	//
	if (!WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS))
	{
		USBERR("WdfRequestSend for %s failed\n", operation));
		status = WdfRequestGetStatus(Request);
		         goto End;
	         }

	         //
	         // Else when the request completes, this completion routine will be
	         // called again.
	         //
	         return;

               End:
	               //
	               // We are here because the request failed or some other call failed.
	               // Dump the request context, complete the request and return.
	               //
	               DbgPrintRWContext(rwContext);

	               USBMSG("%s request completed. status=%Xh\n",
	                      operation, status));

	               WdfRequestComplete(Request, status);

	               return;
                     }

#endif

VOID
ReadWriteWorkItem(
    IN WDFWORKITEM  WorkItem
)
{
	PWORKITEM_CONTEXT pItemContext;
	NTSTATUS status;

	USBMSG0("ReadWriteWorkItem called\n");

	pItemContext = GetWorkItemContext(WorkItem);

	status = ResetPipe(pItemContext->Pipe);
	if (!NT_SUCCESS(status))
	{

		USBERR("ResetPipe failed. status=%Xh\n", status);

		status = ResetDevice(pItemContext->Device);
		if(!NT_SUCCESS(status))
		{

			USBERR("ResetDevice failed. status=%Xh\n", status);
		}
	}

	WdfObjectDelete(WorkItem);

	return;
}

NTSTATUS
QueuePassiveLevelCallback(
    IN WDFDEVICE    Device,
    IN WDFUSBPIPE   Pipe
)
/*++

Routine Description:

    This routine is used to queue workitems so that the callback
    functions can be executed at PASSIVE_LEVEL in the conext of
    a system thread.

Arguments:


Return Value:

--*/
{
	NTSTATUS                       status = STATUS_SUCCESS;
	PWORKITEM_CONTEXT               context;
	WDF_OBJECT_ATTRIBUTES           attributes;
	WDF_WORKITEM_CONFIG             workitemConfig;
	WDFWORKITEM                     hWorkItem;

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&attributes, WORKITEM_CONTEXT);
	attributes.ParentObject = Device;

	WDF_WORKITEM_CONFIG_INIT(&workitemConfig, ReadWriteWorkItem);

	status = WdfWorkItemCreate( &workitemConfig,
	                            &attributes,
	                            &hWorkItem);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	context = GetWorkItemContext(hWorkItem);

	context->Device = Device;
	context->Pipe = Pipe;

	//
	// Execute this work item.
	//
	WdfWorkItemEnqueue(hWorkItem);

	return STATUS_SUCCESS;
}

VOID
DbgPrintRWContext(
    PREQUEST_CONTEXT rwContext
)
{
	UNREFERENCED_PARAMETER(rwContext);

	USBDBG("rwContext->UrbMemory       = %p\n",
	       rwContext->UrbMemory);
	USBDBG("rwContext->Mdl             = %p\n",
	       rwContext->Mdl);
	USBDBG("rwContext->Length          = %d\n",
	       rwContext->Length);
	USBDBG("rwContext->TotalTransferred= %d\n",
	       rwContext->TotalTransferred);
	USBDBG("rwContext->VirtualAddress  = %p\n",
	       rwContext->VirtualAddress);
	return;
}


