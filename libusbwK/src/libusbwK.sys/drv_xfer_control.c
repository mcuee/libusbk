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

#define IF_IS_REQUEST_CONTEXT_INVALID() if ( \
	!deviceContext               || \
	!requestContext              || \
	!requestContext->PipeContext || \
	!requestContext->PipeContext->IsValid)

EVT_WDF_REQUEST_COMPLETION_ROUTINE XferCtrlComplete;

#if (defined(ALLOC_PRAGMA) && defined(PAGING_ENABLED))

#endif

VOID XferCtrl (
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request,
    __in size_t InputBufferLength,
    __in size_t OutputBufferLength)
{
	NTSTATUS                status;
	PDEVICE_CONTEXT         deviceContext;
	PREQUEST_CONTEXT        requestContext;
	WDFMEMORY				transferMemory;
	PWDF_USB_CONTROL_SETUP_PACKET setupPacket;
	WDF_REQUEST_SEND_OPTIONS sendOptions;
	WDFMEMORY_OFFSET		_transferOffset;
	PWDFMEMORY_OFFSET		transferOffset = &_transferOffset;


	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	deviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
	requestContext = GetRequestContext(Request);

	IF_IS_REQUEST_CONTEXT_INVALID()
	{
		USBERR("invalid device or request context\n");
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	setupPacket = (PWDF_USB_CONTROL_SETUP_PACKET)&requestContext->IoControlRequest.control;
	USBDBG("bmDir=%s bmType=%s bmRecipient=%s bmReserved=%03u bRequest=%u wIndex=%u wValue=%u wLength=%u\n",
	       GetBmRequestDirString(setupPacket->Packet.bm.Request.Dir),
	       GetBmRequestTypeString(setupPacket->Packet.bm.Request.Type),
	       GetBmRequestRecipientString(setupPacket->Packet.bm.Request.Recipient),
	       setupPacket->Packet.bm.Request.Reserved,
	       setupPacket->Packet.bRequest,
	       setupPacket->Packet.wIndex.Value,
	       setupPacket->Packet.wValue.Value,
	       setupPacket->Packet.wLength);

	// If the device and config descriptor requests are not handled they will be the "true"
	// descriptors directly from the device. i.e. if the device has two unassociated interfaces
	// the composite layer will split it into two but each virtual device interface would show
	// both interface deacriptors.
	//
	if (setupPacket->Packet.bm.Request.Dir == BMREQUEST_DEVICE_TO_HOST &&
	        setupPacket->Packet.bm.Request.Type == BMREQUEST_STANDARD &&
	        setupPacket->Packet.bRequest == USB_REQUEST_GET_DESCRIPTOR)
	{
		UCHAR descriptorType = setupPacket->Packet.wValue.Bytes.HiByte;
		ULONG descriptorSize = 0;
		PVOID descriptorIn = NULL;
		PVOID outputBuffer = NULL;
		size_t outputBufferLength = 0;

		if (descriptorType == USB_DEVICE_DESCRIPTOR_TYPE)
		{
			descriptorSize = sizeof(deviceContext->UsbDeviceDescriptor);
			descriptorIn = &deviceContext->UsbDeviceDescriptor;
		}
		else if (descriptorType == USB_CONFIGURATION_DESCRIPTOR_TYPE)
		{
			descriptorSize = deviceContext->ConfigurationDescriptorSize;
			descriptorIn = deviceContext->UsbConfigurationDescriptor;
		}

		if (descriptorIn && descriptorSize)
		{
			// handle (or fail) this standard request here.
			status = WdfRequestRetrieveOutputBuffer(Request, descriptorSize, &outputBuffer, &outputBufferLength);
			if (NT_SUCCESS(status))
			{
				RtlCopyMemory(outputBuffer, descriptorIn, descriptorSize);
				WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, descriptorSize);
				return;
			}
			USBERR("WdfRequestRetrieveOutputBuffer failed. status=%Xh\n", status);
			WdfRequestCompleteWithInformation(Request, status, 0);
			return;
		}
	}

	if (METHOD_FROM_CTL_CODE(requestContext->IoControlCode) == METHOD_BUFFERED &&
	        requestContext->RequestType == WdfRequestTypeWrite)
	{
		// support for some of the legacy LIBUSB_IOCTL codes which place the data input
		// buffer at the end of the libusb_request structure.
		status = WdfRequestRetrieveInputMemory(Request, &transferMemory);
		if (!NT_SUCCESS(status))
		{
			USBERR("WdfRequestRetrieveInputMemory failed. status=%Xh\n", status);
			goto Exit;
		}

		if (requestContext->Length < sizeof(libusb_request))
		{
			// this can never happen because the input buffer length is checked for
			// this by the default IoControl event.
			status = STATUS_BUFFER_TOO_SMALL;
			USBERR("input buffer length is less than sizeof(libusb_request) status=%Xh\n", status);
			goto Exit;
		}

		transferOffset->BufferOffset = sizeof(libusb_request);
		transferOffset->BufferLength = requestContext->Length - sizeof(libusb_request);

		if (transferOffset->BufferLength == 0)
		{
			// this is okay but no input data means transferOffset->BufferOffset is pointing
			// to invalid memory; because the length is also zero it is still most likely safe.
			transferOffset = NULL;
			transferMemory = NULL;
		}
	}
	else
	{
		// native control transfers are direct; data comes from/goes to the out buffer whether reading or writing.
		transferOffset = NULL;
		status = WdfRequestRetrieveOutputMemory(Request, &transferMemory);
		if (!NT_SUCCESS(status) && status != STATUS_BUFFER_TOO_SMALL)
		{
			USBERR("WdfRequestRetrieveOutputMemory failed. status=%Xh\n", status);
			goto Exit;
		}

		if (status == STATUS_BUFFER_TOO_SMALL)
		{
			// zero length transfer buffer, this is okay.
			transferMemory = NULL;
			USBMSG("zero-length transfer buffer\n");
		}
	}



	status = WdfUsbTargetDeviceFormatRequestForControlTransfer(
	             deviceContext->WdfUsbTargetDevice,
	             Request,
	             setupPacket,
	             transferMemory,
	             transferOffset);

	if (!NT_SUCCESS(status))
	{
		USBERR("WdfUsbTargetDeviceFormatRequestForControlTransfer failed. status=%Xh\n", status);
		goto Exit;
	}

	WdfRequestSetCompletionRoutine(Request,
	                               XferCtrlComplete,
	                               NULL);

	WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, 0);
	status = SetRequestTimeout(requestContext, Request, &sendOptions);
	if (!NT_SUCCESS(status))
	{
		USBERR("SetRequestTimeout failed. status=%Xh\n", status);
		goto Exit;
	}

	if (!WdfRequestSend(Request,
	                    WdfUsbTargetDeviceGetIoTarget(deviceContext->WdfUsbTargetDevice),
	                    &sendOptions))
	{
		status = WdfRequestGetStatus(Request);
		USBERR("WdfRequestSend failed. status=%Xh\n", status);
	}
	else
	{
		USBE_OK("status=%Xh\n", status);
		return;
	}


Exit:
	if (!NT_SUCCESS(status))
	{
		WdfRequestCompleteWithInformation(Request, status, 0);
	}

	return;
}

VOID XferCtrlComplete(__in WDFREQUEST Request,
                      __in WDFIOTARGET Target,
                      __in PWDF_REQUEST_COMPLETION_PARAMS Params,
                      __in WDFCONTEXT Context)
{
	NTSTATUS status = Params->IoStatus.Status;
	PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams = Params->Parameters.Usb.Completion;
	ULONG length = usbCompletionParams->Parameters.DeviceControlTransfer.Length;

	UNREFERENCED_PARAMETER(Target);
	UNREFERENCED_PARAMETER(Context);

	if (NT_SUCCESS(status))
	{
		USBE_OK("transferred=%u\n", length);
		WdfRequestCompleteWithInformation(Request, status, length);

	}
	else
	{
		USBERR("status=%Xh\n", status);
		WdfRequestComplete(Request, status);
	}
}
