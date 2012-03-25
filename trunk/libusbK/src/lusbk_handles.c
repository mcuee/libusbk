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

PALLK_CONTEXT AllK = NULL;

#define ALLK_INCREF_HANDLE(HandlePtr)			InterlockedIncrement(&ALLK_GETREF_HANDLE(HandlePtr))
#define ALLK_DECREF_HANDLE(HandlePtr)			InterlockedDecrement(&ALLK_GETREF_HANDLE(HandlePtr))

BOOL CheckLibInit()
{
	if (AllK) return TRUE;

	if (!LibK_Context_Init(NULL, NULL))
	{
		USBERRN("Failed initializing default context. ErrorCode=%08Xh", GetLastError());
		return FALSE;
	}

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
