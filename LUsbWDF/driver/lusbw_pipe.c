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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AbortPipes)
#pragma alloc_text(PAGE, ResetOrAbortPipe)
#pragma alloc_text(PAGE, ResetPipe)
#pragma alloc_text(PAGE, StartAllPipes)
#pragma alloc_text(PAGE, StopAllPipes)
#endif

NTSTATUS ResetPipe(IN WDFUSBPIPE Pipe)
/*++

Routine Description:

    This routine resets the pipe.

Arguments:

    Pipe - framework pipe handle

Return Value:

    NT status value

--*/
{
	NTSTATUS status;

	PAGED_CODE();

	//
	//  This routine synchronously submits a URB_FUNCTION_RESET_PIPE
	// request down the stack.
	//
	status = WdfUsbTargetPipeResetSynchronously(Pipe,
	         WDF_NO_HANDLE, // WDFREQUEST
	         NULL  // PWDF_REQUEST_SEND_OPTIONS
	                                           );

	if (NT_SUCCESS(status))
	{
		USBMSG0("ResetPipe - success\n");
		status = STATUS_SUCCESS;
	}
	else
	{
		USBERR0("ResetPipe - failed\n");
	}

	return status;
}

NTSTATUS ResetOrAbortPipe(IN  PDEVICE_CONTEXT deviceContext,
                          IN  BOOLEAN resetPipe,
                          IN  UCHAR pipeID,
                          IN  INT timeout)
{
	NTSTATUS					status = STATUS_INVALID_PARAMETER;
	WDF_REQUEST_SEND_OPTIONS	syncReqOptions;
	PPIPE_CONTEXT				pipeContext;

	PAGED_CODE();

	if (timeout > LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT || !timeout)
	{
		timeout = LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT;
	}

	WDF_REQUEST_SEND_OPTIONS_INIT(&syncReqOptions, 0);
	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&syncReqOptions, WDF_REL_TIMEOUT_IN_MS(timeout));

	pipeContext = &GetContextByPipeID(deviceContext, pipeID);
	if (pipeContext->Pipe)
	{
		USBMSG("[%s] pipeID=%02Xh",
		       resetPipe ? "Reset" : "Abort", pipeID);

		if (resetPipe)
		{
			status = WdfUsbTargetPipeResetSynchronously(pipeContext->Pipe, WDF_NO_HANDLE, &syncReqOptions);
			if (!NT_SUCCESS(status))
			{
				USBERR("WdfUsbTargetPipeResetSynchronously failed pipeID=%02Xh status=%Xh\n", pipeID, status);
			}
		}
		else
		{
			status = WdfUsbTargetPipeAbortSynchronously(pipeContext->Pipe, WDF_NO_HANDLE, &syncReqOptions);
			if (!NT_SUCCESS(status))
			{
				USBERR("WdfUsbTargetPipeAbortSynchronously failed pipeID=%02Xh status=%Xh\n", pipeID, status);
			}
		}
	}
	else
	{
		USBERR("Pipe not found. pipeID=%02Xh\n", pipeID);
	}

	return status;
}

NTSTATUS AbortPipes(IN PDEVICE_CONTEXT deviceContext)
/*++

Routine Description

    sends an abort pipe request on all open pipes.

Arguments:

    Device - Handle to a framework device

Return Value:

    NT status value

--*/
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

VOID StopAllPipes(IN PDEVICE_CONTEXT deviceContext)
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
			WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(pipeContext->Pipe), WdfIoTargetCancelSentIo);
		}
	}
}

VOID StartAllPipes(IN PDEVICE_CONTEXT deviceContext)
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
			WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(pipeContext->Pipe));
		}
	}
}
