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

const USHORT Frames_Per_bInterval_HighSpeed[32] =
{
	1, 2, 4, 8, 16, 32, 32,
	32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32,
	32,
};

const USHORT Frames_Per_bInterval_FullSpeed[32] =
{
	1, 2, 2, 4, 4, 4,
	4, 8, 8, 8, 8, 8,
	8, 8, 8, 16, 16, 16,
	16, 16, 16, 16, 16,
	16, 16, 16, 16, 16,
	16, 16, 16, 32,
};

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#define mXfer_IsoInitUrb(mUrb, mUrbSize, mWdmPipeHandle, mNumberOfPackets, mTransferMdl, mTransferLength, mTransferFlags) do {	\
	RtlZeroMemory(mUrb, mUrbSize);																								\
	mUrb->UrbIsochronousTransfer.Hdr.Length           = (USHORT) mUrbSize;														\
	mUrb->UrbIsochronousTransfer.Hdr.Function         = URB_FUNCTION_ISOCH_TRANSFER;  											\
	mUrb->UrbIsochronousTransfer.PipeHandle           = mWdmPipeHandle;   														\
	mUrb->UrbIsochronousTransfer.NumberOfPackets      = mNumberOfPackets; 														\
	mUrb->UrbIsochronousTransfer.TransferBufferMDL    = mTransferMdl; 															\
	mUrb->UrbIsochronousTransfer.TransferBufferLength = mTransferLength;  														\
	mUrb->UrbIsochronousTransfer.TransferFlags        = mTransferFlags;   														\
}while(0) 																														\
 
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


#define mXfer_IsoReadPacketsFromUrb(mIsoPacketArray,mUrb,mNumberOfPackets) do {										\
	LONG mPos;																										\
	for (mPos = 0; mPos < (mNumberOfPackets); mPos++)																\
	{																												\
		mIsoPacketArray[mPos].Length = (USHORT)(mUrb)->UrbIsochronousTransfer.IsoPacket[mPos].Length;				\
		mIsoPacketArray[mPos].Status = (USHORT)(((mUrb)->UrbIsochronousTransfer.IsoPacket[mPos].Status & 0xFFFF)|((mUrb)->UrbIsochronousTransfer.IsoPacket[mPos].Status >> 15));	\
	}																												\
}while(0)

#define mXfer_IsoCheckInitNextFrameNumber(mStatus, mWdfUsbTargetDevice, mRequestContext, mIsHS, mOut_UseNextStartFrame) do { 				\
	if (mRequestContext->QueueContext->IsFreshPipeReset) 																					\
	{																																		\
		if (!mRequestContext->Policies.IsoAlwaysStartAsap)   																				\
		{																																	\
			mRequestContext->QueueContext->IsFreshPipeReset = FALSE; 																		\
			mStatus = WdfUsbTargetDeviceRetrieveCurrentFrameNumber(mWdfUsbTargetDevice, &mRequestContext->QueueContext->NextFrameNumber);	\
			if (!NT_SUCCESS(mStatus))																										\
			{																																\
				USBWRNN("WdfUsbTargetDeviceRetrieveCurrentFrameNumber failed. Status=%08Xh",mStatus);    									\
				mOut_UseNextStartFrame = FALSE;  																							\
				mRequestContext->QueueContext->NextFrameNumber = 0;  																		\
			}																																\
			else 																															\
			{																																\
				mOut_UseNextStartFrame=TRUE; 																								\
				mRequestContext->QueueContext->NextFrameNumber+=(USHORT)mRequestContext->Policies.IsoStartLatency;   						\
				if (mIsHS && (mRequestContext->QueueContext->NextFrameNumber % 8))   														\
				{																															\
					mRequestContext->QueueContext->NextFrameNumber+=(8-(mRequestContext->QueueContext->NextFrameNumber % 8));				\
				}																															\
			}																																\
		}																																	\
		else 																																\
		{																																	\
			mRequestContext->QueueContext->IsFreshPipeReset = FALSE; 																		\
			mOut_UseNextStartFrame = FALSE;  																								\
		}																																	\
	}																																		\
}while(0)

#define mXfer_IsoAssignNextFrameNumber(mUrb, mQueueContext, mIsHS, mOut_FramesPerPacket) do {						\
	mUrb->UrbIsochronousTransfer.StartFrame = mQueueContext->NextFrameNumber;   									\
	if (mIsHS)  																									\
	{   																											\
		mOut_FramesPerPacket = Frames_Per_bInterval_HighSpeed[(mQueueContext->Info.Interval-1) % 32];   			\
		mQueueContext->NextFrameNumber+=((mUrb->UrbIsochronousTransfer.NumberOfPackets >> 3)*mOut_FramesPerPacket);	\
	}   																											\
	else																											\
	{   																											\
		mOut_FramesPerPacket = Frames_Per_bInterval_FullSpeed[(mQueueContext->Info.Interval-1) % 32];   			\
		mQueueContext->NextFrameNumber+=(mUrb->UrbIsochronousTransfer.NumberOfPackets*mOut_FramesPerPacket);		\
	}   																											\
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

typedef struct _ISO_URB_PACKET_CALC
{
	float	dist;
	float	distPos;
	INT		minBytesPerPacket;
	BOOLEAN	morePlusOneBytes;
	INT		offset;
	INT		remBytes;
	INT		remPackets;
} ISO_URB_PACKET_CALC;

VOID XferIsoWrComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in PREQUEST_CONTEXT requestContext);

VOID XferIsoRdComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in PREQUEST_CONTEXT requestContext);

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

	BOOLEAN isHS;
	USHORT framesPerPacket;
	BOOLEAN useNextStartFrame;

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

	isHS = IsHighSpeedDevice(deviceContext);

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
	mXfer_IsoInitUrb(
	    urb,
	    urbSize,
	    usbdPipeHandle,
	    isoContext->NumberOfPackets,
	    mdl,
	    requestContext->Length,
	    (USB_ENDPOINT_DIRECTION_IN(queueContext->Info.EndpointAddress) ? (USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK) : USBD_TRANSFER_DIRECTION_OUT));

	if (isoContext->Flags & KISO_FLAG_SET_START_FRAME)
	{
		urb->UrbIsochronousTransfer.StartFrame = isoContext->StartFrame;
	}

	///////////////////////////////////////////////////////////////////

	// Init URB iso packets ///////////////////////////////////////////
	mXfer_IsoPacketsToUrb(isoContext->IsoPackets, urb, isoContext->NumberOfPackets, requestContext->Length, goto Exit);
	///////////////////////////////////////////////////////////////////

	if (!(isoContext->Flags & KISO_FLAG_SET_START_FRAME))
	{
		// Set the start frame (and latency if the pipe was freshly reset)
		useNextStartFrame = requestContext->Policies.IsoAlwaysStartAsap ? FALSE : TRUE;
		if (useNextStartFrame)
		{
			mXfer_IsoCheckInitNextFrameNumber(status, deviceContext->WdfUsbTargetDevice, requestContext, isHS, useNextStartFrame);
		}
		if (useNextStartFrame)
		{
			framesPerPacket = 0;
			mXfer_IsoAssignNextFrameNumber(urb, queueContext, isHS, framesPerPacket);
		}
		else
		{
			// Either the user has specified to use ASAP or the request for the frame number failed.
			urb->UrbIsochronousTransfer.TransferFlags |= USBD_START_ISO_TRANSFER_ASAP;
		}
	}
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
	mXfer_IsoReadPacketsFromUrb(IsoContext->IsoPackets, urb, IsoContext->NumberOfPackets);

	if (NT_SUCCESS(status))
	{
		transferred = (ULONG)urb->UrbIsochronousTransfer.TransferBufferLength;

		USBMSGN("Transferred=%u StartFrame=%08Xh Errors=%d Status=%08Xh",
		        transferred, urb->UrbIsochronousTransfer.StartFrame, urb->UrbIsochronousTransfer.ErrorCount, status);
	}

	WdfRequestCompleteWithInformation(Request, status, transferred);
}

VOID XferIsoRdComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in PREQUEST_CONTEXT requestContext)
{
	PURB urb;
	NTSTATUS status;
	ULONG transferred = 0;
#ifdef DEBUG_LOGGING_ENABLED
	LONG fullPackets = 0;
	LONG emptyPackets = 0;
#endif

	UNREFERENCED_PARAMETER(Target);

	status = CompletionParams->IoStatus.Status;

	Xfer_CheckPipeStatus(status, requestContext->QueueContext->Info.EndpointAddress);

	urb = WdfMemoryGetBuffer(requestContext->AutoIso.UrbMemory, NULL);

	mXfer_HandlePipeResetScenariosForComplete(status, requestContext->QueueContext, requestContext);

	if ((NT_SUCCESS(status)) &&
	        (urb->UrbIsochronousTransfer.TransferBufferLength > 0) &&
	        (urb->UrbIsochronousTransfer.TransferBufferLength < requestContext->Length))
	{
		if (USB_ENDPOINT_DIRECTION_IN(requestContext->QueueContext->Info.EndpointAddress))
		{
			LONG posPacket;
			PUCHAR dataBuffer = NULL;
			LONG dataBufferOffset = 0;
			for (posPacket = 0; posPacket < (LONG)urb->UrbIsochronousTransfer.NumberOfPackets; posPacket++)
			{
				LONG packetOffset = (LONG)urb->UrbIsochronousTransfer.IsoPacket[posPacket].Offset;
				LONG packetLength = (LONG)urb->UrbIsochronousTransfer.IsoPacket[posPacket].Length;

#ifdef DEBUG_LOGGING_ENABLED
				if (packetLength == (LONG)requestContext->QueueContext->Info.MaximumPacketSize)
				{
					fullPackets++;
				}
				else if (packetLength == 0)
				{
					emptyPackets++;
				}
#endif
				if (packetLength > 0 && dataBufferOffset < packetOffset)
				{
					if (dataBuffer == NULL)
					{
						status = WdfRequestRetrieveOutputBuffer(Request, requestContext->Length, &dataBuffer, NULL);
						if (!NT_SUCCESS(status))
						{
							transferred = urb->UrbIsochronousTransfer.TransferBufferLength;
							USBERRN("WdfRequestRetrieveOutputBuffer Failed. Status=%08Xh", status);
							goto Exit;
						}
					}
					RtlMoveMemory(&dataBuffer[dataBufferOffset], &dataBuffer[packetOffset], packetLength);
				}
				dataBufferOffset += packetLength;
			}
		}
		else
		{
#ifdef DEBUG_LOGGING_ENABLED
			fullPackets = (LONG)urb->UrbIsochronousTransfer.NumberOfPackets;
#endif
		}
	}

	if (NT_SUCCESS(status))
	{
		transferred = (ULONG)urb->UrbIsochronousTransfer.TransferBufferLength;

#ifdef DEBUG_LOGGING_ENABLED
		USBMSGN("Transferred=%u TotalPackets=%u FullPackets=%d EmptyPackets=%d StartFrame=%08Xh Errors=%d",
		        transferred, urb->UrbIsochronousTransfer.NumberOfPackets, fullPackets, emptyPackets, urb->UrbIsochronousTransfer.StartFrame, urb->UrbIsochronousTransfer.ErrorCount);

#else
		USBMSGN("Transferred=%u StartFrame=%08Xh Errors=%d",
		        transferred, urb->UrbIsochronousTransfer.StartFrame, urb->UrbIsochronousTransfer.ErrorCount);
#endif
	}

Exit:
	WdfRequestCompleteWithInformation(Request, status, transferred);
}

VOID XferIsoWrComplete(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in PREQUEST_CONTEXT requestContext)
{
	NTSTATUS status;
	ULONG transferred = 0;
	PURB urb;

	UNREFERENCED_PARAMETER(Target);

	status = CompletionParams->IoStatus.Status;
	urb = WdfMemoryGetBuffer(requestContext->AutoIso.UrbMemory, NULL);

	Xfer_CheckPipeStatus(status, requestContext->QueueContext->Info.EndpointAddress);

	mXfer_HandlePipeResetScenariosForComplete(status, requestContext->QueueContext, requestContext);

	if (NT_SUCCESS(status))
	{
		transferred = (ULONG)urb->UrbIsochronousTransfer.TransferBufferLength;

		USBMSGN("Transferred=%u StartFrame=%08Xh Errors=%d Status=%08Xh",
		        transferred, urb->UrbIsochronousTransfer.StartFrame, urb->UrbIsochronousTransfer.ErrorCount, status);
	}

	WdfRequestCompleteWithInformation(Request, status, transferred);
}

VOID XferIsoRead(
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
	USBD_PIPE_HANDLE usbdPipeHandle;
	PQUEUE_CONTEXT queueContext;

	LONG numberOfPackets;
	LONG posPacket;
	BOOLEAN isHS;
	USHORT framesPerPacket;
	BOOLEAN useNextStartFrame;

	LONG packetOffsetCount;


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

	isHS = IsHighSpeedDevice(deviceContext);
	usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(queueContext->PipeHandle);
	if (!usbdPipeHandle)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("WdfUsbTargetPipeWdmGetPipeHandle failed. usbdPipeHandle=NULL\n");
		goto Exit;
	}

	if (!queueContext->Info.MaximumPacketSize)
	{
		status = STATUS_INVALID_BUFFER_SIZE;
		USBERRN("PipeID=%02Xh MaximumPacketSize=0", queueContext->Info.EndpointAddress);
		goto Exit;
	}

	numberOfPackets = requestContext->Length / queueContext->Info.MaximumPacketSize;
	if (isHS)
	{
		numberOfPackets -= numberOfPackets % 8;
		if (numberOfPackets > 1024) numberOfPackets = 1024;
	}
	else
	{
		if (numberOfPackets > 256) numberOfPackets = 256;
	}

	if (numberOfPackets <= 0)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("Invalid NumberOfPackets. NumberOfPackets=%u.\n", numberOfPackets);
		goto Exit;
	}
	requestContext->Length = numberOfPackets * queueContext->Info.MaximumPacketSize;

	urbSize = GET_ISO_URB_SIZE(numberOfPackets);
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
	status = WdfMemoryCreate(&attributes, NonPagedPool, POOL_TAG, urbSize, &requestContext->AutoIso.UrbMemory, &urb);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfMemoryCreate failed. size=%u Status=%08Xh\n", urbSize, status);
		goto Exit;
	}
	///////////////////////////////////////////////////////////////////

	// handle pipe reset scenarios: ResetPipeOnResume, AutoClearStall
	mXfer_HandlePipeResetScenarios(status, queueContext, requestContext);

	// Init URB ///////////////////////////////////////////////////////
	mXfer_IsoInitUrb(urb, urbSize, usbdPipeHandle, numberOfPackets, mdl, requestContext->Length, USBD_TRANSFER_DIRECTION_IN);
	urb->UrbIsochronousTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;
	///////////////////////////////////////////////////////////////////

	// Init URB iso packets ///////////////////////////////////////////
	packetOffsetCount = 0;
	for (posPacket = 0; posPacket < numberOfPackets; posPacket++)
	{
		urb->UrbIsochronousTransfer.IsoPacket[posPacket].Offset = packetOffsetCount;
		packetOffsetCount += queueContext->Info.MaximumPacketSize;
	}
	///////////////////////////////////////////////////////////////////

	// Set the start frame (and latency if the pipe was freshly reset)
	useNextStartFrame = requestContext->Policies.IsoAlwaysStartAsap ? FALSE : TRUE;
	if (useNextStartFrame)
	{
		mXfer_IsoCheckInitNextFrameNumber(status, deviceContext->WdfUsbTargetDevice, requestContext, isHS, useNextStartFrame);
	}
	if (useNextStartFrame)
	{
		framesPerPacket = 0;
		mXfer_IsoAssignNextFrameNumber(urb, queueContext, isHS, framesPerPacket);
	}
	else
	{
		// Either the user has specified to use ASAP or the request for the frame number failed.
		urb->UrbIsochronousTransfer.TransferFlags |= USBD_START_ISO_TRANSFER_ASAP;
	}
	///////////////////////////////////////////////////////////////////

	// Assign the new urb to the request and prepare it for WdfRequestSend.
	status = WdfUsbTargetPipeFormatRequestForUrb(queueContext->PipeHandle, Request, requestContext->AutoIso.UrbMemory, NULL);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfUsbTargetPipeFormatRequestForUrb failed. Status=%08Xh\n", status);
		goto Exit;
	}

	USBMSGN("HS=%s NumberOfPackets=%u RequestLength=%u UsingStartFrame=%s",
	        GetBoolString(isHS), numberOfPackets, requestContext->Length, GetBoolString(useNextStartFrame));

	status = SubmitAsyncQueueRequest(queueContext, Request, XferIsoRdComplete, WDF_NO_SEND_OPTIONS, requestContext);
	if (NT_SUCCESS(status))
		return;

	USBERR("SubmitAsyncQueueRequest failed. Status=%08Xh\n", status);

Exit:
	WdfRequestCompleteWithInformation(Request, status, 0);
}

VOID XferIsoWrite(
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
	USBD_PIPE_HANDLE usbdPipeHandle;
	PQUEUE_CONTEXT queueContext;

	LONG posPacket;
	BOOLEAN isHS;
	USHORT framesPerPacket;
	BOOLEAN useNextStartFrame;

	LONG numberOfPackets;
	ISO_URB_PACKET_CALC calc;

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
	isHS = IsHighSpeedDevice(deviceContext);
	RtlZeroMemory(&calc, sizeof(calc));

	usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(queueContext->PipeHandle);
	if (!usbdPipeHandle)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("WdfUsbTargetPipeWdmGetPipeHandle failed. usbdPipeHandle=NULL\n");
		goto Exit;
	}

	numberOfPackets = 0;
	if (requestContext->Policies.IsoNumFixedPackets)
	{
		numberOfPackets = requestContext->Policies.IsoNumFixedPackets;
		if ((requestContext->Length / numberOfPackets) > queueContext->Info.MaximumPacketSize)
		{
			status = STATUS_INVALID_BUFFER_SIZE;
			USBERR("Not enough fixed packets to transfer %d bytes.\n", requestContext->Length);
			goto Exit;
		}
	}
	else
	{
		numberOfPackets = requestContext->Length / queueContext->Info.MaximumPacketSize;
		if ((requestContext->Length % queueContext->Info.MaximumPacketSize) > 0) numberOfPackets++;

		if (isHS)
		{
			calc.remPackets = numberOfPackets % 8;
			if (calc.remPackets > 0) numberOfPackets += (8 - calc.remPackets);

			if (numberOfPackets > 1024)
			{
				status = STATUS_INVALID_BUFFER_SIZE;
				USBERR("Number of iso packets exceeds 1024. numberOfPackets=%u\n", numberOfPackets);
				goto Exit;
			}
		}
		else
		{
			calc.remPackets = 0;
			if (numberOfPackets > 256)
			{
				status = STATUS_INVALID_BUFFER_SIZE;
				USBERR("Number of iso packets exceeds 256. numberOfPackets=%u\n", numberOfPackets);
				goto Exit;
			}
		}

		if (numberOfPackets <= 0)
		{
			status = STATUS_INVALID_DEVICE_REQUEST;
			USBERR("Invalid NumberOfPackets. NumberOfPackets=%u.\n", numberOfPackets);
			goto Exit;
		}
	}

	///////////////////////////////////////////////////////////////////
	calc.minBytesPerPacket = requestContext->Length / numberOfPackets;

	calc.remBytes = requestContext->Length - (calc.minBytesPerPacket * numberOfPackets);
	ASSERT(calc.remBytes >= 0);

	calc.offset = 0;
	calc.morePlusOneBytes = calc.remBytes >= (numberOfPackets / 2) ? 1 : 0;

	calc.dist = calc.remBytes / (float)numberOfPackets;
	if (calc.morePlusOneBytes) calc.dist = 1.0F - calc.dist;

	calc.distPos = calc.morePlusOneBytes ? 0.0F : 1.0F;
	///////////////////////////////////////////////////////////////////

	status = GetTransferMdl(Request, requestContext->ActualRequestType, &mdl);
	if (!NT_SUCCESS(status))
	{
		USBERR("GetTransferMdl failed.Status=%08Xh\n",  status);
		goto Exit;
	}

	// Allocate URB memory 	///////////////////////////////////////////
	urbSize = GET_ISO_URB_SIZE(numberOfPackets);

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = Request;
	status = WdfMemoryCreate(&attributes, NonPagedPool, POOL_TAG, urbSize, &requestContext->AutoIso.UrbMemory, &urb);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfMemoryCreate failed. size=%u Status=%08Xh\n", urbSize, status);
		goto Exit;
	}
	///////////////////////////////////////////////////////////////////

	// handle pipe reset scenarios: ResetPipeOnResume, AutoClearStall
	mXfer_HandlePipeResetScenarios(status, queueContext, requestContext);

	// Init URB ///////////////////////////////////////////////////////
	mXfer_IsoInitUrb(urb, urbSize, usbdPipeHandle, numberOfPackets, mdl, requestContext->Length, USBD_TRANSFER_DIRECTION_OUT);
	///////////////////////////////////////////////////////////////////

	// Init URB iso packets ///////////////////////////////////////////

	// When iso packets are created for writes, the lengths are kept as even as
	// possible.
	for (posPacket = 0; posPacket < numberOfPackets; posPacket++)
	{
		urb->UrbIsochronousTransfer.IsoPacket[posPacket].Offset = calc.offset;
		calc.offset += calc.minBytesPerPacket;
		if (calc.remBytes <= 0) continue;

		calc.distPos += calc.dist;

		if (calc.morePlusOneBytes)
		{
			if (calc.distPos <= 1.0F)
			{
				calc.offset++;
				calc.remBytes--;
			}
			else
			{
				calc.distPos -= 1.0F;
			}
		}
		else
		{
			if (calc.distPos >= 1.0F)
			{
				calc.offset++;
				calc.remBytes--;
				calc.distPos -= 1.0F;
			}
		}
	}
	///////////////////////////////////////////////////////////////////

	// Set the start frame (and latency if the pipe was freshly reset)
	useNextStartFrame = requestContext->Policies.IsoAlwaysStartAsap ? FALSE : TRUE;
	if (useNextStartFrame)
	{
		mXfer_IsoCheckInitNextFrameNumber(status, deviceContext->WdfUsbTargetDevice, requestContext, isHS, useNextStartFrame);
	}
	if (useNextStartFrame)
	{
		framesPerPacket = 0;
		mXfer_IsoAssignNextFrameNumber(urb, queueContext, isHS, framesPerPacket);
	}
	else
	{
		// Either the user has specified to use ASAP or the request for the frame number failed.
		urb->UrbIsochronousTransfer.TransferFlags |= USBD_START_ISO_TRANSFER_ASAP;
	}
	///////////////////////////////////////////////////////////////////

	// Assign the new urb to the request and prepare it for WdfRequestSend.
	status = WdfUsbTargetPipeFormatRequestForUrb(queueContext->PipeHandle, Request, requestContext->AutoIso.UrbMemory, NULL);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfUsbTargetPipeFormatRequestForUrb failed. Status=%08Xh\n", status);
		goto Exit;
	}

	status = SubmitAsyncQueueRequest(queueContext, Request, XferIsoWrComplete, WDF_NO_SEND_OPTIONS, requestContext);
	if (NT_SUCCESS(status))
		return;

	USBERR("SubmitAsyncQueueRequest failed. Status=%08Xh\n", status);

Exit:
	WdfRequestCompleteWithInformation(Request, status, 0);
}
