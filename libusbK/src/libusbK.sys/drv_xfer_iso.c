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

static VOID XferIsoExComplete(__in WDFREQUEST Request,
                              __in WDFIOTARGET Target,
                              __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
                              __in PKISO_CONTEXT IsoContext);
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
} SUB_REQUEST_CONTEXT, *PSUB_REQUEST_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SUB_REQUEST_CONTEXT, GetSubRequestContext)

// iso queue events
EVT_WDF_REQUEST_CANCEL UsbK_EvtRequestCancel;
EVT_WDF_REQUEST_COMPLETION_ROUTINE XferIsoComplete;
EVT_WDF_REQUEST_COMPLETION_ROUTINE ReadWriteCompletion;

VOID
XferIsoCleanup(
    PREQUEST_CONTEXT    MainRequestContext,
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
    __in WDFREQUEST Request,
    __in size_t InputBufferLength,
    __in size_t OutputBufferLength)
{
	ULONG                   packetSize;
	ULONG                   numSubRequests;
	ULONG                   stageSize;
	PUCHAR                  virtualAddress = NULL;
	NTSTATUS               status;
	PDEVICE_CONTEXT         deviceContext;
	PREQUEST_CONTEXT        requestContext;
	WDFCOLLECTION           hCollection = NULL;
	WDFSPINLOCK            hSpinLock = NULL;
	WDF_OBJECT_ATTRIBUTES   attributes;
	WDFREQUEST              subRequest = NULL;
	PSUB_REQUEST_CONTEXT    subReqContext = NULL;
	ULONG                   i, j;
	PLIST_ENTRY             thisEntry;
	LIST_ENTRY              subRequestsList;
	USBD_PIPE_HANDLE        usbdPipeHandle;
	BOOLEAN                 cancelable;
	ULONG					TotalLength;

	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	cancelable = FALSE;

	InitializeListHead(&subRequestsList);

	requestContext = GetRequestContext(Request);
	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

	if (!requestContext->OriginalTransferMDL)
	{
		// ZLPs are not supported for iso.
		USBERR("Zero-length buffer. pipeID=%02Xh\n", GetRequestPipeID(requestContext));
		status = STATUS_BUFFER_TOO_SMALL;
		goto Exit;
	}

	TotalLength = requestContext->Length;

	//
	// Create a collection to store all the sub requests.
	//
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = Request;
	status = WdfCollectionCreate(&attributes, &hCollection);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfCollectionCreate failed. status=%Xh\n", status);
		goto Exit;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = hCollection;
	status = WdfSpinLockCreate(&attributes, &hSpinLock);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfSpinLockCreate failed. status=%Xh\n", status);
		goto Exit;
	}


	//
	// Each packet can hold this much info
	// Use the user defined packet size if available; otherwise use MaximumPacketSize.
	packetSize = requestContext->IoControlRequest.endpoint.packet_size;
	if (!packetSize)
		packetSize = requestContext->PipeContext->PipeInformation.MaximumPacketSize;

	USBMSG("totalLength = %d\n", requestContext->Length);
	USBMSG("packetSize = %d\n", packetSize);

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

	requestContext->SubRequestCollection = hCollection;
	requestContext->SubRequestCollectionLock = hSpinLock;

	virtualAddress = (PUCHAR) MmGetMdlVirtualAddress(requestContext->OriginalTransferMDL);

	for(i = 0; i < numSubRequests; i++)
	{

		WDFMEMORY               subUrbMemory;
		PURB                    subUrb;
		PMDL                    subMdl;
		ULONG                   nPackets;
		ULONG                   siz;
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
		siz = GET_ISO_URB_SIZE(nPackets);

		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
		WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&attributes, SUB_REQUEST_CONTEXT);

		status = WdfRequestCreate(
		             &attributes,
		             WdfUsbTargetDeviceGetIoTarget(deviceContext->WdfUsbTargetDevice),
		             &subRequest);

		if(!NT_SUCCESS(status))
		{

			USBERR("WdfRequestCreate failed. status=%Xh\n", status);
			goto Exit;
		}

		subReqContext = GetSubRequestContext(subRequest);
		subReqContext->UserRequest = Request;

		//
		// Allocate memory for URB.
		//
		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
		attributes.ParentObject = subRequest;
		status = WdfMemoryCreate(
		             &attributes,
		             NonPagedPool,
		             POOL_TAG,
		             siz,
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

		IoBuildPartialMdl(requestContext->OriginalTransferMDL,
		                  subMdl,
		                  (PVOID) virtualAddress,
		                  stageSize);

		subReqContext->SubMdl = subMdl;

		virtualAddress += stageSize;
		TotalLength -= stageSize;

		//
		// Initialize the subsidiary urb
		//
		usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(requestContext->PipeContext->Pipe);

		subUrb->UrbIsochronousTransfer.Hdr.Length = (USHORT) siz;
		subUrb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
		subUrb->UrbIsochronousTransfer.PipeHandle = usbdPipeHandle;
		if(requestContext->RequestType == WdfRequestTypeRead)
		{

			subUrb->UrbIsochronousTransfer.TransferFlags =
			    USBD_TRANSFER_DIRECTION_IN;
		}
		else
		{

			subUrb->UrbIsochronousTransfer.TransferFlags =
			    USBD_TRANSFER_DIRECTION_OUT;
		}

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
		status = WdfUsbTargetPipeFormatRequestForUrb(requestContext->PipeContext->Pipe,
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

		WdfRequestSetCompletionRoutine(subRequest,
		                               XferIsoComplete,
		                               requestContext);

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
		status = WdfCollectionAdd(hCollection, subRequest);
		if (!NT_SUCCESS(status))
		{
			USBERR("WdfCollectionAdd failed. status=%Xh\n", status);
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
		if (WdfRequestSend(subRequest, WdfUsbTargetPipeGetIoTarget(requestContext->PipeContext->Pipe), WDF_NO_SEND_OPTIONS) == FALSE)
		{
			status = WdfRequestGetStatus(subRequest);
			//
			// Insert the subrequest back into the subrequestlist so cleanup can find it and delete it
			//
			InsertHeadList(&subRequestsList, &subReqContext->ListEntry);
			USBMSG("WdfRequestSend failed. status=%Xh\n", status);
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

		if(hCollection)
		{
			XferIsoCleanup(requestContext, &subRequestsList, &completeRequest);
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

			if (requestContext->TotalTransferred > 0 )
			{
				WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, requestContext->TotalTransferred);
			}
			else
			{
				WdfRequestCompleteWithInformation(Request, status, requestContext->TotalTransferred);
			}
		}

	}

	USBMSG("ends\n");
	return;
}

VOID XferIsoHS(
    __in WDFQUEUE         Queue,
    __in WDFREQUEST       Request,
    __in ULONG            Length)
/*++

Routine Description:

    High Speed Isoch Transfers requires packets in multiples of 8.
    (Argument: 8 micro-frames per ms frame)
    Another restriction is that each Irp/Urb pair can be associated
    with a max of 1024 packets.

    Here is one of the ways of creating Irp/Urb pairs.
    Depending on the characteristics of real-world device,
    the algorithm may be different

    This algorithm will distribute data evenly among all the packets.

    Input:
    TotalLength - no. of bytes to be transferred.

    Other parameters:
    packetSize - max size of each packet for this pipe.

    Implementation Details:

    Step 1:
    ASSERT(TotalLength >= 8)

    Step 2:
    Find the exact number of packets required to transfer all of this data

    numberOfPackets = (TotalLength + packetSize - 1) / packetSize

    Step 3:
    Number of packets in multiples of 8.

    if(0 == (numberOfPackets % 8)) {

        actualPackets = numberOfPackets;
    }
    else {

        actualPackets = numberOfPackets +
                        (8 - (numberOfPackets % 8));
    }

    Step 4:
    Determine the min. data in each packet.

    minDataInEachPacket = TotalLength / actualPackets;

    Step 5:
    After placing min data in each packet,
    determine how much data is left to be distributed.

    dataLeftToBeDistributed = TotalLength -
                              (minDataInEachPacket * actualPackets);

    Step 6:
    Start placing the left over data in the packets
    (above the min data already placed)

    numberOfPacketsFilledToBrim = dataLeftToBeDistributed /
                                  (packetSize - minDataInEachPacket);

    Step 7:
    determine if there is any more data left.

    dataLeftToBeDistributed -= (numberOfPacketsFilledToBrim *
                                (packetSize - minDataInEachPacket));

    Step 8:
    The "dataLeftToBeDistributed" is placed in the packet at index
    "numberOfPacketsFilledToBrim"

    Algorithm at play:

    TotalLength  = 8193
    packetSize   = 8
    Step 1

    Step 2
    numberOfPackets = (8193 + 8 - 1) / 8 = 1025

    Step 3
    actualPackets = 1025 + 7 = 1032

    Step 4
    minDataInEachPacket = 8193 / 1032 = 7 bytes

    Step 5
    dataLeftToBeDistributed = 8193 - (7 * 1032) = 969.

    Step 6
    numberOfPacketsFilledToBrim = 969 / (8 - 7) = 969.

    Step 7
    dataLeftToBeDistributed = 969 - (969 * 1) = 0.

    Step 8
    Done :)

    Another algorithm
    Completely fill up (as far as possible) the early packets.
    Place 1 byte each in the rest of them.
    Ensure that the total number of packets is multiple of 8.

    This routine then
    1. Creates a Sub Request for each irp/urb pair.
       (Each irp/urb pair can transfer a max of 1024 packets.)
    2. All the irp/urb pairs are initialized
    3. The subsidiary irps (of the irp/urb pair) are passed
       down the stack at once.
    4. The main Read/Write is completed in the XferIsoComplete
       even if one SubRequest is sent successfully.

Arguments:

    Device - Device handle
    Request - Default queue handle
    Request - Read/Write Request received from the user app.
    TotalLength - Length of the user buffer.

Return Value:

    VOID
--*/
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
	WDFCOLLECTION           hCollection = NULL;
	WDFSPINLOCK             hSpinLock = NULL;
	WDF_OBJECT_ATTRIBUTES   attributes;
	WDFREQUEST              subRequest;
	PSUB_REQUEST_CONTEXT    subReqContext;
	ULONG                   i, j;
	PLIST_ENTRY             thisEntry;
	LIST_ENTRY              subRequestsList;
	USBD_PIPE_HANDLE        usbdPipeHandle;
	BOOLEAN                 cancelable;
	ULONG					TotalLength;

	cancelable = FALSE;
	TotalLength = Length;

	InitializeListHead(&subRequestsList);
	requestContext = GetRequestContext(Request);

	switch(requestContext->RequestType)
	{
	case WdfRequestTypeRead:
		USBMSG("read length=%d\n", TotalLength);
		break;
	case WdfRequestTypeWrite:
		USBMSG("write length=%d\n", TotalLength);
		break;
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("transfer-request types must be either read or write\n");
		goto Exit;
	}

	if(TotalLength < 8)
	{

		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

	//
	// Create a collection to store all the sub requests.
	//
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = Request;
	status = WdfCollectionCreate(&attributes,
	                             &hCollection);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfCollectionCreate failed. status=%Xh\n", status);
		goto Exit;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = hCollection;
	status = WdfSpinLockCreate(&attributes, &hSpinLock);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfSpinLockCreate failed. status=%Xh\n", status);
		goto Exit;
	}

	//
	// each packet can hold this much info
	//
	packetSize = requestContext->PipeContext->PipeInformation.MaximumPacketSize;

	numberOfPackets = (TotalLength + packetSize - 1) / packetSize;

	if(0 == (numberOfPackets % 8))
	{

		actualPackets = numberOfPackets;
	}
	else
	{

		//
		// we need multiple of 8 packets only.
		//
		actualPackets = numberOfPackets +
		                (8 - (numberOfPackets % 8));
	}

	minDataInEachPacket = TotalLength / actualPackets;

	if(minDataInEachPacket == packetSize)
	{

		numberOfPacketsFilledToBrim = actualPackets;
		dataLeftToBeDistributed     = 0;

		USBMSG("TotalLength = %d\n", TotalLength);
		USBMSG("PacketSize  = %d\n", packetSize);
		USBMSG("Each of %d packets has %d bytes\n",
		       numberOfPacketsFilledToBrim,
		       packetSize);
	}
	else
	{

		dataLeftToBeDistributed = TotalLength -
		                          (minDataInEachPacket * actualPackets);

		numberOfPacketsFilledToBrim = dataLeftToBeDistributed /
		                              (packetSize - minDataInEachPacket);

		dataLeftToBeDistributed -= (numberOfPacketsFilledToBrim *
		                            (packetSize - minDataInEachPacket));


		USBMSG("TotalLength = %d\n", TotalLength);
		USBMSG("PacketSize  = %d\n", packetSize);
		USBMSG("Each of %d packets has %d bytes\n",
		       numberOfPacketsFilledToBrim,
		       packetSize);
		if(dataLeftToBeDistributed)
		{

			USBMSG("One packet has %d bytes\n",
			       minDataInEachPacket + dataLeftToBeDistributed);
			USBMSG("Each of %d packets has %d bytes\n",
			       actualPackets - (numberOfPacketsFilledToBrim + 1),
			       minDataInEachPacket);
		}
		else
		{
			USBMSG("Each of %d packets has %d bytes\n",
			       actualPackets - numberOfPacketsFilledToBrim,
			       minDataInEachPacket);
		}
	}

	//
	// determine how many stages of transfer needs to be done.
	// in other words, how many irp/urb pairs required.
	// this irp/urb pair is also called the subsidiary irp/urb pair
	//
	numSubRequests = (actualPackets + 1023) / 1024;

	USBMSG("numSubRequests = %d\n", numSubRequests);

	requestContext->SubRequestCollection = hCollection;
	requestContext->SubRequestCollectionLock = hSpinLock;
//	requestContext->OriginalTransferMDL = TransferMDL;

	if (requestContext->OriginalTransferMDL)
	{
		virtualAddress = (PUCHAR) MmGetMdlVirtualAddress(requestContext->OriginalTransferMDL);
	}
	for(i = 0; i < numSubRequests; i++)
	{

		WDFMEMORY               subUrbMemory;
		PURB                    subUrb;
		PMDL                    subMdl;
		ULONG                   nPackets;
		ULONG                   siz;
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
		if(actualPackets <= 1024)
		{

			nPackets = actualPackets;
			actualPackets = 0;
		}
		else
		{

			nPackets = 1024;
			actualPackets -= 1024;
		}

		USBMSG("nPackets = %d for Irp/URB pair %d\n", nPackets, i);

		ASSERT(nPackets <= 1024);

		siz = GET_ISO_URB_SIZE(nPackets);


		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
		WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&attributes, SUB_REQUEST_CONTEXT            );


		status = WdfRequestCreate(
		             &attributes,
		             WdfUsbTargetDeviceGetIoTarget(deviceContext->WdfUsbTargetDevice),
		             &subRequest);

		if(!NT_SUCCESS(status))
		{

			USBERR("WdfRequestCreate failed. status=%Xh\n", status);
			goto Exit;
		}

		subReqContext = GetSubRequestContext(subRequest);
		subReqContext->UserRequest = Request;

		//
		// Allocate memory for URB.
		//
		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
		attributes.ParentObject = subRequest;
		status = WdfMemoryCreate(
		             &attributes,
		             NonPagedPool,
		             POOL_TAG,
		             siz,
		             &subUrbMemory,
		             (PVOID*) &subUrb);

		if (!NT_SUCCESS(status))
		{
			WdfObjectDelete(subRequest);
			USBERR("Failed to alloc MemoryBuffer for suburb\n");
			goto Exit;
		}

		subReqContext->SubUrb = subUrb;


		if(nPackets > numberOfPacketsFilledToBrim)
		{

			stageSize =  packetSize * numberOfPacketsFilledToBrim;
			stageSize += (minDataInEachPacket *
			              (nPackets - numberOfPacketsFilledToBrim));
			stageSize += dataLeftToBeDistributed;
		}
		else
		{

			stageSize = packetSize * nPackets;
		}

		//
		// allocate a mdl.
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

		IoBuildPartialMdl(requestContext->OriginalTransferMDL,
		                  subMdl,
		                  (PVOID) virtualAddress,
		                  stageSize);

		subReqContext->SubMdl = subMdl;

		virtualAddress += stageSize;
		TotalLength -= stageSize;

		usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(requestContext->PipeContext->Pipe);
		subUrb->UrbIsochronousTransfer.Hdr.Length = (USHORT) siz;
		subUrb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
		subUrb->UrbIsochronousTransfer.PipeHandle = usbdPipeHandle;

		if(requestContext->RequestType == WdfRequestTypeRead)
		{

			subUrb->UrbIsochronousTransfer.TransferFlags =
			    USBD_TRANSFER_DIRECTION_IN;
		}
		else
		{

			subUrb->UrbIsochronousTransfer.TransferFlags =
			    USBD_TRANSFER_DIRECTION_OUT;
		}

		subUrb->UrbIsochronousTransfer.TransferBufferLength = stageSize;
		subUrb->UrbIsochronousTransfer.TransferBufferMDL = subMdl;
		/*
		        This is a way to set the start frame and NOT specify ASAP flag.

		        status = WdfUsbTargetDeviceRetrieveCurrentFrameNumber(wdfUsbDevice, &frameNumber);
		        subUrb->UrbIsochronousTransfer.StartFrame = frameNumber  + SOME_LATENCY;
		*/
		subUrb->UrbIsochronousTransfer.TransferFlags |=
		    USBD_START_ISO_TRANSFER_ASAP;

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

				if(numberOfPacketsFilledToBrim)
				{

					offset += packetSize;
					numberOfPacketsFilledToBrim--;
					status = RtlULongSub(stageSize, packetSize, &stageSize);
					ASSERT(NT_SUCCESS(status));
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
		}
		else
		{

			offset = 0;

			for(j = 0; j < nPackets; j++)
			{

				subUrb->UrbIsochronousTransfer.IsoPacket[j].Offset = offset;

				if(numberOfPacketsFilledToBrim)
				{

					subUrb->UrbIsochronousTransfer.IsoPacket[j].Length = packetSize;
					offset += packetSize;
					numberOfPacketsFilledToBrim--;
					stageSize -= packetSize;
				}
				else if(dataLeftToBeDistributed)
				{

					subUrb->UrbIsochronousTransfer.IsoPacket[j].Length =
					    minDataInEachPacket + dataLeftToBeDistributed;
					offset += (minDataInEachPacket + dataLeftToBeDistributed);
					stageSize -= (minDataInEachPacket + dataLeftToBeDistributed);
					dataLeftToBeDistributed = 0;

				}
				else
				{
					subUrb->UrbIsochronousTransfer.IsoPacket[j].Length = minDataInEachPacket;
					offset += minDataInEachPacket;
					stageSize -= minDataInEachPacket;
				}
			}

			ASSERT(stageSize == 0);
		}

		//
		// Associate the URB with the request.
		//
		status = WdfUsbTargetPipeFormatRequestForUrb(requestContext->PipeContext->Pipe,
		         subRequest,
		         subUrbMemory,
		         NULL );
		if (!NT_SUCCESS(status))
		{
			USBERR("Failed to format requset for urb\n");
			WdfObjectDelete(subRequest);
			IoFreeMdl(subMdl);
			goto Exit;
		}

		WdfRequestSetCompletionRoutine(subRequest,
		                               XferIsoComplete,
		                               requestContext);

		//
		// WdfCollectionAdd takes a reference on the request object and removes
		// it when you call WdfCollectionRemove.
		//
		status = WdfCollectionAdd(hCollection, subRequest);
		if (!NT_SUCCESS(status))
		{
			USBERR("WdfCollectionAdd failed. status=%Xh\n", status);
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
	// NOTE: The reference just keeps the request context  from being deleted but
	// if you need to access anything in the WDFREQUEST itself in the cancel or the
	// completion routine make sure that you synchronise the completion of the request with
	// accessing anything in it.
	// You can also use a spinlock  and use that to synchronise
	// the cancel routine with the completion routine.
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
		USBMSG("Sending subRequest 0x%p\n", subRequest);

		if (WdfRequestSend(subRequest, WdfUsbTargetPipeGetIoTarget(requestContext->PipeContext->Pipe), WDF_NO_SEND_OPTIONS) == FALSE)
		{
			status = WdfRequestGetStatus(subRequest);
			//
			// Insert the subrequest back into the subrequestlist so cleanup can find it and delete it
			//
			InsertHeadList(&subRequestsList, &subReqContext->ListEntry);
			USBERR("WdfRequestSend failed. status=%Xh\n", status);
			WdfVerifierDbgBreakPoint(); // break into the debugger if the registry value is set.
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

		if(hCollection)
		{
			XferIsoCleanup(requestContext, &subRequestsList, &completeRequest);
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

			if (requestContext->TotalTransferred > 0 )
			{
				WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, requestContext->TotalTransferred);
			}
			else
			{
				WdfRequestCompleteWithInformation(Request, status, requestContext->TotalTransferred);
			}

		}

	}

	USBMSG("ends\n");
	return;
}

VOID
XferIsoCleanup(
    PREQUEST_CONTEXT    MainRequestContext,
    PLIST_ENTRY         SubRequestsList,
    PBOOLEAN            CompleteRequest
)
{
	PLIST_ENTRY           thisEntry;
	PSUB_REQUEST_CONTEXT  subReqContext;
	WDFREQUEST            subRequest;
	ULONG                 numPendingRequests;

	*CompleteRequest = TRUE;
	WdfSpinLockAcquire(MainRequestContext->SubRequestCollectionLock);

	while(!IsListEmpty(SubRequestsList))
	{
		thisEntry = RemoveHeadList(SubRequestsList);
		subReqContext = CONTAINING_RECORD(thisEntry, SUB_REQUEST_CONTEXT, ListEntry);
		subRequest = WdfObjectContextGetObject(subReqContext);
		WdfCollectionRemove(MainRequestContext->SubRequestCollection, subRequest);

		if(subReqContext->SubMdl)
		{
			IoFreeMdl(subReqContext->SubMdl);
			subReqContext->SubMdl = NULL;
		}

		WdfObjectDelete(subRequest);
	}
	numPendingRequests = WdfCollectionGetCount(MainRequestContext->SubRequestCollection);
	WdfSpinLockRelease(MainRequestContext->SubRequestCollectionLock);

	if (numPendingRequests > 0)
	{
		*CompleteRequest = FALSE;
	}

	return;
}

VOID
XferIsoComplete(
    __in WDFREQUEST                  Request,
    __in WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    __in WDFCONTEXT                  Context
)
/*++

Routine Description:

    Completion Routine

Arguments:

    Context - Driver supplied context
    Target - Target handle
    Request - Request handle
    Params - request completion params


Return Value:

    VOID

--*/
{
	PURB                    urb;
	ULONG                   i;
	ULONG                   numPendingRequests;
	NTSTATUS                status;
	PREQUEST_CONTEXT        requestContext;
	WDFREQUEST              mainRequest;
	PSUB_REQUEST_CONTEXT    subReqContext;

	UNREFERENCED_PARAMETER(Target);

	subReqContext = GetSubRequestContext(Request);

	urb = (PURB) subReqContext->SubUrb;

	IoFreeMdl(subReqContext->SubMdl);
	subReqContext->SubMdl = NULL;

	requestContext = (PREQUEST_CONTEXT) Context;
	ASSERT(requestContext);

	status = CompletionParams->IoStatus.Status;

	if(NT_SUCCESS(status) &&
	        USBD_SUCCESS(urb->UrbHeader.Status))
	{

		requestContext->TotalTransferred +=
		    urb->UrbIsochronousTransfer.TransferBufferLength;

		USBMSG("requestContext->TotalTransferred = %d\n", requestContext->TotalTransferred);
	}
	else
	{

		USBERR("read-write irp failed. status=%Xh\n", status);
		USBERR("urb header status=%Xh\n", urb->UrbHeader.Status);

	}

	if(!NT_SUCCESS(status))
	{
		for(i = 0; i < urb->UrbIsochronousTransfer.NumberOfPackets; i++)
		{

			USBDBG("IsoPacket[%d].Length = %X IsoPacket[%d].Status = %X\n",
			       i,
			       urb->UrbIsochronousTransfer.IsoPacket[i].Length,
			       i,
			       urb->UrbIsochronousTransfer.IsoPacket[i].Status);
		}
	}

	//
	// Remove the SubRequest from the collection.
	//
	WdfSpinLockAcquire(requestContext->SubRequestCollectionLock);

	WdfCollectionRemove(requestContext->SubRequestCollection, Request);

	numPendingRequests = WdfCollectionGetCount(requestContext->SubRequestCollection);

	WdfSpinLockRelease(requestContext->SubRequestCollectionLock);

	//
	// If all the sub requests are completed. Complete the main request sent
	// by the user application.
	//
	if(numPendingRequests == 0)
	{

		USBMSG("no more pending sub requests\n");

		if(NT_SUCCESS(status))
		{

			USBMSG("urb start frame %X\n",
			       urb->UrbIsochronousTransfer.StartFrame);
		}

		mainRequest = WdfObjectContextGetObject(requestContext);

		//
		// if we transferred some data, main Irp completes with success
		//
		USBMSG("Total data transferred = %u\n", requestContext->TotalTransferred);

		USBMSG("XferIsoComplete %s completed\n",
		       requestContext->RequestType == WdfRequestTypeRead ? "Read" : "Write");
		//
		// Mark the main request as not cancelable before completing it.
		//
		status = WdfRequestUnmarkCancelable(mainRequest);
		if (NT_SUCCESS(status))
		{
			//
			// If WdfRequestUnmarkCancelable returns STATUS_SUCCESS
			// that means the cancel routine has been removed. In that case
			// we release the reference otherwise the cancel routine does it.
			//
			WdfObjectDereference(mainRequest);
		}

		if (requestContext->TotalTransferred > 0 )
		{
			WdfRequestCompleteWithInformation(mainRequest,
			                                  STATUS_SUCCESS, requestContext->TotalTransferred);
		}
		else
		{
			USBMSG("XferIsoComplete failed. status=%Xh\n",
			       CompletionParams->IoStatus.Status);

			WdfRequestCompleteWithInformation(mainRequest,
			                                  CompletionParams->IoStatus.Status, requestContext->TotalTransferred);
		}

	}

	//
	// Since we created the subrequests, we should free it by removing the
	// reference.
	//
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

	if(!requestContext->SubRequestCollection)
	{
		ASSERTMSG("Very unlikely, collection is created before\
                        the request is made cancellable", FALSE);
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
	WdfSpinLockAcquire(requestContext->SubRequestCollectionLock);

	for(i = 0; i < WdfCollectionGetCount(requestContext->SubRequestCollection); i++)
	{


		subRequest = WdfCollectionGetItem(requestContext->SubRequestCollection, i);

		subReqContext = GetSubRequestContext(subRequest);

		WdfObjectReference(subRequest);

		InsertTailList(&cancelList, &subReqContext->ListEntry);
	}

	WdfSpinLockRelease(requestContext->SubRequestCollectionLock);

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

VOID
Queue_OnDefaultStop(
    __in WDFQUEUE         Queue,
    __in WDFREQUEST       Request,
    __in ULONG            ActionFlags
)
/*++

Routine Description:

    This callback is invoked on every inflight request when the device
    is suspended or removed. Since our inflight read and write requests
    are actually pending in the target device, we will just acknowledge
    its presence. Until we acknowledge, complete, or requeue the requests
    framework will wait before allowing the device suspend or remove to
    proceeed. When the underlying USB stack gets the request to suspend or
    remove, it will fail all the pending requests.

Arguments:

Return Value:
    None

--*/
{
	PREQUEST_CONTEXT reqContext;

	UNREFERENCED_PARAMETER(Queue);


	reqContext = GetRequestContext(Request);

	if (ActionFlags & WdfRequestStopActionSuspend )
	{
		WdfRequestStopAcknowledge(Request, FALSE); // Don't requeue
	}
	else if(ActionFlags & WdfRequestStopActionPurge)
	{
		WdfRequestCancelSentRequest(Request);
	}
	return;
}

VOID
Queue_OnDefaultResume(
    __in WDFQUEUE   Queue,
    __in WDFREQUEST Request
)
/*++

Routine Description:

     This callback is invoked for every request pending in the driver - in-flight
     request - to notify that the hardware is ready for contiuing the processing
     of the request.

Arguments:

    Queue - Queue the request currently belongs to
    Request - Request that is currently out of queue and being processed by the driver

Return Value:

    None.

--*/
{
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	// Leave this function here for now in case we want to add anything in the future
}


VOID XferIsoEx(__in WDFQUEUE Queue,
               __in WDFREQUEST Request,
               __in size_t InputBufferLength,
               __in size_t OutputBufferLength)
{
	NTSTATUS status = STATUS_NOT_SUPPORTED;
	PDEVICE_CONTEXT deviceContext;
	PREQUEST_CONTEXT requestContext;
	PKISO_CONTEXT isoContext;
	size_t isoContextSize;
	WDF_OBJECT_ATTRIBUTES attributes;
	PURB urb;
	ULONG urbSize;
	USBD_PIPE_HANDLE usbdPipeHandle;
	ULONG posPacket;
	ULONG nextOffset;
	ULONG nextLength;
	ULONG transferCounter;
	WDF_REQUEST_SEND_OPTIONS sendOptions;

	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	USBDBG("begin");

	// Local vars pre-initialization //////////////////////////////////
	requestContext = GetRequestContext(Request);
	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

	USBDBG("ContextMemory=%p\n", requestContext->Iso.ContextMemory);

	transferCounter = (ULONG)InterlockedIncrement(&requestContext->PipeContext->TransferCounter);

	if (!requestContext->Iso.ContextMemory)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("NULL ISO context.\n");
		goto Exit;
	}

	isoContext = (PKISO_CONTEXT)WdfMemoryGetBuffer(requestContext->Iso.ContextMemory, &isoContextSize);
	if (!isoContext)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		USBERR("WdfMemoryGetBuffer failed. Invalid ISO context.\n");
		goto Exit;
	}

	isoContext->TransferCounter = transferCounter;

	usbdPipeHandle = WdfUsbTargetPipeWdmGetPipeHandle(requestContext->PipeContext->Pipe);
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

	// Allocate URB memory 	///////////////////////////////////////////
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = Request;
	status = WdfMemoryCreate(&attributes, NonPagedPool, POOL_TAG, urbSize, &requestContext->UrbMemory, &urb);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfMemoryCreate failed. size=%u status=%08Xh\n", urbSize, status);
		goto Exit;
	}
	///////////////////////////////////////////////////////////////////

	// Init URB ///////////////////////////////////////////////////////
	RtlZeroMemory(urb, urbSize);
	urb->UrbIsochronousTransfer.Hdr.Length = (USHORT) urbSize;
	urb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
	urb->UrbIsochronousTransfer.PipeHandle = usbdPipeHandle;

	urb->UrbIsochronousTransfer.TransferBufferLength = requestContext->Length;
	urb->UrbIsochronousTransfer.TransferBufferMDL = requestContext->OriginalTransferMDL;
	urb->UrbIsochronousTransfer.StartFrame = isoContext->StartFrame;

	if(requestContext->RequestType == WdfRequestTypeRead)
		urb->UrbIsochronousTransfer.TransferFlags |= USBD_TRANSFER_DIRECTION_IN;
	else
		urb->UrbIsochronousTransfer.TransferFlags |= USBD_TRANSFER_DIRECTION_OUT;

	if (!urb->UrbIsochronousTransfer.StartFrame)
		urb->UrbIsochronousTransfer.TransferFlags |= USBD_START_ISO_TRANSFER_ASAP;

	urb->UrbIsochronousTransfer.NumberOfPackets = isoContext->NumberOfPackets;
	///////////////////////////////////////////////////////////////////

	// Init URB iso packets ///////////////////////////////////////////
	nextOffset = 0;
	nextLength = 0;
	if(requestContext->RequestType == WdfRequestTypeRead)
	{
		// DeviceToHost packet setup
		for (posPacket = 0; posPacket < isoContext->NumberOfPackets; posPacket++)
		{
			ULONG offset = isoContext->IsoPackets[posPacket].Offset;
			isoContext->IsoPackets[posPacket].Status = 0;

			if (offset < nextOffset || offset >= requestContext->Length)
			{
				status = STATUS_INVALID_DEVICE_REQUEST;
				USBERR("Invalid packet offset. IsoPackets[%u].Offset=%u.\n",
				       posPacket, offset);
				goto Exit;
			}

			nextOffset = offset;

			urb->UrbIsochronousTransfer.IsoPacket[posPacket].Offset = nextOffset;
			urb->UrbIsochronousTransfer.IsoPacket[posPacket].Length = 0;
		}
	}
	else
	{
		// HostToDevice packet setup
		for (posPacket = 0; posPacket < isoContext->NumberOfPackets; posPacket++)
		{
			ULONG offset = isoContext->IsoPackets[posPacket].Offset;
			isoContext->IsoPackets[posPacket].Status = 0;

			if (offset < nextOffset || offset >= requestContext->Length)
			{
				status = STATUS_INVALID_DEVICE_REQUEST;
				USBERR("Invalid packet offset. IsoPackets[%u].Offset=%u.\n",
				       posPacket, offset);
				goto Exit;
			}

			nextLength = offset - nextOffset;
			nextOffset = offset;

			urb->UrbIsochronousTransfer.IsoPacket[posPacket].Offset = nextOffset;
			urb->UrbIsochronousTransfer.IsoPacket[posPacket].Length = nextLength;
		}
	}
	///////////////////////////////////////////////////////////////////

	// Assign the new urb to the request and prepare it for WdfRequestSend.
	status = WdfUsbTargetPipeFormatRequestForUrb(requestContext->PipeContext->Pipe, Request, requestContext->UrbMemory, NULL);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfUsbTargetPipeFormatRequestForUrb failed. status=%Xh\n", status);
		goto Exit;
	}

	WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, 0);
	status = SetRequestTimeout(requestContext, Request, &sendOptions);
	if (!NT_SUCCESS(status))
	{
		USBERR("SetRequestTimeout failed. status=%Xh\n", status);
		goto Exit;
	}

	status = SubmitAsyncRequest(requestContext, Request, XferIsoExComplete, &sendOptions, isoContext);
	if (NT_SUCCESS(status))
		return;

	USBERR("SubmitAsyncRequest failed. status=%Xh\n", status);

Exit:
	if (!NT_SUCCESS(status))
	{
		WdfRequestCompleteWithInformation(Request, status, 0);
	}
	USBDBG("end");
}

static VOID XferIsoExComplete(__in WDFREQUEST Request,
                              __in WDFIOTARGET Target,
                              __in PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
                              __in PKISO_CONTEXT IsoContext)
{
	PURB urb;
	NTSTATUS status;
	PREQUEST_CONTEXT requestContext;
	ULONG posPacket;

	UNREFERENCED_PARAMETER(Target);

	requestContext = GetRequestContext(Request);

	status = CompletionParams->IoStatus.Status;
	urb = WdfMemoryGetBuffer(requestContext->UrbMemory, NULL);

	// update the iso context
	IsoContext->ErrorCount = urb->UrbIsochronousTransfer.ErrorCount;
	IsoContext->StartFrame = urb->UrbIsochronousTransfer.StartFrame;

	// update the iso packet status & lengths
	for (posPacket = 0; posPacket < IsoContext->NumberOfPackets; posPacket++)
	{
		IsoContext->IsoPackets[posPacket].Length = urb->UrbIsochronousTransfer.IsoPacket[posPacket].Length;
		IsoContext->IsoPackets[posPacket].Status = urb->UrbIsochronousTransfer.IsoPacket[posPacket].Status;
		requestContext->TotalTransferred += urb->UrbIsochronousTransfer.IsoPacket[posPacket].Length;
	}

	// http://msdn.microsoft.com/en-us/library/ff539084%28v=vs.85%29.aspx
	// The Length parameter should only update for DeviceToHost (IN) transfers, but
	// this is not always the case.
	// * If the host is updating packet lengths, they are used to calculate
	//   TotalTransferred. (above)
	// * If the host is Not updating the packet lengths, the URB
	//   TransferBufferLength is used to calculate TotalTransferred. (below)
	//
	if (NT_SUCCESS(status) && !requestContext->TotalTransferred)
	{
		requestContext->TotalTransferred += urb->UrbIsochronousTransfer.TransferBufferLength;
	}

	USBE_SUCCESS(NT_SUCCESS(status), "transferred=%d start-frame=%08Xh packet-errors=%d status=%08Xh\n",
	             requestContext->TotalTransferred, IsoContext->StartFrame, IsoContext->ErrorCount, status);

	WdfRequestCompleteWithInformation(Request, status, requestContext->TotalTransferred);
}
