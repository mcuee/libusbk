/*!********************************************************************
libusbK - WDF USB driver.
Copyright (C) 2011 All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen     (xiaofanc@gmail.com)

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
#pragma alloc_text(PAGE, Policy_InitPipe)
//#pragma alloc_text(PAGE, Policy_SetPipe)
//#pragma alloc_text(PAGE, Policy_GetPipe)
#pragma alloc_text(PAGE, Policy_InitPower)
#pragma alloc_text(PAGE, Policy_SetPower)
#pragma alloc_text(PAGE, Policy_GetPower)
#endif

POLICY_DEFAULT PipePolicyDefaults[MAX_POLICY] =
{
	{0, 0, 0},	// unused (internal use only)
	{SHORT_PACKET_TERMINATE,	FALSE,		1},
	{AUTO_CLEAR_STALL,			FALSE,		1},
	{PIPE_TRANSFER_TIMEOUT,		0,			4},
	{IGNORE_SHORT_PACKETS,		FALSE,		1},
	{ALLOW_PARTIAL_READS,		TRUE,		1},
	{AUTO_FLUSH,				FALSE,		1},
	{RAW_IO,					FALSE,		1},
	{MAXIMUM_TRANSFER_SIZE,		ULONG_MAX,	4},
	{RESET_PIPE_ON_RESUME,		FALSE,		1},
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{MAX_TRANSFER_STAGE_SIZE, LIBUSB_MAX_READ_WRITE, 4},	// NEW!
};

POLICY_DEFAULT PowerPolicyDefaults[MAX_POLICY] =
{
	{0, 0, 0},	// unused (internal use only)
	{AUTO_SUSPEND, TRUE, 1},
	{0, 0, 0},	// unused
	{SUSPEND_DELAY, 5000, 4},
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
};

POLICY_DEFAULT DevicePolicyDefaults[MAX_POLICY] =
{
	{0, 0, 0},	// unused (internal use only)
	{DEVICE_SPEED, FullSpeed, 1},
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
	{0, 0, 0},	// unused
};


VOID Policy_SetValue(__out PULONG destValue,
                     __in PVOID value,
                     __in ULONG valueLength)
{
	switch(valueLength)
	{
	case 1:
		*destValue = *((PUCHAR)value);
		break;
	case 2:
		*destValue = *((PUSHORT)value);
		break;
	default:
		*destValue = *((PULONG)value);
		break;
	}
}

VOID Policy_GetValue(__out PVOID destValue,
                     __in ULONG value,
                     __in ULONG valueLength)
{
	switch(valueLength)
	{
	case 1:
		*((PUCHAR)destValue) = (UCHAR)value;
		break;
	case 2:
		*((PUSHORT)destValue) = (USHORT)value;
		break;
	default:
		*((PULONG)destValue) = (ULONG)value;
		break;
	}
}

PPOLICY_DEFAULT Policy_GetPipeDefault(__in ULONG policyType)
{

	if (policyType == 0 || policyType >= (sizeof(PipePolicyDefaults) / sizeof(POLICY_DEFAULT)))
		return NULL;
	return &PipePolicyDefaults[policyType];
}

PPOLICY_DEFAULT Policy_GetPowerDefault(__in ULONG policyType)
{
	policyType &= 0x7F;

	if (policyType == 0 || policyType >= (sizeof(PowerPolicyDefaults) / sizeof(POLICY_DEFAULT)))
		return NULL;
	return &PowerPolicyDefaults[policyType];
}

PPOLICY_DEFAULT Policy_GetDeviceDefault(__in ULONG policyType)
{

	if (policyType == 0 || policyType >= (sizeof(DevicePolicyDefaults) / sizeof(POLICY_DEFAULT)))
		return NULL;
	return &DevicePolicyDefaults[policyType];
}

VOID Policy_InitPipe(__inout PPIPE_CONTEXT pipeContext)
{
	PAGED_CODE();

	if (pipeContext)
	{
		ULONG policyCount = (ULONG)(sizeof(pipeContext->Policy) / sizeof(pipeContext->Policy[0]));
		ULONG policyIndex;
		for (policyIndex = 0; policyIndex < policyCount; policyIndex++)
		{
			pipeContext->Policy[policyIndex] = PipePolicyDefaults[policyIndex].DefaultValue;
		}
	}
}

NTSTATUS NONPAGABLE Policy_SetPipe(__inout PDEVICE_CONTEXT deviceContext,
                                   __in UCHAR pipeID,
                                   __in ULONG policyType,
                                   __in PVOID value,
                                   __in ULONG valueLength)
{

	PPIPE_CONTEXT pipeContext;
	PPOLICY_DEFAULT defaultPolicy;

	pipeContext = GetPipeContextByID(deviceContext, pipeID);
	if (!pipeContext->IsValid)
	{
		USBERR("pipe %02Xh not found\n", pipeID);
		return STATUS_INVALID_PARAMETER;
	}

	defaultPolicy = Policy_GetPipeDefault(policyType);
	if (defaultPolicy == NULL || defaultPolicy->PolicyType != policyType)
	{
		USBERR("unknown pipe policy %d\n", policyType);
		return STATUS_INVALID_PARAMETER;
	}

	if (valueLength < defaultPolicy->ValueByteCount)
	{
		USBERR("pipe %02Xh valueLength = 0\n", pipeID);
		return STATUS_BUFFER_TOO_SMALL;
	}

	Policy_SetValue(&GetPolicyValue(policyType, pipeContext->Policy), value, defaultPolicy->ValueByteCount);

	return STATUS_SUCCESS;
}

NTSTATUS NONPAGABLE Policy_GetPipe(__in PDEVICE_CONTEXT deviceContext,
                                   __in UCHAR pipeID,
                                   __in ULONG policyType,
                                   __out PVOID value,
                                   __inout PULONG valueLength)
{
	PPIPE_CONTEXT pipeContext;
	PPOLICY_DEFAULT defaultPolicy;

	pipeContext = GetPipeContextByID(deviceContext, pipeID);
	if (!pipeContext->IsValid)
	{
		USBERR("pipe %02Xh not found\n", pipeID);
		return STATUS_INVALID_PARAMETER;
	}

	defaultPolicy = Policy_GetPipeDefault(policyType);
	if (defaultPolicy == NULL || defaultPolicy->PolicyType != policyType)
	{
		USBERR("unknown pipe policy %d\n", policyType);
		return STATUS_INVALID_PARAMETER;
	}

	if (*valueLength < defaultPolicy->ValueByteCount)
	{
		USBERR("pipe %02Xh valueLength = 0\n", pipeID);
		return STATUS_BUFFER_TOO_SMALL;
	}

	*valueLength = defaultPolicy->ValueByteCount;
	Policy_GetValue(value, GetPolicyValue(policyType, pipeContext->Policy), defaultPolicy->ValueByteCount);

	return STATUS_SUCCESS;
}

NTSTATUS Policy_InitPower(__in PDEVICE_CONTEXT deviceContext)
{

	NTSTATUS    status = STATUS_SUCCESS;
	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
	WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;

	PAGED_CODE();

	//
	// Init wait-wake/idle policy structure.
	//
	WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);
	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0);

	idleSettings.UserControlOfIdleSettings = IdleDoNotAllowUserControl;
	idleSettings.IdleTimeout = deviceContext->DeviceRegSettings.DefaultIdleTimeout;
	idleSettings.Enabled = WdfFalse;

	if (deviceContext->DeviceRegSettings.DeviceIdleEnabled)
	{
		idleSettings.Enabled = WdfTrue;
		if (
		    (deviceContext->RemoteWakeCapable && deviceContext->DeviceRegSettings.DefaultIdleState) ||
		    (deviceContext->DeviceRegSettings.DeviceIdleIgnoreWakeEnable && deviceContext->DeviceRegSettings.DefaultIdleState)
		)
		{
			idleSettings.IdleCaps = IdleUsbSelectiveSuspend;
		}

		if (deviceContext->DeviceRegSettings.UserSetDeviceIdleEnabled)
		{
			idleSettings.UserControlOfIdleSettings = IdleAllowUserControl;
		}
	}

	wakeSettings.Enabled = deviceContext->RemoteWakeCapable ? WdfTrue : WdfFalse;

	status = WdfDeviceAssignS0IdleSettings(deviceContext->WdfDevice, &idleSettings);
	if ( !NT_SUCCESS(status))
	{
		USBERR("WdfDeviceSetPowerPolicyS0IdlePolicy failed. status=%Xh\n", status);
		return status;
	}

	status = WdfDeviceAssignSxWakeSettings(deviceContext->WdfDevice, &wakeSettings);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfDeviceAssignSxWakeSettings failed. status=%Xh\n", status);
		return status;
	}

	return status;
}



NTSTATUS Policy_SetPower(__inout PDEVICE_CONTEXT deviceContext,
                         __in ULONG policyType,
                         __in PVOID value,
                         __in ULONG valueLength)
{

	PPOLICY_DEFAULT defaultPolicy;
	ULONG tempValue = 0;
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();
	UNREFERENCED_PARAMETER(status);

	defaultPolicy = Policy_GetPowerDefault(policyType);
	if (defaultPolicy == NULL || defaultPolicy->PolicyType != policyType)
	{
		USBERR("unknown power policy %d\n", policyType);
		return STATUS_INVALID_PARAMETER;
	}

	if (valueLength < defaultPolicy->ValueByteCount)
	{
		USBERR("buffer to small. policyType=%u\n", policyType);
		return STATUS_BUFFER_TOO_SMALL;
	}
	Policy_SetValue(&tempValue, value, defaultPolicy->ValueByteCount);

	switch(policyType)
	{
	case AUTO_SUSPEND:
		deviceContext->DeviceRegSettings.DeviceIdleEnabled = tempValue ? 1 : 0;
		Policy_InitPower(deviceContext);
		Policy_SetValue(&deviceContext->PowerPolicy[policyType], value, defaultPolicy->ValueByteCount);
		break;
	case SUSPEND_DELAY:
		deviceContext->DeviceRegSettings.DefaultIdleTimeout = tempValue;
		Policy_InitPower(deviceContext);
		Policy_SetValue(&deviceContext->PowerPolicy[policyType], value, defaultPolicy->ValueByteCount);
		break;
	}

	return STATUS_SUCCESS;
}

NTSTATUS Policy_GetPower(__in PDEVICE_CONTEXT deviceContext,
                         __in ULONG policyType,
                         __out PVOID value,
                         __inout PULONG valueLength)
{
	PPOLICY_DEFAULT defaultPolicy;

	PAGED_CODE();


	defaultPolicy = Policy_GetPowerDefault(policyType);
	if (defaultPolicy == NULL || defaultPolicy->PolicyType != policyType)
	{
		USBERR("unknown power policy %u\n", policyType);
		return STATUS_INVALID_PARAMETER;
	}

	if (*valueLength < defaultPolicy->ValueByteCount)
	{
		USBERR("buffer to small. policyType=%u\n", policyType);
		return STATUS_BUFFER_TOO_SMALL;
	}

	*valueLength = defaultPolicy->ValueByteCount;
	Policy_GetValue(value, deviceContext->PowerPolicy[policyType], defaultPolicy->ValueByteCount);

	return STATUS_SUCCESS;
}

NTSTATUS Policy_GetDevice(__in PDEVICE_CONTEXT deviceContext,
                          __in ULONG policyType,
                          __out PVOID value,
                          __inout PULONG valueLength)
{
	PPOLICY_DEFAULT defaultPolicy;

	PAGED_CODE();


	defaultPolicy = Policy_GetDeviceDefault(policyType);
	if (defaultPolicy == NULL || defaultPolicy->PolicyType != policyType)
	{
		USBERR("unknown devie policy %u\n", policyType);
		return STATUS_INVALID_PARAMETER;
	}

	if (*valueLength < defaultPolicy->ValueByteCount)
	{
		USBERR("buffer to small. policyType=%u\n", policyType);
		return STATUS_BUFFER_TOO_SMALL;
	}

	*valueLength = defaultPolicy->ValueByteCount;
	Policy_GetValue(value, deviceContext->DevicePolicy[policyType], defaultPolicy->ValueByteCount);

	return STATUS_SUCCESS;
}
