/*! \file lusbk_device_list.h
*/

#ifndef __LUSBK_DEVICE_LIST_H
#define __LUSBK_DEVICE_LIST_H

#include <windows.h>
#include <objbase.h>
#include "lusbk_dynamic.h"

#include <PSHPACK1.H>

typedef struct _KUSB_DEV_LIST_SEARCH
{
	BOOL EnableRawDeviceInterfaceGuid;

} KUSB_DEV_LIST_SEARCH, *PKUSB_DEV_LIST_SEARCH;

typedef struct _KUSB_DEV_LIST
{
	KUSB_USER_CONTEXT UserContext;

	INT DrvId;
	CHAR DeviceInterfaceGUID[MAX_PATH];
	CHAR DeviceInstance[MAX_PATH];
	CHAR ClassGUID[MAX_PATH];
	CHAR Mfg[MAX_PATH];
	CHAR DeviceDesc[MAX_PATH];
	CHAR Service[MAX_PATH];
	CHAR SymbolicLink[MAX_PATH];
	CHAR DevicePath[MAX_PATH];
	DWORD ReferenceCount;
	DWORD Linked;
	DWORD LUsb0SymbolicLinkIndex;

	struct _KUSB_DEV_LIST* next;
	struct _KUSB_DEV_LIST* prev;

	volatile LONG refCount;
	DWORD cbSize;
} KUSB_DEV_LIST, *PKUSB_DEV_LIST;

#include <POPPACK.H>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif
