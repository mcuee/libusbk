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

#include "private.h"

NTSTATUS StopInterface(IN PDEVICE_CONTEXT deviceContext, IN PINTERFACE_CONTEXT interfaceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	UCHAR pipeIndex;
	PPIPE_CONTEXT pipeContext;

	UNREFERENCED_PARAMETER(deviceContext);

	// mark pipe contexts as InValid
	// stop all of the pipes and purge queues
	for (pipeIndex=0; pipeIndex < interfaceContext->PipeCount; pipeIndex++)
	{
		pipeContext = interfaceContext->PipeContextByIndex[pipeIndex];
		if (pipeContext)
		{
			pipeContext->IsValid = FALSE;
			if (pipeContext->Queue)
			{
				// stop queue, cancel any outstanding requests
				WdfIoQueuePurgeSynchronously(pipeContext->Queue);
			}

			if (pipeContext->Pipe)
			{
				WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(pipeContext->Pipe), WdfIoTargetCancelSentIo);
			}
		}
	}

	return status;
}

NTSTATUS StartInterface(IN  PDEVICE_CONTEXT deviceContext,
                        IN  PINTERFACE_CONTEXT interfaceContext)
{
	NTSTATUS					status;
	UCHAR						pipeIndex;
	PPIPE_CONTEXT				pipeContext;
	WDF_IO_QUEUE_CONFIG			queueConfig;
	WDF_OBJECT_ATTRIBUTES		queueAttributes;
	WDF_IO_QUEUE_DISPATCH_TYPE	queueDispatchType;


	// stop all of the pipes and
	for (pipeIndex=0; pipeIndex < interfaceContext->PipeCount; pipeIndex++)
	{
		pipeContext = interfaceContext->PipeContextByIndex[pipeIndex];
		if (pipeContext)
		{
			if (!pipeContext->Pipe)
			{
				USBERR("Invalid pipe handle. pipeID=%02Xh\n",pipeContext->PipeInformation.EndpointAddress);
				continue;
			}

			// start pipe
			status = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(pipeContext->Pipe));
			if (!NT_SUCCESS(status))
			{
				USBERR("WdfIoTargetStart failed. status=%Xh\n",status);
				continue;
			}

			// if this pipe doesn't have a queue, create it now
			if (!pipeContext->Queue)
			{
				queueDispatchType = WdfIoQueueDispatchInvalid;

				switch(pipeContext->PipeInformation.PipeType)
				{
				case WdfUsbPipeTypeIsochronous:
					queueDispatchType = WdfIoQueueDispatchParallel;
					break;
				case WdfUsbPipeTypeInterrupt:
				case WdfUsbPipeTypeBulk:
//					queueDispatchType = WdfIoQueueDispatchSequential;
					queueDispatchType = WdfIoQueueDispatchParallel;
					break;
				}

				if (queueDispatchType == WdfIoQueueDispatchInvalid)
				{
					USBERR("Invalid queue dispatch type. pipeID=%02Xh\n",pipeContext->PipeInformation.EndpointAddress);
					continue;
				}

				WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,	queueDispatchType);
				WDF_OBJECT_ATTRIBUTES_INIT(&queueAttributes);
				queueAttributes.SynchronizationScope=WdfSynchronizationScopeQueue;

				if (USB_ENDPOINT_DIRECTION_IN(pipeContext->PipeInformation.EndpointAddress))
					queueConfig.EvtIoRead = LUsbW_EvtReadFromPipeQueue;
				else
					queueConfig.EvtIoWrite = LUsbW_EvtWriteFromPipeQueue;

				queueConfig.EvtIoDeviceControl = LUsbW_EvtIoDeviceControlFromPipeQueue;

				status = WdfIoQueueCreate(deviceContext->WdfDevice,
				                          &queueConfig,
				                          &queueAttributes,
				                          &pipeContext->Queue);

			}

			if (NT_SUCCESS(status) && pipeContext->Queue)
			{
				// start queue
				USBMSG("Starting queue. pipeID=%02Xh\n", pipeContext->PipeInformation.EndpointAddress);

				WdfIoQueueStart(pipeContext->Queue);
				pipeContext->IsValid = TRUE;

			}
			else
			{
				USBERR("WdfIoQueueCreate failed. status=%Xh\n", status);
				continue;
			}
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS InitInterfaceContext(IN  PDEVICE_CONTEXT deviceContext,
                              IN  PINTERFACE_CONTEXT interfaceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	UCHAR pipeIndex;

	// get the interface handle
	interfaceContext->Interface = WdfUsbTargetDeviceGetInterface(deviceContext->WdfUsbTargetDevice, interfaceContext->InterfaceIndex);

	// get the configured alt setting index for this inteface
	interfaceContext->SettingIndex = WdfUsbInterfaceGetConfiguredSettingIndex(interfaceContext->Interface);

	// get the interface descriptor
	WdfUsbInterfaceGetDescriptor(
	    interfaceContext->Interface,
	    interfaceContext->SettingIndex,
	    &interfaceContext->InterfaceDescriptor);

	// get the number of configured pipes
	interfaceContext->PipeCount = WdfUsbInterfaceGetNumConfiguredPipes(interfaceContext->Interface);

	// get the pipe handles and information
	for(pipeIndex = 0; pipeIndex < interfaceContext->PipeCount; pipeIndex++)
	{
		WDF_USB_PIPE_INFORMATION pipeInfo;
		WDFUSBPIPE pipe;
		PPIPE_CONTEXT pipeContext;

		// get the pipe handle and information
		WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
		pipe = WdfUsbInterfaceGetConfiguredPipe(interfaceContext->Interface, pipeIndex, &pipeInfo);
		if (!pipe)
		{
			USBERR("WdfUsbInterfaceGetConfiguredPipe returned a null pipe handle at index %u\n", pipeIndex);
			// make sure we can't use this pipe
			interfaceContext->PipeCount = pipeIndex;
			status = STATUS_INSUFFICIENT_RESOURCES;
			return status;
		}

		// get the pipe context by endpoint id from the master pipe list
		pipeContext = &GetContextByPipeID(deviceContext, pipeInfo.EndpointAddress);

		// set the pipe context by index in the interface context
		interfaceContext->PipeContextByIndex[pipeIndex] = pipeContext;

		// always update the pipe handle
		pipeContext->Pipe = pipe;

		// If the endpoint information changed, update the pipe context
		if (RtlCompareMemory(&pipeContext->PipeInformation, &pipeInfo, sizeof(pipeInfo)) != sizeof(pipeInfo))
		{
			USBDBG("Updating pipe. EndpointAddress=%02Xh MaximumPacketSize=%d MaximumTransferSize=%d PipeType=%s\n",
			       pipeInfo.EndpointAddress, pipeInfo.MaximumPacketSize, pipeInfo.MaximumTransferSize, GetPipeTypeString(pipeInfo.PipeType));

			RtlCopyMemory(&pipeContext->PipeInformation, &pipeInfo, sizeof(WDF_USB_PIPE_INFORMATION));
		}
		else
		{
			USBDBG("Found pipe. EndpointAddress=%02Xh MaximumPacketSize=%d MaximumTransferSize=%d PipeType=%s\n",
			       pipeInfo.EndpointAddress, pipeInfo.MaximumPacketSize, pipeInfo.MaximumTransferSize, GetPipeTypeString(pipeInfo.PipeType));
		}

	}

	return status;
}

NTSTATUS SetAltInterface(IN  PDEVICE_CONTEXT deviceContext,
                         IN  UCHAR interfaceNumber,
                         IN  UCHAR altSetting)
{
	NTSTATUS				status;
	PINTERFACE_CONTEXT		pInterfaceContext;
	WDF_OBJECT_ATTRIBUTES	pipesAttributes;
	WDF_USB_INTERFACE_SELECT_SETTING_PARAMS  selectSettingParams;

	pInterfaceContext = GetInterfaceContextByNumber(deviceContext, interfaceNumber);
	if (!pInterfaceContext)
	{
		USBERR("unable to get interface #%u\n", interfaceNumber);
		status = STATUS_INVALID_PARAMETER;
		goto Error;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&pipesAttributes);
	WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&pipesAttributes, PIPE_CONTEXT);
	WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&selectSettingParams, altSetting);

	status = WdfUsbInterfaceSelectSetting(pInterfaceContext->Interface, &pipesAttributes, &selectSettingParams);
	if (!NT_SUCCESS(status))
	{
		USBERR("unable to set alt setting %u on interface %u\n",altSetting, interfaceNumber);
	}
	else
	{
		status = STATUS_SUCCESS;
		USBMSG("selected alt setting %u on interface %u\n",altSetting, interfaceNumber);

		pInterfaceContext->SettingIndex = WdfUsbInterfaceGetConfiguredSettingIndex(pInterfaceContext->Interface);

		status = InitInterfaceContext(deviceContext, pInterfaceContext);
	}

Error:
	return status;
}

NTSTATUS GetAltInterface(IN  PDEVICE_CONTEXT deviceContext,
                         IN  UCHAR interfaceNumber,
                         OUT PUCHAR altSetting)
{
	NTSTATUS				status = STATUS_SUCCESS;
	PINTERFACE_CONTEXT		pInterfaceContext;

	pInterfaceContext = GetInterfaceContextByNumber(deviceContext, interfaceNumber);
	if (!pInterfaceContext)
	{
		USBERR("unable to get alt setting for interface #%u\n", interfaceNumber);
		status = STATUS_INVALID_PARAMETER;
		goto Error;
	}

	*altSetting = pInterfaceContext->SettingIndex;

Error:
	return status;

}

