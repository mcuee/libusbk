#ifndef __LUSBK_DEVICE_LIST_H
#define __LUSBK_DEVICE_LIST_H

#include <windows.h>
#include <objbase.h>
#include "lusbk_dynamic.h"

#define LUSB0_REG_DEFAULT_FILTER_GUID {F9F3FF14-AE21-48A0-8A25-8011A7A931D9}
#define LUSB0_REG_DEFAULT_DEVICE_GUID {20343A29-6DA1-4DB8-8A3C-16E774057BF5}
#define LUSBK_REG_DEFAULT_DEVICE_GUID {6C696275-7362-2D77-696E-33322D574446}

#define KUSB_CONTEXT_SIZE 32

#include <PSHPACK1.H>
#pragma warning(disable:4201)
typedef struct _KUSB_USER_CONTEXT
{
	union
	{
		UCHAR		Byte[KUSB_CONTEXT_SIZE];
		CHAR		Char[KUSB_CONTEXT_SIZE];

		USHORT		Word[KUSB_CONTEXT_SIZE / 2];
		SHORT		Short[KUSB_CONTEXT_SIZE / 2];

		ULONG		ULong[KUSB_CONTEXT_SIZE / 4];
		LONG		Long[KUSB_CONTEXT_SIZE / 4];

		ULONG_PTR	Ptr[KUSB_CONTEXT_SIZE / 8];
	};
} KUSB_USER_CONTEXT, *PKUSB_USER_CONTEXT;
#pragma warning(default:4201)

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
	KUSB_USER_CONTEXT My;

	CHAR DeviceInterfaceGUID[MAX_PATH];
	CHAR DeviceInstance[MAX_PATH];
	CHAR ClassGUID[MAX_PATH];
	CHAR Mfg[MAX_PATH];
	CHAR DeviceDesc[MAX_PATH];
	CHAR Service[MAX_PATH];
	CHAR SymbolicLink[MAX_PATH];
	DWORD ReferenceCount;
	INT DrvId;

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
