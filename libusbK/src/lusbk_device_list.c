/*!********************************************************************
libusbK - Multi-driver USB library.
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

#include <windows.h>
#include <objbase.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
#include <ctype.h>
#include <winioctl.h>
#include "lusbk_device_list.h"
#include "lusbk_stack_collection.h"
#include "lusb_defdi_guids.h"
#include "lusbk_handles.h"

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#define GetDrvIdString(DrvId)	(DrvIdNames[((((LONG)(DrvId))<0) || ((LONG)(DrvId)) >= KUSB_DRVID_COUNT)?KUSB_DRVID_COUNT:(DrvId)])

#define GetRegDevNodeString(DeviceInterfaceElement,ValuenNameString,Key,ValueDataString)	\
	(_stricmp(ValuenNameString,DEFINE_TO_STR(Key))==0 ? CopyLast(';',DeviceInterfaceElement.Key,ValueDataString):NULL)

#define LSTK_INCREF(deviceListInternal,ErrorJumpStatement) ErrorHandle(!PoolHandle_Inc_LstK(deviceListInternal), ErrorJumpStatement, "IncRef-DeviceList")
#define LSTK_DECREF(deviceListInternal) PoolHandle_Dec_LstK(deviceListInternal)

#define lstk_DL_MatchSymbolicLink(PatternEL, MatchEL) DL_MatchPattern((PatternEL)->Public.SymbolicLink, (MatchEL)->Public.SymbolicLink)

typedef struct _KUSB_ENUM_REGKEY_PARAMS
{
	// assigned by LstK_Init, copied by sub enumeration routines
	// (global)
	PKLST_HANDLE_INTERNAL DeviceList;
	PKLST_INIT_PARAMS InitParams;

	// required before calling EnumRegKey
	HKEY hParentKey;
	BOOL (*EnumRegKeyCB) (LPCSTR, struct _KUSB_ENUM_REGKEY_PARAMS* RegEnumParams);

	// optional
	LPCSTR SubKey;
	LPCSTR CurrentGUID;

	// optional
	PVOID Context;
	DWORD ErrorCode;

	// managed by EnumRegKey
	HKEY hOpenedKey;

	PKLST_DEV_INFO_EL TempItem;

} KUSB_ENUM_REGKEY_PARAMS;

typedef KUSB_ENUM_REGKEY_PARAMS* PKUSB_ENUM_REGKEY_PARAMS;
typedef BOOL ENUM_REGKEY_DELEGATE (LPCSTR Name, PKUSB_ENUM_REGKEY_PARAMS RegEnumParams);
typedef ENUM_REGKEY_DELEGATE* PENUM_REGKEY_DELEGATE;

typedef struct _KLST_SYNC_CONTEXT
{
	PKLST_HANDLE_INTERNAL Master;
	PKLST_HANDLE_INTERNAL Slave;
	PKLST_SYNC_PARAMS Params;
} KLST_SYNC_CONTEXT, *PKLST_SYNC_CONTEXT;

typedef struct _SERVICE_DRVID_MAP
{
	INT DrvId;
	LPCSTR* MapNames;
}* PSERVICE_DRVID_MAP, SERVICE_DRVID_MAP;

typedef struct _DEV_INTF_VALUENAME_MAP
{
	LPCSTR ValueName;
	DWORD RegValueType;
}* PDEV_INTF_VALUENAME_MAP, DEV_INTF_VALUENAME_MAP;

#define KEY_DEVICECLASSES "SYSTEM\\CurrentControlSet\\Control\\DeviceClasses"

static LPCSTR DrvIdNames[8] = {"libusbK", "libusb0", "WinUSB", "libusb0 filter", "Unknown", "Unknown", "Unknown"};

static LPCSTR lusb0_Services[] = {"LIBUSB0", NULL};
static LPCSTR lusbk_Services[] = {"LIBUSBK", NULL};
static LPCSTR wusb_Services[]  = {"WINUSB", NULL};


static LPCSTR lusb0_FilterDevGuidNames[] =
{
	DEFINE_TO_STR(_DefLibusb0FilterGuid),
	NULL
};

static LPCSTR lusb0_DevGuidNames[] =
{
	DEFINE_TO_STR(_DefLibusb0DeviceGuid),
	NULL
};

static LPCSTR lusbK_DevGuidNames[] =
{
	DEFINE_TO_STR(_DefLibusbKDeviceGuid),
	NULL
};

static const SERVICE_DRVID_MAP ServiceDrvIdMap[] =
{
	{KUSB_DRVID_LIBUSBK,	lusbk_Services},
	{KUSB_DRVID_LIBUSB0,	lusb0_Services},
	{KUSB_DRVID_WINUSB,		wusb_Services},

	{KUSB_DRVID_INVALID,	NULL}
};

static const SERVICE_DRVID_MAP DevGuidDrvIdMap[] =
{
	{KUSB_DRVID_LIBUSBK,		lusbK_DevGuidNames},
	{KUSB_DRVID_LIBUSB0,		lusb0_DevGuidNames},
	{KUSB_DRVID_LIBUSB0_FILTER,	lusb0_FilterDevGuidNames},

	{KUSB_DRVID_INVALID,	NULL}
};

PCHAR CopyLast(__in CHAR sep, __out PCHAR dst, __in PCHAR src);
BOOL IsUsbRegKey(__in LPCSTR keyName);

LONG RegGetValueDWord(__in HKEY hKeyParent,
                      __in LPCSTR keyBasePath,
                      __in_opt LPCSTR keySubPath,
                      __in LPCSTR valueName,
                      __inout LPDWORD value);

LONG RegGetValueString(__in HKEY hKeyParent,
                       __in LPCSTR keyBasePath,
                       __in_opt LPCSTR keySubPath,
                       __in LPCSTR valueName,
                       __inout LPSTR devInstElementStringData);

LONG RegGetValueString(__in HKEY hKeyParent,
                       __in LPCSTR keyBasePath,
                       __in_opt LPCSTR keySubPath,
                       __in LPCSTR valueName,
                       __inout LPSTR devInstElementStringData)
{
	CHAR keyPath[1024];
	LONG status = ERROR_SUCCESS;
	HKEY hKey = NULL;
	DWORD valueType;
	DWORD valueDataSize = KLST_STRING_MAX_LEN;

	memset(keyPath, 0, sizeof(keyPath));
	if ((status = (LONG)strcat_s(keyPath, sizeof(keyPath) - 1, keyBasePath)) != ERROR_SUCCESS)
		goto Error;

	if (keySubPath)
	{
		if ((status = (LONG)strcat_s(keyPath, sizeof(keyPath) - 1, keySubPath)) != ERROR_SUCCESS)
			goto Error;
	}

	if ((status = RegOpenKeyExA(hKeyParent, keyPath, 0, KEY_READ, &hKey)) != ERROR_SUCCESS)
		goto Error;

	status = RegQueryValueExA(hKey, valueName, 0, &valueType, (LPBYTE)devInstElementStringData, &valueDataSize);
	if (status == ERROR_SUCCESS)
		devInstElementStringData[valueDataSize] = '\0';

Error:
	if (hKey)
		RegCloseKey(hKey);

	return status;
}

LONG RegGetValueDWord(__in HKEY hKeyParent,
                      __in LPCSTR keyBasePath,
                      __in_opt LPCSTR keySubPath,
                      __in LPCSTR valueName,
                      __inout LPDWORD value)
{
	CHAR keyPath[1024];
	LONG status = ERROR_SUCCESS;
	HKEY hKey = NULL;
	DWORD valueType;
	DWORD valueDataSize = sizeof(DWORD);

	memset(keyPath, 0, sizeof(keyPath));
	if ((status = (LONG)strcat_s(keyPath, sizeof(keyPath) - 1, keyBasePath)) != ERROR_SUCCESS)
		goto Error;

	if (keySubPath)
	{
		if ((status = (LONG)strcat_s(keyPath, sizeof(keyPath) - 1, keySubPath)) != ERROR_SUCCESS)
			goto Error;
	}

	if ((status = RegOpenKeyExA(hKeyParent, keyPath, 0, KEY_READ, &hKey)) != ERROR_SUCCESS)
		goto Error;

	status = RegQueryValueExA(hKey, valueName, 0, &valueType, (LPBYTE)value, &valueDataSize);

Error:
	if (hKey)
		RegCloseKey(hKey);

	return status;
}

BOOL IsUsbRegKey(__in LPCSTR keyName)
{
	LPCSTR keyPathPartsNext;
	LPCSTR keyPathParts = keyName;
	while((keyPathPartsNext = strchr(keyPathParts, '#')) != NULL)
	{
		if (_strnicmp(&keyPathPartsNext[1], "usb", 3) == 0)
			return TRUE;
		keyPathParts = keyPathPartsNext + 1;
	}
	return FALSE;
}

PCHAR CopyLast(__in CHAR sep, __out PCHAR dst, __in PCHAR src)
{
	PCHAR next = src;
	while((next = strchr(src, sep)) != NULL)
	{
		src = next + 1;
	}
	strcpy_s(dst, KLST_STRING_MAX_LEN, src);
	return dst;
}

BOOL EnumRegKey(PKUSB_ENUM_REGKEY_PARAMS params)
{
	LONG status;
	DWORD keyIndex = (DWORD) - 1;
	CHAR keyName[1024];

	params->hOpenedKey = NULL;

	status = RegOpenKeyExA(params->hParentKey, params->SubKey, 0, KEY_READ, &params->hOpenedKey);
	if (status != ERROR_SUCCESS)
		return FALSE;

	while(RegEnumKeyA(params->hOpenedKey, ++keyIndex, keyName, sizeof(keyName) - 1) == ERROR_SUCCESS)
	{
		if (!params->EnumRegKeyCB(keyName, params))
			goto Done;
	}

Done:
	// close the key
	if (IsHandleValid(params->hOpenedKey))
		RegCloseKey(params->hOpenedKey);

	params->hOpenedKey = NULL;

	return TRUE;
}

static BOOL KUSB_API lstk_DevEnum_SyncPrep(
    __in KLST_HANDLE DeviceList,
    __in PKLST_DEV_INFO DeviceInfo,
    __in PKLST_SYNC_CONTEXT Context)
{
	UNREFERENCED_PARAMETER(DeviceList);
	UNREFERENCED_PARAMETER(Context);

	DeviceInfo->SyncResults.SyncFlags = SYNC_FLAG_NONE;

	return TRUE;
}

static BOOL KUSB_API lstk_DevEnum_SyncRemoved(
    __in KLST_HANDLE DeviceList,
    __in PKLST_DEV_INFO DeviceInfo,
    __in PKLST_SYNC_CONTEXT Context)
{
	PKLST_DEV_INFO_EL masterDevInfo = (PKLST_DEV_INFO_EL)DeviceInfo;
	PKLST_DEV_INFO_EL slaveDevInfo;

	UNREFERENCED_PARAMETER(DeviceList);

	// Skip elements already processed by previous sync operations.
	if (masterDevInfo->Public.SyncResults.SyncFlags != SYNC_FLAG_NONE) return TRUE;

	DL_SEARCH(Context->Slave->head, slaveDevInfo, masterDevInfo, lstk_DL_MatchSymbolicLink);
	if (slaveDevInfo)
	{
		// This element exists in the slave and master list.
		memcpy(masterDevInfo->Public.DevicePath, slaveDevInfo->Public.DevicePath, KLST_STRING_MAX_LEN);
		if (slaveDevInfo->Public.Connected != masterDevInfo->Public.Connected)
		{
			// Connected/Disconnected.
			masterDevInfo->Public.Connected = slaveDevInfo->Public.Connected;
			if (slaveDevInfo->Public.Connected)
				masterDevInfo->Public.SyncResults.Added = 1;
			else
				masterDevInfo->Public.SyncResults.Removed = 1;


		}
		else
		{
			// Unchanged.
			masterDevInfo->Public.SyncResults.Unchanged = 1;
		}

	}
	else
	{
		// This element exists in the master list only.
		if (masterDevInfo->Public.Connected)
		{
			masterDevInfo->Public.SyncResults.Removed = 1;
			masterDevInfo->Public.Connected = FALSE;
		}
		else
		{
			masterDevInfo->Public.SyncResults.Unchanged = 1;
		}

		// LstK_DetachInfo(DeviceList, DeviceInfo);
		// LstK_FreeInfo(&DeviceInfo);
	}

	return TRUE;
}

static BOOL KUSB_API lstk_DevEnum_SyncAdded(
    __in KLST_HANDLE DeviceList,
    __in PKLST_DEV_INFO DeviceInfo,
    __in PKLST_SYNC_CONTEXT Context)
{
	PKLST_DEV_INFO_EL slaveDevInfo = (PKLST_DEV_INFO_EL)DeviceInfo;
	PKLST_DEV_INFO_EL masterDevInfo;

	UNREFERENCED_PARAMETER(DeviceList);

	DL_SEARCH(Context->Master->head, masterDevInfo, slaveDevInfo, lstk_DL_MatchSymbolicLink);
	if (masterDevInfo)
	{
		// Skip elements already processed by previous sync operations.
		if (masterDevInfo->Public.SyncResults.SyncFlags != SYNC_FLAG_NONE) return TRUE;
		memcpy(masterDevInfo->Public.DevicePath, slaveDevInfo->Public.DevicePath, KLST_STRING_MAX_LEN);

		if (slaveDevInfo->Public.Connected != masterDevInfo->Public.Connected)
		{
			// Connected/Disconnected.
			masterDevInfo->Public.Connected = slaveDevInfo->Public.Connected;
			if (slaveDevInfo->Public.Connected)
				masterDevInfo->Public.SyncResults.Added = 1;
			else
				masterDevInfo->Public.SyncResults.Removed = 1;
		}
		else
		{
			// Unchanged.
			masterDevInfo->Public.SyncResults.Unchanged = 1;
		}
	}
	else
	{
		// Added.
		masterDevInfo = Mem_Alloc(sizeof(KLST_DEV_INFO_EL));
		ErrorNoSet(!IsHandleValid(masterDevInfo), Error, "->Mem_Alloc");
		memcpy(masterDevInfo, slaveDevInfo, sizeof(KLST_DEV_INFO_EL));

		if (slaveDevInfo->Public.Connected)
			masterDevInfo->Public.SyncResults.SyncFlags = SYNC_FLAG_ADDED;
		else
			masterDevInfo->Public.SyncResults.SyncFlags = SYNC_FLAG_UNCHANGED;

		DL_APPEND(Context->Master->head, masterDevInfo);
	}

	return TRUE;
Error:
	return FALSE;
}

#define Match_DevItem_SymbolicLink(check, find) \
	strcmp(check->Public.SymbolicLink, find)

BOOL InterfaceInstance_AddElement(PKLST_HANDLE_INTERNAL DeviceList,
                                  PKLST_DEV_INFO_EL DevItemToAdd,
                                  PKLST_DEV_INFO_EL* DevItemAdded)
{
	PKLST_DEV_INFO_EL newEntry = NULL;

	*DevItemAdded = NULL;

	DL_SEARCH(DeviceList->head, newEntry, DevItemToAdd->Public.SymbolicLink, Match_DevItem_SymbolicLink);
	if (newEntry)
	{
		*DevItemAdded = newEntry;
		return FALSE;
	}

	newEntry = Mem_Alloc(sizeof(*newEntry));
	ErrorMemory(!newEntry, Done);

	memcpy(newEntry, DevItemToAdd, sizeof(*newEntry));
	newEntry->next = NULL;
	newEntry->prev = NULL;
	newEntry->Public.CompositeList = NULL;

	DL_APPEND(DeviceList->head, newEntry);
	*DevItemAdded = newEntry;

	return TRUE;

Done:
	return FALSE;
}

BOOL InterfaceInstance_AssignDriver(PKLST_DEV_INFO_EL devItem)
{

	PSERVICE_DRVID_MAP map;

	// find driver type by device interface guid
	devItem->Public.DrvId = KUSB_DRVID_INVALID;
	map = (PSERVICE_DRVID_MAP)DevGuidDrvIdMap;
	while(devItem->Public.DrvId == KUSB_DRVID_INVALID && map->DrvId != KUSB_DRVID_INVALID)
	{
		LPCSTR* devGuid = map->MapNames;
		while(*devGuid)
		{
			if (_stricmp(devItem->Public.DeviceInterfaceGUID, *devGuid) == 0)
			{
				devItem->Public.DrvId = map->DrvId;
				break;
			}
			devGuid++;
		}
		map++;
	}

	// find driver type by service name (if not found)
	map = (PSERVICE_DRVID_MAP)ServiceDrvIdMap;
	while(devItem->Public.DrvId == KUSB_DRVID_INVALID && map->DrvId != KUSB_DRVID_INVALID)
	{
		LPCSTR* serviceName = map->MapNames;
		while(*serviceName)
		{
			if (_stricmp(devItem->Public.Service, *serviceName) == 0)
			{
				devItem->Public.DrvId = map->DrvId;
				break;
			}
			serviceName++;
		}
		map++;
	}

	if (devItem->Public.DrvId != KUSB_DRVID_INVALID)
	{
		if (devItem->Public.DrvId == KUSB_DRVID_LIBUSB0_FILTER ||
		        devItem->Public.DrvId == KUSB_DRVID_LIBUSB0 )
		{
			if (devItem->Public.LUsb0FilterIndex <= 255)
			{
				sprintf_s(devItem->Public.DevicePath, sizeof(devItem->Public.DevicePath) - 1,
				          "\\\\.\\libusb0-%04d", devItem->Public.LUsb0FilterIndex);

				// new device with  an assigned driver (ready for device list)
				return TRUE;
			}
		}
		else
		{
			// new device with  an assigned driver (ready for device list)
			return TRUE;
		}
	}
	return FALSE;
}

BOOL InterfaceInstance_FillProperties(PKLST_DEV_INFO_EL devItem)
{
	CHAR devRegEnumKey[1024];
	CHAR valueName[KLST_STRING_MAX_LEN];
	CHAR valueData[KLST_STRING_MAX_LEN];

	DWORD valueNameLength, valueDataLength;
	HKEY hDevRegEnumKey = NULL;
	DWORD devRegEnumKeyIndex = (DWORD)(-1);
	DWORD valueType;

	memset(devRegEnumKey, 0, sizeof(devRegEnumKey));
	strcat_s(devRegEnumKey, sizeof(devRegEnumKey), "SYSTEM\\CurrentControlSet\\Enum\\");
	strcat_s(devRegEnumKey, sizeof(devRegEnumKey), devItem->Public.InstanceID);

	// opening device instance reg key node
	// e.g. HKLM\SYSTEM\CurrentControlSet\Enum\USB\VID_1234&PID_0001&MI_00\7&15c836fa&2&0000
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, devRegEnumKey, 0, KEY_READ, &hDevRegEnumKey) != ERROR_SUCCESS)
		return FALSE;

	memset(valueName, 0, sizeof(valueName));
	memset(valueData, 0, sizeof(valueData));
	valueNameLength = sizeof(valueName) - 1;
	valueDataLength = sizeof(valueData) - 1;

	while(RegEnumValueA(hDevRegEnumKey, ++devRegEnumKeyIndex, valueName, &valueNameLength, 0, &valueType, (LPBYTE)valueData, &valueDataLength) == ERROR_SUCCESS)
	{

		// e.g. HKLM\SYSTEM\CurrentControlSet\Enum\USB\VID_1234&PID_0001&MI_00\7&15c836fa&2&0000\[ValueNames]

		if (GetRegDevNodeString(devItem->Public, valueName, ClassGUID, valueData)) {}
		else if (GetRegDevNodeString(devItem->Public, valueName, DeviceDesc, valueData)) {}
		else if (GetRegDevNodeString(devItem->Public, valueName, Mfg, valueData)) {}
		else if (GetRegDevNodeString(devItem->Public, valueName, Service, valueData)) {}

		memset(valueName, 0, sizeof(valueName));
		memset(valueData, 0, sizeof(valueData));
		valueNameLength = sizeof(valueName) - 1;
		valueDataLength = sizeof(valueData) - 1;
	}

	RegCloseKey(hDevRegEnumKey);

	return (strlen(devItem->Public.Service) > 0);
}

VOID Common_ParseDevID(__in PKLST_DEV_INFO_EL devItem)
{
	PKLST_DEV_COMMON_INFO commonInfo = &devItem->Public.Common;
	CHAR devID[KLST_STRING_MAX_LEN];
	PCHAR chLast;
	PCHAR chNext;
	ULONG pos = 0;

	commonInfo->MI = UINT_MAX;
	commonInfo->Vid = UINT_MAX;
	commonInfo->Pid = UINT_MAX;

	strcpy_s(devID, sizeof(devID), devItem->Public.InstanceID);
	_strupr_s(devID, sizeof(devID));
	chLast = chNext = devID;

	while(chLast && chLast[0])
	{
		if ((chNext = strchr(chNext, '\\')) != NULL)
		{
			*chNext = '\0';
			chNext = &chNext[1];
		}
		switch(pos)
		{
		case 0:	// USB
			break;
		case 1:	// VID_%04X&PID_%04X
			if (sscanf_s(chLast, "VID_%04X&PID_%04X&MI_%02X", &commonInfo->Vid, &commonInfo->Pid, &commonInfo->MI) < 2)
				USBWRN("Failed scanning vid/pid into common info.\n");
			break;
		case 2:	// InstanceID
			strcpy_s(commonInfo->InstanceID, sizeof(commonInfo->InstanceID), chLast);
			break;
		default:
			USBWRN("Unknown device instance id element: %s.\n", chLast);
			break;
		}

		pos++;
		chLast = chNext;
	}

	if (commonInfo->Vid != UINT_MAX)
		commonInfo->Vid &= 0xFFFF;

	if (commonInfo->Pid != UINT_MAX)
		commonInfo->Pid &= 0xFFFF;

	if (commonInfo->MI != UINT_MAX)
		commonInfo->MI &= 0x7F;
}

BOOL EnumKeyCB_InterfaceInstance(LPCSTR Name, PKUSB_ENUM_REGKEY_PARAMS RegEnumParams)
{
	LONG status;
	DWORD referenceCount;
	BOOL isNewItem;

	KLST_DEV_INFO_EL devItem;
	PKLST_DEV_INFO_EL newDevItem = NULL;

	if (!IsUsbRegKey(Name))
		return TRUE;

	Mem_Zero(&devItem, sizeof(devItem));

	strcpy_s(devItem.Public.DeviceInterfaceGUID, sizeof(devItem.Public.DeviceInterfaceGUID), RegEnumParams->CurrentGUID);

	// query reference count (connected device instance id count)
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\Control\ReferenceCount
	status = RegGetValueDWord(RegEnumParams->hOpenedKey, Name, "\\Control", "ReferenceCount", &referenceCount);
	if (status != ERROR_SUCCESS)
		return TRUE;

	if (!referenceCount && !RegEnumParams->InitParams->ShowDisconnectedDevices)
		return TRUE;

	devItem.Public.Connected = referenceCount > 0;

	// query device instance id
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\DeviceInstance
	status = RegGetValueString(RegEnumParams->hOpenedKey, Name, NULL, "DeviceInstance", devItem.Public.InstanceID);
	if (status != ERROR_SUCCESS)
		return TRUE;

	// query symbolic link value
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\#\SymbolicLink
	status = RegGetValueString(RegEnumParams->hOpenedKey, Name, "\\#", "SymbolicLink",  devItem.Public.SymbolicLink);
	if (status != ERROR_SUCCESS)
		return TRUE;

	// DevicePath is equal to SymbolicLink unless it ends up being a libusb0 filter device
	strcpy_s(devItem.Public.DevicePath, sizeof(devItem.Public.DevicePath), devItem.Public.SymbolicLink);

	// query LUsb0
	devItem.Public.LUsb0FilterIndex = (ULONG) - 1;
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\#\Device Parameters\LUsb0
	status = RegGetValueDWord(RegEnumParams->hOpenedKey, Name, "\\#\\Device Parameters", "LUsb0", &devItem.Public.LUsb0FilterIndex);

	if (!InterfaceInstance_FillProperties(&devItem))
		return TRUE;

	if (!InterfaceInstance_AssignDriver(&devItem))
		return TRUE;

	isNewItem = InterfaceInstance_AddElement(RegEnumParams->DeviceList, &devItem, &newDevItem);
	if (isNewItem)
	{
		Common_ParseDevID(newDevItem);
		// new element added

	}
	else if (newDevItem)
	{
		// element already exists
	}
	else
	{
		// an error occured
		return TRUE;

	}

	return TRUE;
}
BOOL EnumKeyCB_InterfaceGuid(LPCSTR Name, PKUSB_ENUM_REGKEY_PARAMS RegEnumParams)
{
	// enumeration device interface instances
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}

	KUSB_ENUM_REGKEY_PARAMS enumParamsInterfaceGUIDs;

	memcpy(&enumParamsInterfaceGUIDs, RegEnumParams, sizeof(enumParamsInterfaceGUIDs));

	enumParamsInterfaceGUIDs.EnumRegKeyCB	= EnumKeyCB_InterfaceInstance;
	enumParamsInterfaceGUIDs.hParentKey		= RegEnumParams->hOpenedKey;
	enumParamsInterfaceGUIDs.SubKey			= Name;
	enumParamsInterfaceGUIDs.CurrentGUID    = Name;

	return EnumRegKey(&enumParamsInterfaceGUIDs);
}

#define Match_DevItem_DeviceInterfaceGUID(check, find) \
	_stricmp(check->Public.DeviceInterfaceGUID, find)

VOID ApplyFilter(__in PKLST_INIT_PARAMS InitParams,
                 __inout PKLST_HANDLE_INTERNAL DeviceList)
{
	PKLST_DEV_INFO_EL check = NULL;
	PKLST_DEV_INFO_EL tmp = NULL;

	DL_FOREACH_SAFE(DeviceList->head, check, tmp)
	{
		if (!InitParams->EnableRawDeviceInterfaceGuid)
		{
			if (Match_DevItem_DeviceInterfaceGUID(check, RawDeviceGuidA) == 0)
			{
				DL_DELETE(DeviceList->head, check);
				Mem_Free(&check);
			}
		}
	}
}

BOOL CreateCompositeParent(__in LPCSTR InstanceID,
                           __inout PKLST_DEV_INFO_EL* compositeDevice)
{
	PKLST_DEV_INFO_EL devComposite;
	*compositeDevice = devComposite = Mem_Alloc(sizeof(KLST_DEV_INFO_EL));
	if (!devComposite)
		return LusbwError(ERROR_NOT_ENOUGH_MEMORY);

	strcpy_s(devComposite->Public.InstanceID, sizeof(devComposite->Public.InstanceID) - 1, InstanceID);
	_strupr_s(devComposite->Public.InstanceID, sizeof(devComposite->Public.InstanceID) - 1);

	strcpy_s(devComposite->Public.DeviceDesc, sizeof(devComposite->Public.DeviceDesc) - 1, "USB Composite Device");
	strcpy_s(devComposite->Public.Service, sizeof(devComposite->Public.Service) - 1, "usbccgp");

	Common_ParseDevID(devComposite);

	return TRUE;
}


VOID ApplyCompositeDevicesMode(__in PKLST_INIT_PARAMS InitParams,
                               __inout PKLST_HANDLE_INTERNAL DeviceList)
{
	PKLST_DEV_INFO_EL compositeDeviceElement;
	PKLST_DEV_INFO_EL devEL, devEL_t0, devCompositeEL, devCompositeEL_t0;
	PKLST_DEV_INFO_EL root = DeviceList->head;
	PKLST_DEV_INFO_EL compositeRoot = NULL;
	CHAR deviceInstance[KLST_STRING_MAX_LEN];
	PKLST_HANDLE_INTERNAL newCompositeList;

	if (!InitParams->EnableCompositeDeviceMode)
		return;

	DL_FOREACH_SAFE(root, devEL, devEL_t0)
	{
		PCHAR mi;
		strcpy_s(deviceInstance, sizeof(deviceInstance) - 1, devEL->Public.InstanceID);
		_strupr_s(deviceInstance, sizeof(deviceInstance) - 1);
		mi = strstr(deviceInstance, "&MI_");
		if (mi)
		{
			mi[0] = '\0';
			// this is a composite device
			if (CreateCompositeParent(deviceInstance, &compositeDeviceElement))
			{
				compositeDeviceElement->Public.DrvId = devEL->Public.DrvId;
				DL_FOREACH_SAFE(compositeRoot, devCompositeEL, devCompositeEL_t0)
				{
					if (devCompositeEL->Public.DrvId == compositeDeviceElement->Public.DrvId &&
					        strncmp(devCompositeEL->Public.InstanceID, compositeDeviceElement->Public.InstanceID, strlen(devCompositeEL->Public.InstanceID)) == 0)
					{
						// We already have a composite parent for this one.
						break;
					}
				}

				if (!devCompositeEL)
				{
					// this is a new composite parent.
					DL_APPEND(compositeRoot, compositeDeviceElement);
					devCompositeEL = compositeDeviceElement;
				}
				else
				{
					Mem_Free(&compositeDeviceElement);
				}

				if (!devCompositeEL->Public.CompositeList)
				{
					newCompositeList = (PKLST_HANDLE_INTERNAL) Mem_Alloc(sizeof(KLST_HANDLE_INTERNAL));
					devCompositeEL->Public.CompositeList = (KLST_HANDLE)newCompositeList;
				}
				else
				{
					newCompositeList = (PKLST_HANDLE_INTERNAL)devCompositeEL->Public.CompositeList;
				}

				if (newCompositeList)
				{
					DL_DELETE(root, devEL);
					DL_APPEND(newCompositeList->head, devEL);
				}
			}
		}
	}

	DL_FOREACH_SAFE(compositeRoot, devCompositeEL, devCompositeEL_t0)
	{
		DL_DELETE(compositeRoot, devCompositeEL);
		DL_APPEND(root, devCompositeEL);
	}

	DeviceList->head = root;
}

#define IsLstKHandle(PublicListHandle)	ALLK_VALID_HANDLE(PublicListHandle,LstK)

static BOOL KUSB_API lstk_DevEnum_Free(
    __in KLST_HANDLE DeviceList,
    __in PKLST_DEV_INFO DeviceInfo,
    __in PKLST_SYNC_CONTEXT Context)
{
	UNREFERENCED_PARAMETER(Context);

	LstK_DetachInfo(DeviceList, DeviceInfo);
	LstK_FreeInfo(&DeviceInfo);

	return TRUE;
}

VOID KUSB_API lstk_Free(__inout KLST_HANDLE deviceList)
{
	LstK_Enumerate(deviceList, lstk_DevEnum_Free, NULL);
}

KUSB_EXP BOOL KUSB_API LstK_Count(
    __in KLST_HANDLE DeviceList,
    __inout PULONG Count)
{
	PKLST_HANDLE_INTERNAL deviceList = (PKLST_HANDLE_INTERNAL)DeviceList;
	PKLST_DEV_INFO_EL nextItem = NULL;
	ULONG deviceCount = *Count;

	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");
	ErrorParam(!IsHandleValid(Count), Error, "Count");

	LSTK_INCREF(deviceList, Error);

	DL_FOREACH(deviceList->head, nextItem)
	{
		deviceCount++;
		if (nextItem->Public.CompositeList)
		{
			LstK_Count((PKLST_HANDLE_INTERNAL)&nextItem->Public.CompositeList, Count);
		}
	}
	*Count = deviceCount;

	return LSTK_DECREF(deviceList);

Error:
	return FALSE;
}

KUSB_EXP VOID KUSB_API LstK_Free(__deref_inout KLST_HANDLE* DeviceList)
{
	PKLST_HANDLE_INTERNAL deviceList;

	deviceList = (PKLST_HANDLE_INTERNAL) * DeviceList;
	*DeviceList = NULL;
	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");
	ErrorHandle(!ALLK_INUSE_HANDLE(deviceList), Error, "DeviceList");

	LSTK_DECREF(deviceList);
	return;

Error:
	return;
}

KUSB_EXP BOOL KUSB_API LstK_Init(__deref_out KLST_HANDLE* DeviceList,
                                 __in_opt PKLST_INIT_PARAMS InitParams)
{
	KLST_INIT_PARAMS defInitParams;
	PKLST_INIT_PARAMS initParams = InitParams;
	KUSB_ENUM_REGKEY_PARAMS enumParams;
	BOOL success = FALSE;
	PKLST_HANDLE_INTERNAL deviceList = NULL;

	deviceList = PoolHandle_Acquire_LstK(&lstk_Free);
	ErrorHandle(!deviceList, Done, "DeviceList");

	memset(&defInitParams, 0, sizeof(defInitParams));

	if (!initParams)
		initParams = &defInitParams;

	Mem_Zero(&enumParams, sizeof(enumParams));

	enumParams.DeviceList		= deviceList;
	enumParams.InitParams		= initParams;
	enumParams.EnumRegKeyCB		= EnumKeyCB_InterfaceGuid;
	enumParams.hParentKey		= HKEY_LOCAL_MACHINE;
	enumParams.SubKey			= KEY_DEVICECLASSES;

	// enumerate device interface guids
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}
	if (EnumRegKey(&enumParams))
	{
		if (deviceList->head)
		{
			ApplyFilter(initParams, deviceList);
			ApplyCompositeDevicesMode(initParams, deviceList);
		}

		success = TRUE;
	}

Done:
	if (success)
	{
		*DeviceList = (KLST_HANDLE)deviceList;
	}
	else
	{
		LstK_Free(&(KLST_HANDLE)deviceList);
	}
	return success;
}

KUSB_EXP BOOL KUSB_API LstK_MoveNext(__inout KLST_HANDLE DeviceList,
                                     __deref_out_opt PKLST_DEV_INFO* DeviceInfo)
{
	PKLST_HANDLE_INTERNAL deviceList = (PKLST_HANDLE_INTERNAL)DeviceList;

	if (DeviceInfo) *DeviceInfo = NULL;

	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");

	LSTK_INCREF(deviceList, Error);

	if (!deviceList->current)
	{
		if (!deviceList->head)
		{
			SetLastError(ERROR_EMPTY);
			LSTK_DECREF(deviceList);
			return FALSE;
		}
		deviceList->current = deviceList->head;
		*DeviceInfo = (PKLST_DEV_INFO)deviceList->current;
		LSTK_DECREF(deviceList);
		return TRUE;
	}

	if ((deviceList->current = deviceList->current->next) == NULL)
	{
		SetLastError(ERROR_NO_MORE_ITEMS);
		LSTK_DECREF(deviceList);
		return FALSE;
	}

	if (DeviceInfo)
		*DeviceInfo = (PKLST_DEV_INFO)deviceList->current;

	return LSTK_DECREF(deviceList);

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_Current(__in KLST_HANDLE DeviceList,
                                    __deref_out PKLST_DEV_INFO* DeviceInfo)
{
	PKLST_HANDLE_INTERNAL deviceList = (PKLST_HANDLE_INTERNAL)DeviceList;

	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");

	LSTK_INCREF(deviceList, Error);

	if (!deviceList->current)
	{
		SetLastError(ERROR_NO_MORE_ITEMS);
		LSTK_DECREF(deviceList);
		return FALSE;
	}
	*DeviceInfo = (PKLST_DEV_INFO)deviceList->current;

	return LSTK_DECREF(deviceList);

Error:
	return FALSE;
}

KUSB_EXP VOID KUSB_API LstK_Reset(__inout KLST_HANDLE DeviceList)
{
	PKLST_HANDLE_INTERNAL deviceList = (PKLST_HANDLE_INTERNAL)DeviceList;

	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");

	LSTK_INCREF(deviceList, Error);

	deviceList->current = NULL;

	LSTK_DECREF(deviceList);
	return;

Error:
	return;
}

KUSB_EXP BOOL KUSB_API LstK_Enumerate(__in KLST_HANDLE DeviceList,
                                      __in PKLST_DEV_ENUM_CB EnumDevListCB,
                                      __in_opt PVOID Context)
{
	PKLST_DEV_INFO_EL check, tmp;
	PKLST_HANDLE_INTERNAL deviceList = (PKLST_HANDLE_INTERNAL)DeviceList;

	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");
	ErrorParam(!EnumDevListCB, Error, "EnumDevListCB");

	LSTK_INCREF(deviceList, Error);

	DL_FOREACH_SAFE(deviceList->head, check, tmp)
	{
		if (EnumDevListCB(DeviceList, (PKLST_DEV_INFO)check, Context) == FALSE)
			break;
	}

	return LSTK_DECREF(deviceList);

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_FindByVidPid(__in KLST_HANDLE DeviceList,
        __in UINT Vid,
        __in UINT Pid,
        __deref_out PKLST_DEV_INFO* DeviceInfo)
{
	PKLST_DEV_INFO_EL check = NULL;
	PKLST_HANDLE_INTERNAL deviceList = (PKLST_HANDLE_INTERNAL)DeviceList;

	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");
	ErrorParam(!DeviceInfo, Error, "DeviceInfo");

	LSTK_INCREF(deviceList, Error);

	DL_FOREACH(deviceList->head, check)
	{
		if ((check->Public.Common.Vid & 0xFFFF) == (Vid & 0xFFFF) &&
		        (check->Public.Common.Pid & 0xFFFF) == (Pid & 0xFFFF))
			break;
	}

	*DeviceInfo = (PKLST_DEV_INFO)check;
	if (!check)
	{
		LSTK_DECREF(deviceList);
		return LusbwError(ERROR_NO_MORE_ITEMS);
	}

	return LSTK_DECREF(deviceList);

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_Lock(__in KLST_HANDLE DeviceList, BOOL wait)
{
	PKLST_HANDLE_INTERNAL deviceList = (PKLST_HANDLE_INTERNAL)DeviceList;

	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");

	LSTK_INCREF(deviceList, Error);

	return ALLK_LOCKEX_HANDLE(deviceList, wait);

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_Unlock(__in KLST_HANDLE DeviceList)
{
	PKLST_HANDLE_INTERNAL deviceList = (PKLST_HANDLE_INTERNAL)DeviceList;

	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");

	LSTK_DECREF(deviceList);

	return ALLK_UNLOCK_HANDLE(deviceList);

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_Clone(__in KLST_HANDLE src, __out KLST_HANDLE* dst)
{
	PKLST_HANDLE_INTERNAL deviceList = (PKLST_HANDLE_INTERNAL)src;
	PKLST_HANDLE_INTERNAL newDeviceList = NULL;
	PKLST_DEV_INFO_EL infoEL = NULL;
	PKLST_DEV_INFO_EL newInfoEL = NULL;

	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");
	ErrorHandle(!ALLK_INUSE_HANDLE(deviceList), Error, "DeviceList");

	newDeviceList = PoolHandle_Acquire_LstK(lstk_Free);
	ErrorNoSet(!IsHandleValid(newDeviceList), Error, "->PoolHandle_Acquire_LstK");

	LSTK_INCREF(deviceList, Error);

	DL_FOREACH(deviceList->head, infoEL)
	{
		newInfoEL = NULL;
		LstK_CloneInfo((PKLST_DEV_INFO)infoEL, (PKLST_DEV_INFO*)&newInfoEL);
		if (!IsHandleValid(newInfoEL))
		{
			LSTK_DECREF(deviceList);
			goto Error;
		}
		DL_APPEND(newDeviceList->head, newInfoEL);
	}

	*dst = (KLST_HANDLE)newDeviceList;

	LSTK_DECREF(deviceList);
	return TRUE;

Error:

	if (newDeviceList)
		PoolHandle_Dec_LstK(newDeviceList);

	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_Sync(
    __inout KLST_HANDLE MasterList,
    __in_opt KLST_HANDLE SlaveList,
    __in_opt PKLST_SYNC_PARAMS SyncParams)
{
	KLST_SYNC_CONTEXT context;
	KLST_SYNC_PARAMS defSyncParams;

	context.Params = IsHandleValid(SyncParams) ? SyncParams : Mem_Zero(&defSyncParams, sizeof(defSyncParams));
	context.Master = (PKLST_HANDLE_INTERNAL)MasterList;
	ErrorHandle(!IsLstKHandle(context.Master) || !ALLK_INUSE_HANDLE(context.Master), Error, "MasterList");

	LSTK_INCREF(context.Master, Error);

	if (!SlaveList)
	{
		// Use a new list for the slave list
		KLST_INIT_PARAMS slaveListInit;
		memset(&slaveListInit, 0, sizeof(slaveListInit));

		slaveListInit.ShowDisconnectedDevices = TRUE;

		LstK_Init(&SlaveList, &slaveListInit);
		context.Slave = (PKLST_HANDLE_INTERNAL)SlaveList;
	}
	else
	{
		context.Slave = (PKLST_HANDLE_INTERNAL)SlaveList;
		LSTK_INCREF(context.Slave, Error_IncRefSlave);
	}
	ErrorHandle(!IsLstKHandle(context.Slave) || !ALLK_INUSE_HANDLE(context.Slave), Error, "SlaveList");

	LstK_Enumerate(MasterList, lstk_DevEnum_SyncPrep, &context);
	LstK_Enumerate(SlaveList, lstk_DevEnum_SyncAdded, &context);
	LstK_Enumerate(MasterList, lstk_DevEnum_SyncRemoved, &context);

	LSTK_DECREF(context.Master);
	LSTK_DECREF(context.Slave);
	return TRUE;

Error:
	return FALSE;

Error_IncRefSlave:
	LSTK_DECREF(context.Master);
	return FALSE;

}

KUSB_EXP BOOL KUSB_API LstK_CloneInfo(
    __in PKLST_DEV_INFO SrcInfo,
    __deref_inout PKLST_DEV_INFO* DstInfo)
{
	PKLST_DEV_INFO_EL dstInfo = NULL;
	BOOL success;

	ErrorParam(!IsHandleValid(SrcInfo), Error, "SrcInfo");
	ErrorParam(!IsHandleValid(DstInfo), Error, "DstInfo");

	dstInfo = Mem_Alloc(sizeof(KLST_DEV_INFO_EL));
	ErrorMemory(!IsHandleValid(dstInfo), Error);

	memcpy(dstInfo, SrcInfo, sizeof(KLST_DEV_INFO_EL));
	dstInfo->Public.CompositeList = NULL;
	dstInfo->next = NULL;
	dstInfo->prev = NULL;

	if (SrcInfo->CompositeList)
	{
		success = LstK_Clone(SrcInfo->CompositeList, &dstInfo->Public.CompositeList);
		ErrorNoSet(success, Error, "->LstK_Clone");
	}

	*DstInfo = (PKLST_DEV_INFO)dstInfo;
	return TRUE;

Error:
	Mem_Free(&dstInfo);

	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_DetachInfo(
    __inout KLST_HANDLE DeviceList,
    __in PKLST_DEV_INFO DeviceInfo)
{
	PKLST_HANDLE_INTERNAL deviceList = (PKLST_HANDLE_INTERNAL)DeviceList;
	PKLST_DEV_INFO_EL deviceInfo = (PKLST_DEV_INFO_EL)DeviceInfo;

	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");
	ErrorParam(!IsHandleValid(deviceInfo), Error, "DeviceInfo");

	LSTK_INCREF(deviceList, Error);

	DL_DELETE(deviceList->head, deviceInfo);

	LSTK_DECREF(deviceList);
	return TRUE;
Error:
	return FALSE;

}

KUSB_EXP BOOL KUSB_API LstK_AttachInfo(
    __inout KLST_HANDLE DeviceList,
    __in PKLST_DEV_INFO DeviceInfo)
{
	PKLST_HANDLE_INTERNAL deviceList = (PKLST_HANDLE_INTERNAL)DeviceList;
	PKLST_DEV_INFO_EL deviceInfo = (PKLST_DEV_INFO_EL)DeviceInfo;

	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");
	ErrorParam(!IsHandleValid(deviceInfo), Error, "DeviceInfo");

	LSTK_INCREF(deviceList, Error);

	DL_APPEND(deviceList->head, deviceInfo);

	return LSTK_DECREF(deviceList);
Error:
	return FALSE;

}

KUSB_EXP BOOL KUSB_API LstK_FreeInfo(
    __deref_inout PKLST_DEV_INFO* DeviceInfo)
{
	PKLST_DEV_INFO_EL deviceInfo = (PKLST_DEV_INFO_EL) * DeviceInfo;
	ErrorParam(!IsHandleValid(DeviceInfo), Error, "DeviceInfo");
	ErrorParam(!IsHandleValid(deviceInfo), Error, "DeviceInfo");

	if (deviceInfo->Public.CompositeList)
		LstK_Free(&deviceInfo->Public.CompositeList);

	Mem_Free(DeviceInfo);
	return TRUE;
Error:
	return FALSE;

}
