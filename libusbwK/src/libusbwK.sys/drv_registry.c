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

#include "drv_common.h"

#if (defined(ALLOC_PRAGMA) && defined(PAGING_ENABLED))
#pragma alloc_text(PAGE, Registry_ReadAllDeviceKeys)
#endif

NTSTATUS AddDefaultDeviceInterfaceGUID(__in PDEVICE_CONTEXT deviceContext);

#define GetDeviceRegSettingKey(RegKeyValueName, DefaultValue)													\
	RtlInitUnicodeString(&valueName, DEFINE_TO_STRW(RegKeyValueName));											\
	status = WdfRegistryQueryULong(hKey, &valueName, &deviceContext->DeviceRegSettings.RegKeyValueName);		\
	if (NT_SUCCESS(status))																						\
	{																											\
		USBMSG("%s=%u\n",																						\
		DEFINE_TO_STR(RegKeyValueName),(deviceContext->DeviceRegSettings.RegKeyValueName) );					\
	}																											\
	else																										\
	{																											\
		deviceContext->DeviceRegSettings.RegKeyValueName = DefaultValue;										\
	}

NTSTATUS Registry_ReadAllDeviceKeys(__in PDEVICE_CONTEXT deviceContext)
{
	WDFKEY					hKey = NULL;
	NTSTATUS				status = STATUS_INVALID_DEVICE_STATE;
	UNICODE_STRING			valueName;
	WDF_OBJECT_ATTRIBUTES	collectionAttributes;
	WDF_OBJECT_ATTRIBUTES	stringAttributes;
	GUID					guidTest = {0};

	PAGED_CODE();

	if (!deviceContext->WdfDevice)
	{
		USBERR("deviceContext->WdfDevice is NULL.\n");
		return status;
	}

	// The driver sets the PLUGPLAY_REGKEY_DEVICE flag to open the Device
	// Parameters subkey under the device's hardware key, or it sets the
	// PLUGPLAY_REGKEY_DRIVER flag to open the driver's software key. If the
	// PLUGPLAY_REGKEY_CURRENT_HWPROFILE flag is also set,
	// WdfDeviceOpenRegistryKey opens the copy of the hardware or software key
	// that is in the current hardware profile.
	status = WdfDeviceOpenRegistryKey(deviceContext->WdfDevice,
	                                  PLUGPLAY_REGKEY_DEVICE,
	                                  KEY_READ,
	                                  WDF_NO_OBJECT_ATTRIBUTES,
	                                  &hKey);

	if (!NT_SUCCESS (status))
	{
		USBERR("WdfDeviceOpenRegistryKey failed.\n");
		hKey = NULL;
		return status;
	}

	//////////////////////////////////////////////////////////////////////////
	// Read the device interface guids from the registry.
	// These are set in the inf files [Device_AddReg] group.
	//
	WDF_OBJECT_ATTRIBUTES_INIT(&collectionAttributes);
	collectionAttributes.ParentObject = deviceContext->WdfDevice;

	status = WdfCollectionCreate(&collectionAttributes,
	                             &deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs);
	if (!NT_SUCCESS(status))
	{
		USBERR("collection object could not be allocated. status=%Xh", status);
		goto Done;
	}

	WDF_OBJECT_ATTRIBUTES_INIT(&stringAttributes);
	stringAttributes.ParentObject = deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs;
	RtlInitUnicodeString(&valueName, L"DeviceInterfaceGUIDs");

	status = WdfRegistryQueryMultiString(hKey, &valueName, &stringAttributes, deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs);
	if (NT_SUCCESS(status))
	{
		ULONG guidCount = WdfCollectionGetCount(deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs);
		ULONG guidIndex;
		WDFSTRING wdfGuidString;

		USBMSG("Found %u DeviceInterfaceGUIDs strings.", guidCount);

		for (guidIndex = 0; guidIndex < guidCount; guidIndex++)
		{
			wdfGuidString = (WDFSTRING)WdfCollectionGetItem(deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs, guidIndex);
			status = GUIDFromWdfString(wdfGuidString, &guidTest);
			if (!NT_SUCCESS(status))
			{
				USBERR("removing invalid DeviceInterfaceGUID string at index %u\n", guidIndex);
				WdfCollectionRemoveItem(deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs, guidIndex);
				guidIndex--;
				guidCount--;
			}
		}
	}
	else
	{
		USBWRN("DeviceInterfaceGUIDs registry key does not exist." USB_LN "Ensure the DeviceInterfaceGUIDs "
		       "key has been properly set in the .inf and re-install the device.\n\n");
	}

	if (WdfCollectionGetCount(deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs) == 0)
	{
		status = AddDefaultDeviceInterfaceGUID(deviceContext);
		if (!NT_SUCCESS(status))
		{
			goto Done;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Read the device power policy settings (if any).
	// These are set in the inf files [Device_AddReg] group.
	//
	GetDeviceRegSettingKey(DeviceIdleEnabled, FALSE);
	GetDeviceRegSettingKey(DeviceIdleIgnoreWakeEnable, FALSE);
	GetDeviceRegSettingKey(UserSetDeviceIdleEnabled, FALSE);
	GetDeviceRegSettingKey(DefaultIdleState, FALSE);
	GetDeviceRegSettingKey(DefaultIdleTimeout, 5000);

	status = STATUS_SUCCESS;

Done:
	if (hKey)
	{
		WdfRegistryClose(hKey);
		hKey = NULL;
	}
	return status;
}
