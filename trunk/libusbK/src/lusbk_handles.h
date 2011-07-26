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

#ifndef __LUSBK_HANDLES_H_
#define __LUSBK_HANDLES_H_

#include "lusbk_private.h"

#define KHOT_HANDLE_COUNT				8
#define KLST_HANDLE_COUNT				64
#define KLST_DEVINFO_HANDLE_COUNT		128
#define KUSB_HANDLE_COUNT				64
#define KDEV_HANDLE_COUNT				32
#define KDEV_SHARED_INTERFACE_COUNT		128
#define KOVL_HANDLE_COUNT				1024
#define KOVL_POOL_HANDLE_COUNT			64

#define DL_MatchPattern(FilePattern,FilePath) (MatchPattern(FilePattern,FilePath)?0:-1)

#define ALLK_HANDLE_COUNT(AllKSection) (sizeof(AllK.AllKSection.Handles)/sizeof(AllK.AllKSection.Handles[0]))

#define ALLK_VALID_HANDLE(HandlePtr, AllKSection)															\
	((																										\
		(UINT_PTR)(HandlePtr) >= (UINT_PTR)(&AllK.AllKSection.Handles[0]) &&								\
		(UINT_PTR)(HandlePtr) <= (UINT_PTR)(&AllK.AllKSection.Handles[ALLK_HANDLE_COUNT(AllKSection)-1])	\
	)?TRUE:FALSE)

#define ALLK_INUSE_HANDLE(HandlePtr)	((HandlePtr)->Base.Count.Use==SPINLOCK_HELD)
#define ALLK_GETREF_HANDLE(HandlePtr)	((HandlePtr)->Base.Count.Ref)

#define ALLK_LIVE_HANDLE(HandlePtr,AllKSection)	(ALLK_VALID_HANDLE(HandlePtr,AllKSection) && ALLK_GETREF_HANDLE(HandlePtr) > 0)


#define ALLK_GET_USER_CONTEXT(BaseObjPtr)		(&((BaseObjPtr)->User.Context))
#define ALLK_GET_USER_CONTEXT_SIZE(BaseObjPtr)	(((BaseObjPtr)->User.ContextSize>sizeof(KLIB_USER_CONTEXT)) ? (BaseObjPtr)->User.ContextSize : sizeof(KLIB_USER_CONTEXT))

#define KOVL_GET_PRIVATE_INFO(KOvl_Handle) ((((PKOVL_HANDLE_INTERNAL)(KOvl_Handle))->Private))
#define IS_OVLK(mOverlapped) (ALLK_LIVE_HANDLE(((PKOVL_HANDLE_INTERNAL)mOverlapped),OvlK))

#define PUB_TO_PRIV(AllKSection,HandleType,K_Handle,K_Handle_Internal,ErrorAction)		\
	if (!ALLK_VALID_HANDLE(K_Handle,AllKSection))										\
	{																					\
		K_Handle_Internal=NULL;															\
		USBERRN("Invalid "DEFINE_TO_STR(AllKSection)" handle.");						\
		SetLastError(ERROR_INVALID_HANDLE);												\
		{ErrorAction;}																	\
	}																					\
	else																				\
		K_Handle_Internal=(P##HandleType)(K_Handle)

#define Pub_To_Priv_HotK(KHot_Handle,KHot_Handle_Internal,ErrorAction)										\
	PUB_TO_PRIV(HotK,KHOT_HANDLE_INTERNAL,KHot_Handle,KHot_Handle_Internal,ErrorAction)

#define Pub_To_Priv_LstK(KLst_Handle,KLst_Handle_Internal,ErrorAction)										\
	PUB_TO_PRIV(LstK,KLST_HANDLE_INTERNAL,KLst_Handle,KLst_Handle_Internal,ErrorAction)

#define Pub_To_Priv_LstInfoK(KLst_DevInfo_Handle,KLst_DevInfo_Handle_Internal,ErrorAction)					\
	PUB_TO_PRIV(LstInfoK,KLST_DEVINFO_HANDLE_INTERNAL,KLst_DevInfo_Handle,KLst_DevInfo_Handle_Internal,ErrorAction)

#define Pub_To_Priv_UsbK(KUsb_Handle,KUsb_Handle_Internal,ErrorAction)										\
	PUB_TO_PRIV(UsbK,KUSB_HANDLE_INTERNAL,KUsb_Handle,KUsb_Handle_Internal,ErrorAction)

#define Pub_To_Priv_DevK(KDev_Handle,KDev_Handle_Internal,ErrorAction)										\
	PUB_TO_PRIV(DevK,KDEV_HANDLE_INTERNAL,KDev_Handle,KDev_Handle_Internal,ErrorAction)

#define Pub_To_Priv_OvlK(KOvl_Handle,KOvl_Handle_Internal,ErrorAction)										\
	PUB_TO_PRIV(OvlK,KOVL_HANDLE_INTERNAL,KOvl_Handle,KOvl_Handle_Internal,ErrorAction)

#define Pub_To_Priv_OvlPoolK(KOvl_Pool_Handle,KOvl_Pool_Handle_Internal,ErrorAction)						\
	PUB_TO_PRIV(OvlPoolK,KOVL_POOL_HANDLE_INTERNAL,KOvl_Pool_Handle,KOvl_Pool_Handle_Internal,ErrorAction)

#define PROTO_POOLHANDLE(AllKSection,HandleType)											\
	PKLIB_USER_CONTEXT PoolHandle_GetContext_##AllKSection(P##HandleType PoolHandle);		\
	P## HandleType PoolHandle_Acquire_##AllKSection(PKOBJ_CB EvtCleanup);					\
	BOOL PoolHandle_Inc_##AllKSection(P##HandleType PoolHandle);							\
	BOOL PoolHandle_Dec_##AllKSection(P##HandleType PoolHandle);							\
	BOOL PoolHandle_Acquire_Spin_##AllKSection(P##HandleType PoolHandle, BOOL Required);	\
	BOOL PoolHandle_Release_Spin_##AllKSection(P##HandleType PoolHandle)


#define FindInterfaceEL(mUsbStack,mInterfaceEL,mIsIndex,mNumberOrIndex)	if (mIsIndex)		\
	{																						\
		DL_SEARCH_SCALAR((mUsbStack)->InterfaceList, mInterfaceEL, Index, mNumberOrIndex);	\
	}																						\
	else																					\
	{																						\
		DL_SEARCH_SCALAR((mUsbStack)->InterfaceList, mInterfaceEL, ID, mNumberOrIndex);		\
	}

#define FindAltInterfaceEL(InterfaceEL,mAltInterfaceEL,mIsIndex,mNumberOrIndex)	if (mIsIndex)		\
	{																								\
		DL_SEARCH_SCALAR((InterfaceEL)->AltInterfaceList, mAltInterfaceEL, Index, mNumberOrIndex);	\
	}																								\
	else																							\
	{																								\
		DL_SEARCH_SCALAR((InterfaceEL)->AltInterfaceList, mAltInterfaceEL, ID, mNumberOrIndex);		\
	}

#define FindPipeEL(AltInterfaceEL,mPipeEL,mIsIndex,mNumberOrIndex)	if (mIsIndex)		\
	{																					\
		DL_SEARCH_SCALAR((AltInterfaceEL)->PipeList, mPipeEL, Index, mNumberOrIndex);	\
	}																					\
	else																				\
	{																					\
		DL_SEARCH_SCALAR((AltInterfaceEL)->PipeList, mPipeEL, ID, mNumberOrIndex);		\
	}

#define Dev_Handle() (handle->Device->MasterDeviceHandle)
#define Intf_Handle() (handle->Device->MasterInterfaceHandle)

#define Get_SharedInterface(KUsb_Handle_Internal, Intf_Index) (((KUsb_Handle_Internal)->Device->SharedInterfaces[(Intf_Index) & (KDEV_SHARED_INTERFACE_COUNT-1)]))
#define Get_CurSharedInterface(KUsb_Handle_Internal, KDev_Shared_Interface) \
	KDev_Shared_Interface = &Get_SharedInterface(KUsb_Handle_Internal,(KUsb_Handle_Internal)->Selected_SharedInterface_Index)

#define Update_SharedInterface_AltSetting(KDev_Shared_Interface,mAltSetting) InterlockedExchange(&((KDev_Shared_Interface)->CurrentAltSetting),(LONG)(mAltSetting))

#define PIPEID_TO_IDX(Pipe_Number) ((((Pipe_Number) & 0xF)|(((Pipe_Number)>>3) & 0x10)) & 0x1F)
#define Get_PipeInterfaceHandle(KUsb_Handle_Internal,Pipe_Number) ((KUsb_Handle_Internal)->Device->UsbStack->PipeCache[PIPEID_TO_IDX(Pipe_Number)].InterfaceHandle)

typedef VOID KUSB_API KOBJ_CB(PVOID Handle);
typedef KOBJ_CB* PKOBJ_CB;

typedef enum _KOBJ_STATE
{
    KOBJ_STATE_INVALID,
    KOBJ_STATE_INUSE,
    KOBJ_STATE_DISPOSING,
} KOBJ_STATE;

typedef struct _KDEV_SHARED_INTERFACE
{
	HANDLE InterfaceHandle;
	UCHAR ID;
	UCHAR Index;
	BOOL Claimed;
	volatile long CurrentAltSetting;
} KDEV_SHARED_INTERFACE;
typedef KDEV_SHARED_INTERFACE* PKDEV_SHARED_INTERFACE;

typedef union _KUSB_PIPE_CACHE
{
	volatile HANDLE InterfaceHandle;
} KUSB_PIPE_CACHE;

typedef struct _KUSB_PIPE_EL
{
	UCHAR ID;
	UCHAR Index;

	PUSB_ENDPOINT_DESCRIPTOR Descriptor;

	struct _KUSB_PIPE_EL* next;
	struct _KUSB_PIPE_EL* prev;
}* PKUSB_PIPE_EL, KUSB_PIPE_EL;


typedef struct _KUSB_ALT_INTERFACE_EL
{
	UCHAR ID;
	UCHAR Index;

	PUSB_INTERFACE_DESCRIPTOR Descriptor;

	PKUSB_PIPE_EL PipeList;

	struct _KUSB_ALT_INTERFACE_EL* next;
	struct _KUSB_ALT_INTERFACE_EL* prev;

}* PKUSB_ALT_INTERFACE_EL, KUSB_ALT_INTERFACE_EL;

typedef struct _KUSB_INTERFACE_EL
{
	UCHAR ID;
	UCHAR Index;
	PKDEV_SHARED_INTERFACE SharedInterface;

	PKUSB_ALT_INTERFACE_EL AltInterfaceList;

	struct _KUSB_INTERFACE_EL* next;
	struct _KUSB_INTERFACE_EL* prev;

}* PKUSB_INTERFACE_EL, KUSB_INTERFACE_EL;

typedef struct _KUSB_INTERFACE_STACK
{
	BOOL UsePipeCache;
	KUSB_PIPE_CACHE PipeCache[32];

	struct
	{
		LONG (*GetConfigDescriptor)		(struct _KUSB_INTERFACE_STACK*, LPCSTR, HANDLE, HANDLE, PUSB_CONFIGURATION_DESCRIPTOR*);
		LONG (*OpenInterface)			(struct _KUSB_INTERFACE_STACK*, HANDLE, INT, PHANDLE);
	} Cb;

	PKUSB_INTERFACE_EL InterfaceList;

} KUSB_INTERFACE_STACK;
typedef KUSB_INTERFACE_STACK* PKUSB_INTERFACE_STACK;

//! Structure representing internal \ref KOVL_HANDLE information.
/*
*
* \note
* This structure is populated after it is passes through one of the
* libusbK \ref \c UsbK core transfer functions such as \ref UsbK_ReadPipe
* or \ref UsbK_WritePipe
*
*/
typedef struct _KOVL_OVERLAPPED_INFO
{
	//! Device file handle.
	HANDLE DeviceHandle;

	//! Back-end specific interface handle.
	HANDLE InterfaceHandle;

	//! USB endpoint address for the i/o operation. (if applicable)
	UCHAR PipeID;

	//! Data buffer used for i/o operations.
	PUCHAR DataBuffer;

	//! Size in bytes of \c DataBuffer.
	ULONG DataBufferSize;

	//! Reserved storage space for the various libusbK driver back-ends. (for internal use only).
	union
	{
		libusb_request request;
	} Backend;

	//! Callback function used to abort the i/o operation. (for internal use only).
	KOVL_OVERLAPPED_CANCEL_CB* Cancel;

} KOVL_OVERLAPPED_INFO;
typedef KOVL_OVERLAPPED_INFO* PKOVL_OVERLAPPED_INFO;

typedef struct _KOVL_LINK_EL
{
	struct _KOVL_HANDLE_INTERNAL* OverlappedK;
	struct _KOVL_LINK_EL* next;
	struct _KOVL_LINK_EL* prev;

} KOVL_LINK_EL, *PKOVL_LINK_EL;


typedef struct _KLST_DEVINFO_EL
{
	KLST_DEVINFO Public;

	volatile struct _KLST_HANDLE_INTERNAL* DevListHandle;
	struct _KLST_DEVINFO_HANDLE_INTERNAL* DevInfoHandle;

	struct _KLST_DEVINFO_EL* next;
	struct _KLST_DEVINFO_EL* prev;
} KLST_DEVINFO_EL;
typedef KLST_DEVINFO_EL* PKLST_DEVINFO_EL;

typedef struct _LIBUSBK_BKND_CONTEXT
{
	USER_PIPE_POLICY PipePolicies[32];
	version_t Version;
} LIBUSBK_BKND_CONTEXT;
typedef LIBUSBK_BKND_CONTEXT* PLIBUSBK_BKND_CONTEXT;

typedef struct _WINUSB_BKND_CONTEXT
{
	DWORD _unused;
}* PWINUSB_BKND_CONTEXT, WINUSB_BKND_CONTEXT;

#define Init_Handle_ObjK(BaseObjPtr,AllKSection) do {			\
	memset(&((BaseObjPtr)->Lock),0,sizeof((BaseObjPtr)->Lock));	\
	memset(&((BaseObjPtr)->User),0,sizeof((BaseObjPtr)->User));	\
	(BaseObjPtr)->Evt.Cleanup = NULL;							\
	(BaseObjPtr)->Count.Ref = 1;								\
	(BaseObjPtr)->Disposing = 0;								\
}while(0)
typedef struct _KOBJ_BASE
{
	// Generally used as a spin-lock at a api-backend defined level
	SPIN_LOCK_EX Lock;
	DWORD Disposing;

	struct
	{
		PKOBJ_CB Cleanup;
	} Evt;
	struct
	{
		// Used by the pool acquire / release functions only.
		volatile long Use;

		// Used by the individual API functions as they access this handle.
		volatile long Ref;
	} Count;

	struct
	{
		BOOL Valid;
		BOOL ContextAllocated;
		ULONG ContextSize;
		KLIB_USER_CONTEXT Context;
	} User;
} KOBJ_BASE, *PKOBJ_BASE;

#ifndef INTERNAL_POOL_HANDLE_TYPEDEFS_AND_INIT_DEFINES_________________

#define Init_Handle_DevK(HandlePtr) do {					\
	(HandlePtr)->MasterDeviceHandle = NULL;					\
	(HandlePtr)->MasterInterfaceHandle = NULL;				\
	(HandlePtr)->DevicePath = NULL;							\
	(HandlePtr)->ConfigDescriptor = NULL;					\
	(HandlePtr)->SharedInterfaces = NULL;					\
	(HandlePtr)->DriverID = KUSB_DRVID_INVALID;				\
	(HandlePtr)->DriverAPI = NULL;							\
	(HandlePtr)->UsbStack = NULL;							\
	(HandlePtr)->Backend.Ctx = NULL;						\
}while(0)
typedef struct _KDEV_HANDLE_INTERNAL
{
	KOBJ_BASE Base;

	HANDLE MasterDeviceHandle;
	HANDLE MasterInterfaceHandle;

	LPSTR DevicePath;
	PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor;

	PKDEV_SHARED_INTERFACE SharedInterfaces;

	KUSB_DRVID			DriverID;
	KUSB_DRIVER_API*	DriverAPI;

	PKUSB_INTERFACE_STACK UsbStack;

	union
	{
		PVOID					Ctx;
		PLIBUSBK_BKND_CONTEXT	CtxK;
		PWINUSB_BKND_CONTEXT	CtxW;
	} Backend;

} KDEV_HANDLE_INTERNAL;
typedef KDEV_HANDLE_INTERNAL* PKDEV_HANDLE_INTERNAL;


#define Init_Handle_UsbK(HandlePtr) do {							\
	(HandlePtr)->Device = NULL;										\
	(HandlePtr)->Selected_SharedInterface_Index = 0;				\
	(HandlePtr)->IsClone = FALSE;									\
	memset(&((HandlePtr)->Move), 0, sizeof((HandlePtr)->Move));		\
}while(0)
typedef struct _KUSB_HANDLE_INTERNAL
{
	KOBJ_BASE Base;

	PKDEV_HANDLE_INTERNAL Device;
	volatile long Selected_SharedInterface_Index;
	BOOL IsClone;
	struct
	{
		BOOL End;

		PKUSB_INTERFACE_EL InterfaceEL;
		PKUSB_ALT_INTERFACE_EL AltInterfaceEL;
		PKUSB_PIPE_EL PipeEL;
	} Move;

} KUSB_HANDLE_INTERNAL;
typedef KUSB_HANDLE_INTERNAL* PKUSB_HANDLE_INTERNAL;


#define Init_Handle_OvlK(HandlePtr) do {										\
	memset(&((HandlePtr)->Overlapped), 0, sizeof((HandlePtr)->Overlapped));		\
	(HandlePtr)->Private = NULL;												\
	(HandlePtr)->Pool = NULL;													\
	(HandlePtr)->IsAcquired = 0;												\
}while(0)
typedef struct _KOVL_HANDLE_INTERNAL
{
	OVERLAPPED Overlapped;

	PKOVL_OVERLAPPED_INFO Private;
	KOVL_LINK_EL Link;
	KOVL_LINK_EL MasterLink;
	struct _KOVL_POOL_HANDLE_INTERNAL* Pool;

	volatile long IsAcquired;

	KOBJ_BASE Base;
} KOVL_HANDLE_INTERNAL;
typedef KOVL_HANDLE_INTERNAL* PKOVL_HANDLE_INTERNAL;


#define Init_Handle_OvlPoolK(HandlePtr) do {	\
	(HandlePtr)->ReleasedList = NULL;   		\
	(HandlePtr)->AcquiredList = NULL;   		\
	(HandlePtr)->MasterList = NULL; 			\
	(HandlePtr)->MaxCount = 0;  				\
	(HandlePtr)->Count = 0; 					\
}while(0)
typedef struct _KOVL_POOL_HANDLE_INTERNAL
{
	KOBJ_BASE Base;

	PKOVL_LINK_EL ReleasedList;
	PKOVL_LINK_EL AcquiredList;
	PKOVL_LINK_EL MasterList;

	ULONG MaxCount;
	ULONG Count;

} KOVL_POOL_HANDLE_INTERNAL;
typedef KOVL_POOL_HANDLE_INTERNAL* PKOVL_POOL_HANDLE_INTERNAL;


#define Init_Handle_HotK(HandlePtr) do {   							\
	memset(&((HandlePtr)->Public), 0, sizeof((HandlePtr)->Public));	\
	(HandlePtr)->next = NULL;  										\
	(HandlePtr)->prev = NULL;  										\
}while(0)
typedef struct _KHOT_HANDLE_INTERNAL
{
	KOBJ_BASE Base;

	KHOT_PARAMS Public;

	struct _KHOT_HANDLE_INTERNAL* prev;
	struct _KHOT_HANDLE_INTERNAL* next;

} KHOT_HANDLE_INTERNAL;
typedef KHOT_HANDLE_INTERNAL* PKHOT_HANDLE_INTERNAL;


#define Init_Handle_LstK(HandlePtr) do {	\
	(HandlePtr)->current = NULL;			\
	(HandlePtr)->head = NULL;   			\
}while(0)
typedef struct _KLST_HANDLE_INTERNAL
{
	KOBJ_BASE Base;

	PKLST_DEVINFO_EL current;
	PKLST_DEVINFO_EL head;

} KLST_HANDLE_INTERNAL;
typedef KLST_HANDLE_INTERNAL* PKLST_HANDLE_INTERNAL;

#define Init_Handle_LstInfoK(HandlePtr) do {		\
	(HandlePtr)->DevInfoEL = NULL;					\
}while(0)
typedef struct _KLST_DEVINFO_HANDLE_INTERNAL
{
	KOBJ_BASE Base;
	KLST_DEVINFO_EL* DevInfoEL;
} KLST_DEVINFO_HANDLE_INTERNAL;
typedef KLST_DEVINFO_HANDLE_INTERNAL* PKLST_DEVINFO_HANDLE_INTERNAL;

#endif

#define DEF_POOLED_HANDLE_STRUCT(AllKSection,HandleType,HandlePoolCount)	\
	struct  																\
	{   																	\
		volatile long BusyLock; 											\
		ULONG DefUserContextSize;   										\
		ULONG CurrentPos;   												\
		ULONG NextPos;  													\
		KLIB_INIT_HANDLE_CB* InitHandleCB;  								\
		KLIB_FREE_HANDLE_CB* FreeHandleCB;  								\
		HandleType Handles[HandlePoolCount];								\
	} AllKSection

// structure of all static libusbK handle pools.
typedef struct
{
	volatile BOOL Valid;
	volatile long InitLock;

	HANDLE Heap;

	KDYN_CancelIoEx* CancelIoEx;
	KDYN_PathMatchSpec* PathMatchSpec;

	DEF_POOLED_HANDLE_STRUCT(HotK,		KHOT_HANDLE_INTERNAL,			KHOT_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(LstK,		KLST_HANDLE_INTERNAL,			KLST_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(LstInfoK,	KLST_DEVINFO_HANDLE_INTERNAL,	KLST_DEVINFO_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(UsbK,		KUSB_HANDLE_INTERNAL,			KUSB_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(DevK,		KDEV_HANDLE_INTERNAL,			KDEV_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(OvlK,		KOVL_HANDLE_INTERNAL,			KOVL_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(OvlPoolK,	KOVL_POOL_HANDLE_INTERNAL,		KOVL_POOL_HANDLE_COUNT);
} ALLK_CONTEXT;

extern ALLK_CONTEXT AllK;

//////////////////////////////////////////////////////////////////////////////
// Inline memory allocation functions.
//

FORCEINLINE PVOID KUSB_API Mem_Alloc(__in size_t size)
{
	PVOID memory = HeapAlloc(AllK.Heap, HEAP_ZERO_MEMORY, size);
	if (!memory) LusbwError(ERROR_NOT_ENOUGH_MEMORY);
	return memory;
}

FORCEINLINE VOID KUSB_API Mem_Free(__deref_inout_opt PVOID* memoryRef)
{
	if (memoryRef)
	{
		if (IsHandleValid(*memoryRef))
			HeapFree(AllK.Heap, 0, *memoryRef);

		*memoryRef = NULL;
	}
}

FORCEINLINE LPSTR Str_Dupe(__in_opt LPCSTR string)
{
	LPSTR strDupe = NULL;
	if (!Str_IsNullOrEmpty(string))
	{
		size_t len = strlen(string);
		if (len > 4096) return NULL;

		strDupe = Mem_Alloc(len + 1);
		memcpy(strDupe, string, len);
	}
	return strDupe;
}

void CheckLibInit();

BOOL MatchPattern(LPCSTR Pattern, LPCSTR File);

PROTO_POOLHANDLE(HotK, KHOT_HANDLE_INTERNAL);
PROTO_POOLHANDLE(LstK, KLST_HANDLE_INTERNAL);
PROTO_POOLHANDLE(LstInfoK, KLST_DEVINFO_HANDLE_INTERNAL);
PROTO_POOLHANDLE(UsbK, KUSB_HANDLE_INTERNAL);
PROTO_POOLHANDLE(DevK, KDEV_HANDLE_INTERNAL);
PROTO_POOLHANDLE(OvlK, KOVL_HANDLE_INTERNAL);
PROTO_POOLHANDLE(OvlPoolK, KOVL_POOL_HANDLE_INTERNAL);

#define PoolHandle_Live(KLib_Handle_Internal,AllKSection,KLib_Handle_Type) do { 														\
	if (!(KLib_Handle_Internal)->Base.User.Valid)   																					\
	{   																																\
		(KLib_Handle_Internal)->Base.User.Valid = TRUE; 																				\
		if (AllK.AllKSection.DefUserContextSize > 0)																					\
		{   																															\
			(KLib_Handle_Internal)->Base.User.ContextAllocated = TRUE;  																\
			(KLib_Handle_Internal)->Base.User.ContextSize = AllK.AllKSection.DefUserContextSize;										\
			(KLib_Handle_Internal)->Base.User.Context.Custom = Mem_Alloc(AllK.AllKSection.DefUserContextSize);  						\
		}   																															\
		if (AllK.AllKSection.InitHandleCB)  																							\
			AllK.AllKSection.InitHandleCB((KLib_Handle_Internal), (KLib_Handle_Type), &((KLib_Handle_Internal)->Base.User.Context));	\
	}   																																\
}while(0)

#define PoolHandle_Dead(KLib_Handle_Internal,AllKSection,KLib_Handle_Type) do { 														\
	if ((KLib_Handle_Internal)->Base.User.Valid)																						\
	{   																																\
		(KLib_Handle_Internal)->Base.User.Valid = FALSE;																				\
		if (AllK.AllKSection.FreeHandleCB)  																							\
			AllK.AllKSection.FreeHandleCB((KLib_Handle_Internal), (KLib_Handle_Type), &((KLib_Handle_Internal)->Base.User.Context));	\
		if ((KLib_Handle_Internal)->Base.User.ContextAllocated) 																		\
		{   																															\
			(KLib_Handle_Internal)->Base.User.ContextAllocated = FALSE;   																\
			Mem_Free(&((KLib_Handle_Internal)->Base.User.Context.Custom));  															\
		}   																															\
	}   																																\
}while(0)

#define PoolHandle_Dead_HotK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,HotK,KLIB_HANDLE_TYPE_HOTK)
#define PoolHandle_Dead_LstK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,LstK,KLIB_HANDLE_TYPE_LSTK)
#define PoolHandle_Dead_LstInfoK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,LstInfoK,KLIB_HANDLE_TYPE_LSTINFOK)
#define PoolHandle_Dead_UsbK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,UsbK,KLIB_HANDLE_TYPE_USBK)
#define PoolHandle_Dead_DevK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,DevK,KLIB_HANDLE_TYPE_USBSHAREDK)
#define PoolHandle_Dead_OvlK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,OvlK,KLIB_HANDLE_TYPE_OVLK)
#define PoolHandle_Dead_OvlPoolK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,OvlPoolK,KLIB_HANDLE_TYPE_OVLPOOLK)

#define PoolHandle_Live_HotK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,HotK,KLIB_HANDLE_TYPE_HOTK)
#define PoolHandle_Live_LstK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,LstK,KLIB_HANDLE_TYPE_LSTK)
#define PoolHandle_Live_LstInfoK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,LstInfoK,KLIB_HANDLE_TYPE_LSTINFOK)
#define PoolHandle_Live_UsbK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,UsbK,KLIB_HANDLE_TYPE_USBK)
#define PoolHandle_Live_DevK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,DevK,KLIB_HANDLE_TYPE_USBSHAREDK)
#define PoolHandle_Live_OvlK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,OvlK,KLIB_HANDLE_TYPE_OVLK)
#define PoolHandle_Live_OvlPoolK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,OvlPoolK,KLIB_HANDLE_TYPE_OVLPOOLK)

#endif
