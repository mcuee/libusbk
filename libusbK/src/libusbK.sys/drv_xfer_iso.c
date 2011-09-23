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

#define mXfer_CreateAutoIso(mStatus, mRequest, mRequestContext, mAutoIsoCtx, mErrorAction) do {					\
	/* Create a collection to store all the sub requests */  													\
	WDFCOLLECTION hCollection;   																				\
	WDF_OBJECT_ATTRIBUTES objAttributes; 																		\
 																												\
	WDF_OBJECT_ATTRIBUTES_INIT(&objAttributes);  																\
	objAttributes.ParentObject = mRequest;   																	\
 																												\
	WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&objAttributes, XFER_AUTOISO_COLLECTION_CONTEXT); 					\
	mStatus = WdfCollectionCreate(&objAttributes, &hCollection); 												\
	if (!NT_SUCCESS(mStatus))																					\
	{																											\
		USBERR("WdfCollectionCreate failed. Status=%08Xh\n", mStatus);   										\
		mErrorAction;;   																						\
	}																											\
	mAutoIsoCtx = GetAutoIsoCollectionContext(hCollection);  													\
	mAutoIsoCtx->MainRequest = mRequest; 																		\
	mAutoIsoCtx->MainRequestContext = mRequestContext;   														\
	mAutoIsoCtx->SubRequestCollection = hCollection; 															\
 	mAutoIsoCtx->MainRequestContext->AutoIso.Context = mAutoIsoCtx; 											\
	mStatus = GetTransferMdl(mRequest,mRequestContext->ActualRequestType, &mAutoIsoCtx->OriginalTransferMDL);	\
	if (!NT_SUCCESS(mStatus))																					\
	{																											\
		USBERR("GetTransferMdl Failed. Status=%08Xh\n", mStatus);   											\
		mErrorAction;;   																						\
	}																											\
	if (USB_ENDPOINT_DIRECTION_IN(mRequestContext->QueueContext->Info.EndpointAddress))							\
	{																											\
		mStatus = WdfRequestRetrieveOutputBuffer(mRequest,0, &mAutoIsoCtx->TransferBuffer,NULL);				\
		if (!NT_SUCCESS(mStatus))																				\
		{																										\
			USBERR("WdfRequestRetrieveOutputBuffer Failed. Status=%08Xh\n", mStatus);   						\
			mErrorAction;;   																					\
		} 																										\
	}																											\
	WDF_OBJECT_ATTRIBUTES_INIT(&objAttributes);  																\
	objAttributes.ParentObject = hCollection;																	\
	mStatus = WdfSpinLockCreate(&objAttributes, &mAutoIsoCtx->SubRequestCollectionLock); 						\
	if (!NT_SUCCESS(mStatus))																					\
	{																											\
		USBERR("WdfSpinLockCreate failed. Status=%08Xh\n", mStatus); 											\
		mErrorAction;;   																						\
	}																											\
}while(0)

#define mXfer_IsoReadPacketsFromUrb(mIsoPacketArray,mUrb,mNumberOfPackets,mCumulativeLength) do {					\
	LONG mPos;																										\
	for (mPos = 0; mPos < (mNumberOfPackets); mPos++)																\
	{																												\
		mIsoPacketArray[mPos].Length = (USHORT)(mUrb)->UrbIsochronousTransfer.IsoPacket[mPos].Length;				\
		mIsoPacketArray[mPos].Status = (USHORT)((mUrb)->UrbIsochronousTransfer.IsoPacket[mPos].Status & 0xFFFF);	\
		(mCumulativeLength) = (mCumulativeLength) + (mUrb)->UrbIsochronousTransfer.IsoPacket[mPos].Length;			\
	}																												\
}while(0)

#define mXfer_IsoPacketsToUrb(mIsoPacketArray,mUrb,mNumberOfPackets,mDataLength,mErrorAction) do {  	\
	LONG mPos; 																							\
	for (mPos = 0; mPos < (LONG)mNumberOfPackets; mPos++)  												\
	{  																									\
		if (mIsoPacketArray[mPos].Offset >= mDataLength)   												\
		{  																								\
			status = STATUS_INVALID_BUFFER_SIZE;   														\
			USBERRN("Last packet offset references data that is out-of-range. IsoPacket[%u].Offset=%u",	\
					mPos, mIsoPacketArray[mPos].Offset);   												\
			mErrorAction; 																				\
		}  																								\
		(mUrb)->UrbIsochronousTransfer.IsoPacket[mPos].Offset = mIsoPacketArray[mPos].Offset;  			\
	}  																									\
}while(0)

#define mXfer_AutoIsoNextStageNumberOfPackets(mNumStagePackets, mTotalmAccumulativeNumPackets, mMaxStageNumPackets) do {	\
		if(mTotalmAccumulativeNumPackets <= mMaxStageNumPackets)															\
		{   																												\
			mNumStagePackets = mTotalmAccumulativeNumPackets;																\
			mTotalmAccumulativeNumPackets = 0;  																			\
		}   																												\
		else																												\
		{   																												\
			mNumStagePackets = mMaxStageNumPackets;  																		\
			mTotalmAccumulativeNumPackets -= mMaxStageNumPackets;   														\
		}   																												\
		ASSERT(mNumStagePackets <= mMaxStageNumPackets); 																	\
}while(0)

#define mXfer_AutoIsoCreateSubRequest(mStatus,mMainRequest,mSubRequest, mSubRequestContext,mAttributes, mWdfUsbTargetDevice,ErrorAction) do {	\
		WDF_OBJECT_ATTRIBUTES_INIT(&mAttributes);																								\
		WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&mAttributes, SUB_REQUEST_CONTEXT);   															\
		mStatus = WdfRequestCreate(  																											\
					 &mAttributes,   																											\
					 WdfUsbTargetDeviceGetIoTarget(mWdfUsbTargetDevice), 																		\
					 &mSubRequest);  																											\
 																																				\
		if(!NT_SUCCESS(mStatus)) 																												\
		{																																		\
			USBERR("WdfRequestCreate failed. Status=%08Xh\n", mStatus);  																		\
			ErrorAction; 																														\
		}																																		\
 																																				\
		mSubRequestContext = GetSubRequestContext(mSubRequest);  																				\
		mSubRequestContext->UserRequest = mMainRequest;  																						\
}while(0)

#define	mXfer_AutoIsoCreateSubUrb(mStatus, mSubRequest, mSubRequestContext,mUrbSize,mSubUrbMemory, mSubUrbBuffer, mAttributes, ErrorAction) do { 	\
		WDF_OBJECT_ATTRIBUTES_INIT(&mAttributes);  											\
		mAttributes.ParentObject = mSubRequest;												\
		mStatus = WdfMemoryCreate( 															\
					 &mAttributes, 															\
					 NonPagedPool, 															\
					 POOL_TAG, 																\
					 mUrbSize, 																\
					 &mSubUrbMemory,														\
					 (PVOID*) &mSubUrbBuffer); 												\
   																							\
		if (!NT_SUCCESS(mStatus))  															\
		{  																					\
			WdfObjectDelete(mSubRequest);  													\
			mSubRequest=NULL;  																\
			USBERRN("WdfMemoryCreate failed in AutoIsoCreateSubUrb. Status=%08Xh",mStatus);	\
			ErrorAction;   																	\
		}  																					\
		mSubRequestContext->SubUrb = mSubUrbBuffer;											\
}while(0)

#define mXfer_AutoIsoCreateSubMdl(mStatus, mOriginalMdl, mSubMdl, mSubRequest, mSubRequestContext, mCumulativeVirtualAddress, mAccumulativeTotalLength, mDataBufferStageSize) do { 	\
	mSubMdl = IoAllocateMdl((PVOID) mCumulativeVirtualAddress, mDataBufferStageSize, FALSE, FALSE, NULL);	\
	if(mSubMdl == NULL)  																					\
	{																										\
		mStatus = STATUS_INSUFFICIENT_RESOURCES; 															\
		USBERRN("IoAllocateMdl failed in AutoIsoCreateSubMdl. Status=%08Xh",mStatus);						\
		WdfObjectDelete(mSubRequest);																		\
		goto Exit;   																						\
	}																										\
 																											\
	IoBuildPartialMdl(mOriginalMdl,  																		\
					  mSubMdl,   																			\
					  (PVOID) mCumulativeVirtualAddress, 													\
					  mDataBufferStageSize); 																\
 																											\
	mSubRequestContext->SubMdl = mSubMdl;																	\
 																											\
	mCumulativeVirtualAddress += mDataBufferStageSize;   													\
	mAccumulativeTotalLength -= mDataBufferStageSize;														\
}while(0)

#define mXfer_AutoIsoFormatSubUrb(mWdmPipeHandle, mRequestContext, mSubUrbBuffer, mUrbSize, mNumberOfPackets,mBufferStageSize, mSubMdl) do {	\
	mSubUrbBuffer->UrbIsochronousTransfer.Hdr.Length = (USHORT) mUrbSize;   																	\
	mSubUrbBuffer->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;   														\
	mSubUrbBuffer->UrbIsochronousTransfer.PipeHandle = mWdmPipeHandle;  																		\
																																				\
	mSubUrbBuffer->UrbIsochronousTransfer.TransferFlags = (mRequestContext->RequestType==WdfRequestTypeRead) ?  								\
		USBD_TRANSFER_DIRECTION_IN : USBD_TRANSFER_DIRECTION_OUT;   																			\
																																				\
	mSubUrbBuffer->UrbIsochronousTransfer.TransferBufferLength = mBufferStageSize;  															\
	mSubUrbBuffer->UrbIsochronousTransfer.TransferBufferMDL = mSubMdl;  																		\
																																				\
	mSubUrbBuffer->UrbIsochronousTransfer.TransferFlags |= USBD_START_ISO_TRANSFER_ASAP;														\
																																				\
	mSubUrbBuffer->UrbIsochronousTransfer.NumberOfPackets = mNumberOfPackets;   																\
	mSubUrbBuffer->UrbIsochronousTransfer.UrbLink = NULL;   																					\
}while(0)

#define mXfer_AutoIsoFormatAndAddSubRequest(mStatus, mSubRequestCollection, mSubRequest, mSubRequestContext, mSubRequestsList, mWdfPipeHandle, mSubUrbMemory, mSubMdl, mCompletionRoutine, mCompletionContext, mErrorAction) do { 	\
		mStatus = WdfUsbTargetPipeFormatRequestForUrb(mWdfPipeHandle, mSubRequest, mSubUrbMemory, NULL);						\
		if (!NT_SUCCESS(mStatus))   																							\
		{   																													\
			USBERRN("WdfUsbTargetPipeFormatRequestForUrb failed in AutoIsoFormatAndAddSubRequest. Status=%08Xh\n", mStatus);	\
			WdfObjectDelete(mSubRequest);   																					\
			IoFreeMdl(mSubMdl); 																								\
			mErrorAction;   																									\
		}   																													\
		WdfRequestSetCompletionRoutine(mSubRequest, mCompletionRoutine, mCompletionContext);									\
																																\
		mStatus = WdfCollectionAdd(mSubRequestCollection, mSubRequest);   																\
		if (!NT_SUCCESS(mStatus))   																							\
		{   																													\
			USBERRN("WdfCollectionAdd failed in AutoIsoFormatAndAddSubRequest. Status=%08Xh", mStatus); 						\
			WdfObjectDelete(mSubRequest);   																					\
			IoFreeMdl(mSubMdl); 																								\
			mErrorAction;   																									\
		}   																													\
		InsertTailList(&mSubRequestsList, &mSubRequestContext->ListEntry);  													\
}while(0)

//
// Required for doing ISOCH transfer. This context is associated with every
// subrequest created by the driver to do ISOCH transfer.
//
typedef struct _SUB_REQUEST_CONTEXT
{

	WDFREQUEST  UserRequest;
	PURB        SubUrb;
	PMDL        SubMdl;
	LIST_ENTRY  ListEntry; // used in CancelRoutine
	ULONG		StartOffset;

} SUB_REQUEST_CONTEXT, *PSUB_REQUEST_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SUB_REQUEST_CONTEXT, GetSubRequestContext)

// iso queue events
EVT_WDF_REQUEST_CANCEL UsbK_EvtRequestCancel;
EVT_WDF_REQUEST_COMPLETION_ROUTINE XferIsoComplete;
EVT_WDF_REQUEST_COMPLETION_ROUTINE ReadWriteCompletion;

VOID XferAutoIsoExComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in PREQUEST_CONTEXT requestContext);

VOID XferIsoExComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in PKISO_CONTEXT IsoContext);

VOID XferIsoCleanup(
    PXFER_AUTOISO_COLLECTION_CONTEXT AutoIsoCtx,
    PLIST_ENTRY         SubRequestsList,
    PBOOLEAN            CompleteRequest);

/* ISOCHRONOUS TRANSFERS (for full speed devices)
 *
Routine Description:
    1. Creates a Sub Request for each irp/urb pair.
       (Each irp/urb pair can transfer a max of 1024 packets.)
    2. All the irp/urb pairs are initialized
    3. The subsidiary irps (of the irp/urb pair) are passed
       down the stack at once.
    4. The main Read/Write is completed in the XferIsoComplete
       even if one SubRequest is sent successfully.

Arguments:
    Queue   - Default queue handle
    Request - Read/Write Request received from the user app.
	InputBufferLength  - Length of the transfer buffer for IoWrite request only.
	OutputBufferLength - Length of transfer buffer for IoRead and DeviceIoControl request.
*/
VOID XferIsoFS (
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request)
{
	ULONG                   packetSize;
	ULONG                   numSubRequests;
	ULONG                   stageSize;
	PUCHAR                  virtualAddress = NULL;
	NTSTATUS               status;
	PDEVICE_CONTEXT         deviceContext;
	PREQUEST_CONTEXT        requestContext;
	WDF_OBJECT_ATTRIBUTES   attributes;
	WDFREQUEST              subRequest = NULL;
	PSUB_REQUEST_CONTEXT    subReqContext = NULL;
	ULONG                   i, j;
	PLIST_ENTRY             thisEntry;
	LIST_ENTRY              subRequestsList;
	USBD_PIPE_HANDLE        usbdPipeHandle;
	BOOLEAN                 cancelable;
	ULONG					TotalLength;
	PXFER_AUTOISO_COLLECTION_CONTEXT autoIsoCtx = NULL;
	PQUEUE_CONTEXT			queueContext = NULL;
	ULONG					absoluteDataOffset = 0;

	cancelable = FALSE;

	InitializeListHead(&subRequestsList);

	requestContext = GetRequestContext(Request);
	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status)) goto Exit;

	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context");
		goto Exit;
	}

	TotalLength = requestContext->Length;
	mXfer_CreateAutoIso(status, Request, requestContext, autoIsoCtx, goto Exit);

	// Use a user defined packet size if one was specfied else use MaximumPacketSize
	packetSize = requestContext->IoControlRequest.endpoint.packet_size;
	if (!packetSize)
		packetSize = queueContext->Info.MaximumPacketSize;

	//
	// there is an inherent limit on the number of packets
	// that can be passed down the stack with each
	// irp/urb pair (255)
	// if the number of required packets is > 255,
	// we shall create "required-packets / 255 + 1" number
	// of irp/urb pairs.
	// Each irp/urb pair transfer is also called a stage transfer.
	//
	if(requestContext->Length > (packetSize * 255))
	{

		stageSize = packetSize * 255;
	}
	else
	{

		stageSize = requestContext->Length;
	}

	USBMSG("stageSize = %d\n", stageSize);

	//
	// determine how many stages of transfer needs to be done.
	// in other words, how many irp/urb pairs required.
	// this irp/urb pair is also called the subsidiary irp/urb pair
	//
	numSubRequests = (requestContext->Length + stageSize - 1) / stageSize;

	USBMSG("numSubRequests = %d\n", numSubRequests);

	virtualAddress = (PUCHAR) MmGetMdlVirtualAddress(autoIsoCtx->OriginalTransferMDL);

	for(i = 0; i < numSubRequests; i++)
	{

		WDFMEMORY               subUrbMemory;
		PURB                    subUrb;
		PMDL                    subMdl;
		ULONG                   nPackets;
		ULONG                   subUrbSize;
		ULONG                   offset;

		//
		// For every stage of transfer we need to do the following
		// tasks
		// 1. allocate a request
		// 2. allocate an urb
		// 3. allocate a mdl.
		// 4. Format the request for transfering URB
		// 5. Send the Request.

		//

		nPackets = (stageSize + packetSize - 1) / packetSize;

		USBMSG("nPackets = %d for Irp/URB pair %d\n", nPackets, i);

		ASSERT(nPackets <= 255);
		subUrbSize = GET_ISO_URB_SIZE(nPackets);

		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
		WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&attributes, SUB_REQUEST_CONTEXT);

		status = WdfRequestCreate(
		             &attributes,
		             WdfUsbTargetDeviceGetIoTarget(deviceContext->WdfUsbTargetDevice),
		             &subRequest);

		if(!NT_SUCCESS(status))
		{

			USBERR("WdfRequestCreate failed. Status=%08Xh\n", status);
			goto Exit;
		}

		subReqContext = GetSubRequestContext(subRequest);
		subReqContext->UserRequest = Request;

		subReqContext->StartOffset = absoluteDataOffset;
		absoluteDataOffset+=stageSize;

		//
		// Allocate memory for URB.
		//
		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
		attributes.ParentObject = subRequest;
		status = WdfMemoryCreate(
		             &attributes,
		             NonPagedPool,
		             POOL_TAG,
		             subUrbSize,
		             &subUrbMemory,
		             (PVOID*) &subUrb);

		if (!NT_SUCCESS(status))
		{
			WdfObjectDelete(subRequest);
			USBERR("failed to alloc MemoryBuffer for suburb\n");
			goto Exit;
		}

		subReqContext->SubUrb = subUrb;

		//
		// Allocate a mdl and build the MDL to describe the staged buffer.
		//
		subMdl = IoAllocateMdl((PVOID) virtualAddress,
		                       stageSize,
		                       FALSE,
		                       FALSE,
		                       NULL);

		if(subMdl == NULL)
		{

			USBERR("failed to alloc mem for sub context mdl\n");
			WdfObjectDelete(subRequest);
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto Exit;
		}

		IoBuildPartialMdl(autoIsoCtx->OriginalTransferMDL,
		                  subMdl,
		                  (PVOID) virtualAddress,
		                  stageSize);

		subReqContext->SubMdl = subMdl;

		virtualAddress += stageSize;
		TotalLength -= stageSize;

		//
		// Initialize the subsidiary urb
		//
		usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(queueContext->PipeHandle);

		subUrb->UrbIsochronousTransfer.Hdr.Length = (USHORT) subUrbSize;
		subUrb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
		subUrb->UrbIsochronousTransfer.PipeHandle = usbdPipeHandle;

		if(requestContext->RequestType == WdfRequestTypeRead)
			subUrb->UrbIsochronousTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN;
		else
			subUrb->UrbIsochronousTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_OUT;

		subUrb->UrbIsochronousTransfer.TransferBufferLength = stageSize;
		subUrb->UrbIsochronousTransfer.TransferBufferMDL = subMdl;

		// TODO: Add auto resetting pipe policies for iso_start_frame_latency.


#if 0
		if (!requestContext->PipeContext->NextFrameNumber)
		{
			status = WdfUsbTargetDeviceRetrieveCurrentFrameNumber(deviceContext->WdfUsbTargetDevice, &requestContext->PipeContext->NextFrameNumber);
			if (!NT_SUCCESS(status))
			{
				// ..use ASAP until the user takes action..
				requestContext->PipeContext->NextFrameNumber = ULONG_MAX;
			}
		}
		else
		{
			// set ISO start frame (with user supplied latency)
			// maintain requestContext->PipeContext->NextFrameNumber with the nPackets;
			// FrameNumbers are 1:1 with iso packets.
			// keep using NextFrameNumber as new transfers come in unless an error occurs or the user takes action.
		}

		//
		// This is a way to set the start frame and NOT specify ASAP flag.
		//
		subUrb->UrbIsochronousTransfer.StartFrame = requestContext->PipeContext->NextFrameNumber  + SOME_LATENCY;
#endif

		//
		// when the client driver sets the ASAP flag, it basically
		// guarantees that it will make data available to the HC
		// and that the HC should transfer it in the next transfer frame
		// for the endpoint.(The HC maintains a next transfer frame
		// state variable for each endpoint). If the data does not get to the HC
		// fast enough, the USBD_ISO_PACKET_DESCRIPTOR - Status is
		// USBD_STATUS_BAD_START_FRAME on uhci. On ohci it is 0xC000000E.
		//

		subUrb->UrbIsochronousTransfer.TransferFlags |= USBD_START_ISO_TRANSFER_ASAP;
		subUrb->UrbIsochronousTransfer.NumberOfPackets = nPackets;
		subUrb->UrbIsochronousTransfer.UrbLink = NULL;

		//
		// set the offsets for every packet for reads/writes
		//
		if(requestContext->RequestType == WdfRequestTypeRead)
		{

			offset = 0;

			for(j = 0; j < nPackets; j++)
			{

				subUrb->UrbIsochronousTransfer.IsoPacket[j].Offset = offset;
				subUrb->UrbIsochronousTransfer.IsoPacket[j].Length = 0;

				if(stageSize > packetSize)
				{

					offset += packetSize;
					stageSize -= packetSize;
				}
				else
				{

					offset += stageSize;
					stageSize = 0;
				}
			}
		}
		else
		{

			offset = 0;

			for(j = 0; j < nPackets; j++)
			{

				subUrb->UrbIsochronousTransfer.IsoPacket[j].Offset = offset;

				if(stageSize > packetSize)
				{

					subUrb->UrbIsochronousTransfer.IsoPacket[j].Length = packetSize;
					offset += packetSize;
					stageSize -= packetSize;
				}
				else
				{

					subUrb->UrbIsochronousTransfer.IsoPacket[j].Length = stageSize;
					offset += stageSize;
					stageSize = 0;
					ASSERT(offset == (subUrb->UrbIsochronousTransfer.IsoPacket[j].Length +
					                  subUrb->UrbIsochronousTransfer.IsoPacket[j].Offset));
				}
			}
		}


		//
		// Associate the URB with the request.
		//
		status = WdfUsbTargetPipeFormatRequestForUrb(queueContext->PipeHandle,
		         subRequest,
		         subUrbMemory,
		         NULL);

		if (!NT_SUCCESS(status))
		{
			USBERR("failed to format requset for urb\n");
			WdfObjectDelete(subRequest);
			IoFreeMdl(subMdl);
			goto Exit;
		}

		WdfRequestSetCompletionRoutine(subRequest, XferIsoComplete, autoIsoCtx);

		if(TotalLength > (packetSize * 255))
		{

			stageSize = packetSize * 255;
		}
		else
		{

			stageSize = TotalLength;
		}

		//
		// WdfCollectionAdd takes a reference on the request object and removes
		// it when you call WdfCollectionRemove.
		//
		status = WdfCollectionAdd(autoIsoCtx->SubRequestCollection, subRequest);
		if (!NT_SUCCESS(status))
		{
			USBERR("WdfCollectionAdd failed. Status=%08Xh\n", status);
			WdfObjectDelete(subRequest);
			IoFreeMdl(subMdl);
			goto Exit;
		}

		InsertTailList(&subRequestsList, &subReqContext->ListEntry);

	}

	//
	// There is a subtle race condition which can happen if the cancel
	// routine is running while the sub-request completion routine is
	// trying to mark the request as uncancellable followed by completing
	// the request.
	//
	// We take a reference to prevent the above race condition.
	// The reference is released in the sub-request completion routine
	// if the request wasn't cancelled or in the cancel routine if it was.
	//
	WdfObjectReference(Request);

	//
	// Mark the main request cancelable so that we can cancel the subrequests
	// if the main requests gets cancelled for any reason.
	//
	WdfRequestMarkCancelable(Request, UsbK_EvtRequestCancel);
	cancelable = TRUE;

	while(!IsListEmpty(&subRequestsList))
	{

		thisEntry = RemoveHeadList(&subRequestsList);
		subReqContext = CONTAINING_RECORD(thisEntry, SUB_REQUEST_CONTEXT, ListEntry);
		subRequest = WdfObjectContextGetObject(subReqContext);
		USBMSG("sending subRequest 0x%p\n", subRequest);
		if (WdfRequestSend(subRequest, WdfUsbTargetPipeGetIoTarget(queueContext->PipeHandle), WDF_NO_SEND_OPTIONS) == FALSE)
		{
			status = WdfRequestGetStatus(subRequest);
			//
			// Insert the subrequest back into the subrequestlist so cleanup can find it and delete it
			//
			InsertHeadList(&subRequestsList, &subReqContext->ListEntry);
			USBMSG("WdfRequestSend failed. Status=%08Xh\n", status);
			ASSERT(!NT_SUCCESS(status));
			goto Exit;
		}

		//
		// Don't touch the subrequest after it has been sent.
		// Make a note that at least one subrequest is sent. This will be used
		// in deciding whether we should free the subrequests in case of failure.
		//


	}
Exit:

	//
	// Checking the status besides the number of list entries will help differentiate
	// failures where everything succeeded vs where there were failures before adding
	// list entries.
	//
	if(NT_SUCCESS(status) && IsListEmpty(&subRequestsList))
	{
		//
		// We will let the completion routine to cleanup and complete the
		// main request.
		//
		return;
	}
	else
	{
		BOOLEAN  completeRequest;
		NTSTATUS tempStatus;

		completeRequest = TRUE;
		tempStatus = STATUS_SUCCESS;

		if(autoIsoCtx->SubRequestCollection)
		{
			XferIsoCleanup(autoIsoCtx, &subRequestsList, &completeRequest);
		}

		if (completeRequest)
		{
			if (cancelable)
			{
				//
				// Mark the main request as not cancelable before completing it.
				//
				tempStatus = WdfRequestUnmarkCancelable(Request);
				if (NT_SUCCESS(tempStatus))
				{
					//
					// If WdfRequestUnmarkCancelable returns STATUS_SUCCESS
					// that means the cancel routine has been removed. In that case
					// we release the reference otherwise the cancel routine does it.
					//
					WdfObjectDereference(Request);
				}
			}

			if (autoIsoCtx->TotalTransferred > 0 )
			{
				WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, autoIsoCtx->TotalTransferred);
			}
			else
			{
				WdfRequestCompleteWithInformation(Request, status, autoIsoCtx->TotalTransferred);
			}
		}

	}

	USBMSG("ends\n");
	return;
}

VOID XferIsoHS(
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request)
{
	ULONG                   numberOfPackets;
	ULONG                   actualPackets;
	ULONG                   minDataInEachPacket;
	ULONG                   dataLeftToBeDistributed;
	ULONG                   numberOfPacketsFilledToBrim;
	ULONG                   packetSize;
	ULONG                   numSubRequests;
	ULONG                   stageSize;
	PUCHAR                  virtualAddress = NULL;
	NTSTATUS                status;
	PDEVICE_CONTEXT         deviceContext;
	PREQUEST_CONTEXT        requestContext;
	WDF_OBJECT_ATTRIBUTES   attributes;
	WDFREQUEST              subRequest;
	PSUB_REQUEST_CONTEXT    subReqContext;
	ULONG                   i, j;
	PLIST_ENTRY             thisEntry;
	LIST_ENTRY              subRequestsList;
	USBD_PIPE_HANDLE        usbdPipeHandle;
	BOOLEAN                 cancelable;
	ULONG					TotalLength;
	PXFER_AUTOISO_COLLECTION_CONTEXT autoIsoCtx = NULL;
	PQUEUE_CONTEXT			queueContext = NULL;
	ULONG					absoluteDataOffset = 0;

	cancelable = FALSE;

	InitializeListHead(&subRequestsList);

	requestContext = GetRequestContext(Request);
	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status)) goto Exit;

	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context");
		goto Exit;
	}

	TotalLength = requestContext->Length;
	if(TotalLength < 8)
	{
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}
	mXfer_CreateAutoIso(status, Request, requestContext, autoIsoCtx, goto Exit);

	// Use a user defined packet size if one was specfied else use MaximumPacketSize
	packetSize = requestContext->IoControlRequest.endpoint.packet_size;
	if (!packetSize)
		packetSize = queueContext->Info.MaximumPacketSize;

	// multiples of 8 packets only.
	numberOfPackets	= (TotalLength + packetSize - 1) / packetSize;
	if(numberOfPackets % 8)
		actualPackets = numberOfPackets + (8 - (numberOfPackets % 8));
	else
		actualPackets = numberOfPackets;

	minDataInEachPacket = TotalLength / actualPackets;
	if(minDataInEachPacket == packetSize)
	{
		numberOfPacketsFilledToBrim = actualPackets;
		dataLeftToBeDistributed     = 0;
	}
	else
	{
		dataLeftToBeDistributed = TotalLength - (minDataInEachPacket * actualPackets);
		numberOfPacketsFilledToBrim = dataLeftToBeDistributed / (packetSize - minDataInEachPacket);
		dataLeftToBeDistributed -= (numberOfPacketsFilledToBrim * (packetSize - minDataInEachPacket));
	}

	// Number of transfer stages in this request, each stage will have it's own URB/IRP pair.
	numSubRequests = (actualPackets + 1023) / 1024;

	virtualAddress = (PUCHAR) MmGetMdlVirtualAddress(autoIsoCtx->OriginalTransferMDL);

	for(i = 0; i < numSubRequests; i++)
	{
		WDFMEMORY               subUrbMemory;
		PURB                    subUrb;
		PMDL                    subMdl;
		ULONG                   nPackets;
		ULONG                   subUrbSize;
		ULONG                   offset;

		mXfer_AutoIsoNextStageNumberOfPackets(nPackets, actualPackets, 1024);

		subUrbSize = GET_ISO_URB_SIZE(nPackets);

		mXfer_AutoIsoCreateSubRequest(status, Request, subRequest, subReqContext, attributes, deviceContext->WdfUsbTargetDevice, goto Exit);

		// Create urb memory and assign it to the sub request context.
		mXfer_AutoIsoCreateSubUrb(status, subRequest, subReqContext, subUrbSize, subUrbMemory, subUrb, attributes, goto Exit);

		if(nPackets > numberOfPacketsFilledToBrim)
		{
			stageSize =  packetSize * numberOfPacketsFilledToBrim;
			stageSize += (minDataInEachPacket * (nPackets - numberOfPacketsFilledToBrim));
			stageSize += dataLeftToBeDistributed;
		}
		else
		{
			stageSize = packetSize * nPackets;
		}

		// Create the subMdl describing this section of the buffer; increase virtualAddress and descrease TotalLength by stageSize
		mXfer_AutoIsoCreateSubMdl(status, autoIsoCtx->OriginalTransferMDL, subMdl, subRequest, subReqContext, virtualAddress, TotalLength, stageSize);

		usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(queueContext->PipeHandle);

		mXfer_AutoIsoFormatSubUrb(usbdPipeHandle, requestContext, subUrb, subUrbSize, nPackets, stageSize, subMdl);

		subReqContext->StartOffset = absoluteDataOffset;
		absoluteDataOffset+=stageSize;

		offset = 0;
		for(j = 0; j < nPackets; j++)
		{
			subUrb->UrbIsochronousTransfer.IsoPacket[j].Offset = offset;
			if(numberOfPacketsFilledToBrim)
			{
				offset += packetSize;
				numberOfPacketsFilledToBrim--;
				stageSize -= packetSize;
			}
			else if(dataLeftToBeDistributed)
			{
				offset += (minDataInEachPacket + dataLeftToBeDistributed);
				stageSize -= (minDataInEachPacket + dataLeftToBeDistributed);
				dataLeftToBeDistributed = 0;

			}
			else
			{
				offset += minDataInEachPacket;
				stageSize -= minDataInEachPacket;
			}
		}
		ASSERT(stageSize == 0);

		mXfer_AutoIsoFormatAndAddSubRequest(status, autoIsoCtx->SubRequestCollection, subRequest, subReqContext, subRequestsList, queueContext->PipeHandle, subUrbMemory, subMdl, XferIsoComplete, autoIsoCtx, goto Exit);
	}

	/*
	* There is a subtle race condition which can happen if the cancel
	* routine is running while the sub-request completion routine is
	* trying to mark the request as uncancellable followed by completing
	* the request.
	*
	* We take a reference to prevent the above race condition. The
	* reference is released in the sub-request completion routine if the
	* request wasn't cancelled or in the cancel routine if it was.
	*
	* NOTE: The reference just keeps the request context from being
	* deleted but if you need to access anything in the WDFREQUEST
	* itself in the cancel or the completion routine make sure that you
	* synchronise the completion of the request with accessing anything
	* in it. You can also use a spinlock and use that to synchronise the
	* cancel routine with the completion routine.
	*/
	WdfObjectReference(Request);

	// Mark the main request cancelable so that we can cancel the subrequests; if the main requests gets cancelled for any reason.
	WdfRequestMarkCancelable(Request, UsbK_EvtRequestCancel);
	cancelable = TRUE;

	while(!IsListEmpty(&subRequestsList))
	{
		thisEntry = RemoveHeadList(&subRequestsList);
		subReqContext = CONTAINING_RECORD(thisEntry, SUB_REQUEST_CONTEXT, ListEntry);
		subRequest = WdfObjectContextGetObject(subReqContext);
		if (WdfRequestSend(subRequest, WdfUsbTargetPipeGetIoTarget(queueContext->PipeHandle), WDF_NO_SEND_OPTIONS) == FALSE)
		{
			status = WdfRequestGetStatus(subRequest);
			//
			// Insert the subrequest back into the subrequestlist so cleanup can find it and delete it
			//
			InsertHeadList(&subRequestsList, &subReqContext->ListEntry);
			USBERR("WdfRequestSend failed. Status=%08Xh\n", status);
			WdfVerifierDbgBreakPoint(); // break into the debugger if the registry value is set.
			goto Exit;
		}
	}

Exit:

	//
	// Checking the status besides the number of list entries will help differentiate
	// failures where everything succeeded vs where there were failures before adding
	// list entries.
	//
	if(NT_SUCCESS(status) && IsListEmpty(&subRequestsList))
	{
		// We will let the completion routine to cleanup and complete the main request.
		return;
	}
	else
	{
		BOOLEAN  completeRequest;
		NTSTATUS tempStatus;

		completeRequest = TRUE;
		tempStatus = STATUS_SUCCESS;

		if(autoIsoCtx->SubRequestCollection)
		{
			XferIsoCleanup(autoIsoCtx, &subRequestsList, &completeRequest);
		}

		if (completeRequest)
		{
			if (cancelable)
			{
				// Mark the main request as not cancelable before completing it.
				tempStatus = WdfRequestUnmarkCancelable(Request);
				if (NT_SUCCESS(tempStatus))
				{
					// If WdfRequestUnmarkCancelable returns STATUS_SUCCESS that means the cancel routine has
					// been removed. In that case we release the reference otherwise the cancel routine does it.
					WdfObjectDereference(Request);
				}
			}

			if (autoIsoCtx->TotalTransferred > 0)
				WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, autoIsoCtx->TotalTransferred);
			else
				WdfRequestCompleteWithInformation(Request, status, autoIsoCtx->TotalTransferred);
		}
	}

	return;
}

VOID XferIsoCleanup(
    PXFER_AUTOISO_COLLECTION_CONTEXT AutoIsoCtx,
    PLIST_ENTRY         SubRequestsList,
    PBOOLEAN            CompleteRequest)
{
	PLIST_ENTRY           thisEntry;
	PSUB_REQUEST_CONTEXT  subReqContext;
	WDFREQUEST            subRequest;
	ULONG                 numPendingRequests;

	*CompleteRequest = TRUE;
	if (!AutoIsoCtx->SubRequestCollectionLock)
	{
		USBERR("SubRequestCollectionLock=NULL");
		return;
	}

	WdfSpinLockAcquire(AutoIsoCtx->SubRequestCollectionLock);

	while(!IsListEmpty(SubRequestsList))
	{
		thisEntry = RemoveHeadList(SubRequestsList);
		subReqContext = CONTAINING_RECORD(thisEntry, SUB_REQUEST_CONTEXT, ListEntry);
		subRequest = WdfObjectContextGetObject(subReqContext);
		WdfCollectionRemove(AutoIsoCtx->SubRequestCollection, subRequest);

		if(subReqContext->SubMdl)
		{
			IoFreeMdl(subReqContext->SubMdl);
			subReqContext->SubMdl = NULL;
		}

		WdfObjectDelete(subRequest);
	}
	numPendingRequests = WdfCollectionGetCount(AutoIsoCtx->SubRequestCollection);
	WdfSpinLockRelease(AutoIsoCtx->SubRequestCollectionLock);

	if (numPendingRequests > 0)
	{
		*CompleteRequest = FALSE;
	}

	return;
}

VOID XferIsoComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in WDFCONTEXT Context)

{
	PURB								urb;
	ULONG								i;
	ULONG								numPendingRequests;
	NTSTATUS							status;
	PXFER_AUTOISO_COLLECTION_CONTEXT	autoIsoCtx;
	PSUB_REQUEST_CONTEXT				subReqContext;

	UNREFERENCED_PARAMETER(Target);

	subReqContext = GetSubRequestContext(Request);

	urb = (PURB) subReqContext->SubUrb;

	IoFreeMdl(subReqContext->SubMdl);
	subReqContext->SubMdl = NULL;

	autoIsoCtx	= (PXFER_AUTOISO_COLLECTION_CONTEXT) Context;
	ASSERT(autoIsoCtx);

	status		= CompletionParams->IoStatus.Status;
	autoIsoCtx->TotalTransferred += urb->UrbIsochronousTransfer.TransferBufferLength;


	if(!NT_SUCCESS(status))
	{
		for(i = 0; i < urb->UrbIsochronousTransfer.NumberOfPackets; i++)
		{
			USBDBGN("IsoPacket[%d].Length = %X IsoPacket[%d].Status = %08Xh",
			        i, urb->UrbIsochronousTransfer.IsoPacket[i].Length,
			        i, urb->UrbIsochronousTransfer.IsoPacket[i].Status);
		}
	}
	else if (USB_ENDPOINT_DIRECTION_IN(autoIsoCtx->MainRequestContext->QueueContext->Info.EndpointAddress) && autoIsoCtx->TransferBuffer)
	{
		// Because we are not returning iso packet lengths, pack the data so it is contiguous.
		for(i = 0; i < urb->UrbIsochronousTransfer.NumberOfPackets; i++)
		{
			 ULONG srcOffset = subReqContext->StartOffset+urb->UrbIsochronousTransfer.IsoPacket[i].Offset;
			 ULONG srcLength = urb->UrbIsochronousTransfer.IsoPacket[i].Length;
			 if (autoIsoCtx->AdjustedReadOffset != srcOffset && srcOffset+srcLength <= autoIsoCtx->MainRequestContext->Length)
			 {
				 RtlMoveMemory(&autoIsoCtx->TransferBuffer[autoIsoCtx->AdjustedReadOffset],&autoIsoCtx->TransferBuffer[srcOffset],srcLength);
			 }
			 autoIsoCtx->AdjustedReadOffset+=srcLength;
		}
	}

	// Remove the SubRequest from the collection.
	WdfSpinLockAcquire(autoIsoCtx->SubRequestCollectionLock);

	WdfCollectionRemove(autoIsoCtx->SubRequestCollection, Request);
	numPendingRequests = WdfCollectionGetCount(autoIsoCtx->SubRequestCollection);

	WdfSpinLockRelease(autoIsoCtx->SubRequestCollectionLock);

	// If all the sub requests are completed. Complete the main request sent by the user application.
	if(numPendingRequests == 0)
	{
		// Mark the main request as not cancelable before completing it.
		status = WdfRequestUnmarkCancelable(autoIsoCtx->MainRequest);
		if (NT_SUCCESS(status))
		{
			// If WdfRequestUnmarkCancelable returns STATUS_SUCCESS that means the cancel routine has been
			// removed. In that case we release the reference otherwise the cancel routine does it.
			WdfObjectDereference(autoIsoCtx->MainRequest);
		}

		USBMSG("Transferred=%u Status=%08Xh", autoIsoCtx->TotalTransferred, CompletionParams->IoStatus.Status);
		WdfRequestCompleteWithInformation(autoIsoCtx->MainRequest, CompletionParams->IoStatus.Status, autoIsoCtx->TotalTransferred);
	}

	// Since we created the subrequests, we should free it by removing the reference.
	WdfObjectDelete(Request);
	return;
}

VOID
UsbK_EvtRequestCancel(
    WDFREQUEST Request
)
/*++

Routine Description:

    This is the cancellation routine for the main read/write Irp.

Arguments:


Return Value:

    None

--*/
{
	PREQUEST_CONTEXT        requestContext;
	LIST_ENTRY              cancelList;
	PSUB_REQUEST_CONTEXT    subReqContext;
	WDFREQUEST              subRequest;
	ULONG                   i;
	PLIST_ENTRY             thisEntry;

	USBMSG("begins\n");

	//
	// In this case since the cancel routine just references the request context, a reference
	// on the WDFREQUEST is enough. If it needed to access the underlying WDFREQUEST  to get the
	// underlying IRP etc. consider using a spinlock for synchronisation with the request
	// completion routine.
	//
	requestContext = GetRequestContext(Request);

	if(!requestContext->AutoIso.Context || !requestContext->AutoIso.Context->SubRequestCollection)
	{
		ASSERTMSG("Very unlikely, collection is created before the request is made cancellable", FALSE);
		return;
	}

	InitializeListHead(&cancelList);

	//
	// We cannot call the WdfRequestCancelSentRequest with the collection lock
	// acquired because that call can recurse into our completion routine,
	// when the lower driver completes the request in the CancelRoutine,
	// and can cause deadlock when we acquire the collection to remove the
	// subrequest. So to workaround that, we will get the item from the
	// collection, take an extra reference, and put them in the local list.
	// Then we drop the lock, walk the list and call WdfRequestCancelSentRequest and
	// remove the extra reference.
	//
	WdfSpinLockAcquire(requestContext->AutoIso.Context->SubRequestCollectionLock);

	for(i = 0; i < WdfCollectionGetCount(requestContext->AutoIso.Context->SubRequestCollection); i++)
	{


		subRequest = WdfCollectionGetItem(requestContext->AutoIso.Context->SubRequestCollection, i);

		subReqContext = GetSubRequestContext(subRequest);

		WdfObjectReference(subRequest);

		InsertTailList(&cancelList, &subReqContext->ListEntry);
	}

	WdfSpinLockRelease(requestContext->AutoIso.Context->SubRequestCollectionLock);

	while(!IsListEmpty(&cancelList))
	{
		thisEntry = RemoveHeadList(&cancelList);

		subReqContext = CONTAINING_RECORD(thisEntry, SUB_REQUEST_CONTEXT, ListEntry);

		subRequest = WdfObjectContextGetObject(subReqContext);

		USBMSG("Cancelling subRequest 0x%p\n", subRequest);

		if(!WdfRequestCancelSentRequest(subRequest))
		{
			USBMSG("WdfRequestCancelSentRequest failed\n");
		}

		WdfObjectDereference(subRequest);
	}

	//
	// Release the reference we took earlier on the main request.
	//
	WdfObjectDereference(Request);
	USBMSG("ends\n");

	return;
}

VOID XferIsoEx(__in WDFQUEUE Queue,
               __in WDFREQUEST Request)
{
	NTSTATUS status = STATUS_NOT_SUPPORTED;
	PDEVICE_CONTEXT deviceContext;
	PREQUEST_CONTEXT requestContext;
	PKISO_CONTEXT isoContext;
	size_t isoContextSize;
	WDF_OBJECT_ATTRIBUTES attributes;
	PURB urb;
	PMDL mdl;
	ULONG urbSize;
	USBD_PIPE_HANDLE usbdPipeHandle;
	PQUEUE_CONTEXT queueContext;

	// Local vars pre-initialization //////////////////////////////////
	requestContext = GetRequestContext(Request);
	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status)) goto Exit;

	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context");
		goto Exit;
	}

	USBDBG("ContextMemory=%p\n", requestContext->IsoEx.ContextMemory);

	if (!requestContext->IsoEx.ContextMemory)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("NULL ISO context.\n");
		goto Exit;
	}

	isoContext = (PKISO_CONTEXT)WdfMemoryGetBuffer(requestContext->IsoEx.ContextMemory, &isoContextSize);
	if (!isoContext)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("WdfMemoryGetBuffer failed. Invalid ISO context.\n");
		goto Exit;
	}

	usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(queueContext->PipeHandle);
	if (!usbdPipeHandle)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("WdfUsbTargetPipeWdmGetPipeHandle failed. usbdPipeHandle=NULL\n");
		goto Exit;
	}

	if (!isoContext->NumberOfPackets || isoContext->NumberOfPackets & 0xFFFF8000)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("Invalid NumberOfPackets. NumberOfPackets=%u.\n", isoContext->NumberOfPackets);
		goto Exit;
	}

	if (IsHighSpeedDevice(deviceContext) && (isoContext->NumberOfPackets % 8))
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("Invalid NumberOfPackets. NumberOfPackets=%u.\n", isoContext->NumberOfPackets);
		goto Exit;
	}

	urbSize = GET_ISO_URB_SIZE(isoContext->NumberOfPackets);
	///////////////////////////////////////////////////////////////////

	status = GetTransferMdl(Request, requestContext->ActualRequestType, &mdl);
	if (!NT_SUCCESS(status))
	{
		USBERR("GetTransferMdl failed.Status=%08Xh\n", urbSize, status);
		goto Exit;
	}
	// Allocate URB memory 	///////////////////////////////////////////
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = Request;
	status = WdfMemoryCreate(&attributes, NonPagedPool, POOL_TAG, urbSize, &requestContext->IsoEx.UrbMemory, &urb);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfMemoryCreate failed. size=%u Status=%08Xh\n", urbSize, status);
		goto Exit;
	}
	///////////////////////////////////////////////////////////////////

	// handle pipe reset scenarios: ResetPipeOnResume, AutoClearStall
	mXfer_HandlePipeResetScenarios(status, queueContext, requestContext);

	// Init URB ///////////////////////////////////////////////////////
	RtlZeroMemory(urb, urbSize);
	urb->UrbIsochronousTransfer.Hdr.Length = (USHORT) urbSize;
	urb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
	urb->UrbIsochronousTransfer.PipeHandle = usbdPipeHandle;

	urb->UrbIsochronousTransfer.TransferBufferLength = requestContext->Length;
	urb->UrbIsochronousTransfer.TransferBufferMDL = mdl;

	if(requestContext->RequestType == WdfRequestTypeRead)
		urb->UrbIsochronousTransfer.TransferFlags |= USBD_TRANSFER_DIRECTION_IN;

	if (isoContext->Flags & KISO_FLAG_NO_START_ASAP)
	{
		urb->UrbIsochronousTransfer.StartFrame = isoContext->StartFrame;
	}
	else
	{
		urb->UrbIsochronousTransfer.TransferFlags |= USBD_START_ISO_TRANSFER_ASAP;
	}

	urb->UrbIsochronousTransfer.NumberOfPackets = isoContext->NumberOfPackets;
	///////////////////////////////////////////////////////////////////

	// Init URB iso packets ///////////////////////////////////////////
	mXfer_IsoPacketsToUrb(isoContext->IsoPackets, urb, isoContext->NumberOfPackets, requestContext->Length, goto Exit);
	///////////////////////////////////////////////////////////////////

	// Assign the new urb to the request and prepare it for WdfRequestSend.
	status = WdfUsbTargetPipeFormatRequestForUrb(queueContext->PipeHandle, Request, requestContext->IsoEx.UrbMemory, NULL);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfUsbTargetPipeFormatRequestForUrb failed. Status=%08Xh\n", status);
		goto Exit;
	}

	status = SubmitAsyncQueueRequest(queueContext, Request, XferIsoExComplete, NULL, isoContext);
	if (NT_SUCCESS(status))
		return;

	USBERR("SubmitAsyncQueueRequest failed. Status=%08Xh\n", status);

Exit:
	WdfRequestCompleteWithInformation(Request, status, 0);
}

VOID XferAutoIsoEx(
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request)
{
	NTSTATUS status = STATUS_NOT_SUPPORTED;
	PDEVICE_CONTEXT deviceContext;
	PREQUEST_CONTEXT requestContext;
	WDF_OBJECT_ATTRIBUTES attributes;
	PURB urb;
	PMDL mdl;
	ULONG urbSize;
	PQUEUE_CONTEXT queueContext;
	KISO_PACKET* isoSourcePackets = NULL;
	LONG isoPacketContextLength = 0;
	size_t size;


	// Local vars pre-initialization //////////////////////////////////
	requestContext = GetRequestContext(Request);
	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status)) goto Exit;

	if ((queueContext = GetQueueContext(Queue)) == NULL)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid queue context");
		goto Exit;
	}

	/*
	When PipeContext->SharedAutoIsoExPacketMemory is updated in set pipe policy, the queue sync lock is acquired.
	This guarantees it is valid (or null) while in the main queue transfer function. e.g. OnIoControl, OnRead, OnWrite.
	*/
	requestContext->AutoIsoEx.FixedIsoPackets.Memory = queueContext->PipeContext->SharedAutoIsoExPacketMemory;
	if (!requestContext->AutoIsoEx.FixedIsoPackets.Memory)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Auto iso packets not set.");
		goto Exit;
	}

	requestContext->AutoIsoEx.FixedIsoPackets.Buffer	= WdfMemoryGetBuffer(requestContext->AutoIsoEx.FixedIsoPackets.Memory, &size);
	requestContext->AutoIsoEx.NumPackets				= (SHORT)(((ULONG)size - sizeof(KISO_CONTEXT)) / sizeof(KISO_PACKET));
	requestContext->AutoIsoEx.RequestedLength			= requestContext->Length;

	urbSize					= GET_ISO_URB_SIZE(requestContext->AutoIsoEx.NumPackets);
	isoSourcePackets		= (KISO_PACKET*)&requestContext->AutoIsoEx.FixedIsoPackets.Buffer[sizeof(KISO_CONTEXT)];
	isoPacketContextLength	= sizeof(KISO_CONTEXT) + (requestContext->AutoIsoEx.NumPackets * sizeof(KISO_PACKET));

	if (requestContext->AutoIsoEx.NumPackets < 0 || requestContext->AutoIsoEx.NumPackets > 1024)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERRN("Invalid NumberOfPacket=%i.", requestContext->AutoIsoEx.NumPackets);
		goto Exit;
	}

	if (USB_ENDPOINT_DIRECTION_IN(queueContext->Info.EndpointAddress))
	{
		if ((((LONG)requestContext->Length) - isoPacketContextLength) < 0)
		{
			status = STATUS_INVALID_BUFFER_SIZE;
			USBERRN("PipeID=%02Xh Buffer to small to accommodate KISO_CONTEXT.", queueContext->Info.EndpointAddress);
			goto Exit;
		}
		requestContext->AutoIsoEx.RequestedLength -= isoPacketContextLength;
	}

	///////////////////////////////////////////////////////////////////
	status = GetTransferMdl(Request, requestContext->ActualRequestType, &mdl);
	if (!NT_SUCCESS(status))
	{
		USBERR("GetTransferMdl failed.Status=%08Xh\n", urbSize, status);
		goto Exit;
	}

	// Allocate URB memory 	///////////////////////////////////////////
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = Request;
	status = WdfMemoryCreate(&attributes, NonPagedPool, POOL_TAG, urbSize, &requestContext->AutoIsoEx.UrbMemory, &urb);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfMemoryCreate failed. size=%u Status=%08Xh\n", urbSize, status);
		goto Exit;
	}
	///////////////////////////////////////////////////////////////////

	// handle pipe reset scenarios: ResetPipeOnResume, AutoClearStall
	mXfer_HandlePipeResetScenarios(status, queueContext, requestContext);

	// Init URB ///////////////////////////////////////////////////////
	RtlZeroMemory(urb, urbSize);
	urb->UrbIsochronousTransfer.Hdr.Length				= (USHORT) urbSize;
	urb->UrbIsochronousTransfer.Hdr.Function			= URB_FUNCTION_ISOCH_TRANSFER;
	urb->UrbIsochronousTransfer.PipeHandle				= WdfUsbTargetPipeWdmGetPipeHandle(queueContext->PipeHandle);
	urb->UrbIsochronousTransfer.TransferBufferMDL		= mdl;
	urb->UrbIsochronousTransfer.TransferBufferLength	= requestContext->AutoIsoEx.RequestedLength;
	urb->UrbIsochronousTransfer.NumberOfPackets			= requestContext->AutoIsoEx.NumPackets;

	urb->UrbIsochronousTransfer.TransferFlags |= USBD_START_ISO_TRANSFER_ASAP;

	if (USB_ENDPOINT_DIRECTION_IN(queueContext->Info.EndpointAddress))
		urb->UrbIsochronousTransfer.TransferFlags |= USBD_TRANSFER_DIRECTION_IN;
	///////////////////////////////////////////////////////////////////

	// Init URB iso packets ///////////////////////////////////////////
	mXfer_IsoPacketsToUrb(isoSourcePackets, urb, requestContext->AutoIsoEx.NumPackets, requestContext->AutoIsoEx.RequestedLength, goto Exit);
	///////////////////////////////////////////////////////////////////

	// Assign the new urb to the request and prepare it for WdfRequestSend.
	status = WdfUsbTargetPipeFormatRequestForUrb(queueContext->PipeHandle, Request, requestContext->AutoIsoEx.UrbMemory, NULL);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfUsbTargetPipeFormatRequestForUrb failed. Status=%08Xh\n", status);
		goto Exit;
	}

	status = SubmitAsyncQueueRequest(queueContext, Request, XferAutoIsoExComplete, NULL, requestContext);
	if (NT_SUCCESS(status))
		return;
	USBERR("SubmitAsyncQueueRequest failed. Status=%08Xh\n", status);

Exit:
	WdfRequestCompleteWithInformation(Request, status, 0);
}

VOID XferAutoIsoExComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in PREQUEST_CONTEXT requestContext)
{
	PURB urb;
	NTSTATUS status;
	ULONG transferred = 0;
	ULONG errorCount = 0;
	ULONG startFrame = 0;
	UNREFERENCED_PARAMETER(Target);

	status = CompletionParams->IoStatus.Status;
	urb = WdfMemoryGetBuffer(requestContext->AutoIsoEx.UrbMemory, NULL);

	// update the iso context
	errorCount = urb->UrbIsochronousTransfer.ErrorCount;
	startFrame = urb->UrbIsochronousTransfer.StartFrame;

	mXfer_HandlePipeResetScenariosForComplete(status, requestContext->QueueContext, requestContext);

	if (requestContext->RequestType == WdfRequestTypeRead)
	{
		PUCHAR pCh	= NULL;
		KISO_PACKET* isoDstPackets = NULL;
		KISO_CONTEXT* isoDstContext = NULL;

		NTSTATUS status2 = WdfRequestRetrieveOutputBuffer(Request, requestContext->Length, &pCh, NULL);
		if (!NT_SUCCESS(status2))
		{
			status = status2;
			USBERRN("WdfRequestRetrieveOutputBuffer failed. Status=%08Xh", status);
			goto Exit;
		}

		pCh += requestContext->AutoIsoEx.RequestedLength;

		isoDstContext	= (KISO_CONTEXT*)&pCh[0];
		isoDstPackets	= (KISO_PACKET*) &pCh[sizeof(KISO_CONTEXT)];

		isoDstContext->ErrorCount		= (SHORT)errorCount;
		isoDstContext->StartFrame		= startFrame;
		isoDstContext->NumberOfPackets	= (SHORT)urb->UrbIsochronousTransfer.NumberOfPackets;

		// update the iso packet status & lengths
		mXfer_IsoReadPacketsFromUrb(isoDstPackets, urb, (LONG)requestContext->AutoIsoEx.NumPackets, transferred);
	}

	if (!transferred)
		transferred = urb->UrbIsochronousTransfer.TransferBufferLength;

	USBE_SUCCESS(NT_SUCCESS(status), "Transferred=%u StartFrame=%08Xh Errors=%d Status=%08Xh\n",
	             transferred, startFrame, errorCount, status);
Exit:

	WdfRequestCompleteWithInformation(Request, status, transferred);
}

VOID XferIsoExComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in PKISO_CONTEXT IsoContext)
{
	PURB urb;
	NTSTATUS status;
	PREQUEST_CONTEXT requestContext;
	ULONG transferred = 0;
	UNREFERENCED_PARAMETER(Target);

	requestContext = GetRequestContext(Request);
	status = CompletionParams->IoStatus.Status;

	Xfer_CheckPipeStatus(status, requestContext->QueueContext->Info.EndpointAddress);

	urb = WdfMemoryGetBuffer(requestContext->IsoEx.UrbMemory, NULL);

	mXfer_HandlePipeResetScenariosForComplete(status, requestContext->QueueContext, requestContext);

	// update the iso context
	IsoContext->ErrorCount = (SHORT)urb->UrbIsochronousTransfer.ErrorCount;
	IsoContext->StartFrame = urb->UrbIsochronousTransfer.StartFrame;

	// update the iso packet status & lengths
	mXfer_IsoReadPacketsFromUrb(IsoContext->IsoPackets, urb, IsoContext->NumberOfPackets, transferred);

	if (!transferred)
		transferred = urb->UrbIsochronousTransfer.TransferBufferLength;


	USBE_SUCCESS(NT_SUCCESS(status), "Transferred=%u StartFrame=%08Xh Errors=%d Status=%08Xh\n",
	             transferred, IsoContext->StartFrame, IsoContext->ErrorCount, status);

	WdfRequestCompleteWithInformation(Request, status, transferred);
}
