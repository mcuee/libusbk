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

#define IDT_KHOT_DBT_DEVNODES_CHANGED		0xA
#define IDT_KHOT_DBT_DEVICEARRIVAL			0xB
#define IDT_KHOT_DBT_DEVICEREMOVAL			0xC

typedef struct _KHOT_BROADCAST_EL
{
	HDEVNOTIFY NotifyHandle;
	GUID InterfaceGUID;

	struct _KHOT_BROADCAST_EL* prev;
	struct _KHOT_BROADCAST_EL* next;
} KHOT_BROADCAST_EL, *PKHOT_BROADCAST_EL;

typedef struct _KHOT_NOTIFIER_LIST
{
	volatile long Lock;

	KLST_HANDLE DeviceList;
	ULONG MaxRefreshMS;

	PKHOT_HANDLE_INTERNAL Items;
	LONG Count;

	CHAR WindowName[32];
	HINSTANCE hAppInstance;

	HANDLE ThreadHandle;
	UINT ThreadID;

	PKHOT_BROADCAST_EL BroadcastList;

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
	PKHOT_HANDLE_INTERNAL HotHandle;
	PKUSB_STR_EL DevInstList;
} KLST_NOTIFY_CONTEXT, *PKLST_NOTIFY_CONTEXT;

static LPCSTR g_WindowClassHotK = "HotK_NotificationWindowClass";
static KHOT_NOTIFIER_LIST g_HotNotifierList = {0, NULL, 1000, NULL, 0, {0}, NULL, NULL, 0, NULL, NULL, 0};

static LRESULT CALLBACK hotk_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL hotk_RegisterWindowClass(PKHOT_NOTIFIER_LIST NotifierList);
static BOOL hotk_CreateWindow(PKHOT_NOTIFIER_LIST NotifierList);
static BOOL hotk_CreateThread(PKHOT_NOTIFIER_LIST NotifierList);
static unsigned _stdcall hotk_ThreadProc(PKHOT_NOTIFIER_LIST NotifierList);

static BOOL KUSB_API hotk_DevEnum_PlugWaiters(KLST_HANDLE DeviceList, PKLST_DEV_INFO DeviceInfo, PKLST_NOTIFY_CONTEXT Context);
static BOOL KUSB_API hotk_DevEnum_PlugAllForWaiter(KLST_HANDLE DeviceList, PKLST_DEV_INFO DeviceInfo, PKLST_NOTIFY_CONTEXT Context);
static BOOL KUSB_API hotk_DevEnum_ClearSyncResults(KLST_HANDLE DeviceList, PKLST_DEV_INFO DeviceInfo, PKLST_NOTIFY_CONTEXT Context);
static BOOL hotk_NotifyWaiters(__in_opt PKHOT_HANDLE_INTERNAL HotHandle, BOOL ClearSyncResultsWhenComplete);

static BOOL hotk_NotifyWaiters(__in_opt PKHOT_HANDLE_INTERNAL HotHandle, BOOL ClearSyncResultsWhenComplete)
{
	PKUSB_STR_EL strEL, strTmp;
	KLST_NOTIFY_CONTEXT hotCtx;

	memset(&hotCtx, 0, sizeof(hotCtx));
	hotCtx.HotHandle = HotHandle;

	LstK_Enumerate(g_HotNotifierList.DeviceList, hotk_DevEnum_PlugWaiters, &hotCtx);

	if (ClearSyncResultsWhenComplete)
		LstK_Enumerate(g_HotNotifierList.DeviceList, hotk_DevEnum_ClearSyncResults, &hotCtx);

	KUSB_STR_EL_CLEANUP(hotCtx.DevInstList, strEL, strTmp);

	return TRUE;
}

#define IsPatternMatch(IsMatch,HotHandle,DeviceInfo,FieldName) if (strlen(HotHandle->Public.PatternMatch.FieldName)) {		\
	if (!MatchInstanceID(HotHandle->Public.PatternMatch.FieldName, DeviceInfo->FieldName))									\
		IsMatch = FALSE;																									\
}

static BOOL KUSB_API hotk_DevEnum_UpdateForRemoval(KLST_HANDLE DeviceList, PKLST_DEV_INFO DeviceInfo, PDEV_BROADCAST_DEVICEINTERFACE_A Context)
{
	UNREFERENCED_PARAMETER(DeviceList);

	if (!DeviceInfo->Connected) return TRUE;
	if (MatchInstanceID(DeviceInfo->SymbolicLink, Context->dbcc_name))
	{
		DeviceInfo->SyncResults.Connected = 0;
		DeviceInfo->SyncResults.Removed = 1;
		DeviceInfo->Connected = FALSE;

		return FALSE;
	}

	return TRUE;
}

static BOOL KUSB_API hotk_DevEnum_ClearSyncResults(KLST_HANDLE DeviceList, PKLST_DEV_INFO DeviceInfo, PKLST_NOTIFY_CONTEXT Context)
{
	UNREFERENCED_PARAMETER(DeviceList);
	UNREFERENCED_PARAMETER(Context);

	DeviceInfo->SyncResults.SyncFlags = SYNC_FLAG_NONE;

	return TRUE;
}

#define GUID_MAXSIZE 38
#define GUID_FORMAT_STRING "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"

static BOOL GuidFromString(__inout GUID* GuidVal, __in LPCSTR GuidString)
{
	int scanCount;
	UCHAR guidChars[11 * sizeof(int)];
	GUID* Guid = (GUID*)&guidChars;


	if (GuidString[0] == '{') GuidString++;

	scanCount = sscanf_s(GuidString, GUID_FORMAT_STRING,
	                     &Guid->Data1,
	                     &Guid->Data2,
	                     &Guid->Data3,
	                     &Guid->Data4[0], &Guid->Data4[1], &Guid->Data4[2], &Guid->Data4[3],
	                     &Guid->Data4[4], &Guid->Data4[5], &Guid->Data4[6], &Guid->Data4[7]);

	if (scanCount == 11)
		memcpy(GuidVal, &guidChars, sizeof(GUID));

	return (scanCount == 11);
}

static BOOL GuidToString(__in GUID* Guid, __inout LPSTR GuidString)
{
	int guidLen;

	sprintf_s(GuidString, GUID_MAXSIZE, "{"GUID_FORMAT_STRING"}",
	          Guid->Data1,
	          Guid->Data2,
	          Guid->Data3,
	          Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], Guid->Data4[3],
	          Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]);

	return (guidLen == GUID_MAXSIZE - 1);
}

static BOOL hotk_IsHotMatch(PKHOT_HANDLE_INTERNAL HotHandle, PKLST_DEV_INFO DeviceInfo)
{
	BOOL isMatch = TRUE;

	IsPatternMatch(isMatch, HotHandle, DeviceInfo, InstanceID);
	if (!isMatch) return FALSE;

	IsPatternMatch(isMatch, HotHandle, DeviceInfo, DeviceInterfaceGUID);
	if (!isMatch) return FALSE;

	IsPatternMatch(isMatch, HotHandle, DeviceInfo, DevicePath);
	if (!isMatch) return FALSE;

	return isMatch;
}
#define hotk_CmpBroadcastGuid(BroadcastEL, DevIntfGUID) memcpy(&BroadcastEL->InterfaceGUID,&DevIntfGUID,sizeof(GUID))

static BOOL KUSB_API hotk_DevEnum_RegisterForBroadcast(KLST_HANDLE DeviceList, PKLST_DEV_INFO DeviceInfo, PKHOT_HANDLE_INTERNAL Context)
{
	UNREFERENCED_PARAMETER(DeviceList);

	if (hotk_IsHotMatch(Context, DeviceInfo))
	{
		GUID guid;
		PKHOT_BROADCAST_EL hotBroadcast;
		DEV_BROADCAST_DEVICEINTERFACE_A devBroadcast;
		if (!GuidFromString(&guid, DeviceInfo->DeviceInterfaceGUID))
		{
			USBWRN("GuidFromString failed for %s\n", DeviceInfo->DeviceInterfaceGUID);
			return TRUE;
		}
		DL_SEARCH(g_HotNotifierList.BroadcastList, hotBroadcast, guid, hotk_CmpBroadcastGuid);

		if (hotBroadcast) return TRUE;
		memset(&devBroadcast, 0, sizeof(devBroadcast));

		// New broadcast interface
		hotBroadcast = Mem_Alloc(sizeof(KHOT_BROADCAST_EL));
		ErrorMemory(!IsHandleValid(hotBroadcast), Error);

		memcpy(&hotBroadcast->InterfaceGUID, &guid, sizeof(guid));

		devBroadcast.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		devBroadcast.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE_A);
		memcpy(&devBroadcast.dbcc_classguid, &hotBroadcast->InterfaceGUID, sizeof(guid));

		hotBroadcast->NotifyHandle = RegisterDeviceNotificationA(g_HotNotifierList.Hwnd, &devBroadcast, DEVICE_NOTIFY_WINDOW_HANDLE);
		if (!hotBroadcast->NotifyHandle)
		{
			USBWRN("GuidFromString failed for %s\n", DeviceInfo->DeviceInterfaceGUID);
			Mem_Free(&hotBroadcast);
			return TRUE;
		}

		DL_APPEND(g_HotNotifierList.BroadcastList, hotBroadcast);

	}
	return TRUE;

Error:
	return FALSE;
}

static BOOL hotk_RegisterForBroadcast(PKHOT_HANDLE_INTERNAL HotHandle)
{

	if (HotHandle)
		return LstK_Enumerate(g_HotNotifierList.DeviceList, hotk_DevEnum_RegisterForBroadcast, HotHandle);

	USBDBG("Global %s broadcast re-registration. hot-count=%d\n", g_HotNotifierList.WindowName, g_HotNotifierList.Count);

	DL_FOREACH(g_HotNotifierList.Items, HotHandle)
	{
		LstK_Enumerate(g_HotNotifierList.DeviceList, hotk_DevEnum_RegisterForBroadcast, HotHandle);
	}

	return TRUE;
}

static BOOL KUSB_API hotk_DevEnum_PlugWaiters(KLST_HANDLE DeviceList, PKLST_DEV_INFO DeviceInfo, PKLST_NOTIFY_CONTEXT Context)
{
	PKHOT_HANDLE_INTERNAL hotHandle;
	PKUSB_STR_EL devInstEL = NULL;

	UNREFERENCED_PARAMETER(DeviceList);

	if (Context->HotHandle && !DeviceInfo->Connected) return TRUE;

	DL_FOREACH(g_HotNotifierList.Items, hotHandle)
	{
		if (Context->HotHandle && Context->HotHandle != hotHandle)
			continue;

		if (Context->HotHandle)
		{
			DeviceInfo->SyncResults.Connected = 1;
			DeviceInfo->SyncResults.Added = 1;
		}

		// Nothing to do for this element.
		if (DeviceInfo->SyncResults.SyncFlags == SYNC_FLAG_NONE)
			continue;

		if (hotk_IsHotMatch(hotHandle, DeviceInfo))
		{
			// A device instance will only be notified once per WM_DEVICECHANGE
			DL_FOREACH(Context->DevInstList, devInstEL)
			{
				if (strcmp(devInstEL->Value, DeviceInfo->InstanceID) == 0)
					break;
			}

			if (devInstEL && !hotHandle->Public.Flags.AllowDupeInstanceIDs) continue;

			if (!devInstEL)
			{
				devInstEL = Mem_Alloc(sizeof(*devInstEL));
				devInstEL->Value = DeviceInfo->InstanceID;
				DL_APPEND(Context->DevInstList, devInstEL);
			}

			if (DeviceInfo->SyncResults.Added || DeviceInfo->SyncResults.Connected)
			{
				hotHandle->Public.DeviceInfo = DeviceInfo;

				if (hotHandle->Public.OnHotPlug)
					hotHandle->Public.OnHotPlug(hotHandle, &hotHandle->Public, DeviceInfo, KHOT_PLUG_ARRIVAL);

				if (hotHandle->Public.UserHwnd && hotHandle->Public.UserMessage >= WM_USER)
				{
					if (hotHandle->Public.Flags.PostUserMessage)
						PostMessageA(hotHandle->Public.UserHwnd, hotHandle->Public.UserMessage, (WPARAM)&hotHandle->Public, (LPARAM)KHOT_PLUG_ARRIVAL);
					else
						SendMessageA(hotHandle->Public.UserHwnd, hotHandle->Public.UserMessage, (WPARAM)&hotHandle->Public, (LPARAM)KHOT_PLUG_ARRIVAL);
				}
			}
			else if (DeviceInfo->SyncResults.Removed)
			{
				hotHandle->Public.DeviceInfo = DeviceInfo;

				if (hotHandle->Public.OnHotPlug)
					hotHandle->Public.OnHotPlug(hotHandle, &hotHandle->Public, DeviceInfo, KHOT_PLUG_REMOVAL);

				if (hotHandle->Public.UserHwnd && hotHandle->Public.UserMessage >= WM_USER)
				{
					if (hotHandle->Public.Flags.PostUserMessage)
						PostMessageA(hotHandle->Public.UserHwnd, hotHandle->Public.UserMessage, (WPARAM)&hotHandle->Public, (LPARAM)KHOT_PLUG_REMOVAL);
					else
						SendMessageA(hotHandle->Public.UserHwnd, hotHandle->Public.UserMessage, (WPARAM)&hotHandle->Public, (LPARAM)KHOT_PLUG_REMOVAL);
				}
			}
		}
	}

	return TRUE;
}


#define HOTWND_LOCK_ACQUIRE()do {						\
	SpinLock_Acquire(&g_HotNotifierList.Lock, TRUE);	\
	if (!g_HotNotifierList.Hwnd)						\
	{   												\
		SpinLock_Release(&g_HotNotifierList.Lock);  	\
		return (LRESULT)FALSE;  						\
	}   												\
}   													\
while(0)
#define HOTWND_LOCK_RELEASE() SpinLock_Release(&g_HotNotifierList.Lock)

static LRESULT CALLBACK hotk_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PDEV_BROADCAST_DEVICEINTERFACE_A devInterface;

	switch(msg)
	{
	case WM_TIMER:

		// The WM_TIMER message is a low-priority message. The GetMessage and
		// PeekMessage functions post this message only when no other
		// higher-priority messages are in the thread's message queue.

		switch(wParam)
		{
		case IDT_KHOT_DBT_DEVICEARRIVAL:
			KillTimer(hwnd, wParam);	// !!Kill Timer!!

			HOTWND_LOCK_ACQUIRE();

			// re/sync the device list
			LstK_Sync(g_HotNotifierList.DeviceList, NULL, NULL);

			// notify hot handle waiters
			hotk_NotifyWaiters(NULL, TRUE);

			HOTWND_LOCK_RELEASE();
			break;
		case IDT_KHOT_DBT_DEVNODES_CHANGED:
			KillTimer(hwnd, wParam);	// !!Kill Timer!!

			HOTWND_LOCK_ACQUIRE();

			// dev broadcast re-registration
			hotk_RegisterForBroadcast(NULL);

			HOTWND_LOCK_RELEASE();
			break;
		case IDT_KHOT_DBT_DEVICEREMOVAL:
			KillTimer(hwnd, wParam);	// !!Kill Timer!!

			HOTWND_LOCK_ACQUIRE();

			// notify hot handle waiters
			hotk_NotifyWaiters(NULL, TRUE);

			HOTWND_LOCK_RELEASE();
			break;

		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return (LRESULT)FALSE;

	case WM_DEVICECHANGE:

		devInterface = (PDEV_BROADCAST_DEVICEINTERFACE_A)lParam;

		switch(wParam)
		{
		case DBT_DEVICEARRIVAL:
			if (!devInterface || devInterface->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE)
				break;
			// 1 second minimum DEVICEARRIVAL delay.
			SetTimer(hwnd, IDT_KHOT_DBT_DEVICEARRIVAL, g_HotNotifierList.MaxRefreshMS, (TIMERPROC) NULL);
			break;

		case DBT_DEVICEREMOVECOMPLETE:
		case DBT_DEVICEREMOVEPENDING:
			if (!devInterface || devInterface->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE)
				break;
			if (!strlen(devInterface->dbcc_name))
			{
				USBWRN("Zero length devInterface->dbcc_name.\n");
				SetTimer(hwnd, IDT_KHOT_DBT_DEVICEARRIVAL, g_HotNotifierList.MaxRefreshMS, (TIMERPROC) NULL);
				break;
			}

			HOTWND_LOCK_ACQUIRE();
			LstK_Enumerate(g_HotNotifierList.DeviceList, hotk_DevEnum_UpdateForRemoval, devInterface);
			HOTWND_LOCK_RELEASE();

			// 100 ms delay.
			SetTimer(hwnd, IDT_KHOT_DBT_DEVICEREMOVAL, 100, (TIMERPROC) NULL);

			break;
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
	PKHOT_BROADCAST_EL hotBroadcast, tmp;

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

	if (!hotk_RegisterForBroadcast(g_HotNotifierList.Items))
	{
		USBWRN("hotk_RegisterForBroadcast Failed.\n");
	}

	SpinLock_Release(&g_HotNotifierList.Lock);

	while(IsHandleValid(g_HotNotifierList.Hwnd) && GetMessageA(&msg, g_HotNotifierList.Hwnd, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	SpinLock_Acquire(&g_HotNotifierList.Lock, TRUE);
	DL_FOREACH_SAFE(g_HotNotifierList.BroadcastList, hotBroadcast, tmp)
	{
		UnregisterDeviceNotification(hotBroadcast->NotifyHandle);

		DL_DELETE(g_HotNotifierList.BroadcastList, hotBroadcast);
		Mem_Free(&hotBroadcast);
	}
	SpinLock_Release(&g_HotNotifierList.Lock);

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

	if (!g_HotNotifierList.Items)
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

	isLocked = SpinLock_Acquire(&g_HotNotifierList.Lock, TRUE);

	hotHandle = PoolHandle_Acquire_HotK(&hotk_Free);
	ErrorNoSet(!IsHandleValid(hotHandle), Error, "->PoolHandle_Acquire_HotK");

	memcpy(&hotHandle->Public, InitParams, sizeof(hotHandle->Public));

	DL_APPEND(g_HotNotifierList.Items, hotHandle);

	// Create the top-level window for monitoring. WM_DEVICE_CHANGE.
	if (!g_HotNotifierList.Hwnd)
	{
		KLST_INIT_PARAMS listInit;

		Mem_Zero(&listInit, sizeof(listInit));
		listInit.ShowDisconnectedDevices = TRUE;

		LstK_Init(&g_HotNotifierList.DeviceList, &listInit);
		if (!g_HotNotifierList.hAppInstance)
			g_HotNotifierList.hAppInstance = GetModuleHandle(NULL);

		ErrorNoSet(!hotk_CreateThread(NotifierList), Error, "->hotk_CreateThread");

		ResumeThread(g_HotNotifierList.ThreadHandle);
	}

	if (InitParams->Flags.PlugAllOnInit)
	{
		hotk_NotifyWaiters(hotHandle, TRUE);
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
	PKHOT_HANDLE_INTERNAL hotHandle;
	int count = g_HotNotifierList.Count;

	while(count-- > 0)
	{
		DL_FOREACH(g_HotNotifierList.Items, hotHandle);
		{
			if (count-- <= 0) return;
			if (!HotK_Free(&hotHandle)) return;

			break;
		}
	}
}
