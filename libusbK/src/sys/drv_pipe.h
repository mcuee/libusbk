/*! \file drv_pipe.h
*/

#ifndef __DRV_PIPE_H__
#define __DRV_PIPE_H__

#include "drv_private.h"

#define PipeStart(PipeContext) \
	WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(PipeContext->Pipe))

#define PipeStop(PipeContext,wdfIoTargetSentIoAction) \
	WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(PipeContext->Pipe),wdfIoTargetSentIoAction)

NTSTATUS Pipe_AbortAll(__in PDEVICE_CONTEXT deviceContext);

NTSTATUS Pipe_Abort(__in PDEVICE_CONTEXT deviceContext,
                    __in UCHAR pipeID);

NTSTATUS Pipe_Reset(__in PDEVICE_CONTEXT deviceContext,
                    __in UCHAR pipeID);

VOID Pipe_StopAll(__in PDEVICE_CONTEXT deviceContext,
				  __in BOOLEAN stopIoTarget);

VOID Pipe_StartAll(__in PDEVICE_CONTEXT deviceContext,
				   __in BOOLEAN startIoTarget);

PPIPE_CONTEXT Pipe_GetContextFromName(__in PDEVICE_CONTEXT DeviceContext,
                                      IN PUNICODE_STRING FileName);

NTSTATUS Pipe_InitDefaultContext(__in PDEVICE_CONTEXT deviceContext);

NTSTATUS Pipe_InitContext(__in PDEVICE_CONTEXT deviceContext,
                          __in PPIPE_CONTEXT pipeContext,
						  __in BOOLEAN startIoTarget);

NTSTATUS Pipe_Start(__in PDEVICE_CONTEXT deviceContext,
                    __in PPIPE_CONTEXT pipeContext,
					__in BOOLEAN startIoTarget);

NTSTATUS Pipe_Stop(__in PPIPE_CONTEXT pipeContext,
                   __in WDF_IO_TARGET_SENT_IO_ACTION WdfIoTargetSentIoAction,
                   __in BOOLEAN purgeQueue,
				   __in BOOLEAN stopIoTarget);

NTSTATUS Pipe_RefreshQueue(
    __in PDEVICE_CONTEXT deviceContext,
    __in PPIPE_CONTEXT pipeContext);

ULONG Pipe_CalcMaxTransferSize(
    __in BOOLEAN IsHS, WDF_USB_PIPE_TYPE pipeType,
    __in ULONG maxPacketSize,
    __in ULONG originalMaxTransferSize);

#endif
