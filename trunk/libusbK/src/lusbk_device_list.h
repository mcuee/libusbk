#ifndef __LUSBK_DEVICE_LIST_H
#define __LUSBK_DEVICE_LIST_H

#include <windows.h>
#include <objbase.h>
#include "lusbk_dynamic.h"

#define LUSB0_REG_DEFAULT_FILTER_GUID {F9F3FF14-AE21-48A0-8A25-8011A7A931D9}
#define LUSB0_REG_DEFAULT_DEVICE_GUID {20343A29-6DA1-4DB8-8A3C-16E774057BF5}
#define LUSBK_REG_DEFAULT_DEVICE_GUID {6C696275-7362-2D77-696E-33322D574446}

#include <PSHPACK1.H>

typedef struct _KUSB_DEV_LIST_SEARCH
{
	INT Vid;
	INT Pid;
	INT MI;
	GUID DeviceInterfaceGUID;
	DWORD DrvId;
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
