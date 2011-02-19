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

#ifndef __DRV_QUEUE_H__
#define __DRV_QUEUE_H__

#include "drv_private.h"

// Used to detect when a high speed isochronous transfer function is needed.
#define IsHighSpeedDevice(DeviceContextPtr) (DeviceContextPtr->DeviceSpeed >= UsbHighSpeed ? TRUE : FALSE)

//////////////////////////////////////////////////////////////////////////////
// lusbw_queue.c function prototypes.
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
                                    __in libusb_request* libusbRequest,
                                    __in WDF_REQUEST_TYPE requestType)
{
	NTSTATUS status;
	requestContext->PipeContext = GetPipeContextByID(deviceContext, (UCHAR)libusbRequest->endpoint.endpoint);
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
