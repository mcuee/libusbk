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
EVT_WDF_IO_QUEUE_IO_STOP Queue_OnDefaultStop;
EVT_WDF_IO_QUEUE_IO_RESUME Queue_OnDefaultResume;

EVT_WDF_IO_QUEUE_IO_READ PipeQueue_OnRead;
EVT_WDF_IO_QUEUE_IO_WRITE PipeQueue_OnWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL PipeQueue_OnIoControl;

FORCEINLINE VOID ForwardToQueue(__in WDFREQUEST Request,
                                __in PREQUEST_CONTEXT requestContext)

{
	ULONG                   packetSize;
	NTSTATUS                status = STATUS_SUCCESS;

	packetSize = requestContext->PipeContext->PipeInformation.MaximumPacketSize;
	if(packetSize == 0)
	{
		USBERR("MaximumPacketSize=0\n");
		status = STATUS_INVALID_DEVICE_STATE;
		goto Exit;
	}

	//
	// check that the transfer length is an interval of wMaxPacketSize.
	//
	if(requestContext->Length % packetSize)
	{
		if (requestContext->PipeContext->PipeInformation.PipeType == WdfUsbPipeTypeControl)
		{
			// no restrictions..
		}
		if (requestContext->PipeContext->PipeInformation.PipeType == WdfUsbPipeTypeIsochronous)
		{
			USBERR("isochronous transfers require the transfer length be an interval of wMaxPacketSize. %d %% %d = %d\n",
			       requestContext->Length, packetSize, requestContext->Length % packetSize);
			status = STATUS_INVALID_PARAMETER;
			goto Exit;

		}
		else if (USB_ENDPOINT_DIRECTION_IN(GetRequestPipeID(requestContext)))
		{
			// This is generally not a good thing, but still allowed.
			USBWRN("total transfer length is not an interval of wMaxPacketSize.%d %% %d = %d\n",
			       requestContext->Length, packetSize, requestContext->Length % packetSize);
		}
	}

	//
	// forward the requests to their own queues.
	//
	status = WdfRequestForwardToIoQueue(Request, requestContext->PipeContext->Queue);
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
	requestContext->PipeContext = GetPipeContextByID(deviceContext, PipeID);
	VALIDATE_REQUEST_CONTEXT(requestContext, status);
	if (!NT_SUCCESS(status))
	{
		WdfRequestCompleteWithInformation(Request, status, 0);
		return;
	}

	if (!(requestContext->PipeContext->PipeInformation.EndpointAddress & 0xF))
	{
		USBERR("invalid pipe type\n");
		status = STATUS_INVALID_PIPE_STATE;
		WdfRequestCompleteWithInformation(Request, status, 0);
		return;
	}

	requestContext->RequestType = requestType;
	ForwardToQueue(Request, requestContext);
	return;
}

#endif
