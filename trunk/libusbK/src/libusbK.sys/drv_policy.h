
#ifndef __DRV_POLICY_H__
#define __DRV_POLICY_H__

#include "drv_private.h"

typedef struct _POLICY_DEFAULT
{
	CONST ULONG		PolicyType;
	CONST ULONG		DefaultValue;
	CONST ULONG		ValueByteCount;
} POLICY_DEFAULT, *PPOLICY_DEFAULT;

extern POLICY_DEFAULT PipePolicyDefaults[MAX_POLICY];
extern POLICY_DEFAULT PowerPolicyDefaults[MAX_POLICY];
extern POLICY_DEFAULT DevicePolicyDefaults[MAX_POLICY];

//////////////////////////////////////////////////////////////////////////////
// lusbk_policy.c function prototypes.
// Pipe and power policy functions.
//

#if 1
VOID Policy_InitPipe(__inout PPIPE_CONTEXT pipeContext);

NTSTATUS Policy_SetPipe(__inout PDEVICE_CONTEXT devContext,
                        __in UCHAR pipeID,
                        __in ULONG policyType,
                        __in PVOID value,
                        __in ULONG valueLength);

NTSTATUS Policy_GetPipe(__in PDEVICE_CONTEXT devContext,
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