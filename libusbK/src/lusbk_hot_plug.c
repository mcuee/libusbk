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
#include "lusbk_hot_plug.h"
#include "lusbk_handles.h"
#include <process.h>
#include <dbt.h>

#pragma warning(disable:4127)


typedef struct _KHOT_NOTIFIER_LIST
{
	volatile long Lock;

	PKLST_HANDLE DeviceList;
	ULONG MaxRefreshMS;

	PKHOT_HANDLE_INTERNAL Items;
	LONG Count;

	CHAR WindowName[32];
	HINSTANCE hAppInstance;

	HANDLE ThreadHandle;
	UINT ThreadID;

	volatile HWND Hwnd;
	volatile long UpdatePendingCount;
} KHOT_NOTIFIER_LIST;
typedef KHOT_NOTIFIER_LIST* PKHOT_NOTIFIER_LIST;

#define KUSB_STR_EL_CLEANUP(StrElList, StrEL, StrTmp)	\
	DL_FOREACH_SAFE(StrElList, StrEL, StrTmp)			\
	{													\
		DL_DELETE(StrElList, StrEL);					\
		Mem_Free(&StrEL);								\
	}

typedef struct _KUSB_STR_EL
{
	LPCSTR Value;
	struct _KUSB_STR_EL* next;
	struct _KUSB_STR_EL* prev;

} KUSB_STR_EL, *PKUSB_STR_EL;

typedef struct _KLST_NOTIFY_CONTEXT
{
	KHOT_HANDLE HotHandle;
	PKUSB_STR_EL DevInstList;
} KLST_NOTIFY_CONTEXT, *PKLST_NOTIFY_CONTEXT;

static LPCSTR g_WindowClassHotK = "HotK_NotificationWindowClass";
static KHOT_NOTIFIER_LIST g_HotNotifierList = {0, NULL, 1000, NULL, 0, {0}, NULL, NULL, 0, NULL, 0};

static LRESULT CALLBACK hotk_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL hotk_RegisterWindowClass(PKHOT_NOTIFIER_LIST NotifierList);
static BOOL hotk_CreateWindow(PKHOT_NOTIFIER_LIST NotifierList);
static BOOL hotk_CreateThread(PKHOT_NOTIFIER_LIST NotifierList);
static unsigned _stdcall hotk_ThreadProc(PKHOT_NOTIFIER_LIST NotifierList);
static BOOL KUSB_API hotk_NotifyWaiters(PKLST_HANDLE DeviceList, PKLST_DEV_INFO DeviceInfo, PKLST_NOTIFY_CONTEXT Context);

static BOOL KUSB_API hotk_ForceNotifyWaiters(PKLST_HANDLE DeviceList, PKLST_DEV_INFO DeviceInfo, PKLST_NOTIFY_CONTEXT Context)
{
	DeviceInfo->SyncResults.Arrival = 1;
	return hotk_NotifyWaiters(DeviceList, DeviceInfo, Context);
}

static BOOL KUSB_API hotk_NotifyWaiters(PKLST_HANDLE DeviceList, PKLST_DEV_INFO DeviceInfo, PKLST_NOTIFY_CONTEXT Context)
{
	PKHOT_HANDLE_INTERNAL hotHandle;
	PKUSB_STR_EL devInstEL = NULL;

	UNREFERENCED_PARAMETER(DeviceList);

	DL_FOREACH(g_HotNotifierList.Items, hotHandle)
	{
		if (Context->HotHandle && Context->HotHandle != hotHandle)
			continue;

		if (MatchInstanceID(hotHandle->Public.DevInstIdPatternMatch, DeviceInfo->DeviceInstance))
		{
			// A device instance will only be notified once per WM_DEVICECHANGE
			DL_FOREACH(Context->DevInstList, devInstEL)
			{
				if (strcmp(devInstEL->Value, DeviceInfo->DeviceInstance) == 0)
					break;
			}

			if (devInstEL && !hotHandle->Public.Flags.AllowDupeMatch) continue;

			if (!devInstEL)
			{
				devInstEL = Mem_Alloc(sizeof(*devInstEL));
				devInstEL->Value = DeviceInfo->DeviceInstance;
				DL_APPEND(Context->DevInstList, devInstEL);
			}

			if (DeviceInfo->SyncResults.Arrival)
			{
				hotHandle->Public.Found = DeviceInfo;
				if (hotHandle->Public.OnHotPlugEvent)
					hotHandle->Public.OnHotPlugEvent(hotHandle, &hotHandle->Public, DeviceInfo, KHOT_PLUG_ARRIVAL);
			}
			else if (DeviceInfo->SyncResults.Removal)
			{
				hotHandle->Public.Found = DeviceInfo;
				if (hotHandle->Public.OnHotPlugEvent)
					hotHandle->Public.OnHotPlugEvent(hotHandle, &hotHandle->Public, DeviceInfo, KHOT_PLUG_REMOVAL);

				LstK_RemoveDevInfo(DeviceList, DeviceInfo);
			}
		}
	}

	return TRUE;
}

static LRESULT CALLBACK hotk_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#define IDT_KHOT_REFRESH	0xF

	switch(msg)
	{
	case WM_TIMER:
		if (wParam != IDT_KHOT_REFRESH)
			return DefWindowProc(hwnd, msg, wParam, lParam);

		KillTimer(hwnd, IDT_KHOT_REFRESH);

		SpinLock_Acquire(&g_HotNotifierList.Lock, TRUE);
		if (!g_HotNotifierList.Hwnd)
		{
			SpinLock_Release(&g_HotNotifierList.Lock);
			return (LRESULT)FALSE;
		}

		if (LstK_Sync(g_HotNotifierList.DeviceList, NULL, KLST_SYNC_ALL))
		{
			KLST_NOTIFY_CONTEXT notifyCtx;
			PKUSB_STR_EL strEL, strTmp;

			memset(&notifyCtx, 0, sizeof(notifyCtx));
			LstK_Enumerate(g_HotNotifierList.DeviceList, hotk_NotifyWaiters, &notifyCtx);

			KUSB_STR_EL_CLEANUP(notifyCtx.DevInstList, strEL, strTmp);
		}

		SpinLock_Release(&g_HotNotifierList.Lock);
		return (LRESULT)FALSE;

	case WM_DEVICECHANGE:
		SpinLock_Acquire(&g_HotNotifierList.Lock, TRUE);
		if (!g_HotNotifierList.Hwnd)
		{
			SpinLock_Release(&g_HotNotifierList.Lock);
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		// Start the device list changed count-down.  When no WM_DEVICECHANGE messages
		// have been received within the MaxRefreshMS, the timer message is sent and
		// the hot-plugging begins.
		SetTimer(hwnd, IDT_KHOT_REFRESH, g_HotNotifierList.MaxRefreshMS, (TIMERPROC) NULL);

		SpinLock_Release(&g_HotNotifierList.Lock);
		return (LRESULT)TRUE;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return (LRESULT)FALSE;
}

static BOOL hotk_RegisterWindowClass(PKHOT_NOTIFIER_LIST NotifierList)
{
	WNDCLASSEX wc;

	UNREFERENCED_PARAMETER(NotifierList);

	memset(&wc, 0, sizeof(wc));

	// Registering the Window Class
	wc.cbSize        = sizeof(wc);
	wc.style         = 0;
	wc.lpfnWndProc   = hotk_WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = g_HotNotifierList.hAppInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = g_WindowClassHotK;
	wc.hIconSm       = NULL;

	ErrorNoSet(!RegisterClassExA(&wc), Error, "RegisterClassEx failed.");

	return TRUE;
Error:
	return FALSE;
}

static BOOL hotk_CreateWindow(PKHOT_NOTIFIER_LIST NotifierList)
{
	ULONG count = 0;
	UNREFERENCED_PARAMETER(NotifierList);

	do
	{
		memset(g_HotNotifierList.WindowName, 0, sizeof(g_HotNotifierList.WindowName));
		sprintf_s(g_HotNotifierList.WindowName, sizeof(g_HotNotifierList.WindowName), "HotK_NotificationWindow_%u", count);

		if (!IsHandleValid(FindWindowA(g_WindowClassHotK, g_HotNotifierList.WindowName)))
			break;

	}
	while(++count < 128);

	if (count >= 128)
	{
		USBERR("Too many notification windows currently running. count=%u\n", count);
		return LusbwError(ERROR_TOO_MANY_MODULES);
	}

	g_HotNotifierList.Hwnd = CreateWindowExA(
	                             WS_EX_CLIENTEDGE,
	                             g_WindowClassHotK,
	                             g_HotNotifierList.WindowName,
	                             WS_OVERLAPPEDWINDOW,
	                             -100, -100, 20, 20,
	                             NULL, NULL, g_HotNotifierList.hAppInstance, NULL);

	if(!IsHandleValid(g_HotNotifierList.Hwnd))
	{
		USBERR("RegisterClassEx failed. ErrorCode=%08Xh\n", GetLastError());
		return FALSE;
	}

	ShowWindow(g_HotNotifierList.Hwnd, SW_HIDE);
	EnableWindow(g_HotNotifierList.Hwnd, TRUE);
	return TRUE;
}

static unsigned _stdcall hotk_ThreadProc(PKHOT_NOTIFIER_LIST NotifierList)
{
	MSG msg;
	DWORD exitCode = 0;
	// HDEVNOTIFY hNotify;
	DEV_BROADCAST_DEVICEINTERFACE_A allInterfacesA;

	memset(&allInterfacesA, 0, sizeof(DEV_BROADCAST_DEVICEINTERFACE_A));
	allInterfacesA.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	allInterfacesA.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE_A);

	SpinLock_Acquire(&g_HotNotifierList.Lock, TRUE);

	if (!hotk_RegisterWindowClass(NotifierList))
	{
		exitCode = GetLastError();
		SpinLock_Release(&g_HotNotifierList.Lock);
		goto Done;
	}
	if (!hotk_CreateWindow(NotifierList))
	{
		exitCode = GetLastError();
		SpinLock_Release(&g_HotNotifierList.Lock);
		goto Done;
	}

	SpinLock_Release(&g_HotNotifierList.Lock);

	//hNotify = RegisterDeviceNotificationA(g_HotNotifierList.Hwnd, &allInterfacesA, DEVICE_NOTIFY_WINDOW_HANDLE | 0x00000004);

	while(IsHandleValid(g_HotNotifierList.Hwnd) && GetMessageA(&msg, g_HotNotifierList.Hwnd, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	//if (hNotify)
	//	UnregisterDeviceNotification(hNotify);

Done:
	_endthreadex(exitCode);
	return exitCode;
}

static BOOL hotk_CreateThread(PKHOT_NOTIFIER_LIST NotifierList)
{
	g_HotNotifierList.ThreadHandle = (HANDLE)_beginthreadex( NULL, 0, &hotk_ThreadProc, NotifierList, CREATE_SUSPENDED, &g_HotNotifierList.ThreadID);
	if (!IsHandleValid(g_HotNotifierList.ThreadHandle))
	{
		USBERR("_beginthreadex failed. ErrorCode=%08Xh\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}

VOID KUSB_API hotk_Free(PKHOT_HANDLE_INTERNAL hotHandle)
{
	SpinLock_Acquire(&g_HotNotifierList.Lock, TRUE);

	ALLK_LOCK_HANDLE(hotHandle);
	if (!ALLK_INUSE_HANDLE(hotHandle))
	{
		ALLK_UNLOCK_HANDLE(hotHandle);
		goto Done;
	}

	// remove notifier
	DL_DELETE(g_HotNotifierList.Items, hotHandle);

	ALLK_UNLOCK_HANDLE(hotHandle);

	if ((--g_HotNotifierList.Count) < 1)
	{
		PostMessage(g_HotNotifierList.Hwnd, WM_DESTROY, (WPARAM)0, (LPARAM)0);
		LstK_Free(&g_HotNotifierList.DeviceList);
		g_HotNotifierList.Hwnd = NULL;
	}
Done:
	SpinLock_Release(&g_HotNotifierList.Lock);
}

KUSB_EXP BOOL KUSB_API HotK_Init(
    __deref_out KHOT_HANDLE* Handle,
    __in PKHOT_PARAMS InitParams)
{
	PKHOT_NOTIFIER_LIST NotifierList = &g_HotNotifierList;
	BOOL isLocked = FALSE;
	PKHOT_HANDLE_INTERNAL hotHandle = NULL;

	ErrorHandle(!IsHandleValid(Handle), Error, "Handle");
	ErrorParam(!IsHandleValid(InitParams), Error, "InitParams");

	ErrorParam(!IsHandleValid(InitParams->OnHotPlugEvent), Error, "OnHotPlugCB");
	ErrorParam(!strlen(InitParams->DevInstIdPatternMatch), Error, "DevInstIdMatchPattern");

	isLocked = SpinLock_Acquire(&g_HotNotifierList.Lock, TRUE);

	hotHandle = PoolHandle_Acquire_HotK(&hotk_Free);
	ErrorNoSet(!IsHandleValid(hotHandle), Error, "->PoolHandle_Acquire_HotK");

	memcpy(&hotHandle->Public, InitParams, sizeof(hotHandle->Public));

	DL_APPEND(g_HotNotifierList.Items, hotHandle);

	// Create the top-level window for monitoring. WM_DEVICE_CHANGE.
	if (!g_HotNotifierList.Hwnd)
	{
		LstK_Init(&g_HotNotifierList.DeviceList, NULL);
		if (!g_HotNotifierList.hAppInstance)
			g_HotNotifierList.hAppInstance = GetModuleHandle(NULL);

		ErrorNoSet(!hotk_CreateThread(NotifierList), Error, "->hotk_CreateThread");

		ResumeThread(g_HotNotifierList.ThreadHandle);
	}

	if (InitParams->Flags.PlugAllOnInit)
	{
		PKUSB_STR_EL strEL, strTmp;
		KLST_NOTIFY_CONTEXT hotCtx;

		memset(&hotCtx, 0, sizeof(hotCtx));
		hotCtx.HotHandle = hotHandle;

		LstK_Enumerate(g_HotNotifierList.DeviceList, hotk_ForceNotifyWaiters, hotHandle);

		KUSB_STR_EL_CLEANUP(hotCtx.DevInstList, strEL, strTmp);
	}

	g_HotNotifierList.Count++;
	*Handle = (KHOT_HANDLE)hotHandle;
	SpinLock_Release(&g_HotNotifierList.Lock);

	return TRUE;

Error:
	if (IsHandleValid(hotHandle))
		DL_DELETE(g_HotNotifierList.Items, hotHandle);

	if (isLocked)
		SpinLock_Release(&g_HotNotifierList.Lock);

	if (IsHandleValid(hotHandle))
		PoolHandle_Dec_HotK(hotHandle);

	return FALSE;
}

KUSB_EXP BOOL KUSB_API HotK_Free(
    __deref_inout KHOT_HANDLE* Handle)
{
	PKHOT_HANDLE_INTERNAL hotHandle = *Handle;

	ErrorParam(!IsHandleValid(Handle), Error, "Handle invalid");
	ErrorParam(!IsHandleValid(hotHandle), Error, "Handle null");
	ErrorHandle(!ALLK_VALID_HANDLE(hotHandle, HotK), Error, "Handle not hotk");
	ErrorHandle(!ALLK_INUSE_HANDLE(hotHandle), Error, "Handle not in-use");

	return PoolHandle_Dec_HotK(hotHandle) == FALSE;
Error:
	return FALSE;
}

KUSB_EXP VOID KUSB_API HotK_FreeAll(VOID)
{
}
