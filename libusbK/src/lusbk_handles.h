#ifndef __LUSBK_HANDLES_H_
#define __LUSBK_HANDLES_H_

#include "lusbk_private.h"
#include "lusbk_hot_plug.h"

#define KHOT_HANDLE_POOL_COUNT 32
#define KLST_HANDLE_POOL_COUNT 128

#define DL_MatchPattern(FilePattern,FilePath) (MatchInstanceID(FilePattern,FilePath)?0:-1)

#define ALLK_HANDLE_COUNT(AllKSection) (sizeof(AllK.AllKSection.Handles)/sizeof(AllK.AllKSection.Handles[0]))

#define ALLK_VALID_HANDLE(HandlePtr, AllKSection)															\
	((																										\
		(UINT_PTR)(HandlePtr) >= (UINT_PTR)(&AllK.AllKSection.Handles[0]) &&								\
		(UINT_PTR)(HandlePtr) <= (UINT_PTR)(&AllK.AllKSection.Handles[ALLK_HANDLE_COUNT(AllKSection)-1])	\
	)?TRUE:FALSE)

#define ALLK_INUSE_HANDLE(HandlePtr)	(((HandlePtr)->Base.Count.Use==0)?FALSE:TRUE)
#define ALLK_GETREF_HANDLE(HandlePtr)	((HandlePtr)->Base.Count.Ref)

#define ALLK_LOCKEX_HANDLE(HandlePtr,Required)	SpinLock_AcquireEx(&(HandlePtr)->Base.Lock, Required)
#define ALLK_UNLOCK_HANDLE(HandlePtr)			SpinLock_ReleaseEx(&(HandlePtr)->Base.Lock)
#define ALLK_LOCK_HANDLE(HandlePtr)				ALLK_LOCKEX_HANDLE(HandlePtr, TRUE)
#define ALLK_TRYLOCK_HANDLE(HandlePtr)			ALLK_LOCKEX_HANDLE(HandlePtr, FALSE)
#define ALLK_INCREF_HANDLE(HandlePtr)			InterlockedIncrement(&ALLK_GETREF_HANDLE(HandlePtr))
#define ALLK_DECREF_HANDLE(HandlePtr)			InterlockedDecrement(&ALLK_GETREF_HANDLE(HandlePtr))

#define PROTO_POOLHANDLE(AllKSection,HandleType)												\
	P## HandleType PoolHandle_Acquire_##AllKSection(PKOBJ_EvtCallback EvtCleanup);					\
	BOOL PoolHandle_Inc_##AllKSection(P##HandleType PoolHandle);												\
	BOOL PoolHandle_Dec_##AllKSection(P##HandleType PoolHandle)

typedef VOID KUSB_API KOBJ_EvtCallback(PVOID Object);
typedef KOBJ_EvtCallback* PKOBJ_EvtCallback;

typedef struct _KOBJ_BASE
{
	KUSB_USER_CONTEXT UserContext;

	// Generally used as a spin-lock at a api-backend defined level
	SPIN_LOCK_EX Lock;

	struct
	{
		PKOBJ_EvtCallback Cleanup;
	} Evt;
	struct
	{
		// Used by the pool acquire / release functions only.
		volatile long Use;

		// Used by the individual API functions as they access this handle.
		volatile long Ref;
	} Count;

	BOOL Disposing;

} KOBJ_BASE, *PKOBJ_BASE;

typedef struct _KHOT_HANDLE_INTERNAL
{
	KOBJ_BASE Base;
	KHOT_PARAMS Public;

	struct _KHOT_HANDLE_INTERNAL* prev;
	struct _KHOT_HANDLE_INTERNAL* next;
} KHOT_HANDLE_INTERNAL;
typedef KHOT_HANDLE_INTERNAL* PKHOT_HANDLE_INTERNAL;



// internal device info
typedef struct _KLST_DEV_INFO_EL
{
	KLST_DEV_INFO Public;
	struct _KLST_DEV_INFO_EL* next;
	struct _KLST_DEV_INFO_EL* prev;
} KLST_DEV_INFO_EL;
typedef KLST_DEV_INFO_EL* PKLST_DEV_INFO_EL;

// internal device list
typedef struct _KLST_HANDLE_INTERNAL
{
	KOBJ_BASE Base;
	PKLST_DEV_INFO_EL current;
	PKLST_DEV_INFO_EL head;
} KLST_HANDLE_INTERNAL;
typedef KLST_HANDLE_INTERNAL* PKLST_HANDLE_INTERNAL;

#define DEF_POOLED_HANDLE_STRUCT(AllKSection,HandleType,HandlePoolCount)	\
	struct																	\
	{																		\
		volatile long BusyLock;												\
		ULONG CurrentPos;													\
		ULONG NextPos;														\
		HandleType Handles[HandlePoolCount];								\
	} AllKSection

// structure of all static libusbK handle pools.
typedef struct
{
	volatile BOOL Valid;
	volatile long InitLock;

	DEF_POOLED_HANDLE_STRUCT(HotK, KHOT_HANDLE_INTERNAL, KHOT_HANDLE_POOL_COUNT);
	DEF_POOLED_HANDLE_STRUCT(LstK, KLST_HANDLE_INTERNAL, KLST_HANDLE_POOL_COUNT);

} ALLK_CONTEXT;

extern ALLK_CONTEXT AllK;

void CheckLibInit();
/*
KUSB_EXP BOOL KUSB_API ObjK_LockAcquire(PKOBJ_BASE Object, BOOL Wait);
KUSB_EXP VOID KUSB_API ObjK_LockRelease(PKOBJ_BASE Object);
KUSB_EXP PKUSB_USER_CONTEXT ObjK_GetContext(PKOBJ_BASE Object);
*/
BOOL MatchInstanceID(LPCSTR MatchPattern, LPCSTR CheckInstanceID);


PROTO_POOLHANDLE(HotK, KHOT_HANDLE_INTERNAL);
PROTO_POOLHANDLE(LstK, KLST_HANDLE_INTERNAL);

#endif

