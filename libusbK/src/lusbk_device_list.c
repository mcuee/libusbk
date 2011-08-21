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

#include "lusbk_private.h"
#include "lusbk_handles.h"
#include "lusb_defdi_guids.h"
#include "lusbk_linked_list.h"

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#define GetDrvIdString(DrvId)	(DrvIdNames[((((LONG)(DrvId))<0) || ((LONG)(DrvId)) >= KUSB_DRVID_COUNT)?KUSB_DRVID_COUNT:(DrvId)])

#define GetRegDevNodeString(DeviceInterfaceElement,ValuenNameString,Key,ValueDataString)	\
	(_stricmp(ValuenNameString,DEFINE_TO_STR(Key))==0 ? l_Str_CopyLast(';',DeviceInterfaceElement->Key,ValueDataString):NULL)

#define lstk_DL_MatchSymbolicLink(PatternEL, MatchEL) DL_MatchPattern((PatternEL)->Public.SymbolicLink, (MatchEL)->Public.SymbolicLink)
#define Match_DevItem_SymbolicLink(check, find)	strcmp(check->Public.SymbolicLink, find)

#define KEY_DEVICECLASSES "SYSTEM\\CurrentControlSet\\Control\\DeviceClasses"

typedef struct _KUSB_ENUM_REGKEY_PARAMS
{
	// assigned by LstK_Init, copied by sub enumeration routines
	// (global)
	PKLST_HANDLE_INTERNAL DeviceList;
	KLST_FLAG Flags;

	// required before calling l_Enum_RegKey
	HKEY hParentKey;
	BOOL (*EnumRegKeyCB) (LPCSTR, struct _KUSB_ENUM_REGKEY_PARAMS* RegEnumParams);

	// optional
	LPCSTR SubKey;
	LPCSTR CurrentGUID;

	// optional
	PVOID Context;
	DWORD ErrorCode;

	// managed by l_Enum_RegKey
	HKEY hOpenedKey;

	KLST_DEVINFO_HANDLE TempItem;

} KUSB_ENUM_REGKEY_PARAMS;

typedef BOOL ENUM_REGKEY_DELEGATE (LPCSTR Name, KUSB_ENUM_REGKEY_PARAMS* RegEnumParams);
typedef ENUM_REGKEY_DELEGATE* PENUM_REGKEY_DELEGATE;

typedef struct _KLST_SYNC_CONTEXT
{
	PKLST_HANDLE_INTERNAL Master;
	PKLST_HANDLE_INTERNAL Slave;
	KLST_SYNC_FLAG SyncFlags;
} KLST_SYNC_CONTEXT;

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

static LONG l_GetReg_String(__in HKEY hKeyParent,
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

static LONG l_GetReg_DWord(
    __in HKEY hKeyParent,
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

static BOOL l_IsUsb_RegKey(__in LPCSTR keyName)
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

static PCHAR l_Str_CopyLast(
    __in CHAR sep,
    __out PCHAR dst,
    __in PCHAR src)
{
	PCHAR next = src;
	while((next = strchr(src, sep)) != NULL)
	{
		src = next + 1;
	}
	strcpy_s(dst, KLST_STRING_MAX_LEN, src);
	return dst;
}

static BOOL l_Enum_RegKey(KUSB_ENUM_REGKEY_PARAMS* params)
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

	return params->ErrorCode == ERROR_SUCCESS;
}

static BOOL KUSB_API l_DevEnum_Free_All(
    __in KLST_HANDLE DeviceList,
    __in KLST_DEVINFO* DeviceInfo,
    __in KLST_SYNC_CONTEXT* Context)
{
	UNREFERENCED_PARAMETER(Context);

	LstK_DetachInfo(DeviceList, DeviceInfo);
	LstK_FreeInfo(DeviceInfo);

	return TRUE;
}

static BOOL KUSB_API l_DevEnum_Clone_All(
    __in KLST_HANDLE SrcDeviceList,
    __in KLST_DEVINFO* DeviceInfo,
    __in KLST_HANDLE DstDeviceList)
{
	KLST_DEVINFO* ClonedDevInfo = NULL;

	UNREFERENCED_PARAMETER(SrcDeviceList);

	if (LstK_CloneInfo(DeviceInfo, &ClonedDevInfo))
	{
		LstK_AttachInfo(DstDeviceList, ClonedDevInfo);
		return TRUE;
	}

	return FALSE;
}

static BOOL KUSB_API l_DevEnum_Apply_Filter(
    __in KLST_HANDLE DeviceList,
    __in KLST_DEVINFO* DeviceInfo,
    __in KLST_FLAG* Flags)
{
	if (!(*Flags & KLST_FLAG_INCLUDE_RAWGUID))
	{
		if (_stricmp(DeviceInfo->DeviceInterfaceGUID, RawDeviceGuidA) == 0)
		{
			if (LstK_DetachInfo(DeviceList, DeviceInfo))
				LstK_FreeInfo(DeviceInfo);

			return TRUE;
		}
	}

	PoolHandle_Live_LstInfoK(((KLST_DEVINFO_EL*)DeviceInfo)->DevInfoHandle);
	return TRUE;
}

static void KUSB_API Cleanup_DeviceList(__in PKLST_HANDLE_INTERNAL handle)
{
	PoolHandle_Dead_LstK(handle);
	LstK_Enumerate((KLST_HANDLE)handle, l_DevEnum_Free_All, NULL);
}

static void KUSB_API Cleanup_DevInfo(__in PKLST_DEVINFO_HANDLE_INTERNAL handle)
{
	PoolHandle_Dead_LstInfoK(handle);
	Mem_Free(&handle->DevInfoEL);
}

static BOOL KUSB_API l_DevEnum_SyncPrep(
    __in KLST_HANDLE DeviceList,
    __in KLST_DEVINFO* DeviceInfo,
    __in KLST_SYNC_CONTEXT* Context)
{
	UNREFERENCED_PARAMETER(DeviceList);
	UNREFERENCED_PARAMETER(Context);

	DeviceInfo->SyncFlags = KLST_SYNC_FLAG_NONE;

	return TRUE;
}

static BOOL KUSB_API l_DevEnum_Sync_Master(
    __in KLST_HANDLE DeviceList,
    __in KLST_DEVINFO* DeviceInfo,
    __in KLST_SYNC_CONTEXT* Context)
{
	PKLST_DEVINFO_EL masterDevInfo = (PKLST_DEVINFO_EL)DeviceInfo;
	PKLST_DEVINFO_EL slaveDevInfo;

	UNREFERENCED_PARAMETER(DeviceList);

	// Skip elements already processed by previous sync operations.
	if (masterDevInfo->Public.SyncFlags != KLST_SYNC_FLAG_NONE) return TRUE;
	DL_SEARCH(Context->Slave->head, slaveDevInfo, masterDevInfo, lstk_DL_MatchSymbolicLink);
	if (slaveDevInfo)
	{
		// This element exists in the slave and master list.
		memcpy(masterDevInfo->Public.DevicePath, slaveDevInfo->Public.DevicePath, KLST_STRING_MAX_LEN);
		if (slaveDevInfo->Public.Connected != masterDevInfo->Public.Connected)
		{
			// Connected/Disconnected.
			masterDevInfo->Public.SyncFlags |= (KLST_SYNC_FLAG_CONNECT_CHANGE & Context->SyncFlags);
			masterDevInfo->Public.Connected = slaveDevInfo->Public.Connected;
			if (slaveDevInfo->Public.Connected)
				masterDevInfo->Public.SyncFlags |= (KLST_SYNC_FLAG_ADDED & Context->SyncFlags);
			else
				masterDevInfo->Public.SyncFlags |= (KLST_SYNC_FLAG_REMOVED & Context->SyncFlags);
		}
		else
		{
			// Unchanged.
			masterDevInfo->Public.SyncFlags = KLST_SYNC_FLAG_UNCHANGED;
		}

	}
	else
	{
		// This element exists in the master list only.
		if (masterDevInfo->Public.Connected)
		{
			masterDevInfo->Public.SyncFlags |= (KLST_SYNC_FLAG_REMOVED & Context->SyncFlags);
			if (!masterDevInfo->Public.SyncFlags) masterDevInfo->Public.SyncFlags = KLST_SYNC_FLAG_UNCHANGED;
			masterDevInfo->Public.Connected = FALSE;
		}
		else
		{
			masterDevInfo->Public.SyncFlags = KLST_SYNC_FLAG_UNCHANGED;
		}

		// LstK_DetachInfo(DeviceList, DeviceInfo);
		// LstK_FreeInfo(DeviceInfo);
	}

	return TRUE;
}

static BOOL KUSB_API l_DevEnum_Sync_Slave(
    __in KLST_HANDLE DeviceList,
    __in KLST_DEVINFO* DeviceInfo,
    __in KLST_SYNC_CONTEXT* Context)
{
	PKLST_DEVINFO_EL slaveDevInfo = (PKLST_DEVINFO_EL)DeviceInfo;
	PKLST_DEVINFO_EL masterDevInfo;

	UNREFERENCED_PARAMETER(DeviceList);

	DL_SEARCH(Context->Master->head, masterDevInfo, slaveDevInfo, lstk_DL_MatchSymbolicLink);
	if (masterDevInfo)
	{
		// Skip elements already processed by previous sync operations.
		if (masterDevInfo->Public.SyncFlags != KLST_SYNC_FLAG_NONE) return TRUE;

		// always use the new DevicePath from the slave list
		memcpy(masterDevInfo->Public.DevicePath, slaveDevInfo->Public.DevicePath, KLST_STRING_MAX_LEN);

		if (slaveDevInfo->Public.Connected != masterDevInfo->Public.Connected)
		{
			// Connected/Disconnected.
			masterDevInfo->Public.SyncFlags |= (KLST_SYNC_FLAG_CONNECT_CHANGE & Context->SyncFlags);
			masterDevInfo->Public.Connected = slaveDevInfo->Public.Connected;
			if (slaveDevInfo->Public.Connected)
				masterDevInfo->Public.SyncFlags |= (KLST_SYNC_FLAG_ADDED & Context->SyncFlags);
			else
				masterDevInfo->Public.SyncFlags |= (KLST_SYNC_FLAG_REMOVED & Context->SyncFlags);
		}
		else
		{
			// Unchanged.
			masterDevInfo->Public.SyncFlags = KLST_SYNC_FLAG_UNCHANGED;
		}
	}
	else
	{
		if ((KLST_SYNC_FLAG_ADDED & Context->SyncFlags))
		{
			// Added.
			if (LstK_CloneInfo((KLST_DEVINFO*)slaveDevInfo, (KLST_DEVINFO**)&masterDevInfo))
			{
				LstK_AttachInfo((KLST_HANDLE)Context->Master, (KLST_DEVINFO*)masterDevInfo);
				if (slaveDevInfo->Public.Connected)
					masterDevInfo->Public.SyncFlags = KLST_SYNC_FLAG_ADDED;
				else
					masterDevInfo->Public.SyncFlags = KLST_SYNC_FLAG_UNCHANGED;
			}
		}
	}

	return TRUE;
}

static BOOL l_Alloc_DevInfo(__out PKLST_DEVINFO_HANDLE_INTERNAL* handleRef)
{
	PKLST_DEVINFO_HANDLE_INTERNAL handle;

	*handleRef = NULL;

	handle = PoolHandle_Acquire_LstInfoK(Cleanup_DevInfo);
	ErrorNoSetAction(!IsHandleValid(handle), return FALSE, "->PoolHandle_Acquire_LstInfoK");

	handle->DevInfoEL = Mem_Alloc(sizeof(*handle->DevInfoEL));
	ErrorMemory(!IsHandleValid(handle->DevInfoEL), Error);

	handle->DevInfoEL->DevInfoHandle = handle;
	*handleRef = handle;
	return TRUE;

Error:
	if (handle) PoolHandle_Dec_LstInfoK(handle);
	return FALSE;
}

static DWORD l_Build_AddElement(
    PKLST_HANDLE_INTERNAL DeviceList,
    KLST_DEVINFO_HANDLE SrcDevInfo,
    PKLST_DEVINFO_EL* clonedDevInfo,
    PBOOL IsNew)
{
	PKLST_DEVINFO_HANDLE_INTERNAL clonedHandle = NULL;
	KLST_DEVINFO_EL* existingEntry;

	*clonedDevInfo = NULL;

	DL_SEARCH(DeviceList->head, existingEntry, SrcDevInfo->SymbolicLink, Match_DevItem_SymbolicLink);
	if (existingEntry)
	{
		*IsNew = FALSE;
		*clonedDevInfo = existingEntry;
		return ERROR_SUCCESS;
	}

	if (!l_Alloc_DevInfo(&clonedHandle)) goto Error;
	memcpy(&clonedHandle->DevInfoEL->Public, SrcDevInfo, sizeof(clonedHandle->DevInfoEL->Public));

	// LstInfoK + 1
	PoolHandle_Inc_LstInfoK(clonedHandle);

	clonedHandle->DevInfoEL->DevListHandle = DeviceList;
	clonedHandle->DevInfoEL->DevInfoHandle = clonedHandle;
	DL_APPEND(DeviceList->head, clonedHandle->DevInfoEL);

	*IsNew = TRUE;
	*clonedDevInfo = clonedHandle->DevInfoEL;
	return ERROR_SUCCESS;

Error:
	return GetLastError();
}

static BOOL l_Build_AssignDriver(KLST_DEVINFO_HANDLE devItem)
{

	PSERVICE_DRVID_MAP map;

	// find driver type by device interface guid
	devItem->DrvId = KUSB_DRVID_INVALID;
	map = (PSERVICE_DRVID_MAP)DevGuidDrvIdMap;
	while(devItem->DrvId == KUSB_DRVID_INVALID && map->DrvId != KUSB_DRVID_INVALID)
	{
		LPCSTR* devGuid = map->MapNames;
		while(*devGuid)
		{
			if (_stricmp(devItem->DeviceInterfaceGUID, *devGuid) == 0)
			{
				devItem->DrvId = map->DrvId;
				break;
			}
			devGuid++;
		}
		map++;
	}

	// find driver type by service name (if not found)
	map = (PSERVICE_DRVID_MAP)ServiceDrvIdMap;
	while(devItem->DrvId == KUSB_DRVID_INVALID && map->DrvId != KUSB_DRVID_INVALID)
	{
		LPCSTR* serviceName = map->MapNames;
		while(*serviceName)
		{
			if (_stricmp(devItem->Service, *serviceName) == 0)
			{
				devItem->DrvId = map->DrvId;
				break;
			}
			serviceName++;
		}
		map++;
	}

	if (devItem->DrvId != KUSB_DRVID_INVALID)
	{
		if (devItem->DrvId == KUSB_DRVID_LIBUSB0_FILTER ||
		        devItem->DrvId == KUSB_DRVID_LIBUSB0 )
		{
			if (devItem->LUsb0FilterIndex <= 255)
			{
				sprintf_s(devItem->DevicePath, sizeof(devItem->DevicePath) - 1,
				          "\\\\.\\libusb0-%04d", devItem->LUsb0FilterIndex);

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

static BOOL l_Build_FillProperties(KLST_DEVINFO_HANDLE devItem)
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
	strcat_s(devRegEnumKey, sizeof(devRegEnumKey), devItem->InstanceID);

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

		if (GetRegDevNodeString(devItem, valueName, ClassGUID, valueData)) {}
		else if (GetRegDevNodeString(devItem, valueName, DeviceDesc, valueData)) {}
		else if (GetRegDevNodeString(devItem, valueName, Mfg, valueData)) {}
		else if (GetRegDevNodeString(devItem, valueName, Service, valueData)) {}

		memset(valueName, 0, sizeof(valueName));
		memset(valueData, 0, sizeof(valueData));
		valueNameLength = sizeof(valueName) - 1;
		valueDataLength = sizeof(valueData) - 1;
	}

	RegCloseKey(hDevRegEnumKey);

	return (strlen(devItem->Service) > 0);
}

static void l_Build_Common_Info(__in PKLST_DEVINFO_EL devItem)
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
				USBWRNN("Failed scanning vid/pid into common info.");
			break;
		case 2:	// InstanceID
			strcpy_s(commonInfo->InstanceID, sizeof(commonInfo->InstanceID), chLast);
			break;
		default:
			USBWRNN("Unknown device instance id element: %s.", chLast);
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

static BOOL l_EnumKey_Instances(LPCSTR Name, KUSB_ENUM_REGKEY_PARAMS* RegEnumParams)
{
	LONG status;
	DWORD referenceCount;
	BOOL isNewItem;
	PKLST_DEVINFO_EL newDevItem = NULL;

	if (!l_IsUsb_RegKey(Name)) return TRUE;

	Mem_Zero(RegEnumParams->TempItem, sizeof(*RegEnumParams->TempItem));

	strcpy_s(RegEnumParams->TempItem->DeviceInterfaceGUID, sizeof(RegEnumParams->TempItem->DeviceInterfaceGUID), RegEnumParams->CurrentGUID);

	// query reference count (connected device instance id count)
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\Control\ReferenceCount
	status = l_GetReg_DWord(RegEnumParams->hOpenedKey, Name, "\\Control", "ReferenceCount", &referenceCount);
	if (status != ERROR_SUCCESS)
		return TRUE;

	if (!referenceCount && (!(RegEnumParams->Flags & KLST_FLAG_INCLUDE_DISCONNECT)))
		return TRUE;

	RegEnumParams->TempItem->Connected = referenceCount > 0;

	// query device instance id
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\DeviceInstance
	status = l_GetReg_String(RegEnumParams->hOpenedKey, Name, NULL, "DeviceInstance", RegEnumParams->TempItem->InstanceID);
	if (status != ERROR_SUCCESS)
		return TRUE;

	// query symbolic link value
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\#\SymbolicLink
	status = l_GetReg_String(RegEnumParams->hOpenedKey, Name, "\\#", "SymbolicLink",  RegEnumParams->TempItem->SymbolicLink);
	if (status != ERROR_SUCCESS)
		return TRUE;

	// DevicePath is equal to SymbolicLink unless it ends up being a libusb0 filter device
	strcpy_s(RegEnumParams->TempItem->DevicePath, sizeof(RegEnumParams->TempItem->DevicePath), RegEnumParams->TempItem->SymbolicLink);

	// query LUsb0
	RegEnumParams->TempItem->LUsb0FilterIndex = (ULONG) - 1;
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\#\Device Parameters\LUsb0
	status = l_GetReg_DWord(RegEnumParams->hOpenedKey, Name, "\\#\\Device Parameters", "LUsb0", &RegEnumParams->TempItem->LUsb0FilterIndex);

	if (!l_Build_FillProperties(RegEnumParams->TempItem))
		return TRUE;

	if (!l_Build_AssignDriver(RegEnumParams->TempItem))
		return TRUE;

	RegEnumParams->ErrorCode = l_Build_AddElement(RegEnumParams->DeviceList, RegEnumParams->TempItem, &newDevItem, &isNewItem);
	if (RegEnumParams->ErrorCode != ERROR_SUCCESS)
		return FALSE;

	if (isNewItem)
		l_Build_Common_Info(newDevItem);

	return TRUE;
}

static BOOL l_EnumKey_Guids(LPCSTR Name, KUSB_ENUM_REGKEY_PARAMS* RegEnumParams)
{
	// enumeration device interface instances
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}

	KUSB_ENUM_REGKEY_PARAMS enumParamsInterfaceGUIDs;
	BOOL success;

	memcpy(&enumParamsInterfaceGUIDs, RegEnumParams, sizeof(enumParamsInterfaceGUIDs));

	enumParamsInterfaceGUIDs.EnumRegKeyCB	= l_EnumKey_Instances;
	enumParamsInterfaceGUIDs.hParentKey		= RegEnumParams->hOpenedKey;
	enumParamsInterfaceGUIDs.SubKey			= Name;
	enumParamsInterfaceGUIDs.CurrentGUID    = Name;
	enumParamsInterfaceGUIDs.TempItem		= RegEnumParams->TempItem;

	success = l_Enum_RegKey(&enumParamsInterfaceGUIDs);
	RegEnumParams->ErrorCode = enumParamsInterfaceGUIDs.ErrorCode;

	return success;
}

KUSB_EXP BOOL KUSB_API LstK_Count(
    _in KLST_HANDLE DeviceList,
    _ref PULONG Count)
{
	PKLST_HANDLE_INTERNAL handle;
	PKLST_DEVINFO_EL nextItem = NULL;
	ULONG deviceCount = 0;

	ErrorParamAction(!IsHandleValid(Count), "Count", return FALSE);
	*Count = 0;

	Pub_To_Priv_LstK(DeviceList, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_LstK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_LstK");

	DL_FOREACH(handle->head, nextItem)
	{
		deviceCount++;
	}
	*Count = deviceCount;

	PoolHandle_Dec_LstK(handle);
	return TRUE;
}

KUSB_EXP BOOL KUSB_API LstK_Free(
    _in KLST_HANDLE DeviceList)
{
	PKLST_HANDLE_INTERNAL handle;

	Pub_To_Priv_LstK(DeviceList, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_LstK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_LstK");

	PoolHandle_Dec_LstK(handle);
	PoolHandle_Dec_LstK(handle);
	return TRUE;
}

KUSB_EXP BOOL KUSB_API LstK_Init(
    _out KLST_HANDLE* DeviceList,
    _in KLST_FLAG Flags)
{
	KUSB_ENUM_REGKEY_PARAMS enumParams;
	BOOL success = FALSE;
	PKLST_HANDLE_INTERNAL handle = NULL;
	KLST_DEVINFO TempItem;

	handle = PoolHandle_Acquire_LstK(&Cleanup_DeviceList);
	ErrorNoSetAction(!IsHandleValid(handle), return FALSE, "->PoolHandle_Acquire_LstK");

	Mem_Zero(&enumParams, sizeof(enumParams));

	Mem_Zero(&TempItem, sizeof(TempItem));
	enumParams.DeviceList		= handle;
	enumParams.Flags			= Flags;
	enumParams.EnumRegKeyCB		= l_EnumKey_Guids;
	enumParams.hParentKey		= HKEY_LOCAL_MACHINE;
	enumParams.SubKey			= KEY_DEVICECLASSES;
	enumParams.TempItem			= &TempItem;

	// enumerate device interface guids
	// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}
	if (l_Enum_RegKey(&enumParams))
	{
		if (handle->head)
		{
			LstK_Enumerate((KLST_HANDLE)enumParams.DeviceList, l_DevEnum_Apply_Filter, &enumParams.Flags);
		}

		success = TRUE;
	}

	if (success)
	{
		*DeviceList = (KLST_HANDLE)handle;
		PoolHandle_Live_LstK(handle);
	}
	else
	{
		*DeviceList = NULL;
		if (handle) PoolHandle_Dec_LstK(handle);
	}
	return success;
}

KUSB_EXP BOOL KUSB_API LstK_MoveNext(
    _in KLST_HANDLE DeviceList,
    _outopt KLST_DEVINFO_HANDLE* DeviceInfo)
{
	PKLST_HANDLE_INTERNAL handle;

	if (DeviceInfo) *DeviceInfo = NULL;

	Pub_To_Priv_LstK(DeviceList, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_LstK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_LstK");

	if (!handle->current)
	{
		if (!handle->head)
		{
			SetLastError(ERROR_EMPTY);
			PoolHandle_Dec_LstK(handle);
			return FALSE;
		}
		handle->current = handle->head;
		*DeviceInfo = (KLST_DEVINFO*)handle->current;
		PoolHandle_Dec_LstK(handle);
		return TRUE;
	}

	if ((handle->current = handle->current->next) == NULL)
	{
		SetLastError(ERROR_NO_MORE_ITEMS);
		PoolHandle_Dec_LstK(handle);
		return FALSE;
	}

	if (DeviceInfo)
		*DeviceInfo = (KLST_DEVINFO*)handle->current;

	PoolHandle_Dec_LstK(handle);
	return TRUE;
}

KUSB_EXP BOOL KUSB_API LstK_Current(
    _in KLST_HANDLE DeviceList,
    _out KLST_DEVINFO_HANDLE* DeviceInfo)
{
	PKLST_HANDLE_INTERNAL handle;

	Pub_To_Priv_LstK(DeviceList, handle, return FALSE);

	ErrorSetAction(!PoolHandle_Inc_LstK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_LstK");

	if (!handle->current)
	{
		SetLastError(ERROR_NO_MORE_ITEMS);
		PoolHandle_Dec_LstK(handle);
		return FALSE;
	}
	*DeviceInfo = (KLST_DEVINFO*)handle->current;

	PoolHandle_Dec_LstK(handle);
	return TRUE;
}

KUSB_EXP VOID KUSB_API LstK_MoveReset(
    _in KLST_HANDLE DeviceList)
{
	PKLST_HANDLE_INTERNAL handle;

	Pub_To_Priv_LstK(DeviceList, handle, return);

	ErrorSetAction(!PoolHandle_Inc_LstK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return, "->PoolHandle_Inc_LstK");

	handle->current = NULL;

	PoolHandle_Dec_LstK(handle);
	return;
}

KUSB_EXP BOOL KUSB_API LstK_Enumerate(
    _in KLST_HANDLE DeviceList,
    _in PKLST_ENUM_DEVINFO_CB EnumDevListCB,
    _inopt PVOID Context)
{
	PKLST_DEVINFO_EL check, tmp;
	PKLST_HANDLE_INTERNAL handle;

	Pub_To_Priv_LstK(DeviceList, handle, return FALSE);
	ErrorParam(!EnumDevListCB, Error, "EnumDevListCB");

	ErrorSetAction(!PoolHandle_Inc_LstK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_LstK");

	DL_FOREACH_SAFE(handle->head, check, tmp)
	{
		if (EnumDevListCB(DeviceList, (KLST_DEVINFO*)check, Context) == FALSE)
			break;
	}

	PoolHandle_Dec_LstK(handle);
	return TRUE;

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_FindByVidPid(
    _in KLST_HANDLE DeviceList,
    _in UINT Vid,
    _in UINT Pid,
    _out KLST_DEVINFO_HANDLE* DeviceInfo)
{
	PKLST_DEVINFO_EL check = NULL;
	PKLST_HANDLE_INTERNAL handle;

	Pub_To_Priv_LstK(DeviceList, handle, return FALSE);
	ErrorParam(!DeviceInfo, Error, "DeviceInfo");

	ErrorSetAction(!PoolHandle_Inc_LstK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_LstK");

	DL_FOREACH(handle->head, check)
	{
		if ((check->Public.Common.Vid & 0xFFFF) == (Vid & 0xFFFF) &&
		        (check->Public.Common.Pid & 0xFFFF) == (Pid & 0xFFFF))
			break;
	}

	*DeviceInfo = (KLST_DEVINFO*)check;
	if (!check)
	{
		PoolHandle_Dec_LstK(handle);
		return LusbwError(ERROR_NO_MORE_ITEMS);
	}

	return PoolHandle_Dec_LstK(handle);

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_Clone(
    _in KLST_HANDLE SrcList,
    _out KLST_HANDLE* DstList)
{
	PKLST_HANDLE_INTERNAL handle;
	PKLST_HANDLE_INTERNAL newDeviceList = NULL;
	BOOL success;

	Pub_To_Priv_LstK(SrcList, handle, return FALSE);
	ErrorParamAction(!IsHandleValid(DstList), "dst", return FALSE);
	ErrorSetAction(!PoolHandle_Inc_LstK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_LstK");

	newDeviceList = PoolHandle_Acquire_LstK(Cleanup_DeviceList);
	ErrorNoSet(!IsHandleValid(newDeviceList), Error, "->PoolHandle_Acquire_LstK");

	success = LstK_Enumerate(SrcList, l_DevEnum_Clone_All, newDeviceList);

	*DstList = (KLST_HANDLE)newDeviceList;
	PoolHandle_Dec_LstK(handle);
	return success;
Error:
	if (newDeviceList) PoolHandle_Dec_LstK(newDeviceList);
	PoolHandle_Dec_LstK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_Sync(
    _in KLST_HANDLE MasterList,
    _in KLST_HANDLE SlaveList,
    _inopt KLST_SYNC_FLAG SyncFlags)
{
	KLST_SYNC_CONTEXT context;

	Mem_Zero(&context, sizeof(context));

	Pub_To_Priv_LstK(MasterList, context.Master, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_LstK(context.Master), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_LstK");
	context.SyncFlags = SyncFlags;

	if (!context.SyncFlags) context.SyncFlags = KLST_SYNC_FLAG_MASK;
	if (!SlaveList)
	{
		// Use a new list for the slave list
		KLST_FLAG slaveListInit = KLST_FLAG_INCLUDE_DISCONNECT;
		ErrorNoSetAction(!LstK_Init(&SlaveList, slaveListInit), goto Error_IncRefSlave, "->PoolHandle_Inc_LstK");
		context.Slave = (PKLST_HANDLE_INTERNAL)SlaveList;
	}
	else
	{
		Pub_To_Priv_LstK(SlaveList, context.Slave, goto Error_IncRefSlave);
		ErrorNoSetAction(!PoolHandle_Inc_LstK(context.Slave), goto Error_IncRefSlave, "->PoolHandle_Inc_LstK");
	}

	LstK_Enumerate((KLST_HANDLE)context.Master, l_DevEnum_SyncPrep, &context);
	LstK_Enumerate((KLST_HANDLE)context.Slave, l_DevEnum_Sync_Slave, &context);
	LstK_Enumerate((KLST_HANDLE)context.Master, l_DevEnum_Sync_Master, &context);

	PoolHandle_Dec_LstK(context.Master);
	PoolHandle_Dec_LstK(context.Slave);
	return TRUE;

Error_IncRefSlave:
	if (context.Master)
		PoolHandle_Dec_LstK(context.Master);
	if (context.Slave)
		PoolHandle_Dec_LstK(context.Slave);

	return FALSE;

}

KUSB_EXP BOOL KUSB_API LstK_CloneInfo(
    _in KLST_DEVINFO_HANDLE SrcInfo,
    _out KLST_DEVINFO_HANDLE* DstInfo)
{
	PKLST_DEVINFO_HANDLE_INTERNAL clonedHandle = NULL;
	PKLST_DEVINFO_HANDLE_INTERNAL handle;

	ErrorParamAction(!IsHandleValid(SrcInfo), "SrcInfo", return FALSE);
	ErrorParamAction(!IsHandleValid(DstInfo), "DstInfo", return FALSE);

	Pub_To_Priv_LstInfoK(((PKLST_DEVINFO_EL)SrcInfo)->DevInfoHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_LstInfoK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_LstInfoK");

	ErrorNoSet(!l_Alloc_DevInfo(&clonedHandle), Error, "->LstK_CloneInfo");
	memcpy(&clonedHandle->DevInfoEL->Public, &handle->DevInfoEL->Public, sizeof(clonedHandle->DevInfoEL->Public));

	clonedHandle->DevInfoEL->DevListHandle = NULL;
	*DstInfo = (KLST_DEVINFO*)clonedHandle->DevInfoEL;
	PoolHandle_Dec_LstInfoK(handle);
	PoolHandle_Live_LstInfoK(clonedHandle);
	return TRUE;

Error:
	if (clonedHandle)
		PoolHandle_Dec_LstInfoK(clonedHandle);
	PoolHandle_Dec_LstInfoK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_DetachInfo(
    _in KLST_HANDLE DeviceList,
    _in KLST_DEVINFO_HANDLE DeviceInfo)
{
	PKLST_HANDLE_INTERNAL handle;
	PKLST_DEVINFO_HANDLE_INTERNAL devInfoHandle;
	PKLST_DEVINFO_EL deviceInfo = (PKLST_DEVINFO_EL)DeviceInfo;
	BOOL success = FALSE;

	ErrorParamAction(!IsHandleValid(deviceInfo), "DeviceInfo", return FALSE);

	Pub_To_Priv_LstK(DeviceList, handle, return FALSE);
	Pub_To_Priv_LstInfoK(deviceInfo->DevInfoHandle, devInfoHandle, return FALSE);

	ErrorSetAction(!PoolHandle_Inc_LstK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_LstK");
	ErrorSet(!PoolHandle_Inc_LstInfoK(devInfoHandle), Error_IncRef_DevInfo, ERROR_RESOURCE_NOT_AVAILABLE, "->PoolHandle_Inc_LstInfoK");

	if (InterlockedCompareExchangePointer((volatile PVOID*)&deviceInfo->DevListHandle, (PVOID)NULL, DeviceList) == DeviceList)
		success = TRUE;

	ErrorSet(!success, Done, ERROR_ACCESS_DENIED, "DeviceInfo does not exist in DeviceList");

	DL_DELETE(handle->head, deviceInfo);

	// LstInfoK - 1
	PoolHandle_Dec_LstInfoK(devInfoHandle);

Done:
	PoolHandle_Dec_LstInfoK(devInfoHandle);
	PoolHandle_Dec_LstK(handle);
	return success;

Error_IncRef_DevInfo:
	PoolHandle_Dec_LstK(handle);
	return FALSE;

}

KUSB_EXP BOOL KUSB_API LstK_AttachInfo(
    _in KLST_HANDLE DeviceList,
    _in KLST_DEVINFO_HANDLE DeviceInfo)
{
	PKLST_HANDLE_INTERNAL handle;
	PKLST_DEVINFO_HANDLE_INTERNAL devInfoHandle;
	PKLST_DEVINFO_EL deviceInfo = (PKLST_DEVINFO_EL)DeviceInfo;
	BOOL success = FALSE;

	ErrorParamAction(!IsHandleValid(deviceInfo), "DeviceInfo", return FALSE);

	Pub_To_Priv_LstK(DeviceList, handle, return FALSE);
	Pub_To_Priv_LstInfoK(deviceInfo->DevInfoHandle, devInfoHandle, return FALSE);

	ErrorSetAction(!PoolHandle_Inc_LstK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_LstK");
	ErrorSet(!PoolHandle_Inc_LstInfoK(devInfoHandle), Error_IncRef_DevInfo, ERROR_RESOURCE_NOT_AVAILABLE, "->PoolHandle_Inc_LstInfoK");

	if (InterlockedCompareExchangePointer((volatile PVOID*)&deviceInfo->DevListHandle, DeviceList, NULL) == NULL)
		success = TRUE;

	ErrorSet(!success, Done, ERROR_ACCESS_DENIED, "DeviceInfo already attached");

	DL_APPEND(handle->head, deviceInfo);

	// LstInfoK + 1 (above)
	PoolHandle_Dec_LstK(handle);
	return TRUE;

Done:
	PoolHandle_Dec_LstInfoK(devInfoHandle);
	PoolHandle_Dec_LstK(handle);
	return success;

Error_IncRef_DevInfo:
	PoolHandle_Dec_LstK(handle);
	return FALSE;

}

KUSB_EXP BOOL KUSB_API LstK_FreeInfo(
    _in KLST_DEVINFO_HANDLE DeviceInfo)
{
	PKLST_DEVINFO_EL deviceInfo = (PKLST_DEVINFO_EL) DeviceInfo;
	PKLST_DEVINFO_HANDLE_INTERNAL devInfoHandle;

	ErrorParamAction(!IsHandleValid(DeviceInfo), "DeviceInfo", return FALSE);
	Pub_To_Priv_LstInfoK(deviceInfo->DevInfoHandle, devInfoHandle, return FALSE);

	if (PoolHandle_Inc_LstInfoK(devInfoHandle))
	{
		if (InterlockedCompareExchangePointer((volatile PVOID*)&devInfoHandle->DevInfoEL->DevListHandle, NULL, NULL) == NULL)
		{
			PoolHandle_Dec_LstInfoK(devInfoHandle);
			PoolHandle_Dec_LstInfoK(devInfoHandle);
			return TRUE;
		}
		PoolHandle_Dec_LstInfoK(devInfoHandle);
	}

	ErrorSetAction(TRUE, ERROR_ACCESS_DENIED, return FALSE, "DeviceInfo is attached to a device list.");
}
