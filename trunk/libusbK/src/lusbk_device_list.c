/*!********************************************************************
libusbK - Multi-driver USB library.
Copyright (C) 2012 Travis Lee Robinson. All Rights Reserved.
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
#include <setupapi.h>

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#define mLst_DL_Match_SymbolicLink(PatternEL, MatchEL) (PathMatchSpec((MatchEL)->Public.SymbolicLink, (PatternEL)->Public.SymbolicLink)?0:-1)

#define KEY_DEVICECLASSES "SYSTEM\\CurrentControlSet\\Control\\DeviceClasses"

#define mLst_Get_InterfaceDetail(mRegEnumParamsPtr, mhDevInfo_Interface, mOutDevInfoDataPtr, mOutDevicePathLen, mErrorAction)do { \
		/* This aligns the 'DevicePath' in PSP_DEVICE_INTERFACE_DETAIL_DATA to mRegEnumParamsPtr->TempItem->DevicePath */													\
		PSP_DEVICE_INTERFACE_DETAIL_DATA m_pDevInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)((&(mRegEnumParamsPtr)->TempItem->DevicePath[0]) - sizeof(DWORD));	\
		m_pDevInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA); 																						\
		mOutDevicePathLen = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + sizeof((mRegEnumParamsPtr)->TempItem->DevicePath) - 2; 												\
 																																											\
		/* Get the DevicePath/SymbolicLink */																																\
		if (!SetupDiGetDeviceInterfaceDetailA(   																															\
					mhDevInfo_Interface, 																																	\
					&(mRegEnumParamsPtr)->DevInterfaceData,  																												\
					m_pDevInterfaceDetailData,   																															\
					mOutDevicePathLen,   																																	\
					&mOutDevicePathLen,  																																	\
					mOutDevInfoDataPtr)) 																																	\
		{																																									\
			USBERRN("SetupDiGetDeviceInterfaceDetail Failed. ErrorCode:%08Xh", GetLastError());  																			\
			{mErrorAction;}  																																				\
		}																																									\
		 																																									\
		memset((mRegEnumParamsPtr)->TempItem->DevicePath - sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA), 0, sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA)); 							\
		mOutDevicePathLen = (DWORD)strlen((mRegEnumParamsPtr)->TempItem->DevicePath);																						\
		memcpy((mRegEnumParamsPtr)->TempItem->SymbolicLink, (mRegEnumParamsPtr)->TempItem->DevicePath, mOutDevicePathLen);   												\
		(mRegEnumParamsPtr)->TempItem->DevicePath[mOutDevicePathLen] = (CHAR)0;  																							\
		(mRegEnumParamsPtr)->TempItem->SymbolicLink[mOutDevicePathLen] = (CHAR)0;																							\
}																																											\
while(0)

#define mLst_HandleConnectScenario(mRegEnumParamsPtr, mErrorAction)do { \
	if ((mRegEnumParamsPtr)->DevInterfaceData.Flags & SPINT_REMOVED)   																	\
	{  																																	\
		mErrorAction;  																													\
	}  																																	\
	/* If we are not including disconnected devices and the driver has not enabled the interface guid for this devices then skip it. */	\
	(mRegEnumParamsPtr)->TempItem->Connected = ((mRegEnumParamsPtr)->DevInterfaceData.Flags & SPINT_ACTIVE) ? TRUE : FALSE;				\
	if ((mRegEnumParamsPtr)->TempItem->Connected == FALSE && !((mRegEnumParamsPtr)->Flags & KLST_FLAG_INCLUDE_DISCONNECT)) 				\
	{  																																	\
		mErrorAction;  																													\
	}  																																	\
}  																																		\
while(0)

typedef struct _KUSB_ENUM_REGKEY_PARAMS
{
	// assigned by LstK_Init, copied by sub enumeration routines
	// (global)
	PKLST_HANDLE_INTERNAL DeviceList;

	// Heap used for device info allocation
	HANDLE Heap;

	KLST_FLAG Flags;
	KLST_PATTERN_MATCH* PatternMatch;

	GUID DevInterfaceGuid;
	GUID ClassGuid;

	// optional
	PVOID Context;
	DWORD ErrorCode;

	struct
	{
		BOOL DevInterfaceGuid;
		BOOL ClassGuid;
	} Exclusive;

	KLST_DEVINFO_HANDLE TempItem;

	// SetupAPI
	HDEVINFO DevInfoSet;
	SP_DEVINFO_DATA DevInfoData;
	SP_DEVICE_INTERFACE_DATA DevInterfaceData;
	DWORD DigcFlags;

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
	INT DriverID;
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

#define  mLst_Assign_DrvId_From_Map(mOutDrvId, mInDrvMapValueToFind, mInServiceDrvIdMap) {		\
	PSERVICE_DRVID_MAP _serviceMap = (PSERVICE_DRVID_MAP)mInServiceDrvIdMap;   					\
	while((mOutDrvId) == -1 && _serviceMap->DriverID != -1)  									\
	{																							\
		LPCSTR* _mapName = _serviceMap->MapNames;												\
		while(*_mapName) 																		\
		{																						\
			if (_stricmp((mInDrvMapValueToFind), *_mapName) == 0)								\
			{																					\
				(mOutDrvId) = _serviceMap->DriverID; 											\
				break;   																		\
			}																					\
			_mapName++;  																		\
		}																						\
		_serviceMap++;   																		\
	}																							\
}																								\
 
static const SERVICE_DRVID_MAP DrvIdMap_Services[] =
{
	{KUSB_DRVID_LIBUSBK,	lusbk_Services},
	{KUSB_DRVID_LIBUSB0,	lusb0_Services},
	{KUSB_DRVID_WINUSB,		wusb_Services},

	{ -1,	NULL}
};
#define  mLst_Assign_DrvId_From_Service(mOutDrvId, mInServiceToFind) mLst_Assign_DrvId_From_Map(mOutDrvId, mInServiceToFind, DrvIdMap_Services)


static const SERVICE_DRVID_MAP DevGuidDrvIdMap[] =
{
	{KUSB_DRVID_LIBUSBK,		lusbK_DevGuidNames},
	{KUSB_DRVID_LIBUSB0,		lusb0_DevGuidNames},
	{KUSB_DRVID_LIBUSB0_FILTER,	lusb0_FilterDevGuidNames},

	{ -1,	NULL}
};
#define  mLst_Assign_DrvId_From_InterfaceGuid(mOutDrvId, mInInterFaceGuidToFind) mLst_Assign_DrvId_From_Map(mOutDrvId, mInInterFaceGuidToFind, DevGuidDrvIdMap)

static BOOL KUSB_API l_DevEnum_Free_All(
    __in KLST_HANDLE DeviceList,
    __in KLST_DEVINFO_HANDLE DeviceInfo,
    __in KLST_SYNC_CONTEXT* Context)
{
	UNREFERENCED_PARAMETER(Context);

	LstK_DetachInfo(DeviceList, DeviceInfo);
	LstK_FreeInfo(DeviceInfo);

	return TRUE;
}

static BOOL KUSB_API l_DevEnum_Clone_All(
    __in KLST_HANDLE SrcDeviceList,
    __in KLST_DEVINFO_HANDLE DeviceInfo,
    __in KLST_HANDLE DstDeviceList)
{
	KLST_DEVINFO_HANDLE ClonedDevInfo = NULL;

	UNREFERENCED_PARAMETER(SrcDeviceList);

	if (LstK_CloneInfo(DeviceInfo, &ClonedDevInfo))
	{
		LstK_AttachInfo(DstDeviceList, ClonedDevInfo);
		return TRUE;
	}

	return FALSE;
}

static void KUSB_API Cleanup_DeviceList(__in PKLST_HANDLE_INTERNAL handle)
{
	PoolHandle_Dead_LstK(handle);
	LstK_Enumerate((KLST_HANDLE)handle, l_DevEnum_Free_All, NULL);
}

static void KUSB_API Cleanup_DevInfo(__in PKLST_DEVINFO_HANDLE_INTERNAL handle)
{
	PoolHandle_Dead_LstInfoK(handle);
	if (handle->Heap)
	{
		HeapFree(handle->Heap, 0, handle->DevInfoEL);
		handle->DevInfoEL = NULL;
		handle->Heap = NULL;
	}
	else
	{
		USBERRN("BugCheck! handle->Heap == NULL");
		Mem_Free(&handle->DevInfoEL);
	}
}

static BOOL KUSB_API l_DevEnum_SyncPrep(
    __in KLST_HANDLE DeviceList,
    __in KLST_DEVINFO_HANDLE DeviceInfo,
    __in KLST_SYNC_CONTEXT* Context)
{
	UNREFERENCED_PARAMETER(DeviceList);
	UNREFERENCED_PARAMETER(Context);

	DeviceInfo->SyncFlags = KLST_SYNC_FLAG_NONE;

	return TRUE;
}

static BOOL KUSB_API l_DevEnum_Sync_Master(
    __in KLST_HANDLE DeviceList,
    __in KLST_DEVINFO_HANDLE DeviceInfo,
    __in KLST_SYNC_CONTEXT* Context)
{
	PKLST_DEVINFO_EL masterDevInfo = (PKLST_DEVINFO_EL)DeviceInfo;
	PKLST_DEVINFO_EL slaveDevInfo;

	UNREFERENCED_PARAMETER(DeviceList);

	// Skip elements already processed by previous sync operations.
	if (masterDevInfo->Public.SyncFlags != KLST_SYNC_FLAG_NONE) return TRUE;
	DL_SEARCH(Context->Slave->head, slaveDevInfo, masterDevInfo, mLst_DL_Match_SymbolicLink);
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
	}

	return TRUE;
}

static BOOL KUSB_API l_DevEnum_Sync_Slave(
    __in KLST_HANDLE DeviceList,
    __in KLST_DEVINFO_HANDLE DeviceInfo,
    __in KLST_SYNC_CONTEXT* Context)
{
	PKLST_DEVINFO_EL slaveDevInfo = (PKLST_DEVINFO_EL)DeviceInfo;
	PKLST_DEVINFO_EL masterDevInfo;

	UNREFERENCED_PARAMETER(DeviceList);

	DL_SEARCH(Context->Master->head, masterDevInfo, slaveDevInfo, mLst_DL_Match_SymbolicLink);
	if (masterDevInfo)
	{
		// This element exists on both lists.

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
		// This element exists in the slave but not in the master
		if ((KLST_SYNC_FLAG_ADDED & Context->SyncFlags))
		{
			// Move it to the master list
			LstK_DetachInfo(DeviceList, (KLST_DEVINFO_HANDLE)slaveDevInfo);
			LstK_AttachInfo((KLST_HANDLE)Context->Master, (KLST_DEVINFO_HANDLE)slaveDevInfo);

			// Update 'SyncFlags'
			if (slaveDevInfo->Public.Connected)
				slaveDevInfo->Public.SyncFlags = KLST_SYNC_FLAG_ADDED;
			else
				slaveDevInfo->Public.SyncFlags = KLST_SYNC_FLAG_UNCHANGED;
		}
	}

	return TRUE;
}

static BOOL l_Alloc_DevInfo(_out PKLST_DEVINFO_HANDLE_INTERNAL* handleRef, _inopt HANDLE Heap)
{
	PKLST_DEVINFO_HANDLE_INTERNAL handle;

	*handleRef = NULL;

	handle = PoolHandle_Acquire_LstInfoK(Cleanup_DevInfo);
	ErrorNoSetAction(!IsHandleValid(handle), return FALSE, "->PoolHandle_Acquire_LstInfoK");

	if (Heap)
	{
		handle->DevInfoEL = HeapAlloc(Heap, HEAP_ZERO_MEMORY, sizeof(*handle->DevInfoEL));
		handle->Heap = Heap;
	}
	else
	{
		handle->DevInfoEL = Mem_Alloc(sizeof(*handle->DevInfoEL));
		handle->Heap = AllK->HeapDynamic;
	}

	ErrorMemory(!IsHandleValid(handle->DevInfoEL), Error);

	handle->DevInfoEL->DevInfoHandle = handle;
	*handleRef = handle;
	return TRUE;

Error:
	if (handle) PoolHandle_Dec_LstInfoK(handle);
	return FALSE;
}

static DWORD l_Build_AddElement(
    KUSB_ENUM_REGKEY_PARAMS* RegEnumParams,
    PKLST_DEVINFO_EL* clonedDevInfo)
{
	PKLST_DEVINFO_HANDLE_INTERNAL clonedHandle = NULL;

	*clonedDevInfo = NULL;


	if (!l_Alloc_DevInfo(&clonedHandle, RegEnumParams->Heap)) goto Error;
	memcpy(&clonedHandle->DevInfoEL->Public, RegEnumParams->TempItem, sizeof(clonedHandle->DevInfoEL->Public));

	// LstInfoK + 1
	PoolHandle_Inc_LstInfoK(clonedHandle);

	clonedHandle->DevInfoEL->DevListHandle = RegEnumParams->DeviceList;
	clonedHandle->DevInfoEL->DevInfoHandle = clonedHandle;
	DL_APPEND(RegEnumParams->DeviceList->head, clonedHandle->DevInfoEL);

	*clonedDevInfo = clonedHandle->DevInfoEL;
	return ERROR_SUCCESS;

Error:
	return GetLastError();
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

	strcpy_s(devID, sizeof(devID), devItem->Public.DeviceID);
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


static BOOL l_EnumKey_Instances(KUSB_ENUM_REGKEY_PARAMS* RegEnumParams)
{
	LONG status;
	PKLST_DEVINFO_EL newDevItem = NULL;
	HDEVINFO hDevInfo = NULL;
	DWORD length;
	DWORD iDeviceInterface = (DWORD) - 1;

	if (!RegEnumParams->Exclusive.DevInterfaceGuid)
	{
		// Apply DeviceInterfaceGUID filter:
		mLst_ApplyPatternMatch(RegEnumParams->PatternMatch, DeviceInterfaceGUID, RegEnumParams->TempItem->DeviceInterfaceGUID, goto NextInstance);

		hDevInfo = SetupDiGetClassDevsA(&RegEnumParams->DevInterfaceGuid, RegEnumParams->TempItem->DeviceID, NULL, RegEnumParams->DigcFlags | DIGCF_DEVICEINTERFACE);
		if (!IsHandleValid(hDevInfo))
		{
			USBDBGN("SetupDiGetClassDevsA Failed. ErrorCode:%08Xh", GetLastError());
			return TRUE;
		}
	}
	else
	{
		hDevInfo = RegEnumParams->DevInfoSet;
	}

	do
	{
		if (hDevInfo != RegEnumParams->DevInfoSet)
		{
			RegEnumParams->DevInterfaceData.cbSize = sizeof(RegEnumParams->DevInterfaceData);
			if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &RegEnumParams->DevInterfaceGuid, ++iDeviceInterface, &RegEnumParams->DevInterfaceData))
				goto Done;
		}
		else
		{
			iDeviceInterface++;
			if (iDeviceInterface > 0) break;
		}

		if (!RegEnumParams->Exclusive.DevInterfaceGuid)
		{
			// Update 'Connected' from DevInterfaceData.Flags
			mLst_HandleConnectScenario(RegEnumParams, goto NextInstance);
		}

		// Disconnected filter devices are never listed regardless of the KLST_FLAG_INCLUDE_DISCONNECT flag.
		if (RegEnumParams->TempItem->Connected == FALSE && RegEnumParams->TempItem->DriverID == KUSB_DRVID_LIBUSB0_FILTER)
		{
			USBDBGN("Skipping disconnected filter device: %s (%s)",
			        RegEnumParams->TempItem->DeviceInterfaceGUID, RegEnumParams->TempItem->DeviceID);

			goto NextInstance;
		}

		if (hDevInfo != RegEnumParams->DevInfoSet)
		{
			mLst_Get_InterfaceDetail(RegEnumParams, hDevInfo, NULL, length, goto NextInstance);
		}

		// Since the 'LUsb0FilterIndex' is assigned when the device is connected, there is no point in getting it until it is.
		// IE: It is invalid even if it exists.
		RegEnumParams->TempItem->LUsb0FilterIndex = -1;
		if ((RegEnumParams->TempItem->Connected) && (RegEnumParams->TempItem->DriverID == KUSB_DRVID_LIBUSB0 || RegEnumParams->TempItem->DriverID == KUSB_DRVID_LIBUSB0_FILTER))
		{
			HKEY hKeyDevInterface;

			// 'LUsb0FilterIndex' is stored in the device interface key
			hKeyDevInterface = SetupDiOpenDeviceInterfaceRegKey(hDevInfo, &RegEnumParams->DevInterfaceData, 0, KEY_QUERY_VALUE);
			if (!IsHandleValid(hKeyDevInterface))
			{
				USBERRN("SetupDiOpenDeviceInterfaceRegKey Failed. ErrorCode:%08Xh", GetLastError());
				goto NextInstance;
			}

			length = sizeof(RegEnumParams->TempItem->LUsb0FilterIndex);
			status = RegQueryValueExA(hKeyDevInterface, "LUsb0", 0, NULL, (LPBYTE)&RegEnumParams->TempItem->LUsb0FilterIndex, &length);

			RegCloseKey(hKeyDevInterface);

			if (status != ERROR_SUCCESS)
			{
				USBWRNN("Failed getting 'Lusb0' index for device: %s (%s)",
				        RegEnumParams->TempItem->DeviceInterfaceGUID, RegEnumParams->TempItem->DeviceID);
			}
			else if (RegEnumParams->TempItem->LUsb0FilterIndex > 255 || RegEnumParams->TempItem->LUsb0FilterIndex < 0)
			{
				USBWRNN("Invalid 'Lusb0' index for device: %s (%s)",
				        RegEnumParams->TempItem->DeviceInterfaceGUID, RegEnumParams->TempItem->DeviceID);

				status = ERROR_RANGE_NOT_FOUND;
			}

			if (status == ERROR_SUCCESS)
			{
				sprintf(RegEnumParams->TempItem->DevicePath, "\\\\.\\libusb0-%04d", RegEnumParams->TempItem->LUsb0FilterIndex);
			}
		}

		// Get SPDRP_DEVICEDESC
		if (!SetupDiGetDeviceRegistryPropertyA(RegEnumParams->DevInfoSet, &RegEnumParams->DevInfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)RegEnumParams->TempItem->DeviceDesc, sizeof(RegEnumParams->TempItem->DeviceDesc) - 1, &length))
			length = 0;
		RegEnumParams->TempItem->DeviceDesc[length] = (CHAR)0;

		// Get SPDRP_MFG
		if (!SetupDiGetDeviceRegistryPropertyA(RegEnumParams->DevInfoSet, &RegEnumParams->DevInfoData, SPDRP_MFG, NULL, (PBYTE)RegEnumParams->TempItem->Mfg, sizeof(RegEnumParams->TempItem->Mfg) - 1, &length))
			length = 0;
		RegEnumParams->TempItem->Mfg[length] = (CHAR)0;

		// Get SPDRP_BUSNUMBER
		if (!SetupDiGetDeviceRegistryPropertyA(RegEnumParams->DevInfoSet, &RegEnumParams->DevInfoData, SPDRP_BUSNUMBER, NULL, (PBYTE)&RegEnumParams->TempItem->BusNumber, sizeof(RegEnumParams->TempItem->BusNumber), &length))
			RegEnumParams->TempItem->BusNumber = -1;

		// Get SPDRP_ADDRESS
		if (!SetupDiGetDeviceRegistryPropertyA(RegEnumParams->DevInfoSet, &RegEnumParams->DevInfoData, SPDRP_ADDRESS, NULL, (PBYTE)&RegEnumParams->TempItem->DeviceAddress, sizeof(RegEnumParams->TempItem->DeviceAddress), &length))
			RegEnumParams->TempItem->DeviceAddress = -1;

		RegEnumParams->ErrorCode = l_Build_AddElement(RegEnumParams, &newDevItem);
		if (RegEnumParams->ErrorCode != ERROR_SUCCESS)
		{
			goto Error;
		}

		l_Build_Common_Info(newDevItem);

		strcpy(newDevItem->Public.SerialNumber, newDevItem->Public.DeviceID);
		_strupr(newDevItem->Public.SerialNumber);
		if (strstr(newDevItem->Public.SerialNumber, "&MI_") != NULL)
		{
			// This is a composite device.  The 'SerialNumber' will come from the parent device.

			DWORD hParentInst;
			if (AllK->CM_Get_Parent(&hParentInst, RegEnumParams->DevInfoData.DevInst, 0) == ERROR_SUCCESS)
			{
				if (AllK->CM_Get_Device_ID(hParentInst, newDevItem->Public.SerialNumber, sizeof(newDevItem->Public.SerialNumber) - 1, 0) == ERROR_SUCCESS)
				{
					PCHAR sep = newDevItem->Public.SerialNumber;
					PCHAR sepNext;
					while( (sepNext = strchr(sep, '\\')) != NULL )
						sep = sepNext + 1;

					strcpy(newDevItem->Public.SerialNumber, sep);
					goto NextInstance;
				}
			}
		}

		strcpy(newDevItem->Public.SerialNumber, newDevItem->Public.Common.InstanceID);

NextInstance:
		;
	}
	while(TRUE);

Done:
	if (hDevInfo && hDevInfo != RegEnumParams->DevInfoSet)
		SetupDiDestroyDeviceInfoList(hDevInfo);
	return TRUE;
Error:
	if (hDevInfo && hDevInfo != RegEnumParams->DevInfoSet)
		SetupDiDestroyDeviceInfoList(hDevInfo);
	return FALSE;
}

static BOOL l_EnumKey_Guids(KUSB_ENUM_REGKEY_PARAMS* RegEnumParams)
{
	DWORD devInfoDataIndex		= (DWORD) - 1;
	DWORD length;
	DWORD status;
	CHAR devInterfaceGuidArray[1024];
	BOOL success;

	LPSTR setupEnumerator = "USB";
	GUID* setupClassGuid = NULL;
	DWORD setupAddFlags = DIGCF_ALLCLASSES;

	if (RegEnumParams->PatternMatch)
	{
		// If a specific DeviceInterfaceGUID or DeviceID (or both) is set in the pattern match then we can
		// use these value(s) to dramatically improve performance.
		if (RegEnumParams->PatternMatch->DeviceInterfaceGUID[0] &&
		        strlen(RegEnumParams->PatternMatch->DeviceInterfaceGUID) == GUID_STRING_LENGTH &&
		        String_To_Guid(&RegEnumParams->DevInterfaceGuid, RegEnumParams->PatternMatch->DeviceInterfaceGUID))
		{
			// PatternMatch.DeviceInterfaceGUID specifies an *exact* DeviceInterfaceGUID.
			RegEnumParams->Exclusive.DevInterfaceGuid = TRUE;

			setupClassGuid	= &RegEnumParams->DevInterfaceGuid;
			setupEnumerator = NULL;
			setupAddFlags	= DIGCF_DEVICEINTERFACE;
			strcpy(RegEnumParams->TempItem->DeviceInterfaceGUID, RegEnumParams->PatternMatch->DeviceInterfaceGUID);

			/*
			This is the 2nd best case performance scenario.  The result is:
			Number of Calls  | Function
			-----------------|----------------------------------------------------
			1                | SetupDiGetClassDevs()
			1-Per-Instance   | SetupDiEnumDeviceInterfaces()
			4-Per-Interface  | SetupDiGetDeviceRegistryProperty()
			1-Per-Interface  | SetupDiGetDeviceInterfaceDetail()
			1-Per-Interface  | [libusb0 only] SetupDiOpenDeviceInterfaceRegKey()
			1-Per-Interface  | [libusb0 only] RegQueryValueEx()
			*/
		}

		if (!RegEnumParams->Exclusive.DevInterfaceGuid &&
		        RegEnumParams->PatternMatch->ClassGUID[0] &&
		        strlen(RegEnumParams->PatternMatch->ClassGUID) == GUID_STRING_LENGTH &&
		        String_To_Guid(&RegEnumParams->ClassGuid, RegEnumParams->PatternMatch->ClassGUID))
		{
			// PatternMatch.ClassGUID specifies an *exact* ClassGUID.
			RegEnumParams->Exclusive.ClassGuid = TRUE;

			setupAddFlags  = 0;
			setupClassGuid = &RegEnumParams->ClassGuid;
			strcpy(RegEnumParams->TempItem->ClassGUID, RegEnumParams->PatternMatch->ClassGUID);
		}
	}

	RegEnumParams->DevInfoSet = SetupDiGetClassDevsA(setupClassGuid, setupEnumerator, NULL, RegEnumParams->DigcFlags | setupAddFlags);
	if (!IsHandleValid(RegEnumParams->DevInfoSet))
	{
		USBDBGN("SetupDiGetClassDevs Failed. ErrorCode:%08Xh", GetLastError());
		return TRUE;
	}

	RegEnumParams->DevInfoData.cbSize = sizeof(RegEnumParams->DevInfoData);
	RegEnumParams->DevInterfaceData.cbSize = sizeof(RegEnumParams->DevInterfaceData);

	do
	{
		if (RegEnumParams->Exclusive.DevInterfaceGuid)
		{
			// If the DeviceInterfaceGUID is exclusive, go directly to SetupDiEnumDeviceInterfaces.
			success = SetupDiEnumDeviceInterfaces(RegEnumParams->DevInfoSet, NULL, &RegEnumParams->DevInterfaceGuid, ++devInfoDataIndex, &RegEnumParams->DevInterfaceData);
			if (!success) break;

			// Get the DevicePath/SymbolicLink.
			// SetupDiGetDeviceInterfaceDetail will also return the DevInfoData (Instance Information) for the interface.

			// NOTE: This is also done in l_EnumKey_Instances for listings that do *not* use an exclusive DevInterfaceGuid.
			mLst_Get_InterfaceDetail(RegEnumParams, RegEnumParams->DevInfoSet, (&RegEnumParams->DevInfoData), length, goto NextInstance);

			// Update 'Connected' from DevInterfaceData.Flags
			mLst_HandleConnectScenario(RegEnumParams, goto NextInstance);

		}
		else
		{
			success = SetupDiEnumDeviceInfo(RegEnumParams->DevInfoSet, ++devInfoDataIndex, &RegEnumParams->DevInfoData);
			if (!success) break;
		}


		if (!RegEnumParams->Exclusive.ClassGuid)
		{
			// Get ClassGUID
			if (!Guid_To_String(&RegEnumParams->DevInfoData.ClassGuid, RegEnumParams->TempItem->ClassGUID))
				goto NextInstance;

			// Apply ClassGUID filter:
			mLst_ApplyPatternMatch(RegEnumParams->PatternMatch, ClassGUID, RegEnumParams->TempItem->ClassGUID, goto NextInstance);
		}

		// Get the Device Instance ID
		if (!SetupDiGetDeviceInstanceIdA(RegEnumParams->DevInfoSet, &RegEnumParams->DevInfoData, RegEnumParams->TempItem->DeviceID, sizeof(RegEnumParams->TempItem->DeviceID), NULL))
		{
			USBERRN("SetupDiGetDeviceInstanceId failed. ErrorCode:%08Xh", GetLastError());
			goto NextInstance;
		}

		// We are only interested in *usb device* instances; not root hubs (or anything else)
		if (_strnicmp(RegEnumParams->TempItem->DeviceID, "USB\\VID_", 8) != 0)
		{
			USBDBGN("Skipping %s..", RegEnumParams->TempItem->DeviceID);
			goto NextInstance;
		}

		// Apply PatternMatch->DeviceID
		mLst_ApplyPatternMatch(RegEnumParams->PatternMatch, DeviceID, RegEnumParams->TempItem->DeviceID, goto NextInstance);

		// Get SPDRP_SERVICE
		if (!SetupDiGetDeviceRegistryPropertyA(RegEnumParams->DevInfoSet, &RegEnumParams->DevInfoData, SPDRP_SERVICE, NULL, (PBYTE)RegEnumParams->TempItem->Service, sizeof(RegEnumParams->TempItem->Service) - 1, NULL))
		{
			USBDBGN("SetupDiGetDeviceRegistryProperty SPDRP_SERVICE Failed. ErrorCode:%08Xh", GetLastError());
			goto NextInstance;
		}

		RegEnumParams->TempItem->DriverID = -1;
		mLst_Assign_DrvId_From_Service(RegEnumParams->TempItem->DriverID, RegEnumParams->TempItem->Service);

		if (RegEnumParams->Exclusive.DevInterfaceGuid)
		{
			if (!l_EnumKey_Instances(RegEnumParams))
			{
				goto Error;
			}
		}
		else if (RegEnumParams->TempItem->DriverID == -1)
		{
			// We only care about the libusb0 filter GUID for this device instance because
			// the function driver is unknown.  If the filter is installed, is has enabled
			// the Libusb0FilterGuid.
			memcpy(&RegEnumParams->DevInterfaceGuid, &Libusb0FilterGuid, sizeof(GUID));
			strcpy(RegEnumParams->TempItem->DeviceInterfaceGUID, Libusb0FilterGuidA);
			RegEnumParams->TempItem->DriverID = KUSB_DRVID_LIBUSB0_FILTER;

			if (!l_EnumKey_Instances(RegEnumParams))
			{
				goto Error;
			}
		}
		else
		{
			// The servicename matched a supported driver. EG: (libusbK, WinUSB, or libusb0)
			// We care about the 'DeviceIntefaceGUIDs' multi-sz registry key. These drivers use
			// the same key to enable the interface guids when the device is connected.

			INT devInterfaceGuidArrayPos;
			HKEY hkeyDevInfo = SetupDiOpenDevRegKey(RegEnumParams->DevInfoSet, &RegEnumParams->DevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
			if (!IsHandleValid(hkeyDevInfo))
			{
				USBERRN("SetupDiOpenDevRegKey Failed. ErrorCode:%08Xh", GetLastError());
				goto NextInstance;
			}

			length = sizeof(devInterfaceGuidArray);
			status = RegQueryValueExA(hkeyDevInfo, "DeviceInterfaceGUIDs", NULL, NULL, (LPBYTE)devInterfaceGuidArray, &length);
			if (status != ERROR_SUCCESS)
			{
				USBERRN("RegQueryValueExA Failed. ErrorCode:%08Xh", GetLastError());
				RegCloseKey(hkeyDevInfo);
				goto NextInstance;
			}
			RegCloseKey(hkeyDevInfo);

			devInterfaceGuidArrayPos = 0;
			while(devInterfaceGuidArray[devInterfaceGuidArrayPos])
			{
				length = (DWORD)strlen(&devInterfaceGuidArray[devInterfaceGuidArrayPos]);
				strcpy(RegEnumParams->TempItem->DeviceInterfaceGUID, &devInterfaceGuidArray[devInterfaceGuidArrayPos]);
				if (String_To_Guid(&RegEnumParams->DevInterfaceGuid, RegEnumParams->TempItem->DeviceInterfaceGUID))
				{
					if (!l_EnumKey_Instances(RegEnumParams))
					{
						goto Error;
					}
				}
				devInterfaceGuidArrayPos += length + 1;
			}
		}

NextInstance:
		;
	}
	while(success);

Error:
	SetupDiDestroyDeviceInfoList(RegEnumParams->DevInfoSet);
	return RegEnumParams->ErrorCode == ERROR_SUCCESS;
}

#if 0

static BOOL l_EnumKey_Guids_Direct(KUSB_ENUM_REGKEY_PARAMS* RegEnumParams)
{
	DWORD length;
	DWORD status;
	HKEY hKeyDeviceClassRoot;
	DWORD iDeviceInterfaceGUID = (DWORD) - 1;

	status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, KEY_DEVICECLASSES, 0, KEY_READ, &hKeyDeviceClassRoot);
	if (status != ERROR_SUCCESS)
	{
		USBERRN("Opening 'DeviceClasses' registry key failed. ErrorCode=%08Xh", status);
		return FALSE;
	}

	while(RegEnumKeyA(hKeyDeviceClassRoot, ++iDeviceInterfaceGUID, RegEnumParams->TempItem->DeviceInterfaceGUID, sizeof(RegEnumParams->TempItem->DeviceInterfaceGUID)) == ERROR_SUCCESS)
	{
		HKEY hKeyDeviceInterfaceGUID;

		if (!(RegEnumParams->Flags & KLST_FLAG_INCLUDE_RAWGUID))
		{
			if (_stricmp(RegEnumParams->TempItem->DeviceInterfaceGUID, RawDeviceGuidA) == 0) continue;
		}

		status = RegOpenKeyA(hKeyDeviceClassRoot, RegEnumParams->TempItem->DeviceInterfaceGUID, &hKeyDeviceInterfaceGUID);
		if (status == ERROR_SUCCESS)
		{
			DWORD iSymbolicLink = (DWORD) - 1;

			while(RegEnumKeyA(hKeyDeviceInterfaceGUID, ++iSymbolicLink, RegEnumParams->TempItem->SymbolicLink, sizeof(RegEnumParams->TempItem->SymbolicLink)) == ERROR_SUCCESS)
			{
				HKEY hKeySymbolicLink;

				if (_strnicmp(RegEnumParams->TempItem->SymbolicLink, "##?#USB#VID_", 12) != 0) break;

				status = RegOpenKeyA(hKeyDeviceInterfaceGUID, RegEnumParams->TempItem->SymbolicLink, &hKeySymbolicLink);
				if (status == ERROR_SUCCESS)
				{
					length = sizeof(RegEnumParams->TempItem->DeviceID) - 1;
					status = RegQueryValueExA(hKeySymbolicLink, "DeviceInstance", 0, NULL, (LPBYTE)RegEnumParams->TempItem->DeviceID, &length);
					if (status == ERROR_SUCCESS)
					{
						HKEY hKeyDeviceParameters;
						HDEVINFO hDevInfo;

						RegEnumParams->TempItem->DeviceID[length] = '\0';

						status = RegOpenKeyA(hKeySymbolicLink, "#\\Device Parameters", &hKeyDeviceParameters);
						if (status == ERROR_SUCCESS)
						{
							length = sizeof(RegEnumParams->TempItem->LUsb0FilterIndex);
							status = RegQueryValueExA(hKeyDeviceParameters, "LUsb0", 0, NULL, (LPBYTE)RegEnumParams->TempItem->LUsb0FilterIndex, &length);

							RegCloseKey(hKeyDeviceParameters);

							if (status != ERROR_SUCCESS || RegEnumParams->TempItem->LUsb0FilterIndex > 255 || RegEnumParams->TempItem->LUsb0FilterIndex < 0)
								RegEnumParams->TempItem->LUsb0FilterIndex = -1;
						}

						RegEnumParams->TempItem->DriverID = -1;
						mLst_Assign_DrvId_From_InterfaceGuid(RegEnumParams->TempItem->DriverID, RegEnumParams->TempItem->DeviceInterfaceGUID);

						RegEnumParams->TempItem->SymbolicLink[0] = '\\';
						RegEnumParams->TempItem->SymbolicLink[1] = '\\';
						RegEnumParams->TempItem->SymbolicLink[2] = '?';
						RegEnumParams->TempItem->SymbolicLink[3] = '\\';

						strcpy(RegEnumParams->TempItem->DevicePath, RegEnumParams->TempItem->SymbolicLink);

						RegEnumParams->TempItem->Connected = FALSE;

						l_String_To_Guid(&RegEnumParams->DevInterfaceGuid, RegEnumParams->TempItem->DeviceInterfaceGUID);

						hDevInfo = SetupDiGetClassDevsA(&RegEnumParams->DevInterfaceGuid, RegEnumParams->TempItem->DeviceID, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
						if (IsHandleValid(hDevInfo))
						{
							RegEnumParams->DevInterfaceData.cbSize = sizeof(RegEnumParams->DevInterfaceData);
							if (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &RegEnumParams->DevInterfaceGuid, 0, &RegEnumParams->DevInterfaceData))
							{
								if (RegEnumParams->DevInterfaceData.Flags & SPINT_ACTIVE)
								{
									RegEnumParams->TempItem->Connected = TRUE;
								}
							}
							SetupDiDestroyDeviceInfoList(hDevInfo);
						}

						USBDBGN("%s (%s) Connected:%u",
						        RegEnumParams->TempItem->DeviceInterfaceGUID, RegEnumParams->TempItem->DeviceID, RegEnumParams->TempItem->Connected);

#if 0
						if (!RegEnumParams->TempItem->Connected)
						{
							DEVINST hDevInst;
							if (CM_Locate_DevNodeA(&hDevInst, RegEnumParams->TempItem->DeviceID, CM_LOCATE_DEVNODE_NORMAL) == CR_SUCCESS)
							{
								// DWORD dnStatus;
								// DWORD dnProblem;

								RegEnumParams->TempItem->Connected = TRUE;

								USBMSGN("%s (%s) Connected:TUE\n",
								        RegEnumParams->TempItem->DeviceInterfaceGUID, RegEnumParams->TempItem->DeviceID);
								if (CM_Get_DevNode_Status(&dnStatus, &dnProblem, hDevInst, 0) == CR_SUCCESS)
								{
									printf("%s (%s) Status:%08Xh Problem:%08Xh\n",
									       RegEnumParams->TempItem->DeviceInterfaceGUID, RegEnumParams->TempItem->DeviceID, dnStatus, dnProblem);
								}
							}
						}
#endif
					}
					RegCloseKey(hKeySymbolicLink);
				}
			}
			RegCloseKey(hKeyDeviceInterfaceGUID);
		}
	}
	RegCloseKey(hKeyDeviceClassRoot);

	//return RegEnumParams->ErrorCode == ERROR_SUCCESS;
	return FALSE;
}
#endif

KUSB_EXP BOOL KUSB_API LstK_Count(
    _in KLST_HANDLE DeviceList,
    _ref PUINT Count)
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

BOOL KUSB_API LstK_InitInternal(
    _out KLST_HANDLE* DeviceList,
    _in KLST_FLAG Flags,
    _in KLST_PATTERN_MATCH* PatternMatch,
    _inopt HANDLE Heap)
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
	enumParams.PatternMatch		= PatternMatch;
	enumParams.TempItem			= &TempItem;
	enumParams.Heap				= Heap;

	if (!(Flags & KLST_FLAG_INCLUDE_DISCONNECT))
		enumParams.DigcFlags |= DIGCF_PRESENT;

	if (l_EnumKey_Guids(&enumParams))
	{
		PKLST_DEVINFO_EL devEL;
		DL_FOREACH(handle->head, devEL)
		{
			PoolHandle_Live_LstInfoK(devEL->DevInfoHandle);
		}
		success = TRUE;
	}
	goto Done;

Done:
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

KUSB_EXP BOOL KUSB_API LstK_InitEx(
    _out KLST_HANDLE* DeviceList,
    _in KLST_FLAG Flags,
    _in KLST_PATTERN_MATCH* PatternMatch)
{
	return LstK_InitInternal(DeviceList, Flags, PatternMatch, NULL);
}



KUSB_EXP BOOL KUSB_API LstK_Init(
    _out KLST_HANDLE* DeviceList,
    _in KLST_FLAG Flags)
{
	return LstK_InitInternal(DeviceList, Flags, NULL, NULL);
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
		*DeviceInfo = (KLST_DEVINFO_HANDLE)handle->current;
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
		*DeviceInfo = (KLST_DEVINFO_HANDLE)handle->current;

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
	*DeviceInfo = (KLST_DEVINFO_HANDLE)handle->current;

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
    _in KLST_ENUM_DEVINFO_CB* EnumDevListCB,
    _inopt PVOID Context)
{
	PKLST_DEVINFO_EL check, tmp;
	PKLST_HANDLE_INTERNAL handle;

	Pub_To_Priv_LstK(DeviceList, handle, return FALSE);
	ErrorParam(!EnumDevListCB, Error, "EnumDevListCB");

	ErrorSetAction(!PoolHandle_Inc_LstK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_LstK");

	DL_FOREACH_SAFE(handle->head, check, tmp)
	{
		if (EnumDevListCB(DeviceList, (KLST_DEVINFO_HANDLE)check, Context) == FALSE)
			break;
	}

	PoolHandle_Dec_LstK(handle);
	return TRUE;

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LstK_FindByVidPid(
    _in KLST_HANDLE DeviceList,
    _in INT Vid,
    _in INT Pid,
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
		{
			break;
		}
		else
		{
			USBDBGN("Skipping %04Xh:%04Xh", check->Public.Common.Vid, check->Public.Common.Pid);
		}
	}

	*DeviceInfo = (KLST_DEVINFO_HANDLE)check;
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
    _inopt KLST_HANDLE SlaveList,
    _inopt KLST_SYNC_FLAG SyncFlags,
    _inopt PKLST_PATTERN_MATCH SlaveListPatternMatch,
    _inopt HANDLE Heap)
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
		KLST_FLAG slaveListInit = KLST_FLAG_NONE;
		ErrorNoSetAction(!LstK_InitInternal(&SlaveList, slaveListInit, SlaveListPatternMatch, Heap), goto Error_IncRefSlave, "->PoolHandle_Inc_LstK");
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

	ErrorNoSet(!l_Alloc_DevInfo(&clonedHandle, AllK->HeapDynamic), Error, "->LstK_CloneInfo");
	memcpy(&clonedHandle->DevInfoEL->Public, &handle->DevInfoEL->Public, sizeof(clonedHandle->DevInfoEL->Public));

	clonedHandle->DevInfoEL->DevListHandle = NULL;
	*DstInfo = (KLST_DEVINFO_HANDLE)clonedHandle->DevInfoEL;
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
