/*!********************************************************************
libusbK - Multi-driver USB library.
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

typedef struct _SERVICE_DRVID_MAP
{
	INT DrvId;
	LPCSTR* MapNames;
} SERVICE_DRVID_MAP, *PSERVICE_DRVID_MAP;

typedef struct _DEV_INTF_VALUENAME_MAP
{
	LPCSTR ValueName;
	DWORD RegValueType;
} DEV_INTF_VALUENAME_MAP, *PDEV_INTF_VALUENAME_MAP;

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

BOOL InitElement(__in PKUSB_DEV_LIST element, __in DWORD cbSize);

PKUSB_DEV_LIST AddElementCopy(__deref_inout PKUSB_DEV_LIST* head,
                              __in PKUSB_DEV_LIST elementToClone);

PCHAR CopyLast(__in CHAR sep, __out PCHAR dst, __in PCHAR src);

VOID DumpIntfElements(__deref_inout PKUSB_DEV_LIST* head);

VOID ApplySearchFilter(__in PKUSB_DEV_LIST_SEARCH SearchParameters,
                       __deref_inout PKUSB_DEV_LIST* DeviceList);

VOID ApplyCompositeDeviceMode(__in PKUSB_DEV_LIST_SEARCH SearchParameters,
                              __deref_inout PKUSB_DEV_LIST* DeviceList);

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
	DWORD valueDataSize = MAX_PATH;

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
	strcpy_s(dst, MAX_PATH, src);
	return dst;
}

VOID DumpIntfElements(__deref_inout PKUSB_DEV_LIST* head)
{
	PKUSB_DEV_LIST check = NULL;
	PKUSB_DEV_LIST tmp = NULL;
	PKUSB_DEV_LIST root = *head;
	List_ForEachSafe(root, check, tmp)
	{
		printf("DeviceInterfaceGUID = %s\n", check->DeviceInterfaceGUID);
		printf("DeviceInstance      = %s\n", check->DeviceInstance);
		printf("Connected Instances = %d\n", check->ReferenceCount);
		printf("ClassGUID           = %s\n", check->ClassGUID);
		printf("Mfg                 = %s\n", check->Mfg);
		printf("DeviceDesc          = %s\n", check->DeviceDesc);
		printf("DrvId               = %d (%s)\n", check->DrvId, GetDrvIdString(check->DrvId));
		printf("SymbolicLink        = %s\n", check->SymbolicLink);
		printf("\n");
	}
}
BOOL InitElement(__in PKUSB_DEV_LIST element, __in DWORD cbSize)
{
	if (!element)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (cbSize != element->cbSize)
	{
		element->cbSize = sizeof(*element);
		SetLastError(ERROR_INSTRUCTION_MISALIGNMENT);
		return FALSE;
	}
	element->refCount = 1;
	element->next = NULL;
	element->prev = NULL;
	element->CompositeList = NULL;

	return TRUE;
}

PKUSB_DEV_LIST AddElementCopy(__deref_inout PKUSB_DEV_LIST* head,
                              __in PKUSB_DEV_LIST elementToClone)
{
	PKUSB_DEV_LIST tmp = NULL;
	PKUSB_DEV_LIST newEntry = NULL;
	PKUSB_DEV_LIST root = *head;

	List_ForEachSafe(root, newEntry, tmp)
	{
		if (strstr(newEntry->SymbolicLink, elementToClone->SymbolicLink) == newEntry->SymbolicLink)
		{
			return newEntry;
		}
	}
	newEntry = Mem_Alloc(sizeof(*newEntry));
	memcpy(newEntry, elementToClone, sizeof(*newEntry));
	InitElement(newEntry, sizeof(*newEntry));
	List_AddTail(root, newEntry);
	*head = root;
	return newEntry;

}

BOOL GetCompositeDevice(__in PKUSB_DEV_LIST deviceInterface,
                        __inout PKUSB_DEV_LIST* compositeDevice)
{
	PCHAR miStart;
	PKUSB_DEV_LIST dev;
	*compositeDevice = dev = Mem_Alloc(sizeof(KUSB_DEV_LIST));
	if (!dev)
		return LusbwError(ERROR_NOT_ENOUGH_MEMORY);

	strcpy_s(dev->DeviceInstance, sizeof(dev->DeviceInstance) - 1, deviceInterface->DeviceInstance);
	_strupr_s(dev->DeviceInstance, sizeof(dev->DeviceInstance) - 1);
	if ((miStart = strstr(dev->DeviceInstance, "&MI_")) != NULL)
	{
		*miStart = '\0';
		strcpy_s(dev->DeviceDesc, sizeof(dev->DeviceDesc) - 1, "USB Composite Device");
		strcpy_s(dev->Service, sizeof(dev->Service) - 1, "usbccgp");
		dev->DrvId = deviceInterface->DrvId;

		return TRUE;
	}
	Mem_Free(compositeDevice);

	return FALSE;
}

BOOL NewCompositeElement(__in LPCSTR DeviceInstance,
                         __inout PKUSB_DEV_LIST* compositeDevice)
{
	PKUSB_DEV_LIST dev;
	*compositeDevice = dev = Mem_Alloc(sizeof(KUSB_DEV_LIST));
	if (!dev)
		return LusbwError(ERROR_NOT_ENOUGH_MEMORY);

	strcpy_s(dev->DeviceInstance, sizeof(dev->DeviceInstance) - 1, DeviceInstance);
	_strupr_s(dev->DeviceInstance, sizeof(dev->DeviceInstance) - 1);

	strcpy_s(dev->DeviceDesc, sizeof(dev->DeviceDesc) - 1, "USB Composite Device");
	strcpy_s(dev->Service, sizeof(dev->Service) - 1, "usbccgp");

	return TRUE;
}

VOID ApplyCompositeDeviceMode(__in PKUSB_DEV_LIST_SEARCH SearchParameters,
                              __deref_inout PKUSB_DEV_LIST* DeviceList)
{
	PKUSB_DEV_LIST compositeDeviceElement;
	PKUSB_DEV_LIST devEL, devEL_t0, devCompositeEL, devCompositeEL_t0;
	PKUSB_DEV_LIST root = *DeviceList;
	PKUSB_DEV_LIST compositeRoot = NULL;
	CHAR deviceInstance[MAX_PATH];

	if (!SearchParameters->EnableCompositeDeviceMode)
		return;

	List_ForEachSafe(root, devEL, devEL_t0)
	{
		PCHAR mi;
		strcpy_s(deviceInstance, sizeof(deviceInstance) - 1, devEL->DeviceInstance);
		_strupr_s(deviceInstance, sizeof(deviceInstance) - 1);
		mi = strstr(deviceInstance, "&MI_");
		if (mi)
		{
			mi[0] = '\0';
			// this is a composite device
			if (NewCompositeElement(deviceInstance, &compositeDeviceElement))
			{
				compositeDeviceElement->DrvId = devEL->DrvId;
				List_ForEachSafe(compositeRoot, devCompositeEL, devCompositeEL_t0)
				{
					if (devCompositeEL->DrvId == compositeDeviceElement->DrvId &&
					        strncmp(devCompositeEL->DeviceInstance, compositeDeviceElement->DeviceInstance, strlen(devCompositeEL->DeviceInstance)) == 0)
					{
						// We already have a composite parent for this one.
						break;
					}
				}

				if (!devCompositeEL)
				{
					// this is a new composite parent.
					List_AddTail(compositeRoot, compositeDeviceElement);
					devCompositeEL = compositeDeviceElement;
				}
				else
				{
					Mem_Free(&compositeDeviceElement);
				}

				List_Remove(root, devEL);
				List_AddTail(devCompositeEL->CompositeList, devEL);
			}
		}
	}

	List_ForEachSafe(compositeRoot, devCompositeEL, devCompositeEL_t0)
	{
		List_Remove(compositeRoot, devCompositeEL);
		List_AddTail(root, devCompositeEL);
	}

	*DeviceList = root;
}

VOID ApplySearchFilter(__in PKUSB_DEV_LIST_SEARCH SearchParameters,
                       __deref_inout PKUSB_DEV_LIST* DeviceList)
{
	PKUSB_DEV_LIST check = NULL;
	PKUSB_DEV_LIST tmp = NULL;
	PKUSB_DEV_LIST root = *DeviceList;
	List_ForEachSafe(root, check, tmp)
	{
		if (!SearchParameters->EnableRawDeviceInterfaceGuid)
		{
			if (_stricmp(check->DeviceInterfaceGUID, RawDeviceGuidA) == 0)
			{
				List_Remove(root, check);
				Mem_Free(&check);
			}
		}
	}
	*DeviceList = root;
}

KUSB_EXP LONG KUSB_API LstK_GetDeviceList(
    __deref_inout PKUSB_DEV_LIST* DeviceList,
    __in PKUSB_DEV_LIST_SEARCH SearchParameters)
{
	HKEY hDeviceClasses = NULL;
	LONG status;
	DWORD deviceClasseIndex = (DWORD) - 1;
	PKUSB_DEV_LIST devIntfList = NULL;
	KUSB_DEV_LIST devIntfElement;
	LONG deviceCount = 0;
	PSERVICE_DRVID_MAP map;
	PKUSB_DEV_LIST_SEARCH searchParameters = SearchParameters;
	KUSB_DEV_LIST_SEARCH defSearchParameters;

	memset(&defSearchParameters, 0, sizeof(defSearchParameters));
	if (!searchParameters)
		searchParameters = &defSearchParameters;


	status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, KEY_DEVICECLASSES, 0, KEY_READ, &hDeviceClasses);
	if (status != ERROR_SUCCESS)
		return -status;

	memset(&devIntfElement, 0, sizeof(devIntfElement));
	while(RegEnumKeyA(hDeviceClasses, ++deviceClasseIndex, devIntfElement.DeviceInterfaceGUID, sizeof(devIntfElement.DeviceInterfaceGUID) - 1) == ERROR_SUCCESS)
	{
		// enumeration device interface guids
		// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}
		CHAR deviceInstanceKeyPath[1024];
		HKEY hDeviceInterfaceGuid = NULL;
		DWORD deviceInterfaceIndex = (DWORD) - 1;

		memset(deviceInstanceKeyPath, 0, sizeof(deviceInstanceKeyPath));

		status = RegOpenKeyExA(hDeviceClasses, devIntfElement.DeviceInterfaceGUID, 0, KEY_READ, &hDeviceInterfaceGuid);
		if (status != ERROR_SUCCESS)
			continue;

		while(RegEnumKeyA(hDeviceInterfaceGuid, ++deviceInterfaceIndex, deviceInstanceKeyPath, sizeof(deviceInstanceKeyPath) - 1) == ERROR_SUCCESS)
		{
			// enumeration device interface instances
			// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}
			HKEY hDeviceKeyPath = NULL;

			if (IsUsbRegKey(deviceInstanceKeyPath))
			{
				// query device instance id
				// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\DeviceInstance
				status = RegGetValueString(hDeviceInterfaceGuid, deviceInstanceKeyPath, NULL, "DeviceInstance", devIntfElement.DeviceInstance);
				if (status != ERROR_SUCCESS)
					continue;

				// query symbolic link value
				// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\#\SymbolicLink
				status = RegGetValueString(hDeviceInterfaceGuid, deviceInstanceKeyPath, "\\#", "SymbolicLink", devIntfElement.SymbolicLink);
				if (status != ERROR_SUCCESS)
					continue;
				strcpy_s(devIntfElement.DevicePath, sizeof(devIntfElement.DevicePath) - 1, devIntfElement.SymbolicLink);

				// query reference count (connected device instance id count)
				// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\Control\ReferenceCount
				status = RegGetValueDWord(hDeviceInterfaceGuid, deviceInstanceKeyPath, "\\Control", "ReferenceCount", &devIntfElement.ReferenceCount);
				if (status != ERROR_SUCCESS)
					continue;

				status = ERROR_SUCCESS;

				if (devIntfElement.ReferenceCount)
				{
					CHAR devRegEnumKey[1024];

					CHAR valueName[MAX_PATH];
					CHAR valueData[MAX_PATH];

					DWORD valueType;
					DWORD valueNameLength;
					DWORD valueDataLength;

					HKEY hDevRegEnumKey = NULL;
					DWORD devRegEnumKeyIndex = (DWORD) - 1;

					// query LUsb0
					devIntfElement.LUsb0SymbolicLinkIndex = (ULONG) - 1;
					// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\#\Device Parameters\LUsb0
					status = RegGetValueDWord(hDeviceInterfaceGuid, deviceInstanceKeyPath, "\\#\\Device Parameters", "LUsb0", &devIntfElement.LUsb0SymbolicLinkIndex);

					// query Linked
					// e.g. HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{20343a29-6da1-4db8-8a3c-16e774057bf5}\##?#USB#VID_1234&PID_0001#BMD001#{20343a29-6da1-4db8-8a3c-16e774057bf5}\#\Control\Linked
					devIntfElement.Linked = 0;
					RegGetValueDWord(hDeviceInterfaceGuid, deviceInstanceKeyPath, "\\#\\Control", "Linked", &devIntfElement.Linked);

					memset(devRegEnumKey, 0, sizeof(devRegEnumKey));
					strcat_s(devRegEnumKey, sizeof(devRegEnumKey), "SYSTEM\\CurrentControlSet\\Enum\\");
					strcat_s(devRegEnumKey, sizeof(devRegEnumKey), devIntfElement.DeviceInstance);

					// opening device instance reg key node
					// e.g. HKLM\SYSTEM\CurrentControlSet\Enum\USB\VID_1234&PID_0001&MI_00\7&15c836fa&2&0000
					status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, devRegEnumKey, 0, KEY_READ, &hDevRegEnumKey);
					if (status != ERROR_SUCCESS)
						continue;

					memset(valueName, 0, sizeof(valueName));
					memset(valueData, 0, sizeof(valueData));
					valueNameLength = sizeof(valueName) - 1;
					valueDataLength = sizeof(valueData) - 1;


					memset(devIntfElement.ClassGUID,	0, sizeof(devIntfElement.ClassGUID));
					memset(devIntfElement.DeviceDesc,	0, sizeof(devIntfElement.DeviceDesc));
					memset(devIntfElement.Mfg,			0, sizeof(devIntfElement.Mfg));
					memset(devIntfElement.Service,	0, sizeof(devIntfElement.Service));
					while(RegEnumValueA(hDevRegEnumKey, ++devRegEnumKeyIndex, valueName, &valueNameLength, 0, &valueType, (LPBYTE)valueData, &valueDataLength) == ERROR_SUCCESS)
					{

						// e.g. HKLM\SYSTEM\CurrentControlSet\Enum\USB\VID_1234&PID_0001&MI_00\7&15c836fa&2&0000\[ValueNames]

						if (GetRegDevNodeString(devIntfElement, valueName, ClassGUID, valueData)) {}
						else if (GetRegDevNodeString(devIntfElement, valueName, DeviceDesc, valueData)) {}
						else if (GetRegDevNodeString(devIntfElement, valueName, Mfg, valueData)) {}
						else if (GetRegDevNodeString(devIntfElement, valueName, Service, valueData)) {}

						memset(valueName, 0, sizeof(valueName));
						memset(valueData, 0, sizeof(valueData));
						valueNameLength = sizeof(valueName) - 1;
						valueDataLength = sizeof(valueData) - 1;

					}

					// find driver type by device interface guid
					devIntfElement.DrvId = KUSB_DRVID_INVALID;
					map = (PSERVICE_DRVID_MAP)DevGuidDrvIdMap;
					while(devIntfElement.DrvId == KUSB_DRVID_INVALID && map->DrvId != KUSB_DRVID_INVALID)
					{
						LPCSTR* devGuid = map->MapNames;
						while(*devGuid)
						{
							if (_stricmp(devIntfElement.DeviceInterfaceGUID, *devGuid) == 0)
							{
								devIntfElement.DrvId = map->DrvId;
								break;
							}
							devGuid++;
						}
						map++;
					}

					// find driver type by service name (if not found)
					map = (PSERVICE_DRVID_MAP)ServiceDrvIdMap;
					while(devIntfElement.DrvId == KUSB_DRVID_INVALID && map->DrvId != KUSB_DRVID_INVALID)
					{
						LPCSTR* serviceName = map->MapNames;
						while(*serviceName)
						{
							if (_stricmp(devIntfElement.Service, *serviceName) == 0)
							{
								devIntfElement.DrvId = map->DrvId;
								break;
							}
							serviceName++;
						}
						map++;
					}
					RegCloseKey(hDevRegEnumKey);
					hDevRegEnumKey = NULL;
					if (devIntfElement.DrvId != KUSB_DRVID_INVALID)
					{
						if (devIntfElement.DrvId == KUSB_DRVID_LIBUSB0_FILTER ||
						        devIntfElement.DrvId == KUSB_DRVID_LIBUSB0 )
						{
							if (devIntfElement.LUsb0SymbolicLinkIndex <= 255)
							{
								sprintf_s(devIntfElement.DevicePath, sizeof(devIntfElement.DevicePath) - 1,
								          "\\\\.\\libusb0-%04d", devIntfElement.LUsb0SymbolicLinkIndex);
								AddElementCopy(&devIntfList, &devIntfElement);
								deviceCount++;
							}

						}
						else
						{
							AddElementCopy(&devIntfList, &devIntfElement);
							deviceCount++;
						}
					}
				}
				hDeviceKeyPath = NULL;

			}
			if (hDeviceKeyPath)
				RegCloseKey(hDeviceKeyPath);

		}
		if (hDeviceInterfaceGuid)
			RegCloseKey(hDeviceInterfaceGuid);

		memset(&devIntfElement, 0, sizeof(devIntfElement));
	}

	if (hDeviceClasses)
		RegCloseKey(hDeviceClasses);

	if (devIntfList && searchParameters)
	{
		ApplySearchFilter(searchParameters, &devIntfList);
		ApplyCompositeDeviceMode(searchParameters, &devIntfList);
	}
	*DeviceList = devIntfList;
	return deviceCount;

	//DumpIntfElements(&devIntfList);
	//LstK_FreeDeviceList(&devIntfList);
	//return -ERROR_NOT_SUPPORTED;
}

KUSB_EXP VOID KUSB_API LstK_FreeDeviceList(__deref_inout PKUSB_DEV_LIST* DeviceList)
{
	PKUSB_DEV_LIST check = NULL;
	PKUSB_DEV_LIST tmp = NULL;
	PKUSB_DEV_LIST root = *DeviceList;
	List_ForEachSafe(root, check, tmp)
	{
		List_Remove(root, check);
		if (InterlockedDecrement(&check->refCount) < 1)
		{
			Mem_Free(&check);
		}
	}
	*DeviceList = root;
}
