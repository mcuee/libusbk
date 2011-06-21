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

#if (defined(ALLOC_PRAGMA) && defined(PAGING_ENABLED))
#pragma alloc_text(PAGE, Pipe_AbortAll)
#pragma alloc_text(PAGE, Pipe_Reset)
#pragma alloc_text(PAGE, Pipe_Abort)
#pragma alloc_text(PAGE, Pipe_Start)
#pragma alloc_text(PAGE, Pipe_Stop)
#pragma alloc_text(PAGE, Pipe_StartAll)
#pragma alloc_text(PAGE, Pipe_StopAll)
#pragma alloc_text(PAGE, Pipe_GetContextFromName)
#endif

// The prefix string for pipe file handles.
#define PIPENAME_PREFIX PIPE_

// macros to simplify building the default 'PipeNameToPipeID' array.
#define _DEF_PIPENAME_MAP(Name,PipeID) 	{#Name #PipeID,0x##PipeID,L#Name L#PipeID}
#define  DEF_PIPENAME_MAP(Name,PipeID) _DEF_PIPENAME_MAP(Name,PipeID)

// Structure for mapping a string to a pipe id.
typedef struct _PIPENAME_MAP
{
	LPCSTR Name;		// Display purposes only
	CCHAR PipeID;		// The endpoint address the PIPENAME_MAP is associated with.
	LPCWSTR NameW;		// Used to match the filename passed by the user.
} PIPENAME_MAP, *PPIPENAME_MAP;

// Default libusbK pipe name to pipe id map array.
CONST PIPENAME_MAP PipeNameToPipeID[] =
{
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 01), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 81),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 02), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 82),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 03), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 83),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 04), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 84),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 05), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 85),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 06), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 86),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 07), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 87),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 08), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 88),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 09), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 89),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 0A), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 8A),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 0B), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 8B),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 0C), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 8C),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 0D), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 8D),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 0E), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 8E),
	DEF_PIPENAME_MAP(PIPENAME_PREFIX, 0F), DEF_PIPENAME_MAP(PIPENAME_PREFIX, 8F),
};

NTSTATUS Pipe_Reset(__in PDEVICE_CONTEXT deviceContext,
                    __in UCHAR pipeID)
{
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	PPIPE_CONTEXT pipeContext;

	PAGED_CODE();

	pipeContext = GetPipeContextByID(deviceContext, pipeID);
	if (pipeContext->Pipe)
	{
		USBMSG("pipeID=%02Xh\n", pipeID);

		status = WdfUsbTargetPipeResetSynchronously(pipeContext->Pipe, WDF_NO_HANDLE, NULL);
		if (!NT_SUCCESS(status))
		{
			USBERR("WdfUsbTargetPipeResetSynchronously failed pipeID=%02Xh status=%Xh\n", pipeID, status);
		}
		InterlockedExchange(&pipeContext->TransferCounter, 0);
	}
	else
	{
		USBERR("pipeID=%02Xh not found\n", pipeID);
	}

	return status;
}

NTSTATUS Pipe_Abort(__in PDEVICE_CONTEXT deviceContext,
                    __in UCHAR pipeID)
{
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	PPIPE_CONTEXT pipeContext;

	PAGED_CODE();

	pipeContext = GetPipeContextByID(deviceContext, pipeID);
	if (pipeContext->Pipe)
	{
		USBMSG("pipeID=%02Xh\n", pipeID);

		status = WdfUsbTargetPipeAbortSynchronously(pipeContext->Pipe, WDF_NO_HANDLE, NULL);
		if (!NT_SUCCESS(status))
		{
			USBERR("WdfUsbTargetPipeResetSynchronously failed pipeID=%02Xh status=%Xh\n", pipeID, status);
		}
	}
	else
	{
		USBERR("pipeID=%02Xh not found\n", pipeID);
	}

	return status;
}

NTSTATUS Pipe_Stop(__in PPIPE_CONTEXT pipeContext,
                   __in WDF_IO_TARGET_SENT_IO_ACTION WdfIoTargetSentIoAction)
{
	NTSTATUS status = STATUS_SUCCESS;
	PAGED_CODE();

	if (pipeContext->IsValid == TRUE)
	{
		pipeContext->IsValid = FALSE; // mark context invalid.

		if (pipeContext->Queue)	// stop queue, cancel any outstanding requests
			WdfIoQueuePurgeSynchronously(pipeContext->Queue);

		if (pipeContext->Pipe)
			PipeStop(pipeContext, WdfIoTargetSentIoAction); // stop pipe, cancel any outstanding requests
	}
	else
	{
		USBE_PARAM("pipeContext is not valid!");
		status = STATUS_INVALID_PIPE_STATE;
	}
	return status;
}

// sends an abort pipe request on all open pipes.
NTSTATUS Pipe_AbortAll(__in PDEVICE_CONTEXT deviceContext)
{
	UCHAR              interfaceIndex, pipeIndex;
	UCHAR              interfaceCount, pipeCount;
	NTSTATUS           status;

	PAGED_CODE();

	interfaceCount = deviceContext->InterfaceCount;
	for (interfaceIndex = 0; interfaceIndex < interfaceCount; interfaceIndex++)
	{
		pipeCount = deviceContext->InterfaceContext[interfaceIndex].PipeCount;
		for (pipeIndex = 0; pipeIndex < pipeCount; pipeIndex++)
		{
			USBMSG("interface-number=%u pipe-id=%2Xh",
			       deviceContext->InterfaceContext[interfaceIndex].InterfaceDescriptor.bInterfaceNumber,
			       deviceContext->InterfaceContext[interfaceIndex].PipeContextByIndex[pipeIndex]->PipeInformation.EndpointAddress);

			status = WdfUsbTargetPipeAbortSynchronously(
			             deviceContext->InterfaceContext[interfaceIndex].PipeContextByIndex[pipeIndex]->Pipe,
			             WDF_NO_HANDLE, // WDFREQUEST
			             NULL);//PWDF_REQUEST_SEND_OPTIONS

			if (!NT_SUCCESS(status))
			{
				USBMSG("WdfUsbTargetPipeAbortSynchronously failed %Xh\n", status);
				break;
			}
		}
	}

	return STATUS_SUCCESS;
}

VOID Pipe_StopAll(__in PDEVICE_CONTEXT deviceContext)
{
	PPIPE_CONTEXT pipeContext = NULL;
	UCHAR interfaceIndex, pipeIndex;
	UCHAR interfaceCount, pipeCount;

	PAGED_CODE();

	interfaceCount = deviceContext->InterfaceCount;
	for (interfaceIndex = 0; interfaceIndex < interfaceCount; interfaceIndex++)
	{
		pipeCount = deviceContext->InterfaceContext[interfaceIndex].PipeCount;
		for (pipeIndex = 0; pipeIndex < pipeCount; pipeIndex++)
		{
			pipeContext = deviceContext->InterfaceContext[interfaceIndex].PipeContextByIndex[pipeIndex];
			Pipe_Stop(pipeContext, WdfIoTargetCancelSentIo);
		}
	}
}

//
// PRE REQUIREMENTS:
// * pipeContext->IsValid == FALSE
// * Queue(if any) must be stopped
// * IoTarget must be stopped
//
// On return the pipe and queue are started and the context is marked valid again.
//
NTSTATUS Pipe_InitContext(__in PDEVICE_CONTEXT deviceContext,
                          __in PPIPE_CONTEXT pipeContext)
{
	WDF_IO_QUEUE_DISPATCH_TYPE queueDispatchType = WdfIoQueueDispatchInvalid;
	WDF_IO_QUEUE_CONFIG queueConfig;
	WDF_OBJECT_ATTRIBUTES objectAttributes;
	NTSTATUS status = STATUS_INVALID_HANDLE;

	if (!pipeContext->Pipe && pipeContext->PipeInformation.PipeType != WdfUsbPipeTypeControl)
	{
		USBERR("pipeID=%02Xh invalid pipe handle\n", pipeContext->PipeInformation.EndpointAddress);
		goto Done;
	}

	// A Queue is created only once for each pipe.
	if (!pipeContext->Queue)
	{
		WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);

		// TODO: It would be good to add options for sequential/parallel queueing to the inf.
		// This could be a huge benefit to some for very little effort.
		//
		switch(pipeContext->PipeInformation.PipeType)
		{
		case WdfUsbPipeTypeIsochronous:
			objectAttributes.SynchronizationScope = WdfSynchronizationScopeQueue;
			queueDispatchType = WdfIoQueueDispatchParallel;
			break;
		case WdfUsbPipeTypeInterrupt:
			objectAttributes.SynchronizationScope = WdfSynchronizationScopeQueue;
			queueDispatchType = WdfIoQueueDispatchParallel;
			break;
		case WdfUsbPipeTypeBulk:
			objectAttributes.SynchronizationScope = WdfSynchronizationScopeQueue;
			queueDispatchType = WdfIoQueueDispatchParallel;
			break;
		case WdfUsbPipeTypeControl:
			objectAttributes.SynchronizationScope = WdfSynchronizationScopeQueue;
			queueDispatchType = WdfIoQueueDispatchSequential;
			break;
		}

		if (queueDispatchType != WdfIoQueueDispatchInvalid)
		{
			WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,	queueDispatchType);

			if (pipeContext->PipeInformation.EndpointAddress & 0xF)
			{
				if (USB_ENDPOINT_DIRECTION_IN(pipeContext->PipeInformation.EndpointAddress))
					queueConfig.EvtIoRead = PipeQueue_OnRead;
				else
					queueConfig.EvtIoWrite = PipeQueue_OnWrite;
			}
			queueConfig.EvtIoDeviceControl = PipeQueue_OnIoControl;

			status = WdfIoQueueCreate(deviceContext->WdfDevice,
			                          &queueConfig,
			                          &objectAttributes,
			                          &pipeContext->Queue);
			if (!NT_SUCCESS(status))
			{
				USBERR("WdfIoQueueCreate failed. pipeID=%02Xh status=%Xh\n", pipeContext->PipeInformation.EndpointAddress, status);
			}
		}

	}
	else
	{
		// Queue is already created.
		status = STATUS_SUCCESS;
	}
	if (!pipeContext->Queue && NT_SUCCESS(status))
		status = STATUS_INVALID_PIPE_STATE;

	if (NT_SUCCESS(status))
	{
		if (pipeContext->PipeInformation.PipeType != WdfUsbPipeTypeControl)
		{
			// start pipe
			USBDBG("pipeID=%02Xh starting..\n", pipeContext->PipeInformation.EndpointAddress);
			status = PipeStart(pipeContext);
			if (!NT_SUCCESS(status))
			{
				WdfObjectDelete(pipeContext->Queue);
				pipeContext->Queue = NULL;
				pipeContext->IsValid = FALSE;
				USBERR("WdfIoTargetStart failed. status=%Xh\n", status);
				goto Done;
			}
		}

		if (pipeContext->PipeLock == WDF_NO_HANDLE)
		{
			// create pipe transfer wait lock
			WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
			objectAttributes.ParentObject = deviceContext->WdfDevice;
			status = WdfWaitLockCreate(&objectAttributes, &pipeContext->PipeLock);
			if (!NT_SUCCESS(status))
			{
				if (pipeContext->PipeInformation.PipeType != WdfUsbPipeTypeControl)
					PipeStop(pipeContext, WdfIoTargetCancelSentIo);

				WdfObjectDelete(pipeContext->Queue);
				pipeContext->Queue = NULL;
				pipeContext->IsValid = FALSE;
				USBERR("WdfWaitLockCreate failed. status=%Xh\n", status);
				goto Done;
			}
		}

		// start queue
		USBDBG("pipeID=%02Xh queue starting..\n", pipeContext->PipeInformation.EndpointAddress);
		WdfIoQueueStart(pipeContext->Queue);

		pipeContext->IsValid = TRUE;

	}
	else
	{
		USBERR("WdfIoQueueCreate failed. status=%Xh\n", status);
		pipeContext->IsValid = FALSE;
		goto Done;
	}

Done:
	return status;
}

NTSTATUS Pipe_Start(__in PDEVICE_CONTEXT deviceContext,
                    __in PPIPE_CONTEXT pipeContext)
{
	NTSTATUS status = STATUS_INVALID_PIPE_STATE;

	PAGED_CODE();

	if (!pipeContext)
		return status;

	if (pipeContext->IsValid == FALSE)
	{
		status = Pipe_InitContext(deviceContext, pipeContext);
		if (!NT_SUCCESS(status))
		{
			USBERR("pipeID=%02Xh failed initializing pipe\n",
			       pipeContext->PipeInformation.EndpointAddress);
		}
	}
	else
		USBE_PARAM("pipeContext already valid!");

	return status;
}

VOID Pipe_StartAll(__in PDEVICE_CONTEXT deviceContext)
{
	PPIPE_CONTEXT pipeContext = NULL;
	UCHAR interfaceIndex, pipeIndex;
	UCHAR interfaceCount, pipeCount;
	NTSTATUS status;

	PAGED_CODE();

	interfaceCount = deviceContext->InterfaceCount;
	for (interfaceIndex = 0; interfaceIndex < interfaceCount; interfaceIndex++)
	{
		pipeCount = deviceContext->InterfaceContext[interfaceIndex].PipeCount;
		for (pipeIndex = 0; pipeIndex < pipeCount; pipeIndex++)
		{
			pipeContext = deviceContext->InterfaceContext[interfaceIndex].PipeContextByIndex[pipeIndex];
			status = Pipe_Start(deviceContext, pipeContext);
		}
	}
}


// This routine will pass the string pipe name and fetch the pipe handle.
PPIPE_CONTEXT Pipe_GetContextFromName(
    __in PDEVICE_CONTEXT DeviceContext,
    __in PUNICODE_STRING FileName)
{
	INT					nameLength, index;
	PPIPE_CONTEXT		pipeContext = NULL;
	PAGED_CODE();

	nameLength = (INT)(FileName->Length / sizeof(WCHAR));
	for (index = 0; index < sizeof(PipeNameToPipeID) / sizeof(PipeNameToPipeID[0]); index++)
	{
		INT checkLength = (INT)wcslen(PipeNameToPipeID[index].NameW);
		if (checkLength == nameLength &&
		        wmemcmp(FileName->Buffer, PipeNameToPipeID[index].NameW, nameLength) == 0)
		{
			pipeContext = GetPipeContextByID(DeviceContext, PipeNameToPipeID[index].PipeID);

			if (pipeContext == NULL || !pipeContext->IsValid || !pipeContext->Pipe || !pipeContext->Queue)
			{
				USBERR("pipe filename %s is valid but the pipe does not exist\n", PipeNameToPipeID[index].Name);
				return NULL;
			}

			return pipeContext; // a valid pipe was found.
		}
	}

	// The pipe name was not recognized.  Pipe names are case-sensitive and in the format:
	// 'PIPE_xx' where xx is the two digit hex endpoint address. i.e. PIPE_0A, PIPE_8A.
	USBERR("invalid pipe filename=%wZ\n", FileName->Buffer);
	return NULL;
}

NTSTATUS Pipe_InitDefaultContext(__in PDEVICE_CONTEXT deviceContext)
{
	PPIPE_CONTEXT pipeContext = GetPipeContextByID(deviceContext, 0);
	Policy_InitPipe(pipeContext);

	pipeContext->Pipe = WDF_NO_HANDLE;
	pipeContext->Queue = WDF_NO_HANDLE;

	pipeContext->PipeInformation.MaximumPacketSize = deviceContext->UsbDeviceDescriptor.bMaxPacketSize0;
	pipeContext->PipeInformation.MaximumTransferSize = MAX_CONTROL_TRANSFER_SIZE;
	pipeContext->PipeInformation.EndpointAddress = 0x00;
	pipeContext->PipeInformation.PipeType = WdfUsbPipeTypeControl;

	GetPolicyValue(MAX_TRANSFER_STAGE_SIZE, pipeContext->Policy) = MAX_CONTROL_TRANSFER_SIZE;
	GetPolicyValue(MAXIMUM_TRANSFER_SIZE, pipeContext->Policy) = MAX_CONTROL_TRANSFER_SIZE;

	return Pipe_InitContext(deviceContext, pipeContext);
}


