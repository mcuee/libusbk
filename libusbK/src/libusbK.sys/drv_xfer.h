/*! \file drv_xfer.h
*/

#ifndef __KUSB_XFER_H__
#define __KUSB_XFER_H__

#include "drv_private.h"

#define GetRequestStageSize(ReqestContext) \
	(GetPolicyValue(MAX_TRANSFER_STAGE_SIZE,ReqestContext->PipeContext->Policy))


VOID Xfer (
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request,
    __in size_t InputBufferLength,
    __in size_t OutputBufferLength);

VOID XferCtrl (
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request,
    __in size_t InputBufferLength,
    __in size_t OutputBufferLength);

VOID XferIsoFS (
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request,
    __in size_t InputBufferLength,
    __in size_t OutputBufferLength);

VOID XferIsoHS(__in WDFQUEUE Queue,
               __in WDFREQUEST Request,
               __in ULONG Length);

FORCEINLINE NTSTATUS SetRequestTimeout(__in PREQUEST_CONTEXT requestContext,
                                       __in WDFREQUEST wdfRequest,
                                       __inout PWDF_REQUEST_SEND_OPTIONS wdfSendOptions)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT timeoutMS = ((requestContext->IoControlRequest.timeout > 0)
	                  ? requestContext->IoControlRequest.timeout
	                  : GetPolicyValue(PIPE_TRANSFER_TIMEOUT, requestContext->PipeContext->Policy));

	if (timeoutMS)
	{
		USBDBG("timeout=%ums\n", timeoutMS);
		WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(wdfSendOptions, WDF_REL_TIMEOUT_IN_MS(timeoutMS));
		if (!NT_SUCCESS((status = WdfRequestAllocateTimer(wdfRequest))))
		{
			USBERR("WdfRequestAllocateTimer failed. status=%Xh\n", status);
		}
	}

	return status;
}

FORCEINLINE NTSTATUS SubmitAsyncRequest(
    __in PREQUEST_CONTEXT requestContext,
    __in WDFREQUEST wdfRequest,
    __in EVT_WDF_REQUEST_COMPLETION_ROUTINE completionRoutine,
    __in PWDF_REQUEST_SEND_OPTIONS wdfSendOptions)
{
	NTSTATUS status = STATUS_SUCCESS;
	WdfRequestSetCompletionRoutine(wdfRequest, completionRoutine, NULL);
	if (!WdfRequestSend(wdfRequest, WdfUsbTargetPipeGetIoTarget(requestContext->PipeContext->Pipe), wdfSendOptions))
	{
		status = WdfRequestGetStatus(wdfRequest);
		USBERR("WdfRequestSend failed. pipeID=%02Xh status=%Xh\n",
		       GetRequestPipeID(requestContext), status);

		// [tr] I suspect this could only happen if a lower driver was buggy.
		ASSERT(!NT_SUCCESS(status));
	}
	return status;
}

FORCEINLINE NTSTATUS UrbFormatBulkRequestContext(
    __in WDFREQUEST Request,
    __in PREQUEST_CONTEXT requestContext,
    __out_opt PULONG stageTransferSize,
    __out_opt PULONG remainingTransferSize)
{
	NTSTATUS status;
	PURB urb = NULL;
	ULONG remaining = requestContext->Length - requestContext->TotalTransferred;
	ULONG stageSize = GetRequestStageSize(requestContext);
	ULONG nextStageSize = remaining > stageSize ? stageSize : remaining;
	UCHAR pipeID = GetRequestPipeID(requestContext);

	// Return the length of the next stage transfer (bytes we are sending for this one) to the caller.
	if (stageTransferSize)
		*stageTransferSize = nextStageSize;

	// Return the the 'projected' remaing bytes (after a successful completion of this stage) to the caller.
	if (remainingTransferSize)
		*remainingTransferSize = (remaining - nextStageSize);

	if (requestContext->UrbMemory == WDF_NO_HANDLE)
	{
		// This is the first stage of transfer(s).
		// Create the URB and set all the initial requestContext members.

		WDF_OBJECT_ATTRIBUTES urbAttributes;
		USBD_PIPE_HANDLE usbdPipeHandle;
		ULONG urbFlags = (requestContext->RequestType == WdfRequestTypeRead) ?
		                 USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK : USBD_TRANSFER_DIRECTION_OUT;

		CONST PCHAR dirString = GetEndpointDirString(pipeID);
		CONST PCHAR requestTypeString = GetRequestTypeString(requestContext->RequestType);

		if (dirString != requestTypeString)
		{
			USBERR("cannot %s to a %s pipe pipeID=%02Xh\n",
			       requestTypeString, dirString, pipeID);
			status = STATUS_BAD_DEVICE_TYPE;
			goto Exit;
		}

		if (!requestContext->OriginalTransferMDL)
		{
			if (nextStageSize != 0)
			{
				USBERR("nextStageSize=%u and OriginalTransferMDL is NULL. pipeID=%02Xh\n",
				       nextStageSize,
				       pipeID);
				status = STATUS_INSUFFICIENT_RESOURCES;
				goto Exit;

			}

			if (USB_ENDPOINT_DIRECTION_IN(pipeID))
			{
				// Can only happen if the completion event submits a zero length transfer to a read (out) pipe. (never).
				USBERR("Zero-length buffer. pipeID=%02Xh\n", pipeID);
				status = STATUS_BUFFER_TOO_SMALL;
				goto Exit;
			}

			USBWRN("zero-length transfer. pipeID=%02Xh\n", pipeID);
		}
		else
		{
			requestContext->VirtualAddress = (ULONG_PTR) MmGetMdlVirtualAddress(requestContext->OriginalTransferMDL);
			requestContext->Mdl = IoAllocateMdl((PVOID) requestContext->VirtualAddress, requestContext->Length, FALSE, FALSE, NULL);
			if (requestContext->Mdl == NULL)
			{
				USBERR("IoAllocateMdl failed for requestContext->Mdl\n");
				status = STATUS_INSUFFICIENT_RESOURCES;
				goto Exit;
			}

			//
			// Build an mdl that will describe stage sections of the buffer. (VirtualAddress=Start offset) (nextStageSize=transfer length).
			// As Transfers complete successfully, the VirtualAddress is increased by the number of bytes transferred.
			// (usually nextStageSize depending on pipe policy settings).
			//
			IoBuildPartialMdl(requestContext->OriginalTransferMDL,
			                  requestContext->Mdl,
			                  (PVOID) requestContext->VirtualAddress,
			                  nextStageSize);
		}


		// When the framework deletrs the request, let it delete urbMemory too.
		WDF_OBJECT_ATTRIBUTES_INIT(&urbAttributes);
		urbAttributes.ParentObject = Request;

		status = WdfMemoryCreate(&urbAttributes,
		                         NonPagedPool,
		                         POOL_TAG,
		                         sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
		                         &requestContext->UrbMemory,
		                         (PVOID*) &urb);

		if (!NT_SUCCESS(status))
		{
			USBERR("WdfMemoryCreate failed for urbMemory.\n");
			goto Exit;
		}

		// get the wdm pipe handle. It is needed to FormatRequestForUrb.
		usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(requestContext->PipeContext->Pipe);
		if (!usbdPipeHandle)
		{
			USBERR("WdfUsbTargetPipeWdmGetPipeHandle failed for requestContext->PipeContext->Pipe\n");
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto Exit;
		}

		// Fill the URB structure.
		UsbBuildInterruptOrBulkTransferRequest(urb,
		                                       sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
		                                       usbdPipeHandle,
		                                       NULL,
		                                       requestContext->Mdl,
		                                       nextStageSize,
		                                       urbFlags,
		                                       NULL);
	}
	else // (stages) URB has already been created; this is a continuation of a larger transfer.
	{
		if (requestContext->Length == 0)
		{
			// Special case when a final zlp request is submitted for the SHORT_PACKET_TERMINATE policy.
			remaining = 0;
			nextStageSize = 0;
		}
		if (nextStageSize == 0 && USB_ENDPOINT_DIRECTION_IN(pipeID))
		{
			// Can only happen if the completion event submits a zero length transfer to a read (out) pipe. (never).
			USBERR("zero-length buffer. pipeID=%02Xh\n", pipeID);
			status = STATUS_BUFFER_TOO_SMALL;
			goto Exit;
		}
		// NOTE: requestContext->TotalTransferred and requestContext->VirtualAddress must be updated in the
		//       completion event before calling this function from the completion routine.

		// Fetch the urb structure for updates.
		urb = (PURB) WdfMemoryGetBuffer(requestContext->UrbMemory, NULL);
		if (!urb)
		{
			USBERR("WdfMemoryGetBuffer failed for requestContext->UrbMemory\n");
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto Exit;
		}

		// Update the URB transfer length.
		urb->UrbBulkOrInterruptTransfer.TransferBufferLength = nextStageSize;
		if (nextStageSize > 0)
		{
			// Continue reusing the Mdl created in the first stage transfer.
			MmPrepareMdlForReuse(requestContext->Mdl);

			// Update the mdl start offset and length.
			// requestContext->VirtualAddress must be updated in the complete event before calling this function again!
			IoBuildPartialMdl(requestContext->OriginalTransferMDL, requestContext->Mdl, (PVOID) requestContext->VirtualAddress, nextStageSize);
		}
		else
		{
			// Occurs when SHORT_PACKET_TERMINATE policy is TRUE.
			USBDBG("sending Zero-length-packet. pipeID=%02Xh\n", pipeID);
			urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
		}
	}

	// Assign the new urb to the request and prepare it for WdfRequestSend.
	status = WdfUsbTargetPipeFormatRequestForUrb(requestContext->PipeContext->Pipe, Request, requestContext->UrbMemory, NULL);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfUsbTargetPipeFormatRequestForUrb failed. status=%Xh\n", status);
		goto Exit;
	}

Exit:
	return status;
}

#endif
