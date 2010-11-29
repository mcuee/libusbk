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

#ifndef _LUSBW_IOCTL_H
#define _LUSBW_IOCTL_H

#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LUsbW_EvtIoDeviceControl)
#endif

#define GET_OUT_BUFFER(MinimumBufferSize, BufferRef, LengthRef, ErrorText) \
	status = WdfRequestRetrieveOutputBuffer(Request, MinimumBufferSize, BufferRef, LengthRef); \
	if(!NT_SUCCESS(status)) \
	{ \
		USBERR("%s WdfRequestRetrieveOutputBuffer failed status=%Xh\n", ErrorText, status); \
		break; \
	}

#define GET_IN_BUFFER(MinimumBufferSize, BufferRef, LengthRef, ErrorText) \
	status = WdfRequestRetrieveInputBuffer(Request, MinimumBufferSize, BufferRef, LengthRef); \
	if(!NT_SUCCESS(status)) \
	{ \
		USBERR("%s WdfRequestRetrieveInputBuffer failed status=%Xh\n", ErrorText, status); \
		break; \
	}

#define GET_OUT_MEMORY(ErrorText) \
	status = WdfRequestRetrieveOutputMemory(Request, &memory); \
	if(!NT_SUCCESS(status)) \
	{ \
		USBERR("%s: WdfRequestRetrieveOutputMemory failed status=%Xh\n", ErrorText, status); \
		break; \
	}

#define GET_IN_MEMORY(ErrorText) \
	status = WdfRequestRetrieveInputMemory(Request, &memory); \
	if(!NT_SUCCESS(status)) \
	{ \
		USBERR("%s: WdfRequestRetrieveInputMemory failed status=%Xh\n", ErrorText, status); \
		break; \
	}

VOID
LUsbW_EvtIoDeviceControl(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t     OutputBufferLength,
    IN size_t     InputBufferLength,
    IN ULONG      IoControlCode
)
/*++

Routine Description:

This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
requests from the system.

Arguments:

Queue - Handle to the framework queue object that is associated
with the I/O request.
Request - Handle to a framework request object.

OutputBufferLength - length of the request's output buffer,
if an output buffer is available.
InputBufferLength - length of the request's input buffer,
if an input buffer is available.

IoControlCode - the driver-defined or system-defined I/O control code
(IOCTL) that is associated with the request.
Return Value:

VOID

--*/
{
	WDFDEVICE				device;
	NTSTATUS				status;
	PDEVICE_CONTEXT			pDevContext;
	PFILE_CONTEXT			pFileContext;
	ULONG					length = 0;
	libusb_request*			request;
	WDF_MEMORY_DESCRIPTOR	memoryDescriptor;
	WDFMEMORY_OFFSET		memoryOffset;
	WDFMEMORY				memory;
	PVOID					OutputBuffer;
	size_t					OutputBufferLen, InputBufferLen;
	PPIPE_CONTEXT			pipeContext;

	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(pFileContext);

	USBMSG("Entered LUsbW_DispatchDevCtrl\n");

	PAGED_CODE();

	//
	// initialize variables
	//
	device = WdfIoQueueGetDevice(Queue);
	pDevContext = GetDeviceContext(device);

	switch(IoControlCode)
	{
	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "interrupt_or_bulk_read");

		pipeContext = &GetContextByPipeID(pDevContext, (UCHAR)request->endpoint.endpoint);
		if (!pipeContext || !pipeContext->IsValid)
		{
			USBERR0("interrupt_or_bulk_read: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		AddRequestToPipeQueue(Queue, Request, (ULONG)OutputBufferLength, pipeContext);
		return;

	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "interrupt_or_bulk_write");

		pipeContext = &GetContextByPipeID(pDevContext, (UCHAR)request->endpoint.endpoint);
		if (!pipeContext || !pipeContext->IsValid)
		{
			USBERR0("interrupt_or_bulk_write: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		AddRequestToPipeQueue(Queue, Request, (ULONG)OutputBufferLength, pipeContext);
		return;

	case LIBUSB_IOCTL_ISOCHRONOUS_READ:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "isochronous_read");

		pipeContext = &GetContextByPipeID(pDevContext, (UCHAR)request->endpoint.endpoint);
		if (!pipeContext || !pipeContext->IsValid)
		{
			USBERR0("isochronous_read: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		AddRequestToPipeQueue(Queue, Request, (ULONG)OutputBufferLength, pipeContext);
		return;

	case LIBUSB_IOCTL_ISOCHRONOUS_WRITE:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "isochronous_write");

		pipeContext = &GetContextByPipeID(pDevContext, (UCHAR)request->endpoint.endpoint);
		if (!pipeContext || !pipeContext->IsValid)
		{
			USBERR0("isochronous_write: invalid pipe context\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		AddRequestToPipeQueue(Queue, Request, (ULONG)OutputBufferLength, pipeContext);
		return;

	case LIBUSB_IOCTL_GET_VERSION:


		GET_OUT_BUFFER(sizeof(libusb_request), &request, &OutputBufferLen, "get_version");

		length = sizeof(libusb_request);

		request->version.major = VERSION_MAJOR;
		request->version.minor = VERSION_MINOR;
		request->version.micro = VERSION_MICRO;
		request->version.nano  = VERSION_NANO;
		request->version.mod_value = 1;

		status = STATUS_SUCCESS;

		break;

	case LIBUSB_IOCTL_GET_DESCRIPTOR:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "get_descriptor");

		GET_OUT_MEMORY("get_descriptor");
		WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&memoryDescriptor, memory, NULL);

		status = SetGetDescriptor(pDevContext,
		                          &memoryDescriptor,
		                          FALSE,
		                          (WDF_USB_BMREQUEST_RECIPIENT)request->descriptor.recipient,
		                          (UCHAR) request->descriptor.type,
		                          (UCHAR) request->descriptor.index,
		                          (USHORT) request->descriptor.language_id,
		                          &length,
		                          request->timeout);

		break;

	case LIBUSB_IOCTL_SET_DESCRIPTOR:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "set_descriptor");
		GET_IN_MEMORY("set_descriptor");

		memoryOffset.BufferOffset = sizeof(libusb_request);
		memoryOffset.BufferLength = InputBufferLen - sizeof(libusb_request);

		WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&memoryDescriptor, memory, &memoryOffset);

		status = SetGetDescriptor(pDevContext,
		                          &memoryDescriptor,
		                          TRUE,
		                          (WDF_USB_BMREQUEST_RECIPIENT)request->descriptor.recipient,
		                          (UCHAR) request->descriptor.type,
		                          (UCHAR) request->descriptor.index,
		                          (USHORT) request->descriptor.language_id,
		                          &length,
		                          request->timeout);

		break;

	case LIBUSB_IOCTL_SET_CONFIGURATION:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "set_configuration");

		if ((pDevContext->UsbConfigurationDescriptor) &&
		        pDevContext->UsbConfigurationDescriptor->bConfigurationValue == request->configuration.configuration)
		{
			status = STATUS_SUCCESS;
		}
		else
		{
			status = STATUS_NOT_IMPLEMENTED;
		}

		break;

	case LIBUSB_IOCTL_GET_CACHED_CONFIGURATION:
	case LIBUSB_IOCTL_GET_CONFIGURATION:

		GET_OUT_BUFFER(sizeof(libusb_request), &request, &OutputBufferLen, "get_configuration");

		if (pDevContext->UsbConfigurationDescriptor)
		{
			*((UCHAR*)&request) = pDevContext->UsbConfigurationDescriptor->bConfigurationValue;
		}
		else
		{
			*((UCHAR*)&request) = 0;
		}
		length = 1;
		status = STATUS_SUCCESS;

		break;

	case LIBUSB_IOCTL_CLAIM_INTERFACE:

		status = STATUS_SUCCESS;
		// status = claim_interface(dev, stack_location->FileObject, request->interface.interface);
		break;

	case LIBUSB_IOCTL_RELEASE_INTERFACE:

		status = STATUS_SUCCESS;
		// status = release_interface(dev, stack_location->FileObject, request->interface.interface);
		break;

	case LIBUSB_IOCTL_SET_INTERFACE:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "set_interface");

		status = SetAltInterface(pDevContext,
		                         (UCHAR)request->interface.interface,
		                         (UCHAR)request->interface.altsetting);

		break;

	case LIBUSB_IOCTL_GET_INTERFACE:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "get_interface");
		GET_OUT_BUFFER(1, &OutputBuffer, &OutputBufferLen, "get_interface");

		status = GetAltInterface(pDevContext, (UCHAR)request->interface.interface, (UCHAR*)OutputBuffer);
		if (NT_SUCCESS(status))
		{
			length = 1;
		}
		break;

	case LIBUSB_IOCTL_SET_FEATURE:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "set_feature");

		status = SetClearFeature(pDevContext,
		                         (WDF_USB_BMREQUEST_RECIPIENT) request->feature.recipient,
		                         (USHORT) request->feature.index,
		                         (USHORT)request->feature.feature,
		                         TRUE,
		                         request->timeout);

		break;

	case LIBUSB_IOCTL_CLEAR_FEATURE:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "clear_feature");

		status = SetClearFeature(pDevContext,
		                         (WDF_USB_BMREQUEST_RECIPIENT) request->feature.recipient,
		                         (USHORT) request->feature.index,
		                         (USHORT)request->feature.feature,
		                         FALSE,
		                         request->timeout);

		break;

	case LIBUSB_IOCTL_VENDOR_READ:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "vendor_read");

		GET_IN_MEMORY("vendor_read");
		WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&memoryDescriptor, memory, NULL);

		status = VendorReadWrite(pDevContext,
		                         &memoryDescriptor,
		                         BmRequestDeviceToHost,
		                         (WDF_USB_BMREQUEST_RECIPIENT)request->vendor.recipient,
		                         (BYTE)request->vendor.request,
		                         (USHORT)request->vendor.value,
		                         (USHORT)request->vendor.index,
		                         request->timeout,
		                         &length);
		break;

	case LIBUSB_IOCTL_VENDOR_WRITE:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "vendor_write");

		GET_OUT_MEMORY("vendor_write");
		memoryOffset.BufferOffset = sizeof(libusb_request);
		memoryOffset.BufferLength = InputBufferLen - sizeof(libusb_request);
		WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&memoryDescriptor, memory, &memoryOffset);

		status = VendorReadWrite(pDevContext,
		                         &memoryDescriptor,
		                         BmRequestHostToDevice,
		                         (WDF_USB_BMREQUEST_RECIPIENT)request->vendor.recipient,
		                         (BYTE)request->vendor.request,
		                         (USHORT)request->vendor.value,
		                         (USHORT)request->vendor.index,
		                         request->timeout,
		                         &length);

		break;

	case LIBUSB_IOCTL_GET_STATUS:

		if (OutputBufferLength < 2)
		{
			USBERR0("get_status: invalid output buffer\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "get_status");

		GET_OUT_MEMORY("get_status");
		WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&memoryDescriptor, memory, NULL);

		status = GetStatus(pDevContext,
		                   &memoryDescriptor,
		                   (WDF_USB_BMREQUEST_RECIPIENT)request->status.recipient,
		                   (USHORT)request->status.index,
		                   request->timeout,
		                   &length);

		break;

	case LIBUSB_IOCTL_RESET_ENDPOINT:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "reset_endpoint");
		status = ResetOrAbortPipe(pDevContext, TRUE, (UCHAR)request->endpoint.endpoint, request->timeout);
		break;

	case LIBUSB_IOCTL_ABORT_ENDPOINT:

		GET_IN_BUFFER(sizeof(libusb_request), &request, &InputBufferLen, "abort_endpoint");
		status = ResetOrAbortPipe(pDevContext, FALSE, (UCHAR)request->endpoint.endpoint, request->timeout);
		break;

	case LIBUSB_IOCTL_RESET_DEVICE:

		status = ResetDevice(device);
		break;
#if 0
	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ:

		dispCtlCode = "INTERRUPT_OR_BULK_READ";

		// check if the request and buffer is valid
		if (!request || !transfer_buffer_mdl || input_buffer_length < sizeof(libusb_request))
		{
			USBERR("%s: invalid transfer request\n",
			       dispCtlCode);
			status = STATUS_INVALID_PARAMETER;
			goto IOCTL_Done;
		}

		// check if the pipe exists and get the pipe information
		TRANSFER_IOCTL_GET_PIPEINFO();

		// must be a bulk or interrupt pipe
		if (!IS_BULK_PIPE(pipe_info) && !IS_INTR_PIPE(pipe_info))
		{
			USBERR("%s: incorrect pipe type: %02Xh\n",
			       dispCtlCode, pipe_info->pipe_type);
			status = STATUS_INVALID_PARAMETER;
			goto IOCTL_Done;
		}

		// read buffer length must be equal to or an interval of the max packet size
		TRANSFER_IOCTL_CHECK_READ_BUFFER();

		urbFunction = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
		usbdDirection = USBD_TRANSFER_DIRECTION_IN;
		maxTransferSize = GetMaxTransferSize(pipe_info, request->endpoint.max_transfer_size);

		// ensure that the urb function and direction we set matches the
		// pipe information
		//
		TRANSFER_IOCTL_CHECK_FUNCTION_AND_DIRECTION();

		// calls the transfer function and returns NTSTATUS
		TRANSFER_IOCTL_EXECUTE();

	case LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE:

		dispCtlCode = "INTERRUPT_OR_BULK_WRITE";

		/* we don't check 'transfer_buffer_mdl' here because it might be NULL */
		/* if the DLL requests to send a zero-length packet */
		if (!request || input_buffer_length < sizeof(libusb_request))
		{
			USBERR("%s: invalid transfer request\n", dispCtlCode);
			status = STATUS_INVALID_PARAMETER;
			goto IOCTL_Done;
		}

		// check if the pipe exists and get the pipe information
		TRANSFER_IOCTL_GET_PIPEINFO();

		// must be a bulk or interrupt pipe
		if (!IS_BULK_PIPE(pipe_info) && !IS_INTR_PIPE(pipe_info))
		{
			USBERR("%s: incorrect pipe type: %02Xh\n",
			       dispCtlCode, pipe_info->pipe_type);
			status = STATUS_INVALID_PARAMETER;
			goto IOCTL_Done;
		}

		urbFunction = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
		usbdDirection = USBD_TRANSFER_DIRECTION_OUT;
		maxTransferSize = GetMaxTransferSize(pipe_info, request->endpoint.max_transfer_size);

		// ensure that the urb function and direction we set matches the
		// pipe information
		//
		TRANSFER_IOCTL_CHECK_FUNCTION_AND_DIRECTION();

		// calls the transfer function and returns NTSTATUS
		TRANSFER_IOCTL_EXECUTE();

	case LIBUSB_IOCTL_ISOCHRONOUS_READ:

		dispCtlCode = "ISOCHRONOUS_READ";

		// check if the request and buffer is valid
		if (!request || !transfer_buffer_mdl || input_buffer_length < sizeof(libusb_request))
		{
			USBERR("%s: invalid transfer request\n", dispCtlCode);
			status = STATUS_INVALID_PARAMETER;
			goto IOCTL_Done;
		}

		// check if the pipe exists and get the pipe information
		TRANSFER_IOCTL_GET_PIPEINFO();

		// read buffer length must be equal to or an interval of the max packet size
		TRANSFER_IOCTL_CHECK_READ_BUFFER();

		// must be an isochronous endpoint
		if (!IS_ISOC_PIPE(pipe_info))
		{
			USBERR("%s: incorrect pipe type: %02Xh\n",
			       dispCtlCode, pipe_info->pipe_type);
			status = STATUS_INVALID_PARAMETER;
			goto IOCTL_Done;
		}

		urbFunction = URB_FUNCTION_ISOCH_TRANSFER;
		usbdDirection = USBD_TRANSFER_DIRECTION_IN;
		maxTransferSize = GetMaxTransferSize(pipe_info, request->endpoint.max_transfer_size);

		// ensure that the urb function and direction we set matches the
		// pipe information
		//
		TRANSFER_IOCTL_CHECK_FUNCTION_AND_DIRECTION();

		// calls the transfer function and returns NTSTATUS
		TRANSFER_IOCTL_EXECUTE();

	case LIBUSB_IOCTL_ISOCHRONOUS_WRITE:

		dispCtlCode = "ISOCHRONOUS_WRITE";

		// check if the request and buffer is valid
		if (!transfer_buffer_mdl || !request || input_buffer_length < sizeof(libusb_request))
		{
			USBERR("%s: invalid transfer request\n", dispCtlCode);
			status = STATUS_INVALID_PARAMETER;
			goto IOCTL_Done;
		}

		// check if the pipe exists and get the pipe information
		TRANSFER_IOCTL_GET_PIPEINFO();

		// must be an isochronous endpoint
		if (!IS_ISOC_PIPE(pipe_info))
		{
			USBERR("%s: incorrect pipe type: %02Xh\n",
			       dispCtlCode, pipe_info->pipe_type);
			status = STATUS_INVALID_PARAMETER;
			goto IOCTL_Done;
		}

		urbFunction = URB_FUNCTION_ISOCH_TRANSFER;
		usbdDirection = USBD_TRANSFER_DIRECTION_OUT;
		TRANSFER_IOCTL_CHECK_FUNCTION_AND_DIRECTION();

		maxTransferSize = GetMaxTransferSize(pipe_info, request->endpoint.max_transfer_size);

		// ensure that the urb function and direction we set matches the
		// pipe information
		//
		TRANSFER_IOCTL_CHECK_FUNCTION_AND_DIRECTION();

		TRANSFER_IOCTL_EXECUTE();
	}

	///////////////////////////////////
	// METHOD_BUFFERED control codes //
	///////////////////////////////////
	if (!request || input_buffer_length < sizeof(libusb_request)
	        || input_buffer_length > LIBUSB_MAX_READ_WRITE
	        || output_buffer_length > LIBUSB_MAX_READ_WRITE
	        || transfer_buffer_length > LIBUSB_MAX_READ_WRITE)
	{
		USBERR0("invalid input or output buffer\n");

		status = complete_irp(irp, STATUS_INVALID_PARAMETER, 0);
		remove_lock_release(dev);
		return status;
	}
#endif

#if 0
	switch(control_code)
	{



	case LIBUSB_IOCTL_GET_STATUS:

		if (!output_buffer || output_buffer_length < 2)
		{
			USBERR0("get_status: invalid output buffer\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		status = get_status(dev, request->status.recipient,
		                    request->status.index, output_buffer,
		                    &ret, request->timeout);

		break;

	case LIBUSB_IOCTL_SET_DESCRIPTOR:

		if (input_buffer_length <= sizeof(libusb_request))
		{
			USBERR0("set_descriptor: invalid input buffer\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		status = set_descriptor(dev,
		                        input_buffer + sizeof(libusb_request),
		                        input_buffer_length - sizeof(libusb_request),
		                        request->descriptor.type,
		                        request->descriptor.recipient,
		                        request->descriptor.index,
		                        request->descriptor.language_id,
		                        &ret, request->timeout);

		break;

	case LIBUSB_IOCTL_VENDOR_READ:

		if (output_buffer_length && !output_buffer)
		{
			USBERR0("vendor_read: invalid output buffer\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		status = vendor_class_request(dev,
		                              request->vendor.type,
		                              request->vendor.recipient,
		                              request->vendor.request,
		                              request->vendor.value,
		                              request->vendor.index,
		                              output_buffer,
		                              output_buffer_length,
		                              USBD_TRANSFER_DIRECTION_IN,
		                              &ret, request->timeout);
		break;

	case LIBUSB_IOCTL_VENDOR_WRITE:

		status =
		    vendor_class_request(dev,
		                         request->vendor.type,
		                         request->vendor.recipient,
		                         request->vendor.request,
		                         request->vendor.value,
		                         request->vendor.index,
		                         input_buffer_length == sizeof(libusb_request) ?
		                         NULL : input_buffer + sizeof(libusb_request),
		                         input_buffer_length - sizeof(libusb_request),
		                         USBD_TRANSFER_DIRECTION_OUT,
		                         &ret, request->timeout);
		break;

	case LIBUSB_IOCTL_RESET_ENDPOINT:

		status = reset_endpoint(dev, request->endpoint.endpoint,
		                        request->timeout);
		break;

	case LIBUSB_IOCTL_ABORT_ENDPOINT:

		status = abort_endpoint(dev, request->endpoint.endpoint,
		                        request->timeout);
		break;

	case LIBUSB_IOCTL_RESET_DEVICE:

		status = reset_device(dev, request->timeout);
		break;

	case LIBUSB_IOCTL_SET_DEBUG_LEVEL:
		usb_log_set_level(request->debug.level);
		break;

	case LIBUSB_IOCTL_CLAIM_INTERFACE:
		status = claim_interface(dev, stack_location->FileObject,
		                         request->interface.interface);
		break;

	case LIBUSB_IOCTL_RELEASE_INTERFACE:
		status = release_interface(dev, stack_location->FileObject,
		                           request->interface.interface);
		break;

	case LIBUSB_IOCTL_GET_DEVICE_PROPERTY:
		if (!request || output_buffer_length < sizeof(libusb_request))
		{
			USBERR0("get_device_property: invalid output buffer\n");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		status = reg_get_device_property(
		             dev->physical_device_object,
		             request->device_property.property,
		             output_buffer,
		             output_buffer_length, &ret);
		break;

	case LIBUSB_IOCTL_GET_CUSTOM_REG_PROPERTY:
		if (!input_buffer || (input_buffer_length < sizeof(libusb_request)))
		{
			USBERR0("get_custom_reg_property: invalid buffer\n");
			status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		status=reg_get_custom_property(
		           dev->physical_device_object,
		           input_buffer,
		           output_buffer_length,
		           request->device_registry_key.name_offset,
		           &ret);
		break;
#endif

	case IOCTL_USBSAMP_GET_CONFIG_DESCRIPTOR:


		if (pDevContext->UsbConfigurationDescriptor)
		{

			length = pDevContext->UsbConfigurationDescriptor->wTotalLength;

			GET_OUT_BUFFER(length, &OutputBuffer, &OutputBufferLen, "IOCTL_USBSAMP_GET_CONFIG_DESCRIPTOR");

			RtlCopyMemory(OutputBuffer, pDevContext->UsbConfigurationDescriptor, length);

			status = STATUS_SUCCESS;
		}
		else
		{
			status = STATUS_INVALID_DEVICE_STATE;
		}

		break;

	case IOCTL_USBSAMP_RESET_DEVICE:

		status = ResetDevice(device);
		break;

	default :
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	WdfRequestCompleteWithInformation(Request, status, length);

	return;
}

#endif
