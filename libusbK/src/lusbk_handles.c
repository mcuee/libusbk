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

#include "lusbk_handles.h"

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#define ALLK_LOCKEX_HANDLE(HandlePtr,Required)	SpinLock_AcquireEx(&(HandlePtr)->Base.Lock, Required)
#define ALLK_UNLOCK_HANDLE(HandlePtr)			SpinLock_ReleaseEx(&(HandlePtr)->Base.Lock)
#define ALLK_INCREF_HANDLE(HandlePtr)			InterlockedIncrement(&ALLK_GETREF_HANDLE(HandlePtr))
#define ALLK_DECREF_HANDLE(HandlePtr)			InterlockedDecrement(&ALLK_GETREF_HANDLE(HandlePtr))

#define ALLK_DBG_PRINT_SECTION(AllKSection)	\
	USBLOG_PRINTLN("  %u %s Handles: HandleSize %u PoolSize %u (bytes)",(sizeof(AllK->AllKSection.Handles)/sizeof(AllK->AllKSection.Handles[0])),DEFINE_TO_STR(AllKSection),sizeof(AllK->AllKSection.Handles[0]), sizeof(AllK->AllKSection))



#define ALLK_HANDLES_INIT(AllKSection) do {										\
	memset(AllK->AllKSection.Handles, 0, sizeof(AllK->AllKSection.Handles));	\
}while(0)

#define ALLK_INIT_SECTION(AllKSection) 	\
/* AllKSection			= */{			\
/* Index				= */	-1,		\
/* Handles				= */	{0}, 	\
							}
ALLK_CONTEXT AllK =
{
	/* Valid			= */	FALSE,
	/* InitLock			= */	0,
	/* Heap				= */	NULL,

	/* CancelIoEx		= */	(KDYN_CancelIoEx*)NULL,
	/* PathMatchSpec	= */	(KDYN_PathMatchSpec*)NULL,

	ALLK_INIT_SECTION(HotK),
	ALLK_INIT_SECTION(LstK),
	ALLK_INIT_SECTION(LstInfoK),
	ALLK_INIT_SECTION(UsbK),
	ALLK_INIT_SECTION(DevK),
	ALLK_INIT_SECTION(OvlK),
	ALLK_INIT_SECTION(OvlPoolK),
	ALLK_INIT_SECTION(StmK),
};

BOOL MatchPattern(LPCSTR Pattern, LPCSTR File)
{
	return AllK.PathMatchSpec(File, Pattern);
}

static BOOL AllK_Context_Initialize(ALLK_CONTEXT* AllK)
{
	HMODULE shlwapi_dll		= LoadLibraryA("shlwapi");
	HMODULE kernel32_dll	= GetModuleHandleA("kernel32");

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

	AllK->PathMatchSpec = (KDYN_PathMatchSpec*)GetProcAddress(shlwapi_dll, "PathMatchSpecA");
	AllK->CancelIoEx	= (KDYN_CancelIoEx*)GetProcAddress(kernel32_dll, "CancelIoEx");

	USBLOG_PRINTLN("KLST_DEVINFO = %u bytes", sizeof(KLST_DEVINFO));
	return TRUE;
}

void CheckLibInit()
{
	if (AllK.Valid) return;

	mSpin_Acquire(&AllK.InitLock);
	if (AllK.Valid)
	{
		mSpin_Release(&AllK.InitLock);
		return;
	}
	AllK.Heap = GetProcessHeap();
	AllK.Valid = AllK_Context_Initialize(&AllK);
	mSpin_Release(&AllK.InitLock);
}

#define POOLHANDLE_ACQUIRE(ReturnHandle,AllKSection) do { 																\
	long nextPos,startPos;																								\
	if (!AllK.Valid) CheckLibInit();   																					\
	nextPos = startPos = (IncLock(AllK.AllKSection.Index)) % ALLK_HANDLE_COUNT(AllKSection);							\
	do																													\
	{ 																													\
		(ReturnHandle) = &AllK.AllKSection.Handles[nextPos];  															\
		if (mSpin_Try_Acquire(&(ReturnHandle)->Base.Count.Use)) 														\
		{ 																												\
			Init_Handle_ObjK(&(ReturnHandle)->Base,AllKSection);  														\
			Init_Handle_##AllKSection((ReturnHandle));																	\
			break;																										\
		} 																												\
		(ReturnHandle) = NULL;																							\
		nextPos = (IncLock(AllK.AllKSection.Index)) % ALLK_HANDLE_COUNT(AllKSection);   								\
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

/*
	BOOL PoolHandle_Acquire_Spin_##AllKSection(P##HandleType PoolHandle, BOOL Required)	\
	{																					\
		if (PoolHandle->Base.Disposing)													\
			return PoolHandle->Base.Disposing == GetCurrentThreadId() ? TRUE : FALSE;	\
		if (!PoolHandle_Inc_##AllKSection(PoolHandle)) return FALSE;					\
		if (!ALLK_LOCKEX_HANDLE(PoolHandle,Required))									\
		{																				\
			PoolHandle_Dec_##AllKSection(PoolHandle);									\
			return FALSE;																\
		}																				\
		return TRUE;																	\
	}																					\
	BOOL PoolHandle_Release_Spin_##AllKSection(P##HandleType PoolHandle)				\
	{																					\
		if (PoolHandle->Base.Disposing)													\
			return PoolHandle->Base.Disposing == GetCurrentThreadId() ? TRUE : FALSE;	\
		ALLK_UNLOCK_HANDLE(PoolHandle);													\
		return PoolHandle_Dec_##AllKSection(PoolHandle);								\
	}
*/

ALDEF_LIST_FUNCTIONS(ALC, KSTM_XFER, XferLink);
ALDEF_LIST_FUNCTIONS(ALC, KOVL, Ovl);

FN_POOLHANDLE(HotK, KHOT_HANDLE_INTERNAL)
FN_POOLHANDLE(LstK, KLST_HANDLE_INTERNAL)
FN_POOLHANDLE(LstInfoK, KLST_DEVINFO_HANDLE_INTERNAL)
FN_POOLHANDLE(UsbK, KUSB_HANDLE_INTERNAL)
FN_POOLHANDLE(DevK, KDEV_HANDLE_INTERNAL)
FN_POOLHANDLE(OvlK, KOVL_HANDLE_INTERNAL)
FN_POOLHANDLE(OvlPoolK, KOVL_POOL_HANDLE_INTERNAL)
FN_POOLHANDLE(StmK, KSTM_HANDLE_INTERNAL)
