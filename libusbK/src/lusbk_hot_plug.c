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
#include "lusbk_linked_list.h"
#include <process.h>
#include <dbt.h>

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#define DEFER_THRU_TIMER

#define IDT_KHOT_DBT_DEVNODES_CHANGED		0xA
#define IDT_KHOT_DBT_DEVICEARRIVAL			0xB
#define IDT_KHOT_DBT_DEVICEREMOVAL			0xC

// Dyanmic HotK memory is only accessed from the internal hot thread.  It uses it's own
// private non-serialized heap.
#define Hot_Mem_Alloc(mSize) HeapAlloc(g_HotNotifierList.ActiveHeap, HEAP_ZERO_MEMORY, mSize)
#define Hot_Mem_Free(mMemory) HeapFree(g_HotNotifierList.ActiveHeap, 0, mMemory)

#define KUSB_STR_EL_CLEANUP(StrElList, StrEL, StrTmp)	\
	DL_FOREACH_SAFE(StrElList, StrEL, StrTmp)			\
	{													\
		DL_DELETE(StrElList, StrEL);					\
		Hot_Mem_Free(StrEL);							\
	}

#define hotk_CmpBroadcastGuid(BroadcastEL, DevIntfGUID) memcmp(&BroadcastEL->InterfaceGUID,&DevIntfGUID,sizeof(GUID))

typedef struct _KHOT_BROADCAST_EL
{
	HDEVNOTIFY NotifyHandle;
	GUID InterfaceGUID;

	struct _KHOT_BROADCAST_EL* prev;
	struct _KHOT_BROADCAST_EL* next;
} KHOT_BROADCAST_EL, *PKHOT_BROADCAST_EL;

typedef struct _KUSB_STR_EL
{
	LPCSTR Value;
	struct _KUSB_STR_EL* next;
	struct _KUSB_STR_EL* prev;

} KUSB_STR_EL, *PKUSB_STR_EL;

typedef struct _KLST_NOTIFY_CONTEXT
{
	PKHOT_HANDLE_INTERNAL HotHandle;
	PKUSB_STR_EL DevInstList;
} KLST_NOTIFY_CONTEXT, *PKLST_NOTIFY_CONTEXT;

typedef struct _KHOT_NOTIFIER_LIST
{
	volatile HWND Hwnd;
	volatile long HotLockCount;
	volatile long HotInitCount;

	ATOM WindowAtom;

	KLST_HANDLE DeviceList;
	ULONG MaxRefreshMS;

	PKHOT_HANDLE_INTERNAL Items;

	CHAR WindowName[32];
	HINSTANCE hAppInstance;

	HANDLE ThreadHandle;
	UINT ThreadID;

	PKHOT_BROADCAST_EL BroadcastList;

	HANDLE Heap;
	HANDLE ActiveHeap;

} KHOT_NOTIFIER_LIST;
typedef KHOT_NOTIFIER_LIST* PKHOT_NOTIFIER_LIST;

static LPCSTR g_WindowClassHotK = "HotK_NotificationWindowClass";
static KHOT_NOTIFIER_LIST g_HotNotifierList = {NULL, 0, 0, 0, NULL, 1000, NULL, {0}, NULL, NULL, 0, NULL, NULL, NULL};

static LRESULT CALLBACK h_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static unsigned _stdcall h_ThreadProc(PKHOT_NOTIFIER_LIST NotifierList);

static BOOL h_Register_Atom(PKHOT_NOTIFIER_LIST NotifierList);
static BOOL h_Create_Thread(PKHOT_NOTIFIER_LIST NotifierList);
static BOOL h_Create_Hwnd(PKHOT_NOTIFIER_LIST NotifierList, HWND* hwnd);

static BOOL KUSB_API h_DevEnum_PlugWaiters(KLST_HANDLE DeviceList, KLST_DEVINFO_HANDLE DeviceInfo, PKLST_NOTIFY_CONTEXT Context);
static BOOL KUSB_API h_DevEnum_ClearSyncResults(KLST_HANDLE DeviceList, KLST_DEVINFO_HANDLE DeviceInfo, PKLST_NOTIFY_CONTEXT Context);
static BOOL KUSB_API h_DevEnum_RegisterForBroadcast(KLST_HANDLE DeviceList, KLST_DEVINFO_HANDLE DeviceInfo, PKHOT_HANDLE_INTERNAL Context);
static BOOL KUSB_API h_DevEnum_UpdateForRemoval(KLST_HANDLE DeviceList, KLST_DEVINFO_HANDLE DeviceInfo, PDEV_BROADCAST_DEVICEINTERFACE_A Context);
static BOOL KUSB_API h_DevEnum_PowerBroadcast(KLST_HANDLE DeviceList, KLST_DEVINFO_HANDLE DeviceInfo, VOID* PbtEvent);

static BOOL h_NotifyWaiters(__in_opt PKHOT_HANDLE_INTERNAL HotHandle, BOOL ClearSyncResultsWhenComplete);
static BOOL h_RegisterForBroadcast(PKHOT_HANDLE_INTERNAL HotHandle);

static BOOL h_IsHotMatch(PKHOT_HANDLE_INTERNAL HotHandle, KLST_DEVINFO_HANDLE DeviceInfo);

static BOOL h_NotifyWaiters(__in_opt PKHOT_HANDLE_INTERNAL HotHandle, BOOL ClearSyncResultsWhenComplete)
{
	PKUSB_STR_EL strEL, strTmp;
	KLST_NOTIFY_CONTEXT hotCtx;

	memset(&hotCtx, 0, sizeof(hotCtx));
	hotCtx.HotHandle = HotHandle;

	LstK_Enumerate(g_HotNotifierList.DeviceList, h_DevEnum_PlugWaiters, &hotCtx);

	if (ClearSyncResultsWhenComplete)
		LstK_Enumerate(g_HotNotifierList.DeviceList, h_DevEnum_ClearSyncResults, &hotCtx);

	KUSB_STR_EL_CLEANUP(hotCtx.DevInstList, strEL, strTmp);

	return TRUE;
}

static void KUSB_API Cleanup_HotK(PKHOT_HANDLE_INTERNAL handle)
{
	PKHOT_HANDLE_INTERNAL nextHandle;
	DL_FOREACH(g_HotNotifierList.Items, nextHandle)
	{
		if (nextHandle == handle)
		{
			PoolHandle_Dead_HotK(handle);
			DL_DELETE(g_HotNotifierList.Items, handle);
			break;
		}
	}
}

static BOOL KUSB_API h_DevEnum_UpdateForRemoval(KLST_HANDLE DeviceList, KLST_DEVINFO_HANDLE DeviceInfo, PDEV_BROADCAST_DEVICEINTERFACE_A Context)
{
	UNREFERENCED_PARAMETER(DeviceList);

	if (!DeviceInfo->Connected) return TRUE;
	if (PathMatchSpec(Context->dbcc_name, DeviceInfo->SymbolicLink))
	{
		DeviceInfo->SyncFlags = KLST_SYNC_FLAG_REMOVED;
		DeviceInfo->Connected = FALSE;

		return FALSE;
	}

	return TRUE;
}

static BOOL KUSB_API h_DevEnum_PowerBroadcast(KLST_HANDLE DeviceList, KLST_DEVINFO_HANDLE DeviceInfo, VOID* PbtEvent)
{
	PKHOT_HANDLE_INTERNAL handle;

	UNREFERENCED_PARAMETER(DeviceList);

	/*
	Broadcast the power message to all connected devices registered for hot-plug detection.
	Different registrations that match the same device(s) are allowed; all will be notified.
	*/
	if (!DeviceInfo->Connected) return TRUE;

	DL_FOREACH(g_HotNotifierList.Items, handle)
	{
		if (h_IsHotMatch(handle, DeviceInfo))
		{
			// If the handle was not registered with an OnPowerBroadcast callback; skip it.
			if (!handle->Public.OnPowerBroadcast) continue;

			/* Broadcast the PBT_xx event:
			http://msdn.microsoft.com/en-us/library/windows/desktop/aa373247%28v=vs.85%29.aspx
			Applications will generally be interested in only PBT_APMRESUMEAUTOMATIC and PBT_APMSUSPEND
			*/
			handle->Public.OnPowerBroadcast((KHOT_HANDLE)handle, DeviceInfo, (UINT)(UINT_PTR)PbtEvent);
		}
	}
	return TRUE;
}

static BOOL KUSB_API h_DevEnum_ClearSyncResults(KLST_HANDLE DeviceList, KLST_DEVINFO_HANDLE DeviceInfo, KLST_NOTIFY_CONTEXT* Context)
{
	UNREFERENCED_PARAMETER(DeviceList);
	UNREFERENCED_PARAMETER(Context);

	DeviceInfo->SyncFlags = KLST_SYNC_FLAG_UNCHANGED;

	return TRUE;
}

static BOOL h_Wait_Hwnd(BOOL WaitForExit)
{
	if (WaitForExit)
	{
		if (g_HotNotifierList.Hwnd)
		{
			while (InterlockedCompareExchangePointer(&g_HotNotifierList.Hwnd, NULL, NULL) != NULL)
			{
				SwitchToThread();
				Sleep(0);
			}
		}
		return TRUE;
	}
	else
	{
		if (!g_HotNotifierList.Hwnd)
		{
			while (g_HotNotifierList.ThreadHandle && InterlockedCompareExchangePointer(&g_HotNotifierList.Hwnd, NULL, NULL) == NULL)
			{
				SwitchToThread();
				Sleep(0);
			}
			ErrorSetAction(!g_HotNotifierList.ThreadHandle, ERROR_THREAD_NOT_IN_PROCESS, return FALSE, "->InterlockedCompareExchangePointer");
		}
	}

	return TRUE;
}

static BOOL h_IsHotMatch(PKHOT_HANDLE_INTERNAL HotHandle, KLST_DEVINFO_HANDLE DeviceInfo)
{
	if (g_HotNotifierList.HotInitCount == 1) return TRUE;

	mLst_ApplyPatternMatch(&HotHandle->Public.PatternMatch, DeviceInterfaceGUID, DeviceInfo->DeviceInterfaceGUID, return FALSE);

	mLst_ApplyPatternMatch(&HotHandle->Public.PatternMatch, ClassGUID, DeviceInfo->ClassGUID, return FALSE);

	mLst_ApplyPatternMatch(&HotHandle->Public.PatternMatch, DeviceID, DeviceInfo->DeviceID, return FALSE);

	return TRUE;
}

static BOOL KUSB_API h_DevEnum_RegisterForBroadcast(KLST_HANDLE DeviceList, KLST_DEVINFO_HANDLE DeviceInfo, PKHOT_HANDLE_INTERNAL Context)
{
	UNREFERENCED_PARAMETER(DeviceList);

	if (h_IsHotMatch(Context, DeviceInfo))
	{
		GUID guid;
		PKHOT_BROADCAST_EL hotBroadcast;
		DEV_BROADCAST_DEVICEINTERFACE_A devBroadcast;
		if (!String_To_Guid(&guid, DeviceInfo->DeviceInterfaceGUID))
		{
			USBWRNN("h_String_To_Guid failed for %s", DeviceInfo->DeviceInterfaceGUID);
			return TRUE;
		}
		DL_SEARCH(g_HotNotifierList.BroadcastList, hotBroadcast, guid, hotk_CmpBroadcastGuid);

		if (hotBroadcast) return TRUE;
		memset(&devBroadcast, 0, sizeof(devBroadcast));

		// New broadcast interface
		hotBroadcast = Hot_Mem_Alloc(sizeof(KHOT_BROADCAST_EL));
		ErrorMemory(!IsHandleValid(hotBroadcast), Error);

		memcpy(&hotBroadcast->InterfaceGUID, &guid, sizeof(guid));

		devBroadcast.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		devBroadcast.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE_A);
		memcpy(&devBroadcast.dbcc_classguid, &hotBroadcast->InterfaceGUID, sizeof(guid));

		hotBroadcast->NotifyHandle = RegisterDeviceNotificationA(g_HotNotifierList.Hwnd, &devBroadcast, DEVICE_NOTIFY_WINDOW_HANDLE);
		if (!hotBroadcast->NotifyHandle)
		{
			USBWRNN("h_String_To_Guid failed for %s", DeviceInfo->DeviceInterfaceGUID);
			Hot_Mem_Free(hotBroadcast);
			return TRUE;
		}
		USBDBGN("Registered:hotBroadcast->NotifyHandle = %p", hotBroadcast->NotifyHandle);

		DL_APPEND(g_HotNotifierList.BroadcastList, hotBroadcast);

	}
	return TRUE;

Error:
	return FALSE;
}

static BOOL h_RegisterForBroadcast(PKHOT_HANDLE_INTERNAL HotHandle)
{

#ifdef DEFER_THRU_TIMER
	if (HotHandle)
		return LstK_Enumerate(g_HotNotifierList.DeviceList, h_DevEnum_RegisterForBroadcast, HotHandle);

	USBDBGN("Global %s broadcast re-registration. hot-count=%d", g_HotNotifierList.WindowName, g_HotNotifierList.HotInitCount);

	DL_FOREACH(g_HotNotifierList.Items, HotHandle)
	{
		LstK_Enumerate(g_HotNotifierList.DeviceList, h_DevEnum_RegisterForBroadcast, HotHandle);
	}
#endif

	return TRUE;
}

static BOOL KUSB_API h_DevEnum_PlugWaiters(KLST_HANDLE DeviceList, KLST_DEVINFO_HANDLE DeviceInfo, KLST_NOTIFY_CONTEXT* Context)
{
	PKHOT_HANDLE_INTERNAL handle;
	PKUSB_STR_EL devInstEL = NULL;

	UNREFERENCED_PARAMETER(DeviceList);

	if (Context->HotHandle && !DeviceInfo->Connected) return TRUE;

	DL_FOREACH(g_HotNotifierList.Items, handle)
	{
		if (Context->HotHandle && Context->HotHandle != handle)
			continue;

		if (Context->HotHandle)
			DeviceInfo->SyncFlags |= KLST_SYNC_FLAG_ADDED;

		// Nothing to do for this element.
		if (DeviceInfo->SyncFlags == KLST_SYNC_FLAG_NONE || DeviceInfo->SyncFlags == KLST_SYNC_FLAG_UNCHANGED)
			continue;

		if (h_IsHotMatch(handle, DeviceInfo))
		{
			// A device instance will only be notified once per WM_DEVICECHANGE
			DL_FOREACH(Context->DevInstList, devInstEL)
			{
				if (strcmp(devInstEL->Value, DeviceInfo->DeviceID) == 0)
					break;
			}

			if (devInstEL && !(handle->Public.Flags & KHOT_FLAG_PASS_DUPE_INSTANCE)) continue;

			if (!devInstEL)
			{
				devInstEL = Hot_Mem_Alloc(sizeof(*devInstEL));
				devInstEL->Value = DeviceInfo->DeviceID;
				DL_APPEND(Context->DevInstList, devInstEL);
			}

			if (DeviceInfo->SyncFlags & KLST_SYNC_FLAG_ADDED)
			{
				if (handle->Public.OnHotPlug)
					handle->Public.OnHotPlug((KHOT_HANDLE)handle, DeviceInfo, KLST_SYNC_FLAG_ADDED);

				if (handle->Public.UserHwnd && handle->Public.UserMessage >= WM_USER)
				{
					if (handle->Public.Flags & KHOT_FLAG_POST_USER_MESSAGE)
						PostMessageA(handle->Public.UserHwnd, (UINT)handle->Public.UserMessage + 1, (WPARAM)handle, (LPARAM)DeviceInfo);
					else
						SendMessageA(handle->Public.UserHwnd, (UINT)handle->Public.UserMessage + 1, (WPARAM)handle, (LPARAM)DeviceInfo);
				}
			}
			else if (DeviceInfo->SyncFlags & KLST_SYNC_FLAG_REMOVED)
			{
				if (handle->Public.OnHotPlug)
					handle->Public.OnHotPlug((KHOT_HANDLE)handle, DeviceInfo, KLST_SYNC_FLAG_REMOVED);

				if (handle->Public.UserHwnd && handle->Public.UserMessage >= WM_USER)
				{
					if (handle->Public.Flags & KHOT_FLAG_POST_USER_MESSAGE)
						PostMessageA(handle->Public.UserHwnd, (UINT)handle->Public.UserMessage, (WPARAM)handle, (LPARAM)DeviceInfo);
					else
						SendMessageA(handle->Public.UserHwnd, (UINT)handle->Public.UserMessage, (WPARAM)handle, (LPARAM)DeviceInfo);
				}
			}
		}
	}

	return TRUE;
}

static LRESULT CALLBACK h_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PDEV_BROADCAST_DEVICEINTERFACE_A devInterface;
	KHOT_HANDLE Handle;
	PKHOT_BROADCAST_EL hotBroadcast, hotBroadcastTmp;
	PKHOT_HANDLE_INTERNAL handle = NULL;
	PKHOT_HANDLE_INTERNAL handleTmp = NULL;
	PKHOT_HANDLE_INTERNAL* handleRef;
	KHOT_PARAMS* InitParams;

	switch(msg)
	{
	case WM_USER_INIT_HOT_HANDLE:
		handleRef	= (PKHOT_HANDLE_INTERNAL*)wParam;
		InitParams	= (KHOT_PARAMS*)lParam;

		ErrorHandleAction(!IsHandleValid(handleRef), "Handle", return (LRESULT)ERROR_INVALID_HANDLE);
		ErrorParamAction(!IsHandleValid(InitParams), "InitParams", return (LRESULT)ERROR_INVALID_PARAMETER);

		handle = PoolHandle_Acquire_HotK(NULL);
		ErrorNoSetAction(!IsHandleValid(handle), return (LRESULT)GetLastError(), "->PoolHandle_Acquire_HotK");

		memcpy(&handle->Public, InitParams, sizeof(handle->Public));
		*handleRef = handle;

		// Add to the list and set the cleaunup callback for the hot handle
		handle->Base.Evt.Cleanup = Cleanup_HotK;

		if (IncLock(g_HotNotifierList.HotInitCount) == 1)
		{
		}

		DL_APPEND(g_HotNotifierList.Items, handle);

		h_RegisterForBroadcast(handle);
		USBDBGN("h_RegisterForBroadcast(handle):WM_USER_INIT_HOT_HANDLE");

		PoolHandle_Live_HotK(handle);

		if (handle->Public.Flags & KHOT_FLAG_PLUG_ALL_ON_INIT)
			h_NotifyWaiters(handle, TRUE);

		return (LRESULT)ERROR_SUCCESS;

	case WM_USER_FREE_HOT_HANDLE:
		Handle = (KHOT_HANDLE)wParam;
		Pub_To_Priv_HotK(Handle, handle, return (LRESULT)FALSE);
		ErrorNoSetAction(!PoolHandle_Inc_HotK(handle), return (LRESULT)FALSE, "->PoolHandle_Inc_HotK");

		DecLock(g_HotNotifierList.HotInitCount);

		PoolHandle_Dec_HotK(handle);
		PoolHandle_Dec_HotK(handle);
		return (LRESULT)TRUE;

	case WM_TIMER:

		// The WM_TIMER message is a low-priority message. The GetMessage and
		// PeekMessage functions post this message only when no other
		// higher-priority messages are in the thread's message queue.

		switch(wParam)
		{
		case IDT_KHOT_DBT_DEVICEARRIVAL:
			KillTimer(hwnd, wParam);	// !!Kill Timer!!

			/*
			Sync the current device list with a new one.
			If we only have one hot handle, then we can pass the PatternMatch to the list and improve performance.
			*/
			LstK_Sync(g_HotNotifierList.DeviceList, NULL, KLST_SYNC_FLAG_MASK, g_HotNotifierList.HotInitCount == 1 ? &g_HotNotifierList.Items->Public.PatternMatch : NULL, g_HotNotifierList.ActiveHeap);

			// notify hot handle waiters
			h_NotifyWaiters(NULL, TRUE);

			break;
		case IDT_KHOT_DBT_DEVNODES_CHANGED:
			KillTimer(hwnd, wParam);	// !!Kill Timer!!

			/*
			Sync the current device list with a new one.
			If we only have one hot handle, then we can pass the PatternMatch to the list and improve performance.
			*/
			LstK_Sync(g_HotNotifierList.DeviceList, NULL, KLST_SYNC_FLAG_MASK, g_HotNotifierList.HotInitCount == 1 ? &g_HotNotifierList.Items->Public.PatternMatch : NULL, g_HotNotifierList.ActiveHeap);

			// notify hot handle waiters
			h_NotifyWaiters(NULL, TRUE);

			// dev broadcast re-registration
			h_RegisterForBroadcast(NULL);
			USBDBGN("h_RegisterForBroadcast(NULL):IDT_KHOT_DBT_DEVNODES_CHANGED");

			break;
		case IDT_KHOT_DBT_DEVICEREMOVAL:
			KillTimer(hwnd, wParam);	// !!Kill Timer!!

			// notify hot handle waiters
			h_NotifyWaiters(NULL, TRUE);

			break;

		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return (LRESULT)FALSE;

	case WM_DEVICECHANGE:

		devInterface = (PDEV_BROADCAST_DEVICEINTERFACE_A)lParam;

		switch(wParam)
		{

#ifdef DEFER_THRU_TIMER
		case DBT_DEVICEARRIVAL:
			if (!devInterface || devInterface->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE)
				break;

			// MaxRefreshMS (1 second) minimum DEVICEARRIVAL delay.
			SetTimer(hwnd, IDT_KHOT_DBT_DEVICEARRIVAL, g_HotNotifierList.MaxRefreshMS, (TIMERPROC) NULL);

			break;

		case DBT_DEVICEREMOVECOMPLETE:
			if (!devInterface || devInterface->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE)
				break;
			if (!strlen(devInterface->dbcc_name))
			{
				USBWRNN("Zero length devInterface->dbcc_name.");
				SetTimer(hwnd, IDT_KHOT_DBT_DEVICEARRIVAL, g_HotNotifierList.MaxRefreshMS, (TIMERPROC) NULL);
				break;
			}
			USBDBGN("[DBT_DEVICEREMOVECOMPLETE] dbcc_name: %s", devInterface->dbcc_name);

			LstK_Enumerate(g_HotNotifierList.DeviceList, h_DevEnum_UpdateForRemoval, devInterface);
			SetTimer(hwnd, IDT_KHOT_DBT_DEVICEREMOVAL, 1, (TIMERPROC) NULL);

			break;
#endif
		case DBT_DEVNODES_CHANGED:
			// 2 second minimum DEVNODES_CHANGED delay.
			SetTimer(hwnd, IDT_KHOT_DBT_DEVNODES_CHANGED, 2000, (TIMERPROC) NULL);
			break;
		default:
			break;

		}

		return (LRESULT)TRUE;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		// free remaining hot handles
		DL_FOREACH_SAFE(g_HotNotifierList.Items, handle, handleTmp)
		{
			PoolHandle_Dec_HotK(handle);
		}

		// free remaining broadcast registrations
		DL_FOREACH_SAFE(g_HotNotifierList.BroadcastList, hotBroadcast, hotBroadcastTmp)
		{
			if (UnregisterDeviceNotification(hotBroadcast->NotifyHandle))
			{
				USBDBGN("Unregistered:hotBroadcast->NotifyHandle = %p", hotBroadcast->NotifyHandle);
			}
			else
			{
				USBERRN("Failed unregistering hotBroadcast->NotifyHandle = %p", hotBroadcast->NotifyHandle);
			}

			DL_DELETE(g_HotNotifierList.BroadcastList, hotBroadcast);
			Hot_Mem_Free(hotBroadcast);
		}

		// free the master device list
		LstK_Free(g_HotNotifierList.DeviceList);
		g_HotNotifierList.DeviceList = NULL;
		InterlockedExchangePointer(&g_HotNotifierList.Hwnd, NULL);
		PostQuitMessage(0);
		break;
	case WM_POWERBROADCAST:

		if (wParam == PBT_APMRESUMEAUTOMATIC || wParam == PBT_APMSUSPEND)
		{
			LstK_Enumerate(g_HotNotifierList.DeviceList, h_DevEnum_PowerBroadcast, (PVOID)wParam);
		}

		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return (LRESULT)FALSE;
}

static BOOL h_Register_Atom(PKHOT_NOTIFIER_LIST NotifierList)
{
	WNDCLASSEXA wc;

	if (NotifierList->WindowAtom) return TRUE;

	memset(&wc, 0, sizeof(wc));

	// Registering the Window Class
	wc.cbSize        = sizeof(wc);
	wc.style         = 0;
	wc.lpfnWndProc   = h_WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = g_HotNotifierList.hAppInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
#pragma warning(disable:4306)
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
#pragma warning(default:4306)
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = g_WindowClassHotK;
	wc.hIconSm       = NULL;

	NotifierList->WindowAtom = RegisterClassExA(&wc);
	ErrorNoSet(!NotifierList->WindowAtom, Error, "RegisterClassEx failed.");

	return TRUE;
Error:
	return FALSE;
}

static BOOL h_Create_Hwnd(PKHOT_NOTIFIER_LIST NotifierList, HWND* hwnd)
{
	ULONG count = 0;
	UNREFERENCED_PARAMETER(NotifierList);

	*hwnd = NULL;

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
		USBERRN("Too many notification windows currently running. count=%u", count);
		return LusbwError(ERROR_TOO_MANY_MODULES);
	}

	*hwnd = g_HotNotifierList.Hwnd = CreateWindowExA(
	                                     WS_EX_CLIENTEDGE,
	                                     g_WindowClassHotK,
	                                     g_HotNotifierList.WindowName,
	                                     WS_OVERLAPPEDWINDOW,
	                                     -100, -100, 20, 20,
	                                     NULL, NULL, g_HotNotifierList.hAppInstance, NULL);

	if(!IsHandleValid(g_HotNotifierList.Hwnd))
	{
		USBERRN("RegisterClassEx failed. ErrorCode=%08Xh", GetLastError());
		return FALSE;
	}

	ShowWindow(g_HotNotifierList.Hwnd, SW_HIDE);
	EnableWindow(g_HotNotifierList.Hwnd, TRUE);
	return TRUE;
}


static unsigned _stdcall h_ThreadProc(PKHOT_NOTIFIER_LIST NotifierList)
{
	HWND hwnd;
	MSG msg;
	DWORD exitCode = 0;

	g_HotNotifierList.ActiveHeap = g_HotNotifierList.Heap;

	if (!h_Register_Atom(NotifierList))
	{
		exitCode = GetLastError();
		goto Done;
	}
	if (!h_Create_Hwnd(NotifierList, &hwnd))
	{
		exitCode = GetLastError();
		goto Done;
	}

	while(GetMessageA(&msg, hwnd, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

Done:
	g_HotNotifierList.ThreadHandle = NULL;
	HeapDestroy(g_HotNotifierList.ActiveHeap);

	_endthreadex(exitCode);
	return exitCode;
}

static BOOL h_Create_Thread(PKHOT_NOTIFIER_LIST NotifierList)
{
	g_HotNotifierList.ThreadHandle = (HANDLE)_beginthreadex( NULL, 0, &h_ThreadProc, NotifierList, CREATE_SUSPENDED, &g_HotNotifierList.ThreadID);
	if (!IsHandleValid(g_HotNotifierList.ThreadHandle))
	{
		USBERRN("_beginthreadex failed. ErrorCode=%08Xh", GetLastError());
		return FALSE;
	}

	return TRUE;
}

KUSB_EXP BOOL KUSB_API HotK_Init(
    _out KHOT_HANDLE* Handle,
    _ref PKHOT_PARAMS InitParams)
{
	DWORD errorCode;
	BOOL success;
	PKLST_PATTERN_MATCH patternMatch;

	// Create the top-level window for monitoring. WM_DEVICE_CHANGE.
	if (IncLock(g_HotNotifierList.HotLockCount) == 1)
	{
		patternMatch = &InitParams->PatternMatch;
		if (!patternMatch->DeviceInterfaceGUID[0] && !patternMatch->DeviceID[0] && !patternMatch->ClassGUID[0])
			patternMatch = NULL;

		if (!g_HotNotifierList.hAppInstance) g_HotNotifierList.hAppInstance = GetModuleHandle(NULL);

		g_HotNotifierList.Heap = HeapCreate(HEAP_NO_SERIALIZE | HEAP_GENERATE_EXCEPTIONS, 16384, 0);

		success = LstK_InitInternal(&g_HotNotifierList.DeviceList, KLST_FLAG_NONE, patternMatch, g_HotNotifierList.Heap);
		ErrorNoSet(!success, Error, "Failed creating master device list.");

		success = h_Create_Thread(&g_HotNotifierList);
		ErrorNoSet(!success, Error, "->h_Create_Thread");

		errorCode = ResumeThread(g_HotNotifierList.ThreadHandle);
		ErrorNoSet(errorCode == ((DWORD) - 1), Error, "->ResumeThread");

		// wait for the window to load
		ErrorNoSet(!h_Wait_Hwnd(FALSE), Error, "->h_Wait_Hwnd");
	}

	errorCode = (DWORD)SendMessageA(g_HotNotifierList.Hwnd, WM_USER_INIT_HOT_HANDLE, (WPARAM)Handle, (LPARAM)InitParams);
	if (errorCode != ERROR_SUCCESS)
	{
		if (DecLock(g_HotNotifierList.HotLockCount) == 0)
		{
			HotK_FreeAll();
			h_Wait_Hwnd(TRUE);
		}
		SetLastError(errorCode);
		return FALSE;
	}

	return errorCode == ERROR_SUCCESS;

Error:
	LstK_Free(g_HotNotifierList.DeviceList);
	g_HotNotifierList.DeviceList = NULL;
	return FALSE;
}

KUSB_EXP BOOL KUSB_API HotK_Free(
    _in KHOT_HANDLE Handle)
{
	PKHOT_HANDLE_INTERNAL handle;
	long lockCnt;

	ErrorSetAction(!g_HotNotifierList.Hwnd, ERROR_NOT_FOUND, return FALSE, "The notifier window is not active.");

	Pub_To_Priv_HotK(Handle, handle, return FALSE);
	if ((lockCnt = DecLock(g_HotNotifierList.HotLockCount)) < 0)
	{
		IncLock(g_HotNotifierList.HotLockCount);
		return FALSE;
	}

	if (!SendMessageA(g_HotNotifierList.Hwnd, WM_USER_FREE_HOT_HANDLE, (WPARAM)handle, (LPARAM)NULL))
		return FALSE;

	if (lockCnt == 0)
	{
		USBMSGN("All HotK handles freed; closing notifier thread..");
		HotK_FreeAll();

		// wait for the window to exit
		h_Wait_Hwnd(TRUE);
	}
	return TRUE;
}

KUSB_EXP VOID KUSB_API HotK_FreeAll(VOID)
{
	ErrorSetAction(!g_HotNotifierList.Hwnd, ERROR_NOT_FOUND, return, "The notifier window is not active.");
	SendMessageA(g_HotNotifierList.Hwnd, WM_DESTROY, (WPARAM)0, (LPARAM)0);
}
