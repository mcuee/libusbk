/*!********************************************************************
libusbK - WDF USB driver.
Copyright (C) 2012 Travis Lee Robinson. All Rights Reserved.
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

//! Standard USB descriptor types. For more information, see section 9-5 of the USB 3.0 specifications.
typedef enum _USB_DESCRIPTOR_TYPE
{
    //! Device descriptor type.
    USB_DESCRIPTOR_TYPE_DEVICE = 0x01,

    //! Configuration descriptor type.
    USB_DESCRIPTOR_TYPE_CONFIGURATION = 0x02,

    //! String descriptor type.
    USB_DESCRIPTOR_TYPE_STRING = 0x03,

    //! Interface descriptor type.
    USB_DESCRIPTOR_TYPE_INTERFACE = 0x04,

    //! Endpoint descriptor type.
    USB_DESCRIPTOR_TYPE_ENDPOINT = 0x05,

    //! Device qualifier descriptor type.
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER = 0x06,

    //! Config power descriptor type.
    USB_DESCRIPTOR_TYPE_CONFIG_POWER = 0x07,

    //! Interface power descriptor type.
    USB_DESCRIPTOR_TYPE_INTERFACE_POWER = 0x08,

    //! Interface association descriptor type.
    USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION = 0x0B,
} USB_DESCRIPTOR_TYPE;

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
		// UCHAR descriptorIndex = setupPacket->Packet.wValue.Bytes.LowByte;
		ULONG descriptorSize = 0;
		PVOID descriptorIn = NULL;
		PVOID outputBuffer = NULL;
		size_t outputBufferLength = 0;

		if (requestContext->IoControlCode == LIBUSB_IOCTL_GET_DESCRIPTOR)
		{
			switch(descriptorType)
			{
			case USB_DESCRIPTOR_TYPE_DEVICE:
				descriptorSize = sizeof(deviceContext->UsbDeviceDescriptor);
				descriptorIn = &deviceContext->UsbDeviceDescriptor;
				break;
			case USB_DESCRIPTOR_TYPE_CONFIGURATION:
				if (setupPacket->Packet.wValue.Bytes.LowByte == 0)
				{
					descriptorSize = deviceContext->ConfigurationDescriptorSize;
					descriptorIn = deviceContext->UsbConfigurationDescriptor;
				}
				else
				{
					// we only support the one for now. ;)
					WdfRequestCompleteWithInformation(Request, STATUS_NO_MORE_ENTRIES, 0);
					return;
				}
				break;
			}

			if (descriptorIn && descriptorSize)
			{
				// handle (or fail) this standard request here.
				status = WdfRequestRetrieveOutputBuffer(Request, 2, &outputBuffer, &outputBufferLength);
				if (NT_SUCCESS(status))
				{
					descriptorSize = (ULONG)min(descriptorSize, outputBufferLength);
					RtlCopyMemory(outputBuffer, descriptorIn, descriptorSize);
					WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, descriptorSize);
					return;
				}
				USBERR("WdfRequestRetrieveOutputBuffer failed. status=%Xh\n", status);
				WdfRequestCompleteWithInformation(Request, status, 0);
				return;
			}
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
