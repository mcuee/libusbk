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

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

// internal device info
typedef struct _KUSB_DEV_INFO_EL
{
	KUSB_DEV_INFO Public;

	DWORD ReferenceCount;
	DWORD Linked;
	DWORD LUsb0SymbolicLinkIndex;

	struct _KUSB_DEV_INFO_EL* next;
	struct _KUSB_DEV_INFO_EL* prev;
} KUSB_DEV_INFO_EL;
typedef KUSB_DEV_INFO_EL* PKUSB_DEV_INFO_EL;

// internal device list
typedef struct _KUSB_DEV_LIST_EL
{
	KUSB_DEV_LIST Public;

	PKUSB_DEV_INFO_EL current;
	PKUSB_DEV_INFO_EL head;

	struct
	{
		SPIN_LOCK_EX Acquire;
		volatile long UsageCount;
	} Locks;

} KUSB_DEV_LIST_EL;
typedef KUSB_DEV_LIST_EL* PKUSB_DEV_LIST_EL;

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
#define GetDrvIdString(DrvId)	(DrvIdNames[((((LONG)(DrvId))<0) || ((LONG)(DrvId)) >= KUSB_DRVID_COUNT)?KUSB_DRVID_COUNT:(DrvId)])

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

KUSB_DEV_LIST_EL DevListHandlePool[KUSB_MAX_INTERFACE_HANDLES];
volatile BOOL DevListInitialized = FALSE;
volatile long DevListInitializedLockCount = 0;
volatile long DevListHandlePos = -1;

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

#define GetRegDevNodeString(DeviceInterfaceElement,ValuenNameString,Key,ValueDataString)	\
	(_stricmp(ValuenNameString,DEFINE_TO_STR(Key))==0 ? CopyLast(';',DeviceInterfaceElement.Key,ValueDataString):NULL)

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
	DWORD valueDataSize = LSTK_STRING_MAX_LEN;

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
	strcpy_s(dst, LSTK_STRING_MAX_LEN, src);
	return dst;
}

VOID CheckDevListInitialized()
{
recheck:
	if (!DevListInitialized)
	{
		if (InterlockedIncrement(&DevListInitializedLockCount) == 1)
		{
			Mem_Zero(&DevListHandlePool, sizeof(DevListHandlePool));
			DevListInitialized = TRUE;
			USBDBG(
			    "Memory Usage:\r\n"
			    "\tDevListHandlePool  : %u bytes (%u each)\r\n",
			    sizeof(DevListHandlePool), sizeof(DevListHandlePool[0])
			);
		}
		else
		{
			InterlockedDecrement(&DevListInitializedLockCount);
			SwitchToThread();
			goto recheck;
		}
	}
}

PKUSB_DEV_LIST_EL DevList_Acquire()
{
	int count = 0;
	int maxHandles = sizeof(DevListHandlePool) / sizeof(DevListHandlePool[0]);

	CheckDevListInitialized();

	while(count++ < maxHandles)
	{
		PKUSB_DEV_LIST_EL next = &DevListHandlePool[InterlockedIncrement(&DevListHandlePos) & (maxHandles - 1)];
		if (next->Locks.UsageCount == 0)
		{
			if (InterlockedIncrement(&next->Locks.UsageCount) == 1)
			{
				next->current = NULL;
				next->head = NULL;
				memset(&next->Public, 0, sizeof(next->Public));

				InterlockedIncrement(&next->Locks.UsageCount);
				return next;
			}
			else
			{
				InterlockedDecrement(&next->Locks.UsageCount);
				SwitchToThread();
			}
		}
	}
	USBERR("no more device list handles! (max=%d)\n", maxHandles);
	LusbwError(ERROR_OUT_OF_STRUCTURES);
	return NULL;
}


BOOL DevList_Release(PKUSB_DEV_LIST_EL deviceList)
{
	if (InterlockedDecrement(&deviceList->Locks.UsageCount) < 0)
		InterlockedIncrement(&deviceList->Locks.UsageCount);

	return FALSE;
}








typedef struct _KUSB_ENUM_REGKEY_PARAMS
{
	// assigned by LstK_Init, copied by sub enumeration routines
	// (global)
	PKUSB_DEV_LIST_EL DeviceList;
	PKUSB_DEV_LIST_INIT_PARAMS InitParams;

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

	PKUSB_DEV_INFO_EL TempItem;

} KUSB_ENUM_REGKEY_PARAMS;
typedef KUSB_ENUM_REGKEY_PARAMS* PKUSB_ENUM_REGKEY_PARAMS;
typedef BOOL ENUM_REGKEY_DELEGATE (LPCSTR Name, PKUSB_ENUM_REGKEY_PARAMS RegEnumParams);
typedef ENUM_REGKEY_DELEGATE* PENUM_REGKEY_DELEGATE;

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


#define Match_DevItem_SymbolicLink(check, find) \
	strcmp(check->Public.SymbolicLink, find)

BOOL InterfaceInstance_AddElement(PKUSB_DEV_LIST_EL DeviceList,
                                  PKUSB_DEV_INFO_EL DevItemToAdd,
                                  PKUSB_DEV_INFO_EL* DevItemAdded)
{
	PKUSB_DEV_INFO_EL newEntry = NULL;

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

BOOL InterfaceInstance_AssignDriver(PKUSB_DEV_INFO_EL devItem)
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
			if (devItem->LUsb0SymbolicLinkIndex <= 255)
			{
				sprintf_s(devItem->Public.DevicePath, sizeof(devItem->Public.DevicePath) - 1,
				          "\\\\.\\libusb0-%04d", devItem->LUsb0SymbolicLinkIndex);

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

BOOL InterfaceInstance_FillProperties(PKUSB_DEV_INFO_EL devItem)
{
	CHAR devRegEnumKey[1024];
	CHAR valueName[LSTK_STRING_MAX_LEN];
	CHAR valueData[LSTK_STRING_MAX_LEN];

	DWORD valueNameLength, valueDataLength;
	HKEY hDevRegEnumKey = NULL;
	DWORD devRegEnumKeyIndex = (DWORD)(-1);
	DWORD valueType;

	memset(devRegEnumKey, 0, sizeof(devRegEnumKey));
	strcat_s(devRegEnumKey, sizeof(devRegEnumKey), "SYSTEM\\CurrentControlSet\\Enum\\");
	strcat_s(devRegEnumKey, sizeof(devRegEnumKey), devItem->Public.DeviceInstance);

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

VOID Common_ParseDevID(__in PKUSB_DEV_INFO_EL devItem)
{
	PKUSB_DEV_COMMON_INFO commonInfo = &devItem->Public.Common;
	PCHAR chLast = NULL;
	PCHAR chNext = devItem->Public.DeviceInstance;
	commonInfo->MI = UINT_MAX;
	while((chNext = strchr(chNext, '\\')) != NULL)
	{
		chNext++;
		chLast = chNext;
		if (!commonInfo->Vid || !commonInfo->Pid)
			sscanf_s(chNext, "VID_%04X&PID_%04X&MI_%02X", &commonInfo->Vid, &commonInfo->Pid, &commonInfo->MI);
	}
	if (commonInfo->MI != UINT_MAX)
		commonInfo->MI &= 0x7F;

	commonInfo->InstanceID = chLast;
}

BOOL EnumKeyCB_InterfaceInstance(LPCSTR Name, PKUSB_ENUM_REGKEY_PARAMS RegEnumParams)
{
	LONG status;
	DWORD referenceCount;
	BOOL isNewItem;

	KUSB_DEV_INFO_EL devItem;
	PKUSB_DEV_INFO_EL newDevItem = NULL;

	if (!IsUsbRegKey(Name))
		return TRUE;

	Mem_Zero(&devItem, sizeof(devItem));

	strcpy_s(devItem.Public.DeviceInterfaceGUID, sizeof(devItem.Public.DeviceInterfaceGUID), RegEnumParams->CurrentGUID);

	// query reference count (connected device instance id count)
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\Control\ReferenceCount
	status = RegGetValueDWord(RegEnumParams->hOpenedKey, Name, "\\Control", "ReferenceCount", &referenceCount);
	if (status != ERROR_SUCCESS || !referenceCount)
		return TRUE;

	// query device instance id
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\DeviceInstance
	status = RegGetValueString(RegEnumParams->hOpenedKey, Name, NULL, "DeviceInstance", devItem.Public.DeviceInstance);
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
	devItem.LUsb0SymbolicLinkIndex = (ULONG) - 1;
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\#\Device Parameters\LUsb0
	status = RegGetValueDWord(RegEnumParams->hOpenedKey, Name, "\\#\\Device Parameters", "LUsb0", &devItem.LUsb0SymbolicLinkIndex);

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

VOID ApplyFilter(__in PKUSB_DEV_LIST_INIT_PARAMS InitParams,
                 __inout PKUSB_DEV_LIST_EL DeviceList)
{
	PKUSB_DEV_INFO_EL check = NULL;
	PKUSB_DEV_INFO_EL tmp = NULL;

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

BOOL CreateCompositeParent(__in LPCSTR DeviceInstance,
                           __inout PKUSB_DEV_INFO_EL* compositeDevice)
{
	PKUSB_DEV_INFO_EL devComposite;
	*compositeDevice = devComposite = Mem_Alloc(sizeof(KUSB_DEV_INFO_EL));
	if (!devComposite)
		return LusbwError(ERROR_NOT_ENOUGH_MEMORY);

	strcpy_s(devComposite->Public.DeviceInstance, sizeof(devComposite->Public.DeviceInstance) - 1, DeviceInstance);
	_strupr_s(devComposite->Public.DeviceInstance, sizeof(devComposite->Public.DeviceInstance) - 1);

	strcpy_s(devComposite->Public.DeviceDesc, sizeof(devComposite->Public.DeviceDesc) - 1, "USB Composite Device");
	strcpy_s(devComposite->Public.Service, sizeof(devComposite->Public.Service) - 1, "usbccgp");

	Common_ParseDevID(devComposite);

	return TRUE;
}


VOID ApplyCompositeDevicesMode(__in PKUSB_DEV_LIST_INIT_PARAMS InitParams,
                               __inout PKUSB_DEV_LIST_EL DeviceList)
{
	PKUSB_DEV_INFO_EL compositeDeviceElement;
	PKUSB_DEV_INFO_EL devEL, devEL_t0, devCompositeEL, devCompositeEL_t0;
	PKUSB_DEV_INFO_EL root = DeviceList->head;
	PKUSB_DEV_INFO_EL compositeRoot = NULL;
	CHAR deviceInstance[LSTK_STRING_MAX_LEN];
	PKUSB_DEV_LIST_EL newCompositeList;

	if (!InitParams->EnableCompositeDeviceMode)
		return;

	DL_FOREACH_SAFE(root, devEL, devEL_t0)
	{
		PCHAR mi;
		strcpy_s(deviceInstance, sizeof(deviceInstance) - 1, devEL->Public.DeviceInstance);
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
					        strncmp(devCompositeEL->Public.DeviceInstance, compositeDeviceElement->Public.DeviceInstance, strlen(devCompositeEL->Public.DeviceInstance)) == 0)
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
					newCompositeList = (PKUSB_DEV_LIST_EL) Mem_Alloc(sizeof(KUSB_DEV_LIST_EL));
					devCompositeEL->Public.CompositeList = (PKUSB_DEV_LIST)newCompositeList;
				}
				else
				{
					newCompositeList = (PKUSB_DEV_LIST_EL)devCompositeEL->Public.CompositeList;
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

VOID DevList_RefreshCounts(__inout PKUSB_DEV_LIST_EL DeviceList)
{
	PKUSB_DEV_INFO_EL nextItem = NULL;
	DeviceList->Public.DeviceCount = 0;

	DL_FOREACH(DeviceList->head, nextItem)
	{
		DeviceList->Public.DeviceCount++;
		if (nextItem->Public.CompositeList)
		{
			DevList_RefreshCounts((PKUSB_DEV_LIST_EL)&nextItem->Public.CompositeList);
		}
	}
}

#define IsLstKHandle(PublicListHandle)	IsStaticHandle(PublicListHandle,DevListHandlePool)

KUSB_EXP VOID KUSB_API LstK_Free(__deref_inout PKUSB_DEV_LIST* DeviceList)
{
	PKUSB_DEV_INFO_EL check = NULL;
	PKUSB_DEV_INFO_EL tmp = NULL;
	PKUSB_DEV_INFO_EL check2 = NULL;
	PKUSB_DEV_INFO_EL tmp2 = NULL;
	PKUSB_DEV_LIST_EL deviceList;

	deviceList = (PKUSB_DEV_LIST_EL) * DeviceList;
	*DeviceList = NULL;
	ErrorHandle(!IsLstKHandle(deviceList), Error, "DeviceList");

	if (deviceList->Locks.UsageCount < 1)
		return;

	if (InterlockedDecrement(&deviceList->Locks.UsageCount) != 1)
	{
		InterlockedIncrement(&deviceList->Locks.UsageCount);
		return;
	}

	DL_FOREACH_SAFE(deviceList->head, check, tmp)
	{
		DL_DELETE(deviceList->head, check);

		if (check->Public.CompositeList)
		{
			PKUSB_DEV_LIST_EL compositeList = (PKUSB_DEV_LIST_EL)check->Public.CompositeList;
			DL_FOREACH_SAFE(compositeList->head, check2, tmp2)
			{
				DL_DELETE(compositeList->head, check2);
				Mem_Free(&check2);
			}
			Mem_Free(&check->Public.CompositeList);
		}

		Mem_Free(&check);
	}

	DevList_Release(deviceList);


Error:
	return;
}

KUSB_EXP BOOL KUSB_API LstK_Init(__deref_out PKUSB_DEV_LIST* DeviceList,
                                 __in_opt PKUSB_DEV_LIST_INIT_PARAMS InitParams)
{
	KUSB_DEV_LIST_INIT_PARAMS defInitParams;
	PKUSB_DEV_LIST_INIT_PARAMS initParams = InitParams;
	KUSB_ENUM_REGKEY_PARAMS enumParams;
	BOOL success = FALSE;
	PKUSB_DEV_LIST_EL deviceList = NULL;

	deviceList = DevList_Acquire();
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

		DevList_RefreshCounts(deviceList);

		success = TRUE;
	}

Done:
	if (success)
	{
		*DeviceList = (PKUSB_DEV_LIST)deviceList;
	}
	else
	{
		LstK_Free(&(PKUSB_DEV_LIST)deviceList);
	}
	return success;
}

KUSB_EXP BOOL KUSB_API LstK_MoveNext(__inout PKUSB_DEV_LIST DeviceList,
                                     __deref_out_opt PKUSB_DEV_INFO* DeviceInfo)
{
	PKUSB_DEV_LIST_EL deviceList = (PKUSB_DEV_LIST_EL)DeviceList;

	ErrorHandle(!deviceList, Error, "DeviceList");
	ErrorHandle(deviceList->Locks.UsageCount < 1, Error, "DeviceList");

	if (DeviceInfo)
		*DeviceInfo = NULL;

	if (!deviceList->current)
	{
		if (!deviceList->head)
		{
			SetLastError(ERROR_EMPTY);
			return FALSE;
		}
		deviceList->current = deviceList->head;
		*DeviceInfo = (PKUSB_DEV_INFO)deviceList->current;
		return TRUE;
	}

	if ((deviceList->current = deviceList->current->next) == NULL)
	{
		SetLastError(ERROR_NO_MORE_ITEMS);
		return FALSE;
	}

	if (DeviceInfo)
		*DeviceInfo = (PKUSB_DEV_INFO)deviceList->current;

	return TRUE;

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_Current(__in PKUSB_DEV_LIST DeviceList,
                                    __deref_out PKUSB_DEV_INFO* DeviceInfo)
{
	PKUSB_DEV_LIST_EL deviceList = (PKUSB_DEV_LIST_EL)DeviceList;

	ErrorHandle(!deviceList, Error, "DeviceList");
	ErrorHandle(deviceList->Locks.UsageCount < 1, Error, "DeviceList");

	if (!deviceList->current)
	{
		SetLastError(ERROR_NO_MORE_ITEMS);
		return FALSE;
	}
	*DeviceInfo = (PKUSB_DEV_INFO)deviceList->current;

	return TRUE;

Error:
	return FALSE;
}

KUSB_EXP VOID KUSB_API LstK_Reset(__inout PKUSB_DEV_LIST DeviceList)
{
	PKUSB_DEV_LIST_EL deviceList = (PKUSB_DEV_LIST_EL)DeviceList;

	ErrorHandle(!deviceList, Error, "DeviceList");
	ErrorHandle(deviceList->Locks.UsageCount < 1, Error, "DeviceList");

	deviceList->current = NULL;

Error:
	return;
}

KUSB_EXP BOOL KUSB_API LstK_Enumerate(__in PKUSB_DEV_LIST DeviceList,
                                      __in PENUM_DEV_LIST_CB EnumDevListCB,
                                      __in_opt PVOID Context)
{
	PKUSB_DEV_INFO_EL check = NULL;
	PKUSB_DEV_LIST_EL deviceList = (PKUSB_DEV_LIST_EL)DeviceList;

	ErrorHandle(!deviceList, Error, "DeviceList");
	ErrorHandle(deviceList->Locks.UsageCount < 1, Error, "DeviceList");
	ErrorParam(!EnumDevListCB, Error, "EnumDevListCB");

	DL_FOREACH(deviceList->head, check)
	{
		if (EnumDevListCB(DeviceList, (PKUSB_DEV_INFO)check, Context) == FALSE)
			break;
	}
	return TRUE;

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_FindByVidPid(__in PKUSB_DEV_LIST DeviceList,
        __in UINT Vid,
        __in UINT Pid,
        __deref_out PKUSB_DEV_INFO* DeviceInfo)
{
	PKUSB_DEV_INFO_EL check = NULL;
	PKUSB_DEV_LIST_EL deviceList = (PKUSB_DEV_LIST_EL)DeviceList;

	ErrorHandle(!deviceList, Error, "DeviceList");
	ErrorParam(!DeviceInfo, Error, "DeviceInfo");
	ErrorHandle(deviceList->Locks.UsageCount < 1, Error, "DeviceList");

	DL_FOREACH(deviceList->head, check)
	{
		if ((check->Public.Common.Vid & 0xFFFF) == (Vid & 0xFFFF) &&
		        (check->Public.Common.Pid & 0xFFFF) == (Pid & 0xFFFF))
			break;
	}

	*DeviceInfo = (PKUSB_DEV_INFO)check;

	if (!check)
		return LusbwError(ERROR_NO_MORE_ITEMS);

	return TRUE;

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_Lock(__in PKUSB_DEV_LIST DeviceList, BOOL wait)
{
	PKUSB_DEV_LIST_EL deviceList = (PKUSB_DEV_LIST_EL)DeviceList;

	ErrorHandle(!deviceList, Error, "DeviceList");
	ErrorHandle(deviceList->Locks.UsageCount < 1, Error, "DeviceList");

	return SpinLock_AcquireEx(&deviceList->Locks.Acquire, wait);

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_Unlock(__in PKUSB_DEV_LIST DeviceList)
{
	PKUSB_DEV_LIST_EL deviceList = (PKUSB_DEV_LIST_EL)DeviceList;

	ErrorHandle(!deviceList, Error, "DeviceList");
	ErrorHandle(deviceList->Locks.UsageCount < 1, Error, "DeviceList");

	return SpinLock_ReleaseEx(&deviceList->Locks.Acquire);

Error:
	return FALSE;
}