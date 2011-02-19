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
#include "drv_xfer.h"

#if (defined(ALLOC_PRAGMA) && defined(PAGING_ENABLED))
#endif

EVT_WDF_REQUEST_COMPLETION_ROUTINE Xfer_OnComplete;

VOID Xfer(__in WDFQUEUE Queue,
          __in WDFREQUEST Request)
{
	NTSTATUS                status;
	PREQUEST_CONTEXT        requestContext = NULL;
	PDEVICE_CONTEXT         deviceContext;
	ULONG					stageLength = 0;
	ULONG					remaining = 0;
	UCHAR					pipeID = 0;
	BOOLEAN					queueLocked = TRUE;
	WDF_REQUEST_SEND_OPTIONS sendOptions;

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	requestContext = GetRequestContext(Request);

	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status))
	{
		WdfRequestCompleteWithInformation(Request, status, 0);
		return;
	}

	pipeID = GetRequestPipeID(requestContext);

	// the MaximumTransferSize in field was modified to be a stage size.
	if (requestContext->Length > GetPolicyValue(MAXIMUM_TRANSFER_SIZE, requestContext->PipeContext->Policy))
	{
		USBERR("pipeID=%02Xh transfer length cannot be greater than the MaximumTransferSize (%u bytes)\n",
		       pipeID, GetPolicyValue(MAXIMUM_TRANSFER_SIZE, requestContext->PipeContext->Policy));
		status = STATUS_INVALID_BUFFER_SIZE;
		goto Exit;
	}

	USBMSG("[%s-%s] pipeID=%02Xh length=%u stageLength=%u\n",
	       GetPipeTypeString(requestContext->PipeContext->PipeInformation.PipeType),
	       GetRequestTypeString(requestContext->RequestType),
	       pipeID,
	       requestContext->Length,
	       GetRequestStageSize(requestContext));

	status = UrbFormatBulkRequestContext(Request, requestContext, &stageLength, &remaining);
	if (!NT_SUCCESS(status))
	{
		USBERR("pipeID=%02Xh UrbFormatBulkRequestContext failed. status=%Xh\n", pipeID, status);
		goto Exit;
	}

	WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, 0);
	status = SetRequestTimeout(requestContext, Request, &sendOptions);
	if (!NT_SUCCESS(status))
		goto Exit;


	if (!remaining &&
	        !GetPolicyValue(IGNORE_SHORT_PACKETS, requestContext->PipeContext->Policy) &&
	        !GetPolicyValue(SHORT_PACKET_TERMINATE, requestContext->PipeContext->Policy))
	{
		queueLocked = FALSE;
	}
	else
	{
		queueLocked = TRUE;
	}

	requestContext->QueueLocked = queueLocked;
	status = WdfWaitLockAcquire(requestContext->PipeContext->PipeLock, NULL);
	if (!NT_SUCCESS(status))
	{
		USBERR("pipeID=%02Xh WdfWaitLockAcquire failed. status=%Xh\n", pipeID, status);
		goto Exit;
	}

	status = SubmitAsyncRequest(requestContext, Request, Xfer_OnComplete, &sendOptions);
	if (!NT_SUCCESS(status) || !queueLocked)
	{
		WdfWaitLockRelease(requestContext->PipeContext->PipeLock);
	}

	if (NT_SUCCESS(status))
		return;


Exit:
	if (!NT_SUCCESS(status))
	{
		WdfRequestCompleteWithInformation(Request, status, 0);

		if (requestContext)
		{
			if (requestContext->Mdl)
			{
				IoFreeMdl(requestContext->Mdl);
				requestContext->Mdl = NULL;
			}
		}
	}
	return;
}

VOID Xfer_OnComplete(__in WDFREQUEST Request,
                     __in WDFIOTARGET Target,
                     __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
                     __in WDFCONTEXT Context)
{
	NTSTATUS status;
	PREQUEST_CONTEXT requestContext;
	PURB urb;
	ULONG transferBufferLength = 0;
	ULONG stageRequested;
	ULONG remaining;
	UCHAR pipeID = 0;

	UNREFERENCED_PARAMETER(Target);
	UNREFERENCED_PARAMETER(Context);

	// Make sure the request context is still valid.
	requestContext = GetRequestContext(Request);
	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (status != STATUS_SUCCESS)
		goto Exit;

	pipeID = GetRequestPipeID(requestContext);
	status = CompletionParams->IoStatus.Status;
	switch(status)
	{
	case STATUS_SUCCESS:
		break;
	case STATUS_TIMEOUT:
		status = STATUS_IO_TIMEOUT;
	case STATUS_IO_TIMEOUT:
		USBWRN("[Timeout] pipeID=%02Xh ioStatus=%Xh requestStatus=%Xh\n",
		       pipeID, CompletionParams->IoStatus.Status, WdfRequestGetStatus(Request));
		break;
	case STATUS_CANCELLED:
		USBWRN("[Cancelled] pipeID=%02Xh ioStatus=%Xh requestStatus=%Xh\n",
		       pipeID, CompletionParams->IoStatus.Status, WdfRequestGetStatus(Request));
		break;
	default:
		// TODO: Queue a workitem to reset the pipe because the completion could be running at DISPATCH_LEVEL.
		// i.e: QueuePassiveLevelCallback(WdfIoTargetGetDevice(Target), pipe);
		USBERR("pipeID=%02Xh ioStatus=%Xh requestStatus=%Xh\n",
		       pipeID, CompletionParams->IoStatus.Status, WdfRequestGetStatus(Request));
		break;
	}

	if ((urb = (PURB) WdfMemoryGetBuffer(requestContext->UrbMemory, NULL)) != NULL)
	{
		transferBufferLength = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

		stageRequested = GetRequestStageSize(requestContext);

		requestContext->TotalTransferred += transferBufferLength;
		requestContext->VirtualAddress += transferBufferLength;
		remaining = requestContext->Length - requestContext->TotalTransferred;
	}
	else
	{
		USBERR("urb is NULL!\n");
		if (NT_SUCCESS(status)) status = STATUS_FATAL_USER_CALLBACK_EXCEPTION;
		goto Exit;
	}

	if (!NT_SUCCESS(status)) goto Exit;
	//////////////////////////////////////////////////////////////////////////
	// The transfer was successful. If there is more data, continue to send it,
	// If not, complete the request with 'TotalTransferred' information.

	if (transferBufferLength != stageRequested)
	{
		if (remaining)
		{
			USBDBG("short transfer %u bytes. remaining=%u bytes\n", transferBufferLength, remaining);
			// All of the bytes requested were not submitted (Short packet(s))
			if (USB_ENDPOINT_DIRECTION_IN(pipeID) &&
			        GetPolicyValue(IGNORE_SHORT_PACKETS, requestContext->PipeContext->Policy)) // Continue.
			{
				////////////////////////////////////////////////////////////////////////////////
				// IGNORE_SHORT_PACKETS=TRUE policy. A successful read but more data remains. //
				////////////////////////////////////////////////////////////////////////////////
				goto Continue;
			}
			else // Done... (remaining > 0)
				goto Exit;
		}
		else // Done! (remaining == 0) All bytes transferred; the last transfer was shorter than the rest.
			goto Exit;
	}
	else if (remaining) // Continue! (remaining > 0) Transferred exactly what was requested, but there is more data; continue staging.
	{
		goto Continue;
	}
	else // Done! (remaining == 0) Everything came out even.  All Bytes sent in transfers exact intervals of stageRequested
		goto Exit;

Continue:
	if (remaining)
	{
		USBDBG("pipeID=%02Xh stage (%u bytes) complete. soFar=%u remaining=%u\n",
		       pipeID, transferBufferLength, requestContext->TotalTransferred, remaining);
	}
	else
	{
		USBMSG("pipeID=%02Xh finalizing with ZLP (SHORT_PACKET_TERMINATE)\n", pipeID);
	}
	if (!NT_SUCCESS((status = UrbFormatBulkRequestContext(Request, requestContext, NULL,  NULL))))
		goto Exit;

	status = SubmitAsyncRequest(requestContext, Request, Xfer_OnComplete, NULL);
	if (NT_SUCCESS(status))
		return;


Exit:
	if (NT_SUCCESS(status) &&
	        transferBufferLength &&
	        USB_ENDPOINT_DIRECTION_OUT(pipeID) &&
	        GetPolicyValue(SHORT_PACKET_TERMINATE, requestContext->PipeContext->Policy))
	{
		//////////////////////////////////////////////////////////////////////////////////////
		// SHORT_PACKET_TERMINATE=TRUE policy. A successful write sending 1 or more bytes.  //
		//////////////////////////////////////////////////////////////////////////////////////
		transferBufferLength = 0;		// If the ZLP fails don't keep trying. :)
		requestContext->Length = 0;	// remaining may be > 0; this ensures UrbFormatBulkRequestContext will send the ZLP we want.
		remaining = 0;				// only used in dbg message at this point.
		goto Continue;
	}
	if (requestContext->QueueLocked)
	{
		WdfWaitLockRelease(requestContext->PipeContext->PipeLock);
	}
	if (requestContext->Mdl)
	{
		IoFreeMdl(requestContext->Mdl);
		requestContext->Mdl = NULL;
	}

	USBMSG("[%s-%s] TotalTransferred=%u (bytes)\n",
	       GetPipeTypeString(requestContext->PipeContext->PipeInformation.PipeType),
	       GetEndpointDirString(pipeID),
	       requestContext->TotalTransferred);

	// Always complete with TotalTransferred information regardless of status.
	WdfRequestCompleteWithInformation(Request, status, requestContext->TotalTransferred);

	return;
}

