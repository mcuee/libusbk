#include "lusbk_handles.h"

#pragma warning(disable:4127)

#define DYN_FUNC_LOAD(LibNameA, FuncNameA) GetProcAddress(LoadLibraryA(LibNameA),FuncNameA)

#define INIT_ALLK_POOL(AllKSection)	\
/* AllKSection = */	{				\
/* BusyLock    = */		0,			\
/* CurrentPos  = */		0,			\
/* NextPos     = */		0,			\
/* Handles     = */		{0},	}

ALLK_CONTEXT AllK =
{
	/* Valid       = */	FALSE,
	/* InitLock    = */	0,
	INIT_ALLK_POOL(HotK),
	INIT_ALLK_POOL(LstK),
};

typedef BOOL WINAPI KDYN_PathMatchSpec(__in LPCSTR pszFile, __in LPCSTR pszSpec);
static KDYN_PathMatchSpec* PathMatchSpec = NULL;

BOOL MatchInstanceID(LPCSTR MatchPattern, LPCSTR CheckInstanceID)
{
	if (!PathMatchSpec)
	{
		PathMatchSpec = (KDYN_PathMatchSpec*)DYN_FUNC_LOAD("shlwapi", "PathMatchSpecA");
		if (!PathMatchSpec) return FALSE;
	}
	return PathMatchSpec(CheckInstanceID, MatchPattern);
}

static BOOL AllK_Context_Initialize(ALLK_CONTEXT* AllK)
{
	// one-time AllK initialize

	memset(AllK->HotK.Handles, 0, sizeof(AllK->HotK.Handles));
	memset(AllK->LstK.Handles, 0, sizeof(AllK->LstK.Handles));

	return TRUE;
}

void CheckLibInit()
{
	if (!AllK.Valid)
	{
		SpinLock_Acquire(&AllK.InitLock, TRUE);
		if (AllK.Valid) return;
		AllK.Valid = AllK_Context_Initialize(&AllK);
		SpinLock_Release(&AllK.InitLock);
	}
}

#define Init_Handle_ObjK(BaseObjPtr) do {															\
	(BaseObjPtr)->Count.Ref = 1;																	\
	memset(&(BaseObjPtr)->Lock,0,sizeof((BaseObjPtr)->Lock));										\
}while(0)

#define Init_Handle_HotK(HandlePtr) do {															\
	memset(&(HandlePtr)->Public, 0, sizeof((HandlePtr)->Public));									\
	(HandlePtr)->next = NULL;																		\
	(HandlePtr)->prev = NULL;																		\
}while(0)

#define Init_Handle_LstK(HandlePtr) do {															\
	(HandlePtr)->current = NULL;																	\
	(HandlePtr)->head = NULL;																		\
}while(0)

#define POOLHANDLE_ACQUIRE(ReturnHandle,AllKSection) do {																\
	CheckLibInit();																										\
																														\
	SpinLock_Acquire(&AllK.AllKSection.BusyLock, TRUE);																	\
																														\
	AllK.AllKSection.NextPos = AllK.AllKSection.CurrentPos;																\
	while  ((AllK.AllKSection.NextPos = ((AllK.AllKSection.NextPos + 1) % ALLK_HANDLE_COUNT(AllKSection))) !=			\
			AllK.AllKSection.CurrentPos)																				\
	{																													\
		(ReturnHandle) = &AllK.AllKSection.Handles[AllK.AllKSection.NextPos];											\
		if (SpinLock_Acquire(&(ReturnHandle)->Base.Count.Use, FALSE))													\
		{																												\
			Init_Handle_ObjK(&(ReturnHandle)->Base);																	\
			Init_Handle_##AllKSection((ReturnHandle));																	\
			AllK.AllKSection.CurrentPos = AllK.AllKSection.NextPos;														\
			break;																										\
		}																												\
		(ReturnHandle) = NULL;																							\
	}																													\
																														\
	SpinLock_Release(&AllK.AllKSection.BusyLock);																		\
																														\
	if (!(ReturnHandle))																								\
	{																													\
																														\
		USBERR("no more internal " DEFINE_TO_STR(AllKSection) " handles! (max=%d)\n",  ALLK_HANDLE_COUNT(AllKSection));	\
																														\
		LusbwError(ERROR_OUT_OF_STRUCTURES);																			\
	}																													\
}while(0)

#define FN_POOLHANDLE(AllKSection,HandleType)													\
	P##HandleType PoolHandle_Acquire_##AllKSection(PKOBJ_EvtCallback EvtCleanup)				\
	{																							\
		P##HandleType next = NULL;																\
		POOLHANDLE_ACQUIRE(next, AllKSection);													\
		if (next) next->Base.Evt.Cleanup=EvtCleanup;											\
		return next;																			\
	}																							\
	BOOL PoolHandle_Inc_##AllKSection(P##HandleType PoolHandle)									\
	{																							\
		if (PoolHandle->Base.Disposing) return TRUE;											\
		if (ALLK_GETREF_HANDLE(PoolHandle) < 1) return FALSE;									\
		if (ALLK_INCREF_HANDLE(PoolHandle) > 1)													\
			return TRUE;																		\
		ALLK_DECREF_HANDLE(PoolHandle);															\
		return FALSE;																			\
	}																							\
	BOOL PoolHandle_Dec_##AllKSection(P##HandleType PoolHandle)									\
	{																							\
		long lockCnt;																			\
		if (PoolHandle->Base.Disposing) return TRUE;											\
		if (ALLK_GETREF_HANDLE(PoolHandle) < 1) return FALSE;									\
		if ((lockCnt=ALLK_DECREF_HANDLE(PoolHandle)) == 0)										\
		{																						\
			PoolHandle->Base.Disposing=TRUE;													\
			if (PoolHandle->Base.Evt.Cleanup)													\
				PoolHandle->Base.Evt.Cleanup(PoolHandle);										\
			PoolHandle->Base.Disposing=FALSE;													\
			SpinLock_Release(&PoolHandle->Base.Count.Use);										\
			return FALSE;																		\
		}																						\
		return (lockCnt > 0);																	\
	}

FN_POOLHANDLE(HotK, KHOT_HANDLE_INTERNAL)
FN_POOLHANDLE(LstK, KLST_HANDLE_INTERNAL)

/*

KUSB_EXP BOOL KUSB_API ObjK_LockAcquire(PKOBJ_BASE Object, BOOL Wait)
{
	return ALLK_LOCKEX_HANDLE(Object, Wait);
}

KUSB_EXP VOID KUSB_API ObjK_LockRelease(PKOBJ_BASE Object)
{
	ALLK_UNLOCK_HANDLE(Object);
}

KUSB_EXP PKUSB_USER_CONTEXT ObjK_GetContext(PKOBJ_BASE Object)
{
	return &Object->UserContext;
}

*/