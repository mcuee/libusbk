/*! \file drv_policy.h
*/

#ifndef __DRV_POLICY_H__
#define __DRV_POLICY_H__

#include "drv_private.h"

//////////////////////////////////////////////////////////////////////////////
// lusbk_policy.c function prototypes.
// Pipe and power policy functions.
//

#if 1
VOID Policy_InitPipe(
    __in PDEVICE_CONTEXT deviceContext,
    __inout PPIPE_CONTEXT pipeContext);

VOID Policy_SetAllPipesToDefault(__in PDEVICE_CONTEXT deviceContext);

NTSTATUS Policy_SetPipe(
    __inout PDEVICE_CONTEXT devContext,
    __in UCHAR pipeID,
    __in ULONG policyType,
    __in PVOID value,
    __in ULONG valueLength);

NTSTATUS Policy_GetPipe(
    __in PDEVICE_CONTEXT devContext,
    __in UCHAR pipeID,
    __in ULONG policyType,
    __out PVOID value,
    __inout PULONG valueLength);

NTSTATUS Policy_InitPower(__in PDEVICE_CONTEXT deviceContext);

NTSTATUS Policy_GetPower(__in PDEVICE_CONTEXT devContext,
                         __in ULONG policyType,
                         __out PVOID value,
                         __inout PULONG valueLength);

NTSTATUS Policy_SetPower(__inout PDEVICE_CONTEXT devContext,
                         __in ULONG policyType,
                         __in PVOID value,
                         __in ULONG valueLength);

NTSTATUS Policy_GetDevice(__in PDEVICE_CONTEXT deviceContext,
                          __in ULONG policyType,
                          __out PVOID value,
                          __inout PULONG valueLength);
#endif



#endif