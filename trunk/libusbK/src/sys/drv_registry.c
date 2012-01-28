/*!********************************************************************
libusbK - WDF USB driver.
Copyright (C) 2011 All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Lee Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen         (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/

#include "drv_common.h"
#include "lusb_defdi_guids.h"

#if (defined(ALLOC_PRAGMA) && defined(PAGING_ENABLED))
#pragma alloc_text(PAGE, Registry_ReadAllDeviceKeys)
#endif

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
		BOOLEAN removeGuidFromCollection;

		USBMSG("Found %u DeviceInterfaceGUIDs strings.", guidCount);

		for (guidIndex = 0; guidIndex < guidCount; guidIndex++)
		{
			removeGuidFromCollection = TRUE;

			wdfGuidString = (WDFSTRING)WdfCollectionGetItem(deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs, guidIndex);
			status = GUIDFromWdfString(wdfGuidString, &guidTest);
			if (!NT_SUCCESS(status))
			{
				USBERR("removing invalid DeviceInterfaceGUID string at index %u\n", guidIndex);
			}
			else
			{
				if (IsEqualGUID(&guidTest, &Libusb0DeviceGuid))
				{
					USBWRN("libusb0 device DeviceInterfaceGUID found. skippng..\n");
				}
				else if (IsEqualGUID(&guidTest, &Libusb0FilterGuid))
				{
					USBWRN("libusb0 filter DeviceInterfaceGUID found. skippng..\n");
				}
				else if (IsEqualGUID(&guidTest, &LibusbKDeviceGuid))
				{
					USBWRN("libusbK default device DeviceInterfaceGUID found. skippng..\n");
				}
				else
				{
					removeGuidFromCollection = FALSE;
				}
			}

			if (removeGuidFromCollection)
			{
				WdfCollectionRemoveItem(deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs, guidIndex);
				guidIndex--;
				guidCount--;
			}
		}
	}
	else
	{
		USBWRN("DeviceInterfaceGUIDs registry key does not exist.\n"
		       "    Ensure the DeviceInterfaceGUIDs key has been properly set in the .inf and re-install the device.\n\n");
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
	GetDeviceRegSettingKey(SystemWakeEnabled, FALSE);

	status = STATUS_SUCCESS;

Done:
	if (hKey)
	{
		WdfRegistryClose(hKey);
		hKey = NULL;
	}
	return status;
}

NTSTATUS AddDefaultDeviceInterfaceGUID(__in PDEVICE_CONTEXT deviceContext)
{

	WDF_OBJECT_ATTRIBUTES	stringAttributes;
	UNICODE_STRING defaultDeviceInterfaceGUID_UnicodeString;
	WDFSTRING defaultDeviceInterfaceGUID = NULL;
	GUID guidTest = {0};
	NTSTATUS status;

	WDF_OBJECT_ATTRIBUTES_INIT(&stringAttributes);

	stringAttributes.ParentObject = deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs;

	RtlInitUnicodeString(&defaultDeviceInterfaceGUID_UnicodeString, LibusbKDeviceGuidW);

	status = WdfStringCreate(&defaultDeviceInterfaceGUID_UnicodeString, &stringAttributes, &defaultDeviceInterfaceGUID);
	if (!NT_SUCCESS(status))
	{
		USBERR("WdfStringCreate failed. status=%Xh\n", status);
		return status;
	}

	status = GUIDFromWdfString(defaultDeviceInterfaceGUID, &guidTest);
	if (!NT_SUCCESS(status))
	{
		USBERR("GUIDFromWdfString failed. status=%Xh\n", status);
		return status;
	}

	USBWRN("using default GUID {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
	       guidTest.Data1,
	       guidTest.Data2,
	       guidTest.Data3,
	       guidTest.Data4[0], guidTest.Data4[1],
	       guidTest.Data4[2], guidTest.Data4[3], guidTest.Data4[4], guidTest.Data4[5], guidTest.Data4[6], guidTest.Data4[7]);

	return WdfCollectionAdd(deviceContext->DeviceRegSettings.DeviceInterfaceGUIDs, defaultDeviceInterfaceGUID);

}
