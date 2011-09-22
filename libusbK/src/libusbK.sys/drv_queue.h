/*! \file drv_queue.h
*/

#ifndef __DRV_QUEUE_H__
#define __DRV_QUEUE_H__

#include "drv_private.h"

// Used to detect when a high speed isochronous transfer function is needed.
#define IsHighSpeedDevice(DeviceContextPtr) (DeviceContextPtr->DeviceSpeed >= UsbHighSpeed ? TRUE : FALSE)

#define FUNCTION_FROM_CTL_CODE(ctrlCode) (((ULONG)(ctrlCode & 0x3FFC)) >> 2)

//////////////////////////////////////////////////////////////////////////////
// lusbk_queue.c function prototypes.
// Default queue events.
//
EVT_WDF_IO_QUEUE_IO_READ DefaultQueue_OnRead;
EVT_WDF_IO_QUEUE_IO_WRITE DefaultQueue_OnWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL DefaultQueue_OnIoControl;

EVT_WDF_IO_QUEUE_IO_STOP Queue_OnIsoStop;
EVT_WDF_IO_QUEUE_IO_STOP Queue_OnReadBulkStop;
#define Queue_OnReadBulkRawStop Queue_OnIsoStop

EVT_WDF_IO_QUEUE_IO_READ PipeQueue_OnRead;
EVT_WDF_IO_QUEUE_IO_WRITE PipeQueue_OnWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL PipeQueue_OnIoControl;

FORCEINLINE VOID ForwardToQueue(__in WDFREQUEST Request,
                                __in PREQUEST_CONTEXT requestContext)

{
	ULONG                   packetSize;
	NTSTATUS                status = STATUS_SUCCESS;

	packetSize = requestContext->QueueContext->Info.MaximumPacketSize;

	// forward the requests to their own queues.
	//
	status = WdfRequestForwardToIoQueue(Request, WdfObjectContextGetObject(requestContext->QueueContext));
	if(!NT_SUCCESS(status))
	{
		USBERR("WdfRequestForwardToIoQueue failed. status=%Xh\n", status);
		goto Exit;
	}
	else
	{
		return;
	}

Exit:
	WdfRequestCompleteWithInformation(Request, status, 0);
	return;
}

FORCEINLINE VOID ForwardToPipeQueue(__in WDFREQUEST Request,
                                    __in PDEVICE_CONTEXT deviceContext,
                                    __in PREQUEST_CONTEXT requestContext,
                                    __in UCHAR PipeID,
                                    __in WDF_REQUEST_TYPE requestType)
{

	NTSTATUS status;
	WDFQUEUE pipeQueue;

	if ((pipeQueue = GetPipeContextByID(deviceContext, PipeID)->Queue) == NULL)
	{
		status = STATUS_INVALID_DEVICE_STATE;
		USBERR("Invalid pipe queue.\n");
		goto Exit;
	}

	requestContext->QueueContext = GetQueueContext(pipeQueue);
	requestContext->RequestType = requestType;

	status = WdfRequestForwardToIoQueue(Request, pipeQueue);
	if(!NT_SUCCESS(status))
	{
		USBERR("WdfRequestForwardToIoQueue failed. status=%Xh\n", status);
		goto Exit;
	}

	return;
Exit:
	WdfRequestCompleteWithInformation(Request, status, 0);
	return;

}

#endif
