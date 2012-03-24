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

#include "lusbk_handles.h"

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

volatile long AllKInitLock = 0;

PALLK_CONTEXT AllK = NULL;

#define ALLK_INCREF_HANDLE(HandlePtr)			InterlockedIncrement(&ALLK_GETREF_HANDLE(HandlePtr))
#define ALLK_DECREF_HANDLE(HandlePtr)			InterlockedDecrement(&ALLK_GETREF_HANDLE(HandlePtr))

#define ALLK_DBG_PRINT_SECTION(AllKSection)	\
	USBLOG_PRINTLN("  %u %s Handles: HandleSize %u PoolSize %u (bytes)",(sizeof(AllK->AllKSection.Handles)/sizeof(AllK->AllKSection.Handles[0])),DEFINE_TO_STR(AllKSection),sizeof(AllK->AllKSection.Handles[0]), sizeof(AllK->AllKSection))



#define ALLK_HANDLES_INIT(AllKSection) do {										\
		memset(AllK->AllKSection.Handles, 0, sizeof(AllK->AllKSection.Handles));	\
	}while(0)

VOID WINAPI AllK_Context_Free(VOID)
{
	if (AllK == NULL) return;

#ifdef DEBUG_LOGGING_ENABLED
	POOLHANDLE_LIB_EXIT_CHECK(HotK);
	POOLHANDLE_LIB_EXIT_CHECK(LstK);
	POOLHANDLE_LIB_EXIT_CHECK(LstInfoK);
	POOLHANDLE_LIB_EXIT_CHECK(UsbK);
	POOLHANDLE_LIB_EXIT_CHECK(DevK);
	POOLHANDLE_LIB_EXIT_CHECK(OvlK);
	POOLHANDLE_LIB_EXIT_CHECK(OvlPoolK);
	POOLHANDLE_LIB_EXIT_CHECK(StmK);
#endif

	if (AllK->Dlls.hShlwapi)
	{
		FreeLibrary(AllK->Dlls.hShlwapi);
		AllK->Dlls.hShlwapi = NULL;
	}
	if (AllK->Dlls.hWinTrust)
	{
		FreeLibrary(AllK->Dlls.hWinTrust);
		AllK->Dlls.hWinTrust = NULL;
	}

	if (AllK->HeapDynamic != NULL)
	{
		if (AllK->HeapProcess == AllK->HeapDynamic)
		{
			HeapFree(AllK->HeapDynamic, 0, AllK);
		}
		else
		{
			HeapDestroy(AllK->HeapDynamic);
		}
	}

	AllK = NULL;
}

BOOL WINAPI AllK_Context_Init(HANDLE Heap, PVOID Reserved)
{
	HMODULE kernel32_dll;
	HANDLE processHeap;
	UNREFERENCED_PARAMETER(Reserved);

	kernel32_dll = GetModuleHandleA("kernel32");

	if (AllK) AllK_Context_Free();

	processHeap = GetProcessHeap();
	ErrorMemory(processHeap == NULL, Error);

	AllK = HeapAlloc(processHeap, HEAP_ZERO_MEMORY, sizeof(ALLK_CONTEXT));
	ErrorMemory(AllK == NULL, Error);

	AllK->HeapProcess = processHeap;
	AllK->HeapDynamic = Heap;
	if (AllK->HeapDynamic == NULL) AllK->HeapDynamic = AllK->HeapProcess;

	AllK->Dlls.hShlwapi = LoadLibraryA("shlwapi");
	AllK->Dlls.hWinTrust = LoadLibraryA("wintrust");

	// one-time AllK initialize
	USBLOG_PRINTLN("");
	USBLOG_PRINTLN("AllK required contiguous memory = %u (%sbit)", sizeof(ALLK_CONTEXT), sizeof(PVOID) == 8 ? "64" : "32");
	ALLK_DBG_PRINT_SECTION(HotK);
	ALLK_DBG_PRINT_SECTION(LstK);
	ALLK_DBG_PRINT_SECTION(LstInfoK);
	ALLK_DBG_PRINT_SECTION(UsbK);
	ALLK_DBG_PRINT_SECTION(DevK);
	ALLK_DBG_PRINT_SECTION(OvlK);
	ALLK_DBG_PRINT_SECTION(OvlPoolK);
	ALLK_DBG_PRINT_SECTION(StmK);
	USBLOG_PRINTLN("");

	ALLK_HANDLES_INIT(HotK);
	ALLK_HANDLES_INIT(LstK);
	ALLK_HANDLES_INIT(LstInfoK);
	ALLK_HANDLES_INIT(UsbK);
	ALLK_HANDLES_INIT(DevK);
	ALLK_HANDLES_INIT(OvlK);
	ALLK_HANDLES_INIT(OvlPoolK);
	ALLK_HANDLES_INIT(StmK);

	AllK->PathMatchSpec = (KDYN_PathMatchSpec*)GetProcAddress(AllK->Dlls.hShlwapi, "PathMatchSpecA");
	AllK->CancelIoEx	= (KDYN_CancelIoEx*)GetProcAddress(kernel32_dll, "CancelIoEx");

	AllK->DevK.Index = -1;
	AllK->HotK.Index = -1;
	AllK->LstInfoK.Index = -1;
	AllK->LstK.Index = -1;
	AllK->OvlK.Index = -1;
	AllK->OvlPoolK.Index = -1;
	AllK->StmK.Index = -1;
	AllK->UsbK.Index = -1;

	USBLOG_PRINTLN("Dynamically allocated as needed:");
	USBLOG_PRINTLN("\tKLST_DEVINFO = %u bytes each", sizeof(KLST_DEVINFO));

	return TRUE;

Error:
	AllK = NULL;
	return FALSE;
}

BOOL CheckLibInit()
{
	if (AllK) return TRUE;

	mSpin_Acquire(&AllKInitLock);
	if (!AllK_Context_Init(NULL, NULL))
	{
		USBERRN("Failed initializing default context. ErrorCode=%08Xh", GetLastError());
		mSpin_Release(&AllKInitLock);
		return FALSE;
	}
	mSpin_Release(&AllKInitLock);

	return TRUE;
}

#define POOLHANDLE_ACQUIRE(ReturnHandle,AllKSection) do { 																\
		long nextPos,startPos;																								\
		(ReturnHandle) = NULL;																								\
		if (AllK==NULL && CheckLibInit()==FALSE) break;																		\
		nextPos = startPos = (IncLock(AllK->AllKSection.Index)) % ALLK_HANDLE_COUNT(AllKSection);							\
		do																													\
		{ 																													\
			(ReturnHandle) = &AllK->AllKSection.Handles[nextPos];  															\
			if (mSpin_Try_Acquire(&(ReturnHandle)->Base.Count.Use)) 														\
			{ 																												\
				Init_Handle_ObjK(&(ReturnHandle)->Base,AllKSection);  														\
				Init_Handle_##AllKSection((ReturnHandle));																	\
				break;																										\
			} 																												\
			(ReturnHandle) = NULL;																							\
			nextPos = (IncLock(AllK->AllKSection.Index)) % ALLK_HANDLE_COUNT(AllKSection);   								\
		}while(nextPos!=startPos);																							\
																															\
		if (!(ReturnHandle))  																								\
		{ 																													\
			USBERRN("no more internal " DEFINE_TO_STR(AllKSection) " handles! (max=%d)",  ALLK_HANDLE_COUNT(AllKSection));	\
			LusbwError(ERROR_OUT_OF_STRUCTURES);  																			\
		} 																													\
	}while(0)


#define FN_POOLHANDLE(AllKSection,HandleType)											\
	P##HandleType PoolHandle_Acquire_##AllKSection(PKOBJ_CB EvtCleanup)					\
	{																					\
		P##HandleType next = NULL;														\
		POOLHANDLE_ACQUIRE(next, AllKSection);											\
		if (next) next->Base.Evt.Cleanup=EvtCleanup;									\
		return next;																	\
	}																					\
	BOOL PoolHandle_Inc_##AllKSection(P##HandleType PoolHandle)							\
	{																					\
		if (!ALLK_INUSE_HANDLE(PoolHandle)) return FALSE;								\
		if (PoolHandle->Base.Disposing)													\
			return PoolHandle->Base.Disposing == GetCurrentThreadId() ? TRUE : FALSE;	\
		if (ALLK_INCREF_HANDLE(PoolHandle) > 1)											\
			return TRUE;																\
		ALLK_DECREF_HANDLE(PoolHandle);													\
		return FALSE;																	\
	}																					\
	BOOL PoolHandle_IncEx_##AllKSection(P##HandleType PoolHandle, long* lockCount)		\
	{																					\
		*lockCount=-1;																	\
		if (!ALLK_INUSE_HANDLE(PoolHandle)) return FALSE;								\
		*lockCount=0;																	\
		if (PoolHandle->Base.Disposing)													\
			return PoolHandle->Base.Disposing == GetCurrentThreadId() ? TRUE : FALSE;	\
		if ((*lockCount = ALLK_INCREF_HANDLE(PoolHandle)) > 1)							\
			return TRUE;																\
		*lockCount = ALLK_DECREF_HANDLE(PoolHandle);									\
		return FALSE;																	\
	}																					\
	BOOL PoolHandle_Dec_##AllKSection(P##HandleType PoolHandle)							\
	{																					\
		long lockCnt;																	\
		if (!ALLK_INUSE_HANDLE(PoolHandle)) return FALSE;								\
		if (PoolHandle->Base.Disposing)													\
			return PoolHandle->Base.Disposing == GetCurrentThreadId() ? TRUE : FALSE;	\
		if ((lockCnt=ALLK_DECREF_HANDLE(PoolHandle)) == 0)								\
		{																				\
			PoolHandle->Base.Disposing=GetCurrentThreadId();							\
			if (PoolHandle->Base.Evt.Cleanup)											\
			{																			\
				PoolHandle->Base.Evt.Cleanup(PoolHandle);								\
				PoolHandle->Base.Evt.Cleanup=NULL;										\
			}																			\
			mSpin_Release(&PoolHandle->Base.Count.Use);									\
			return FALSE;																\
		}																				\
		else if (lockCnt < 0)															\
		{																				\
			USBWRNN("LockCnt=%d",lockCnt);												\
			return FALSE;																\
		}																				\
		return TRUE;																	\
	}

FN_POOLHANDLE(HotK, KHOT_HANDLE_INTERNAL)
FN_POOLHANDLE(LstK, KLST_HANDLE_INTERNAL)
FN_POOLHANDLE(LstInfoK, KLST_DEVINFO_HANDLE_INTERNAL)
FN_POOLHANDLE(UsbK, KUSB_HANDLE_INTERNAL)
FN_POOLHANDLE(DevK, KDEV_HANDLE_INTERNAL)
FN_POOLHANDLE(OvlK, KOVL_HANDLE_INTERNAL)
FN_POOLHANDLE(OvlPoolK, KOVL_POOL_HANDLE_INTERNAL)
FN_POOLHANDLE(StmK, KSTM_HANDLE_INTERNAL)
