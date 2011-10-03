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

PINTERFACE_CONTEXT Interface_GetContextByNumber(__in PDEVICE_CONTEXT deviceContext,
        __in UCHAR interfaceNumber)
{
	UCHAR interfaceIndex;
	UCHAR interfaceCount;

	interfaceCount = deviceContext->InterfaceCount;
	for (interfaceIndex = 0; interfaceIndex < interfaceCount; interfaceIndex++)
	{
		if (deviceContext->InterfaceContext[interfaceIndex].InterfaceDescriptor.bInterfaceNumber == interfaceNumber)
		{
			return &deviceContext->InterfaceContext[interfaceIndex];
		}
	}
	return NULL;
}

PINTERFACE_CONTEXT Interface_GetContextByIndex(__in PDEVICE_CONTEXT deviceContext,
        __in UCHAR interfaceIndex)
{

	if (interfaceIndex >= deviceContext->InterfaceCount) return NULL;
	return &deviceContext->InterfaceContext[interfaceIndex];
}

NTSTATUS Interface_Stop(__in PDEVICE_CONTEXT deviceContext, __in PINTERFACE_CONTEXT interfaceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	UCHAR pipeIndex;
	PPIPE_CONTEXT pipeContext;

	UNREFERENCED_PARAMETER(deviceContext);

	// mark pipe contexts as InValid
	// stop all of the pipes and purge queues
	for (pipeIndex = 0; pipeIndex < interfaceContext->PipeCount; pipeIndex++)
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

NTSTATUS Interface_Start(__in PDEVICE_CONTEXT deviceContext,
                         __in PINTERFACE_CONTEXT interfaceContext)
{
	NTSTATUS					status = STATUS_SUCCESS;
	UCHAR						pipeIndex;

	// start all the pipes on interfaceContext.
	for (pipeIndex = 0; pipeIndex < interfaceContext->PipeCount; pipeIndex++)
		status = Pipe_Start(deviceContext, interfaceContext->PipeContextByIndex[pipeIndex]);

	return STATUS_SUCCESS;
}

NTSTATUS Interface_DeletePipeQueues(__in PINTERFACE_CONTEXT interfaceContext)
{
	UCHAR						pipeIndex;

	// start all the pipes on interfaceContext.
	for (pipeIndex = 0; pipeIndex < interfaceContext->PipeCount; pipeIndex++)
	{
		PPIPE_CONTEXT pipeContext = interfaceContext->PipeContextByIndex[pipeIndex];
		WDFQUEUE queue = pipeContext->Queue;

		if (pipeContext->IsValid == FALSE &&  queue && (pipeContext->PipeInformation.EndpointAddress & 0xF))
		{
			USBDBGN("pipeID=%02Xh Destroying pipe queue.", pipeContext->PipeInformation.EndpointAddress);
			WdfObjectDelete(queue);
			InterlockedExchangePointer(&pipeContext->Queue, (PVOID)NULL);
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS Interface_InitContext(__in PDEVICE_CONTEXT deviceContext,
                               __in PINTERFACE_CONTEXT interfaceContext)
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
		pipeContext = GetPipeContextByID(deviceContext, pipeInfo.EndpointAddress);

		// set the default pipe polices
		if (!pipeContext->Pipe)
		{
			Policy_InitPipe(deviceContext, pipeContext);
		}

		// set the pipe context by index in the interface context
		interfaceContext->PipeContextByIndex[pipeIndex] = pipeContext;

		// always update the pipe handle
		pipeContext->Pipe = pipe;

		pipeInfo.MaximumTransferSize = Pipe_CalcMaxTransferSize(IsHighSpeedDevice(deviceContext), pipeInfo.PipeType, pipeInfo.MaximumPacketSize, pipeInfo.MaximumTransferSize);

		// update the pipe information
		RtlCopyMemory(&pipeContext->PipeInformation, &pipeInfo, sizeof(WDF_USB_PIPE_INFORMATION));

		USBDBG("configured %s pipe: PipeID=%02Xh MaximumPacketSize=%u MaximumTransferSize=%u PipeType=%s\n",
		       GetEndpointDirString(pipeInfo.EndpointAddress), pipeInfo.EndpointAddress, pipeInfo.MaximumPacketSize, pipeInfo.MaximumTransferSize, GetPipeTypeString(pipeInfo.PipeType));
	}

	return status;
}

NTSTATUS Interface_SetAltSetting(__in  PDEVICE_CONTEXT deviceContext,
                                 __in  PREQUEST_CONTEXT requestContext,
                                 __out PINTERFACE_CONTEXT* interfaceContext)
{
	NTSTATUS				status;
	WDF_OBJECT_ATTRIBUTES	pipesAttributes;
	WDF_USB_INTERFACE_SELECT_SETTING_PARAMS  selectSettingParams;
	USB_INTERFACE_DESCRIPTOR  interfaceDescriptor;
	UCHAR altSettingCount;
	UCHAR altsetting_index = BYTE_MAX;

	status = GetInterfaceContextFromRequest(deviceContext, requestContext, interfaceContext);
	if (!NT_SUCCESS(status))
	{
		USBERR("GetInterfaceContextFromRequest failed. status=%Xh\n", status);
		goto Done;
	}

	status = GetInterfaceAltSettingIndexFromRequest(requestContext, (*interfaceContext), &altsetting_index, &altSettingCount, &interfaceDescriptor);
	if (!NT_SUCCESS(status))
	{
		USBERR("GetInterfaceAltSettingIndexFromRequest failed. status=%Xh\n", status);
		goto Done;
	}

	if ((*interfaceContext)->SettingIndex == altsetting_index)
	{
		USBMSG("alternate interface index %u already selected\n",
		       requestContext->IoControlRequest.intf.altsetting_number);

		status = STATUS_SUCCESS;
		goto Done;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&pipesAttributes);
	WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&selectSettingParams, altsetting_index);

	status = Interface_Stop(deviceContext, (*interfaceContext));
	if (!NT_SUCCESS(status))
	{
		USBERR("Interface_Stop failed. status=%Xh", status);
		goto Done;
	}
	status = WdfUsbInterfaceSelectSetting((*interfaceContext)->Interface, &pipesAttributes, &selectSettingParams);
	if (!NT_SUCCESS(status))
	{
		USBERR("unable to set alt setting index %u on interface number %u\n", altsetting_index,
		       (*interfaceContext)->InterfaceDescriptor.bInterfaceNumber);

		if (!NT_SUCCESS(Interface_Start(deviceContext, (*interfaceContext))), FALSE)
		{
			USBERR("Interface_Start failed. status=%Xh", status);
		}
	}
	else
	{
		USBMSG("selected alt setting index %u on interface number %u\n", altsetting_index,
		       (*interfaceContext)->InterfaceDescriptor.bInterfaceNumber);

		(*interfaceContext)->SettingIndex = WdfUsbInterfaceGetConfiguredSettingIndex((*interfaceContext)->Interface);

		status = Interface_DeletePipeQueues((*interfaceContext));
		if (!NT_SUCCESS(status))
		{
			USBERR("Interface_DeletePipeQueues failed. status=%Xh", status);
			goto Done;
		}

		status = Interface_InitContext(deviceContext, (*interfaceContext));
		if (!NT_SUCCESS(status))
		{
			USBERR("Interface_InitContext failed. status=%Xh", status);
			goto Done;
		}

		status = Interface_Start(deviceContext, (*interfaceContext));
		if (!NT_SUCCESS(status))
		{
			USBERR("Interface_Start failed. status=%Xh", status);
			goto Done;
		}
	}

Done:
	return status;
}

NTSTATUS Interface_GetAltSetting(
    __in PDEVICE_CONTEXT deviceContext,
    __in PREQUEST_CONTEXT requestContext,
    __out PINTERFACE_CONTEXT* interfaceContext)
{
	NTSTATUS				status = STATUS_SUCCESS;

	status = GetInterfaceContextFromRequest(deviceContext, requestContext, interfaceContext);
	if (!NT_SUCCESS(status))
	{
		USBERR("GetInterfaceContextFromRequest failed. status=%Xh%d\n", status);
		goto Error;
	}

Error:
	return status;

}

// STATUS_OBJECT_NAME_COLLISION = ERROR_ALREADY_EXISTS
// STATUS_INVALID_DEVICE_STATE = ERROR_BAD_COMMAND
NTSTATUS Interface_Claim(__in  PDEVICE_CONTEXT deviceContext,
                         __in  PREQUEST_CONTEXT requestContext,
                         __in  PFILE_OBJECT fileObject,
                         __out PINTERFACE_CONTEXT* interfaceContext)
{
	NTSTATUS status = GetInterfaceContextFromRequest(deviceContext, requestContext, interfaceContext);
	if (!NT_SUCCESS(status))
	{
		USBERR("GetInterfaceContextFromRequest failed. status=%Xh\n", status);
		return status;
	}

	// interface already claimed by this fileObject.
	if ((*interfaceContext)->ClaimedByFileObject == fileObject)
		return STATUS_SUCCESS;

	// interface already claimed but not by this fileObject.
	if ((*interfaceContext)->ClaimedByFileObject)
	{
		USBERR("interface number %u is already claimed\n", (*interfaceContext)->InterfaceDescriptor.bInterfaceNumber);
		return STATUS_OBJECT_NAME_COLLISION;
	}

	// Claim this interface with the specified fileObject.
	(*interfaceContext)->ClaimedByFileObject = fileObject;
	return STATUS_SUCCESS;
}

NTSTATUS Interface_Release(
    __in PDEVICE_CONTEXT deviceContext,
    __in PREQUEST_CONTEXT requestContext,
    __in PFILE_OBJECT fileObject,
    __out PINTERFACE_CONTEXT* interfaceContext)
{
	NTSTATUS status = GetInterfaceContextFromRequest(deviceContext, requestContext, interfaceContext);
	if (!NT_SUCCESS(status))
	{
		USBERR("GetInterfaceContextFromRequest failed. status=%Xh\n", status);
		return status;
	}

	// nobody is claiming this interface.
	if (!(*interfaceContext)->ClaimedByFileObject)
	{
		return STATUS_SUCCESS;
	}

	// cannot release because it doesn't belong to fileObject.
	if ((*interfaceContext)->ClaimedByFileObject != fileObject)
	{
		USBERR("interface number %u is not bound to this file object\n",
		       (*interfaceContext)->InterfaceDescriptor.bInterfaceNumber);
		return STATUS_OBJECT_NAME_COLLISION;
	}

	// fileObject owns it; release.
	(*interfaceContext)->ClaimedByFileObject = NULL;

	return STATUS_SUCCESS;
}

NTSTATUS Interface_ReleaseAll(__in  PDEVICE_CONTEXT deviceContext,
                              __in  PFILE_OBJECT fileObject)
{
	INT i;

	USBMSG("releasing all interfaces bound to file object 0x%x\n", fileObject);

	for (i = 0; i < deviceContext->InterfaceCount; i++)
	{
		if (deviceContext->InterfaceContext[i].ClaimedByFileObject == fileObject)
		{
			deviceContext->InterfaceContext[i].ClaimedByFileObject = NULL;
		}
	}

	return STATUS_SUCCESS;
}

