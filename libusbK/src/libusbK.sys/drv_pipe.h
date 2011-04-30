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

VOID Pipe_StopAll(__in PDEVICE_CONTEXT deviceContext);

VOID Pipe_StartAll(__in PDEVICE_CONTEXT deviceContext);

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
