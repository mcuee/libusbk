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
#include "drv_xfer.h"

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#if (defined(ALLOC_PRAGMA) && defined(PAGING_ENABLED))
#endif

EVT_WDF_REQUEST_COMPLETION_ROUTINE Xfer_ReadBulkRawComplete;
EVT_WDF_REQUEST_COMPLETION_ROUTINE Xfer_ReadBulkComplete;

EVT_WDF_REQUEST_COMPLETION_ROUTINE Xfer_WriteBulkRawComplete;
EVT_WDF_REQUEST_COMPLETION_ROUTINE Xfer_WriteBulkComplete;

#define mXfer_CopyPartialReadToUserMemory(mStatus,mQueueContext, mTransferBuffer, mTransferLength, ErrorAction)	do {	\
		mStatus = WdfMemoryCopyFromBuffer( 																					\
		          mQueueContext->Xfer.UserMem,   																					\
		          mQueueContext->Xfer.Transferred,   																				\
		          mTransferBuffer,   																								\
		          mTransferLength);  																								\
		\
		if (!NT_SUCCESS(mStatus))  																							\
		{  																													\
			USBERR("WdfMemoryCopyFromBuffer failed. Status=%08Xh\n", mStatus); 												\
			ErrorAction;   																									\
		}  																													\
		\
		mQueueContext->OverOfs.BufferOffset	+= mTransferLength; 															\
		mQueueContext->OverOfs.BufferLength	-= mTransferLength; 															\
		mQueueContext->Xfer.Transferred		+= mTransferLength; 															\
	}while(0)


/*
* Sumbmits a bulk/interrupt read stage transfer.
* REQUIRES:
* - ULONG remainingLength [out]
* - ULONG stageLength [out]
* - ULONG remainderLength [out]
* - WDF_REQUEST_SEND_OPTIONS sendOptions [out]
*/
#define mXfer_SubmitNextRead(mStatus, mQueueContext, mRequestContext, mRequest, mCompletionRoutine, mErrorAction) do { 							\
		/* One or more transfers required  */  																										\
		remainingLength	= mQueueContext->Xfer.Length - mQueueContext->Xfer.Transferred; 															\
		stageLength		= remainingLength > mQueueContext->Info.MaximumTransferSize ? mQueueContext->Info.MaximumTransferSize : remainingLength;	\
		remainderLength = stageLength % mQueueContext->Info.MaximumPacketSize; 																		\
		\
		\
		if (stageLength < mQueueContext->Info.MaximumPacketSize)   																					\
		{  																																			\
			if (!mRequestContext->Policies.AllowPartialReads)  																						\
			{  																																		\
				mStatus = STATUS_INVALID_BUFFER_SIZE;  																								\
				USBERRN("Read buffer is not an interval of MaximumPacketSize. MaximumPacketSize: %u RemainderLength: %u",  							\
				        mQueueContext->Info.MaximumPacketSize, remainderLength);   																	\
				mErrorAction;  																														\
			}  																																		\
			/* Use the over-run buffer; CompletionRoutine must update UserMem */   																	\
			mQueueContext->OverOfs.BufferOffset = 0;   																								\
			mQueueContext->OverOfs.BufferLength = stageLength; 																						\
			\
			mStatus = WdfUsbTargetPipeFormatRequestForRead(mQueueContext->PipeHandle, mRequest, mQueueContext->OverMem, NULL); 						\
		}  																																			\
		else   																																		\
		{  																																			\
			/* One or more whole packets can be read directly into the user buffer. */ 																\
			stageLength -= remainderLength;																											\
			mQueueContext->Xfer.UserOfs.BufferOffset = mQueueContext->Xfer.Transferred;																\
			mQueueContext->Xfer.UserOfs.BufferLength = stageLength;																					\
			\
			mStatus = WdfUsbTargetPipeFormatRequestForRead(mQueueContext->PipeHandle, mRequest, mQueueContext->Xfer.UserMem, &mQueueContext->Xfer.UserOfs); 	\
		}  																																			\
		\
		if (!NT_SUCCESS(mStatus))  																													\
		{  																																			\
			USBERR("WdfIoTargetFormatRequestForRead failed. Status=%08Xh\n", mStatus); 																\
			mErrorAction;  																															\
		}  																																			\
		\
		WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, 0);																								\
		mStatus = SetRequestTimeout(mRequestContext, mRequest, &sendOptions);  																		\
		if (!NT_SUCCESS(mStatus))  																													\
		{  																																			\
			USBERR("SetRequestTimeout failed. Status=%08Xh\n", mStatus);   																			\
			mErrorAction;  																															\
		}  																																			\
		\
		USBMSG("PipeID=%02Xh Staging=%u\n",	mQueueContext->Info.EndpointAddress, stageLength);  													\
		mStatus = SubmitAsyncQueueRequest(mQueueContext, mRequest, mCompletionRoutine, &sendOptions, mQueueContext);   								\
		if (!NT_SUCCESS(mStatus))  																													\
		{  																																			\
			USBERR("SubmitAsyncQueueRequest failed. Status=%08Xh\n", mStatus); 																		\
			mErrorAction;  																															\
		}  																																			\
	}while(0)

#define mXfer_SubmitNextWrite(mStatus, mQueueContext, mRequestContext, mRequest, mCompletionRoutine, mErrorAction) do {							\
		/* One or more transfers required  */  																										\
		remainingLength	= mQueueContext->Xfer.Length - mQueueContext->Xfer.Transferred; 															\
		stageLength		= remainingLength > mQueueContext->Info.MaximumTransferSize ? mQueueContext->Info.MaximumTransferSize : remainingLength;	\
		\
		mQueueContext->Xfer.UserOfs.BufferOffset = mQueueContext->Xfer.Transferred;																	\
		mQueueContext->Xfer.UserOfs.BufferLength = stageLength;																						\
		\
		if (stageLength)   																															\
			mStatus = WdfUsbTargetPipeFormatRequestForWrite(mQueueContext->PipeHandle, mRequest, mQueueContext->Xfer.UserMem, &mQueueContext->Xfer.UserOfs); 	\
		else   																																		\
			mStatus = WdfUsbTargetPipeFormatRequestForWrite(mQueueContext->PipeHandle, mRequest, NULL, NULL);  										\
		\
		if (!NT_SUCCESS(mStatus))  																													\
		{  																																			\
			USBERR("WdfIoTargetFormatRequestForWrite failed. Status=%08Xh\n", mStatus);																\
			mErrorAction;  																															\
		}  																																			\
		\
		WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, 0);																								\
		mStatus = SetRequestTimeout(mRequestContext, mRequest, &sendOptions);  																		\
		if (!NT_SUCCESS(mStatus))  																													\
		{  																																			\
			USBERR("SetRequestTimeout failed. Status=%08Xh\n", mStatus);   																			\
			mErrorAction;  																															\
		}  																																			\
		\
		USBMSG("PipeID=%02Xh Staging=%u\n",	mQueueContext->Info.EndpointAddress, stageLength);  													\
		mStatus = SubmitAsyncQueueRequest(mQueueContext, mRequest, mCompletionRoutine, &sendOptions, mQueueContext);   								\
		if (!NT_SUCCESS(mStatus))  																													\
		{  																																			\
			USBERR("SubmitAsyncQueueRequest failed. Status=%08Xh\n", mStatus); 																		\
			mErrorAction;  																															\
		}  																																			\
	}while(0)

VOID Xfer_ReadBulk (
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request)
{
	NTSTATUS                status;
	PREQUEST_CONTEXT        requestContext = NULL;
	PQUEUE_CONTEXT			queueContext = NULL;
	PDEVICE_CONTEXT         deviceContext;
	ULONG					remainingLength;
	ULONG					stageLength = 0;
	ULONG					remainderLength = 0;
	WDF_REQUEST_SEND_OPTIONS sendOptions;
	PUCHAR					transferBuffer;


	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	requestContext = GetRequestContext(Request);

	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status)) goto Exit;

	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context");
		goto Exit;
	}

	queueContext->Xfer.Transferred = 0;
	queueContext->Xfer.Length = requestContext->Length;

	if (!queueContext->OverMem)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context:OverMem");
		goto Exit;
	}

	if (!queueContext->Info.MaximumPacketSize)
	{
		status = STATUS_INVALID_BUFFER_SIZE;
		USBERRN("PipeID=%02Xh MaximumPacketSize=0", queueContext->Info.EndpointAddress);
		goto Exit;
	}

	// handle pipe reset scenarios: ResetPipeOnResume, AutoClearStall
	mXfer_HandlePipeResetScenarios(status, queueContext, requestContext);

	// Handle special case scenario where read length = 0
	if (!queueContext->Xfer.Length)
	{
		// if AllowPartialReads complete successfully for 0 bytes
		status = requestContext->Policies.AllowPartialReads ? STATUS_SUCCESS : STATUS_INVALID_BUFFER_SIZE;
		goto Exit;
	}

	// Init queue user memory
	status = GetTransferMemory(Request, requestContext->ActualRequestType, &queueContext->Xfer.UserMem);
	if (!NT_SUCCESS(status))
	{
		USBERR("GetTransferMemory failed. Status=%08Xh\n", status);
		goto Exit;
	}
	queueContext->Xfer.UserOfs.BufferOffset = 0;
	queueContext->Xfer.UserOfs.BufferLength = queueContext->Xfer.Length;

	// Check if there are bytes left over from a previous transfer.
	if (queueContext->OverOfs.BufferLength > 0)
	{
		// Copy partial read bytes bytes into UserMem
		remainingLength = queueContext->Xfer.Length - queueContext->Xfer.Transferred;
		stageLength = (ULONG)queueContext->OverOfs.BufferLength > remainingLength ? remainingLength : (ULONG)queueContext->OverOfs.BufferLength;
		transferBuffer = &queueContext->OverBuf[queueContext->OverOfs.BufferOffset];

		mXfer_CopyPartialReadToUserMemory(status, queueContext, transferBuffer, stageLength, goto Exit);

		USBDBGN("PipeID=%02Xh Transferred %u bytes from a previous partial read.",
		        queueContext->Info.EndpointAddress, queueContext->Xfer.Transferred);

		if (queueContext->Xfer.Transferred >= queueContext->Xfer.Length)
		{
			if (requestContext->Policies.AutoFlush)
			{
				// discard any extra partial read bytes
				queueContext->OverOfs.BufferLength = 0;
				queueContext->OverOfs.BufferOffset = 0;
			}

			USBMSGN("PipeID=%02Xh DoneReason: Transferred==Requested. Transferred=%u",
			        queueContext->Info.EndpointAddress,	queueContext->Xfer.Transferred);
			status = STATUS_SUCCESS;
			goto Exit;
		}

		/*
		It would seem if IgnoreShortPackets=FALSE we would complete the request here.
		However, WinUSB does not handle it this way and will still submit the request.
		*/
#if 0
		else if (!requestContext->Policies.IgnoreShortPackets)
		{
			USBMSGN("PipeID=%02Xh DoneReason: IgnoreShortPackets=FALSE. Transferred=%u",
			        queueContext->Info.EndpointAddress,	queueContext->Xfer.Transferred);
			status = STATUS_SUCCESS;
			goto Exit;
		}
#endif
	}

	mXfer_SubmitNextRead(status, queueContext, requestContext, Request, Xfer_ReadBulkComplete, goto Exit);
	return;

Exit:
	if (queueContext)
	{
		WdfRequestCompleteWithInformation(Request, status, queueContext->Xfer.Transferred);
		return;
	}
	WdfRequestCompleteWithInformation(Request, status, 0);
}

VOID Xfer_ReadBulkComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in WDFCONTEXT Context)
{
	NTSTATUS                status;
	PREQUEST_CONTEXT        requestContext = NULL;
	PQUEUE_CONTEXT			queueContext = NULL;
	ULONG					remainingLength;
	ULONG					transferredLength;
	ULONG					stageLength = 0;
	ULONG					remainderLength = 0;
	WDF_REQUEST_SEND_OPTIONS sendOptions;
	PUCHAR					transferBuffer;
	PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;

	UNREFERENCED_PARAMETER(Target);

	// Make sure the request context is still valid.
	requestContext	= GetRequestContext(Request);
	queueContext	= (PQUEUE_CONTEXT)Context;

	status				= CompletionParams->IoStatus.Status;
	usbCompletionParams = CompletionParams->Parameters.Usb.Completion;
	transferredLength	= (ULONG)usbCompletionParams->Parameters.PipeRead.Length;

	if (!queueContext || !usbCompletionParams)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context.");
		goto Exit;
	}

	Xfer_CheckPipeStatus(status, queueContext->Info.EndpointAddress);

	mXfer_HandlePipeResetScenariosForComplete(status, queueContext, requestContext);

	if (queueContext->OverOfs.BufferLength)
	{
		// Received data to queue OverMem
		if (transferredLength)
		{
			// Copy partial read bytes bytes into UserMem
			stageLength		= (ULONG)queueContext->OverOfs.BufferLength > transferredLength ? transferredLength : (ULONG)queueContext->OverOfs.BufferLength;
			transferBuffer	= queueContext->OverBuf;
			queueContext->OverOfs.BufferLength = transferredLength;

			mXfer_CopyPartialReadToUserMemory(status, queueContext, transferBuffer, stageLength, goto Exit);

			if (queueContext->Xfer.Transferred >= queueContext->Xfer.Length)
			{
				if (requestContext->Policies.AutoFlush)
				{
					// discard any extra partial read bytes
					queueContext->OverOfs.BufferLength = 0;
					queueContext->OverOfs.BufferOffset = 0;
				}

				// DONE REASON: Transferred==Requested
				USBDBGN("PipeID=%02Xh DoneReason: Transferred==Requested. Staged=%u Total=%u Requested=%u",
				        queueContext->Info.EndpointAddress,	stageLength, queueContext->Xfer.Transferred, queueContext->Xfer.Length);
				goto Exit;
			}
			if (!NT_SUCCESS(status)) goto Exit;

		}
		else
		{
			if (!NT_SUCCESS(status)) goto Exit;

			// Received a ZLP
			USBDBGN("PipeID=%02Xh ZLP received.", queueContext->Info.EndpointAddress);
			queueContext->OverOfs.BufferLength = 0;
		}
		if (!NT_SUCCESS(status)) goto Exit;

		if (!requestContext->Policies.IgnoreShortPackets)
		{
			// DONE REASON: IgnoreShortPackets = 0
			USBDBGN("PipeID=%02Xh DoneReason: IgnoreShortPackets=FALSE. Staged=%u Total=%u Requested=%u",
			        queueContext->Info.EndpointAddress,	stageLength, queueContext->Xfer.Transferred, queueContext->Xfer.Length);

			goto Exit;
		}
	}
	else
	{
		// Received data to UserMem
		queueContext->Xfer.Transferred += transferredLength;
		if (!NT_SUCCESS(status)) goto Exit;

		if (!transferredLength)
		{
			// Received a ZLP
			USBDBGN("PipeID=%02Xh ZLP received.", queueContext->Info.EndpointAddress);
		}

		if (queueContext->Xfer.Transferred >= queueContext->Xfer.Length)
		{
			// DONE REASON: Transferred==Requested
			USBDBGN("PipeID=%02Xh DoneReason: Transferred==Requested. Staged=%u Total=%u Requested=%u",
			        queueContext->Info.EndpointAddress,	transferredLength, queueContext->Xfer.Transferred, queueContext->Xfer.Length);
			goto Exit;
		}
		else if (transferredLength && transferredLength == queueContext->Xfer.UserOfs.BufferLength)
		{
			// bytes transferred equals bytes requested and we have more bytes to go. (split transfers)
			goto NextRead;
		}

		if (!requestContext->Policies.IgnoreShortPackets)
		{
			// DONE REASON: IgnoreShortPackets = 0
			USBDBGN("PipeID=%02Xh DoneReason: IgnoreShortPackets=FALSE. Staged=%u Total=%u Requested=%u",
			        queueContext->Info.EndpointAddress,	transferredLength, queueContext->Xfer.Transferred, queueContext->Xfer.Length);
			goto Exit;
		}
	}

NextRead:
	mXfer_SubmitNextRead(status, queueContext, requestContext, Request, Xfer_ReadBulkComplete, goto Exit);
	return;

Exit:
	WdfRequestCompleteWithInformation(Request, status, queueContext->Xfer.Transferred);

}

VOID Xfer_ReadBulkRaw (
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request)
{
	NTSTATUS                status;
	PREQUEST_CONTEXT        requestContext = NULL;
	PDEVICE_CONTEXT         deviceContext;
	WDFMEMORY				transferMemory;
	PQUEUE_CONTEXT			queueContext = NULL;

	WDF_REQUEST_SEND_OPTIONS sendOptions;

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	requestContext = GetRequestContext(Request);

	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status)) goto Exit;

	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context");
		goto Exit;
	}

	if (!queueContext->Info.MaximumPacketSize)
	{
		status = STATUS_INVALID_BUFFER_SIZE;
		USBERRN("PipeID=%02Xh MaximumPacketSize=0", queueContext->Info.EndpointAddress);
		goto Exit;
	}

	if ((requestContext->Length % queueContext->Info.MaximumPacketSize) > 0)
	{
		USBERR("PipeID=%02Xh transfer must be an interval of the MaximumPacketSize (%u bytes)\n",
		       queueContext->Info.EndpointAddress, queueContext->Info.MaximumPacketSize);
		status = STATUS_INVALID_BUFFER_SIZE;
		goto Exit;
	}

	status = GetTransferMemory(Request, requestContext->ActualRequestType, &transferMemory);
	if (!NT_SUCCESS(status))
	{
		USBERR("GetTransferMemory failed. Status=%08Xh\n", status);
		goto Exit;
	}

	status = WdfUsbTargetPipeFormatRequestForRead(
	             queueContext->PipeHandle,
	             Request,
	             transferMemory,
	             NULL);

	if (!NT_SUCCESS(status))
	{
		USBERR("WdfIoTargetFormatRequestForRead failed. Status=%08Xh\n", status);
		goto Exit;
	}

	WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, 0);
	status = SetRequestTimeout(requestContext, Request, &sendOptions);
	if (!NT_SUCCESS(status))
	{
		USBERR("SetRequestTimeout failed. Status=%08Xh\n", status);
		goto Exit;
	}

	USBMSG("PipeID=%02Xh Length=%u\n",	queueContext->Info.EndpointAddress,	requestContext->Length);
	status = SubmitAsyncQueueRequest(queueContext, Request, Xfer_ReadBulkRawComplete, &sendOptions, queueContext);
	if (!NT_SUCCESS(status))
	{
		USBERR("SubmitAsyncRequest failed. Status=%08Xh\n", status);
		goto Exit;
	}

	return;

Exit:
	WdfRequestCompleteWithInformation(Request, status, 0);
}

VOID Xfer_ReadBulkRawComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in WDFCONTEXT Context)
{
	NTSTATUS status;
	PREQUEST_CONTEXT requestContext;
	size_t length = 0;
	PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;
	PQUEUE_CONTEXT queueContext;

	UNREFERENCED_PARAMETER(Target);

	// Make sure the request context is still valid.
	requestContext = GetRequestContext(Request);
	queueContext = (PQUEUE_CONTEXT)Context;

	status = CompletionParams->IoStatus.Status;
	usbCompletionParams = CompletionParams->Parameters.Usb.Completion;

	length = usbCompletionParams->Parameters.PipeRead.Length;
	Xfer_CheckPipeStatus(status, queueContext->Info.EndpointAddress);

	mXfer_HandlePipeResetScenariosForComplete(status, queueContext, requestContext);

	if (NT_SUCCESS(status) || length)
	{
		USBMSGN("PipeID=%02Xh Done. Total=%u Requested=%u", queueContext->Info.EndpointAddress,	length, requestContext->Length);
	}
	WdfRequestCompleteWithInformation(Request, status, length);

}

VOID Xfer_WriteBulk (
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request)
{
	NTSTATUS                status;
	PREQUEST_CONTEXT        requestContext = NULL;
	PQUEUE_CONTEXT			queueContext = NULL;
	PDEVICE_CONTEXT         deviceContext;
	ULONG					remainingLength;
	ULONG					stageLength = 0;
	WDF_REQUEST_SEND_OPTIONS sendOptions;

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	requestContext = GetRequestContext(Request);

	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status)) goto Exit;

	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context");
		goto Exit;
	}

	queueContext->Xfer.Transferred = 0;
	queueContext->Xfer.Length = requestContext->Length;

	if (!queueContext->Info.MaximumPacketSize && queueContext->Xfer.Length)
	{
		status = STATUS_INVALID_BUFFER_SIZE;
		USBERRN("PipeID=%02Xh MaximumPacketSize=0", queueContext->Info.EndpointAddress);
		goto Exit;
	}

	mXfer_HandlePipeResetScenarios(status, queueContext, requestContext);

	// Handle zlp scenario
	if (queueContext->Xfer.Length)
	{
		// Init queue user memory
		status = GetTransferMemory(Request, requestContext->ActualRequestType, &queueContext->Xfer.UserMem);
		if (!NT_SUCCESS(status))
		{
			USBERR("GetTransferMemory failed. Status=%08Xh\n", status);
			goto Exit;
		}
		queueContext->Xfer.UserOfs.BufferOffset = 0;
		queueContext->Xfer.UserOfs.BufferLength = queueContext->Xfer.Length;
	}
	else
	{
		queueContext->Xfer.UserMem = NULL;
		queueContext->Xfer.UserOfs.BufferOffset = 0;
		queueContext->Xfer.UserOfs.BufferLength = 0;
	}

	queueContext->Xfer.Zlps.Required = (UCHAR)requestContext->Policies.ShortPacketTerminate;
	mXfer_SubmitNextWrite(status, queueContext, requestContext, Request, Xfer_WriteBulkComplete, goto Exit);
	return;

Exit:
	if (queueContext)
	{
		WdfRequestCompleteWithInformation(Request, status, queueContext->Xfer.Transferred);
		return;
	}
	WdfRequestCompleteWithInformation(Request, status, 0);
}

VOID Xfer_WriteBulkComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in WDFCONTEXT Context)
{
	NTSTATUS                status;
	PREQUEST_CONTEXT        requestContext = NULL;
	PQUEUE_CONTEXT			queueContext = NULL;
	ULONG					remainingLength;
	ULONG					transferredLength;
	ULONG					stageLength = 0;
	WDF_REQUEST_SEND_OPTIONS sendOptions;
	PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;

	UNREFERENCED_PARAMETER(Target);

	// Make sure the request context is still valid.
	requestContext	= GetRequestContext(Request);
	queueContext	= (PQUEUE_CONTEXT)Context;

	status				= CompletionParams->IoStatus.Status;
	usbCompletionParams = CompletionParams->Parameters.Usb.Completion;
	transferredLength	= (ULONG)usbCompletionParams->Parameters.PipeWrite.Length;

	if (!queueContext || !usbCompletionParams)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context.");
		goto Exit;
	}

	Xfer_CheckPipeStatus(status, queueContext->Info.EndpointAddress);

	queueContext->Xfer.Transferred += transferredLength;
	if (NT_SUCCESS(status))
	{
		if (queueContext->Xfer.Transferred >= queueContext->Xfer.Length)
		{
			if (queueContext->Xfer.Zlps.Sent >= queueContext->Xfer.Zlps.Required)
			{
				// DONE REASON: Transferred==Requested
				USBDBGN("PipeID=%02Xh DoneReason: Transferred==Requested. Staged=%u Total=%u Requested=%u",
				        queueContext->Info.EndpointAddress,	transferredLength, queueContext->Xfer.Transferred, queueContext->Xfer.Length);
				goto Exit;
			}
			else
			{
				queueContext->Xfer.Zlps.Sent++;
				USBDBGN("PipeID=%02Xh Terminating with ZLP %u of %u..",
				        queueContext->Info.EndpointAddress, queueContext->Xfer.Zlps.Sent, queueContext->Xfer.Zlps.Required);

				goto NextWrite;

			}
		}
	}
	else
	{
		// Cannot continue;
		goto Exit;
	}

	/* More bytes to transfer and no errors have occured. */

NextWrite:
	mXfer_SubmitNextWrite(status, queueContext, requestContext, Request, Xfer_WriteBulkComplete, goto Exit);

	return;

Exit:
	WdfRequestCompleteWithInformation(Request, status, queueContext->Xfer.Transferred);

}

VOID Xfer_WriteBulkRaw (
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request)
{
	NTSTATUS                status;
	PREQUEST_CONTEXT        requestContext = NULL;
	PDEVICE_CONTEXT         deviceContext;
	WDFMEMORY				transferMemory;
	PQUEUE_CONTEXT			queueContext = NULL;

	WDF_REQUEST_SEND_OPTIONS sendOptions;

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	requestContext = GetRequestContext(Request);

	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status)) goto Exit;

	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context");
		goto Exit;
	}

	if (!queueContext->Info.MaximumPacketSize && requestContext->Length)
	{
		status = STATUS_INVALID_BUFFER_SIZE;
		USBERRN("PipeID=%02Xh MaximumPacketSize=0", queueContext->Info.EndpointAddress);
		goto Exit;
	}

	if (requestContext->Length)
	{
		status = GetTransferMemory(Request, requestContext->ActualRequestType, &transferMemory);
		if (!NT_SUCCESS(status))
		{
			USBERR("GetTransferMemory failed. Status=%08Xh\n", status);
			goto Exit;
		}
	}
	else
	{
		// Zlp write
		transferMemory = NULL;
	}
	status = WdfUsbTargetPipeFormatRequestForWrite(
	             queueContext->PipeHandle,
	             Request,
	             transferMemory,
	             NULL);

	if (!NT_SUCCESS(status))
	{
		USBERR("WdfIoTargetFormatRequestForWrite failed. Status=%08Xh\n", status);
		goto Exit;
	}

	WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, 0);
	status = SetRequestTimeout(requestContext, Request, &sendOptions);
	if (!NT_SUCCESS(status))
	{
		USBERR("SetRequestTimeout failed. Status=%08Xh\n", status);
		goto Exit;
	}

	USBMSG("PipeID=%02Xh Length=%u\n",	queueContext->Info.EndpointAddress,	requestContext->Length);
	status = SubmitAsyncQueueRequest(queueContext, Request, Xfer_WriteBulkRawComplete, &sendOptions, queueContext);
	if (!NT_SUCCESS(status))
	{
		USBERR("SubmitAsyncRequest failed. Status=%08Xh\n", status);
		goto Exit;
	}

	return;

Exit:
	WdfRequestCompleteWithInformation(Request, status, 0);
}

VOID Xfer_WriteBulkRawComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in WDFCONTEXT Context)
{
	NTSTATUS status;
	PREQUEST_CONTEXT requestContext;
	size_t length = 0;
	PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;
	PQUEUE_CONTEXT queueContext;

	UNREFERENCED_PARAMETER(Target);

	// Make sure the request context is still valid.
	requestContext = GetRequestContext(Request);
	queueContext = (PQUEUE_CONTEXT)Context;

	status = CompletionParams->IoStatus.Status;
	usbCompletionParams = CompletionParams->Parameters.Usb.Completion;

	length = usbCompletionParams->Parameters.PipeWrite.Length;
	Xfer_CheckPipeStatus(status, queueContext->Info.EndpointAddress);

	if (NT_SUCCESS(status) || length)
	{
		USBMSGN("PipeID=%02Xh Done. Total=%u Requested=%u", queueContext->Info.EndpointAddress,	length, requestContext->Length);
	}
	WdfRequestCompleteWithInformation(Request, status, length);
}
