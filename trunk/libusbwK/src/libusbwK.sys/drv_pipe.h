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

#ifndef __DRV_PIPE_H__
#define __DRV_PIPE_H__

#include "drv_private.h"

#define PipeStart(PipeContext) \
	WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(PipeContext->Pipe))

#define PipeStop(PipeContext,wdfIoTargetSentIoAction) \
	WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(PipeContext->Pipe),wdfIoTargetSentIoAction)

#if 1
NTSTATUS Pipe_AbortAll(__in PDEVICE_CONTEXT deviceContext);

NTSTATUS Pipe_Abort(__in PDEVICE_CONTEXT deviceContext,
                    __in UCHAR pipeID);

NTSTATUS Pipe_Reset(__in PDEVICE_CONTEXT deviceContext,
                    __in UCHAR pipeID);

VOID Pipe_StopAll(__in PDEVICE_CONTEXT deviceContext);

VOID Pipe_StartAll(__in PDEVICE_CONTEXT deviceContext);

NTSTATUS Pipe_QueryInformation(__in PDEVICE_CONTEXT deviceContext,
                               __in  UCHAR InterfaceIndex,
                               __in  UCHAR AlternateInterfaceIndex,
                               __in  UCHAR PipeIndex,
                               __out PPIPE_INFORMATION PipeInformation);

PPIPE_CONTEXT Pipe_GetContextFromName(__in PDEVICE_CONTEXT DeviceContext,
                                      IN PUNICODE_STRING FileName);

NTSTATUS Pipe_InitDefaultContext(__in PDEVICE_CONTEXT deviceContext);

NTSTATUS Pipe_InitContext(__in PDEVICE_CONTEXT deviceContext,
                          __in PPIPE_CONTEXT pipeContext);

NTSTATUS Pipe_Start(__in PDEVICE_CONTEXT deviceContext,
                    __in PPIPE_CONTEXT pipeContext);

NTSTATUS Pipe_Stop(__in PPIPE_CONTEXT pipeContext,
                   __in WDF_IO_TARGET_SENT_IO_ACTION WdfIoTargetSentIoAction);
#endif

#endif
