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