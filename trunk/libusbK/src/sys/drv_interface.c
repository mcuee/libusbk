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
			Pipe_Stop(pipeContext, WdfIoTargetCancelSentIo, TRUE, FALSE);
		}
	}

	return status;
}

NTSTATUS Interface_Start(__in PDEVICE_CONTEXT deviceContext,
                         __in PINTERFACE_CONTEXT interfaceContext)
{
	NTSTATUS					status = STATUS_SUCCESS;
	UCHAR						pipeIndex;
	PPIPE_CONTEXT pipeContext;

	// start all the pipes on interfaceContext.
	for (pipeIndex = 0; pipeIndex < interfaceContext->PipeCount; pipeIndex++)
	{
		pipeContext = interfaceContext->PipeContextByIndex[pipeIndex];
		if (pipeContext)
		{
			Pipe_Start(deviceContext, pipeContext, FALSE);
		}
	}
	return status;
}

VOID Interface_DeletePipesAndQueues(__in PINTERFACE_CONTEXT interfaceContext)
{
	UCHAR pipeIndex;

	for (pipeIndex = 0; pipeIndex < interfaceContext->PipeCount; pipeIndex++)
	{
		PPIPE_CONTEXT pipeContext = interfaceContext->PipeContextByIndex[pipeIndex];
		
		// this needs to be re-initialized in subsequent code but we null it here
		// because nothing else should be using it at this point.
		interfaceContext->PipeContextByIndex[pipeIndex] = NULL;

		if (!pipeContext)
		{
			// should certainly never happen; indicates corrupted memory
			USBERRN("NULL pipeContext at index %u. Memory may be corrupt!", pipeIndex);
			continue;
		}

		// the context should always be marked invalid at this point
		// delete the queues for all pipes associated with this interface
		if (pipeContext->IsValid == FALSE && 
			pipeContext->Queue != WDF_NO_HANDLE && 
			(pipeContext->PipeInformation.EndpointAddress & 0xF))
		{
			USBDBGN("pipeID=%02Xh Destroying pipe queue.", pipeContext->PipeInformation.EndpointAddress);
			WdfObjectDelete(pipeContext->Queue);
			pipeContext->Queue = WDF_NO_HANDLE;
		}

		// mark the pipe handle as null.  The framework destroys these for us.
		pipeContext->Pipe = WDF_NO_HANDLE;
	}

	// set the pipe count to zero
	interfaceContext->PipeCount = 0;
}

NTSTATUS Interface_InitContext(__in PDEVICE_CONTEXT deviceContext,
                               __in PINTERFACE_CONTEXT interfaceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	UCHAR pipeIndex;

	if (interfaceContext->Interface == WDF_NO_HANDLE)
	{
		// interface indexes are assigned only once during configuration.
		// memory may be corrupt
		// invalid config descriptor?
		// WDF decided not to give us an interface handle for some unknown reason
		USBERR("WdfUsbTargetDeviceGetInterface returned a null interface handle at index %u\n", interfaceContext->InterfaceIndex);
		return STATUS_FILE_CORRUPT_ERROR;
	}

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
		// this is never null
		pipeContext = GetPipeContextByID(deviceContext, pipeInfo.EndpointAddress);

		// set the pipe context by index in the interface context
		interfaceContext->PipeContextByIndex[pipeIndex] = pipeContext;

		// update the pipe information
		// this needs to be done BEFORE calling Policy_InitPipe
		RtlCopyMemory(&pipeContext->PipeInformation, &pipeInfo, sizeof(WDF_USB_PIPE_INFORMATION));

		// set the default pipe polices
		// NOTE: This is done only once for any given endpoint ID
		Policy_InitPipe(deviceContext, pipeContext);

		// always update the pipe handle
		pipeContext->Pipe = pipe;

		pipeInfo.MaximumTransferSize = Pipe_CalcMaxTransferSize(IsHighSpeedDevice(deviceContext), pipeInfo.PipeType, pipeInfo.MaximumPacketSize, pipeInfo.MaximumTransferSize);
		pipeContext->PipeInformation.MaximumTransferSize = pipeInfo.MaximumTransferSize;

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

	if ((*interfaceContext)->Interface == WDF_NO_HANDLE)
	{
		status = STATUS_NO_MORE_ENTRIES;
		USBERR("Interface handle is NULL. status=%Xh\n", status);
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

		if (!NT_SUCCESS(Interface_Start(deviceContext, (*interfaceContext))))
		{
			USBERR("Interface_Start failed. status=%Xh", status);
		}
	}
	else
	{
		USBMSG("selected alt setting index %u on interface number %u\n", altsetting_index,
		       (*interfaceContext)->InterfaceDescriptor.bInterfaceNumber);

		// fetch the setting back from WDF
		(*interfaceContext)->SettingIndex = WdfUsbInterfaceGetConfiguredSettingIndex((*interfaceContext)->Interface);

		// delete the old queues and pipes
		Interface_DeletePipesAndQueues((*interfaceContext));

		// initialize the new queues and pipes
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

