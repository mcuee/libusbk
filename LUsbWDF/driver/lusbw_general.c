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

#ifndef _LUSBW_GENERAL_H
#define _LUSBW_GENERAL_H

#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LUsbW_ReadDeviceRegistryKeys)
#endif
typedef enum _LUSBW_DEVREG_KEY_VALUE_TYPE
{

} LUSBW_DEVREG_KEY_VALUE_TYPE;

typedef struct _LUSBW_DEVREG_KEY
{
	CONST PWCHAR	ValueName;
	CONST PCHAR		DisplayName;
	CONST ULONG		DefaultValue;
	CONST ULONG		ValueByteCount;
} LUSBW_DEVREG_KEY,*PLUSBW_DEVREG_KEY;

CONST PCHAR PipeTypeStrings[8] = {"Invalid", "Control", "Isochronous", "Bulk", "Interrupt", "", "", ""};
CONST PCHAR DeviceSpeedStrings[8] = {"Low", "Full", "High", "Super", "", "", "", ""};
CONST PCHAR BoolStrings[2] = {"False", "True"};

CONST LUSBW_DEVREG_KEY LUsbWDevRegKeys[LUSBW_DEVREG_KEYID_TYPE_MAX] =
{
	// DeviceIdleEnabled	This is a DWORD value. This registry value indicates whether or not the device is capable of being powered down when idle (Selective Suspend).
	//
	//    * A value of zero, or the absence of this value indicates that the device does not support being powered down when idle.
	//    * A non-zero value indicates that the device supports being powered down when idle.
	//    * If DeviceIdleEnabled is not set, the value of the AUTO_SUSPEND power policy setting is ignored.	{"DeviceIdleEnabled", WdfFalse},
	//	Example: HKR,,LUsbW_DeviceIdleEnabled,0x00010001,1
	//
	{L"LUsbW_DeviceIdleEnabled", "DeviceIdleEnabled", WdfFalse},

	// DeviceIdleIgnoreWakeEnable	When set to a nonzero value, it suspends the device even if it does not support RemoteWake.
	//
	{L"LUsbW_DeviceIdleIgnoreWakeEnable", "DeviceIdleIgnoreWakeEnable", WdfFalse},

	// UserSetDeviceIdleEnabled	This value is a DWORD value. This registry value indicates whether or not a check box should be enabled in the device Properties page that allows a user to override the idle defaults. When UserSetDeviceIdleEnabled is set to a nonzero value the check box is enabled and the user can disable powering down the device when idle. A value of zero, or the absence of this value indicates that the check box is not enabled.
	//
	//    * If the user disables device power savings, the value of the AUTO_SUSPEND power policy setting is ignored.
	//    * If the user enables device power savings, then the value of AUTO_SUSPEND is used to determine whether or not to suspend the device when idle.
	//
	// The UserSetDeviceIdleEnabled is ignored if DeviceIdleEnabled is not set.
	//	Example: HKR,,LUsbW_UserSetDeviceIdleEnabled,0x00010001,1
	//
	{L"LUsbW_UserSetDeviceIdleEnabled", "UserSetDeviceIdleEnabled", WdfFalse},

	// DefaultIdleState	This is a DWORD value. This registry value sets the default value of the AUTO_SUSPEND power policy setting. This registry key is used to enable or disable selective suspend when a handle is not open to the device.
	//
	//    * A value of zero or the absence of this value indicates that by default, the device is not suspended when idle. The device be allowed to suspend when idle only when the AUTO_SUSPEND power policy is enabled.
	//    * A non-zero value indicates that by default the device is allowed to be suspended when idle.
	//
	// This value is ignored if DeviceIdleEnabled is not set.
	//	Example: HKR,,LUsbW_DefaultIdleState,0x00010001,1
	//
	{L"LUsbW_DefaultIdleState", "DefaultIdleState", WdfFalse},


	// DefaultIdleTimeout	This is a DWORD value. This registry value sets the default state of the SUSPEND_DELAY power policy setting.
	//
	// The value indicates the amount of time in milliseconds to wait before determining that a device is idle.
	//	Example: HKR,,LUsbW_DefaultIdleTimeout,0x00010001,100
	//
	{L"LUsbW_DefaultIdleTimeout", "DefaultIdleTimeout", 5000},

};

PINTERFACE_CONTEXT GetInterfaceContextByNumber(IN  PDEVICE_CONTEXT deviceContext,
        IN  UCHAR interfaceNumber)
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

NTSTATUS LUsbW_ReadDeviceRegistryKeys(IN OUT  PDEVICE_CONTEXT deviceContext)
{
	WDFKEY			hKey = NULL;
	NTSTATUS		status = STATUS_INVALID_DEVICE_STATE;
	UNICODE_STRING	valueName;
	ULONG			value = 0;
	ULONG			regKeyIndex;

	WDF_OBJECT_ATTRIBUTES	collectionAttributes;
	WDF_OBJECT_ATTRIBUTES	stringAttributes;
	WDFCOLLECTION			collection = NULL;

	PAGED_CODE();

	if (!deviceContext->WdfDevice)
	{
		USBERR0("deviceContext->WdfDevice is NULL.\n");
		return status;
	}

	status = WdfDeviceOpenRegistryKey(deviceContext->WdfDevice,
	                                  PLUGPLAY_REGKEY_DEVICE,
	                                  KEY_READ,
	                                  WDF_NO_OBJECT_ATTRIBUTES,
	                                  &hKey);

	if (!NT_SUCCESS (status))
	{
		USBERR0("WdfDeviceOpenRegistryKey failed.\n");
		hKey = NULL;
		return status;
	}

	// First read the device interface guids from the registry.
	//
	WDF_OBJECT_ATTRIBUTES_INIT(&collectionAttributes);
	collectionAttributes.ParentObject = deviceContext->WdfDevice;
	status = WdfCollectionCreate(&collectionAttributes, &collection);
	if (!NT_SUCCESS(status))
	{
		USBERR("A collection object could not be allocated. status=%Xh", status);
		goto Done;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&stringAttributes);
	stringAttributes.ParentObject = collection;
	RtlInitUnicodeString(&valueName,LUsbW_DeviceInterfaceGUIDs_ValueName);

	status = WdfRegistryQueryMultiString(hKey, &valueName, &stringAttributes, collection);
	if (!NT_SUCCESS(status))
	{
		USBWRN("LUsbW_DeviceInterfaceGUIDs registry key not found. The default GUID will be used. status=%Xh",
		       status);
	}
	else
	{
		USBMSG("Found %d LUsbW_DeviceInterfaceGUIDs strings.", WdfCollectionGetCount(collection));
	}

	// TODO: MARK
	if (collection)
	{
		WdfObjectDelete(collection);
		collection = NULL;
	}

	USBDBG("LUsbWDevRegKeys count=%d\n",sizeof(LUsbWDevRegKeys)/sizeof(LUsbWDevRegKeys[0]));
	for (regKeyIndex=0; regKeyIndex < (sizeof(LUsbWDevRegKeys)/sizeof(LUsbWDevRegKeys[0])); regKeyIndex++)
	{
		CONST LUSBW_DEVREG_KEY 	regKey			= LUsbWDevRegKeys[regKeyIndex];
		PLUSBW_USER_DEVREG_KEY	userRegKey		= &deviceContext->PowerPolicy.UserKeys[regKeyIndex];

		// Initialize default values.
		userRegKey->IsSet = FALSE;
		userRegKey->Value = regKey.DefaultValue;
		userRegKey->DefaultValue = regKey.DefaultValue;
		userRegKey->ValueByteCount = regKey.ValueByteCount;

		RtlInitUnicodeString(&valueName,regKey.ValueName);

		status = WdfRegistryQueryULong(hKey, &valueName, &value);
		if (NT_SUCCESS(status))
		{
			USBMSG("%s=%d\n",regKey.DisplayName, value);

			userRegKey->IsSet = TRUE;
			userRegKey->Value = value;
		}
	}

	status = STATUS_SUCCESS;

Done:
	if (hKey)
	{
		WdfRegistryClose(hKey);
		hKey = NULL;
	}
	return status;

}

#endif
