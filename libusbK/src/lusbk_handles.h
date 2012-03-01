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

#ifndef __LUSBK_HANDLES_H_
#define __LUSBK_HANDLES_H_

#include "lusbk_private.h"
#include "lusbk_lockedarray_list.h"

#define KHOT_HANDLE_COUNT				8
#define KLST_HANDLE_COUNT				64
#define KLST_DEVINFO_HANDLE_COUNT		128
#define KUSB_HANDLE_COUNT				64
#define KDEV_HANDLE_COUNT				32
#define KDEV_SHARED_INTERFACE_COUNT		128
#define KOVL_HANDLE_COUNT				1024
#define KOVL_POOL_HANDLE_COUNT			64
#define KSTM_HANDLE_COUNT				32

#define DL_MatchPattern(FilePattern,FilePath) (AllK.PathMatchSpec(FilePath, FilePattern)?0:-1)

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

#define ALLK_HANDLE_FOR_LOOP(HandlePosVar,AllKSection)	\
	for (HandlePosVar=0; HandlePosVar < sizeof(AllK->AllKSection.Handles)/sizeof(AllK->AllKSection.Handles[0]); HandlePosVar++)

#define POOLHANDLE_LIB_EXIT_CHECK(AllKSection)	do {												\
		int pos;  																						\
		for (pos=0; pos < sizeof(AllK.AllKSection.Handles)/sizeof(AllK.AllKSection.Handles[0]); pos++)	\
		{ 																								\
			if (AllK.AllKSection.Handles[pos].Base.Count.Ref != 0)  									\
			{ 																							\
				USBWRNN("Invalid %s handle reference count %d at index %d",   							\
				        DEFINE_TO_STR(AllKSection),   														\
				        AllK.AllKSection.Handles[pos].Base.Count.Ref,   									\
				        pos); 																				\
			} 																							\
		} 																								\
	}while(0)

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

#define Pub_To_Priv_StmK(KStm_Pool_Handle,KStm_Pool_Handle_Internal,ErrorAction)						\
	PUB_TO_PRIV(StmK,KSTM_HANDLE_INTERNAL,KStm_Pool_Handle,KStm_Pool_Handle_Internal,ErrorAction)

#define PROTO_POOLHANDLE(AllKSection,HandleType)											\
	KLIB_USER_CONTEXT PoolHandle_GetContext_##AllKSection(P##HandleType PoolHandle);		\
	P## HandleType PoolHandle_Acquire_##AllKSection(PKOBJ_CB EvtCleanup);					\
	BOOL PoolHandle_Inc_##AllKSection(P##HandleType PoolHandle);							\
	BOOL PoolHandle_IncEx_##AllKSection(P##HandleType PoolHandle, long* lockCount);			\
	BOOL PoolHandle_Dec_##AllKSection(P##HandleType PoolHandle);							\
	 

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

//! Synchronize the device info elements of two device lists.
/*!
*
* \param[in] MasterList
* The device list handle to use as the master list.
*
* \param[in] SlaveList
* The device list handle to use as the master list.
*
* \param[in] SyncFlags
* One or more \ref KLST_SYNC_FLAG.
*
* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
*
*/
KUSB_EXP BOOL KUSB_API LstK_Sync(
    _in KLST_HANDLE MasterList,
    _in KLST_HANDLE SlaveList,
    _inopt KLST_SYNC_FLAG SyncFlags);

//! Creates a copy of an existing device list.
/*!
*
* \param[in] SrcList
* The device list to copy.
*
* \param[out] DstList
* Reference to a pointer that receives the cloned device list.
*
* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
*
*/
KUSB_EXP BOOL KUSB_API LstK_Clone(
    _in KLST_HANDLE SrcList,
    _out KLST_HANDLE* DstList);

//! Creates a copy of an existing device info handle.
/*!
*
* \param[in] SrcInfo
* The device info to copy.
*
* \param[out] DstInfo
* Reference to a pointer that receives the cloned device info.
*
* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
*
*/
KUSB_EXP BOOL KUSB_API LstK_CloneInfo(
    _in KLST_DEVINFO_HANDLE SrcInfo,
    _out KLST_DEVINFO_HANDLE* DstInfo);

//! Removes a device info handle for a device list.
/*!
*
* \param[in] DeviceList
* The device list of the info element.
*
* \param[in] DeviceInfo
* The device info element to remove from the list.
*
* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
*
*/
KUSB_EXP BOOL KUSB_API LstK_DetachInfo(
    _in KLST_HANDLE DeviceList,
    _in KLST_DEVINFO_HANDLE DeviceInfo);

//! Appends a device info handle for a device list.
/*!
*
* \param[in] DeviceList
* The device list to add the info element.
*
* \param[in] DeviceInfo
* The device info element to add to the list.
*
* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
*
*/
KUSB_EXP BOOL KUSB_API LstK_AttachInfo(
    _in KLST_HANDLE DeviceList,
    _in KLST_DEVINFO_HANDLE DeviceInfo);

//! Frees a device info handle that was detached with \ref LstK_DetachInfo.
/*!
*
* \param[in] DeviceInfo
* The device info element to free.
*
* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
*
*/
KUSB_EXP BOOL KUSB_API LstK_FreeInfo(
    _in KLST_DEVINFO_HANDLE DeviceInfo);



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


typedef struct _KSTM_XFER_LINK_EL
{
	struct _KSTM_XFER_INTERNAL* Xfer;

	struct _KSTM_XFER_LINK_EL* next;
	struct _KSTM_XFER_LINK_EL* prev;

} KSTM_XFER_LINK_EL, *PKSTM_XFER_LINK_EL;
typedef KSTM_XFER_LINK_EL KSTM_XFER_ITEM, *PKSTM_XFER_ITEM;
ALDEF_LIST_HDR(KSTM_XFER, XferLink);

typedef struct _KSTM_OVERLAPPED_EL
{
	OVERLAPPED Overlapped;

	struct _KSTM_OVERLAPPED_EL* next;
	struct _KSTM_OVERLAPPED_EL* prev;

} KSTM_OVERLAPPED_EL, *PKSTM_OVERLAPPED_EL;

typedef struct _KSTM_XFER_INTERNAL
{
	LPOVERLAPPED Overlapped;

	KSTM_XFER_CONTEXT Public;

	KSTM_XFER_LINK_EL Link;

	KSTM_XFER_LINK_EL MasterLink;

	INT Index;

	struct _KSTM_HANDLE_INTERNAL* StreamHandle;

	ULONG BufferSize;
	PUCHAR Buffer;

} KSTM_XFER_INTERNAL, *PKSTM_XFER_INTERNAL;


#define Init_Handle_ObjK(BaseObjPtr,AllKSection) do {			\
		memset(&((BaseObjPtr)->User),0,sizeof((BaseObjPtr)->User));	\
		(BaseObjPtr)->Evt.Cleanup = NULL;							\
		(BaseObjPtr)->Count.Ref = 1;								\
		(BaseObjPtr)->Disposing = 0;								\
	}while(0)
typedef struct _KOBJ_BASE
{
	// Generally used as a spin-lock at a api-backend defined level
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
		KLIB_HANDLE_CLEANUP_CB* CleanupCB;
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
		(HandlePtr)->Pool = NULL;													\
		(HandlePtr)->IsAcquired = 0;												\
	}while(0)
typedef struct _KOVL_HANDLE_INTERNAL
{
	OVERLAPPED Overlapped;

	struct _KOVL_POOL_HANDLE_INTERNAL* Pool;

	volatile long IsAcquired;

	struct _KOVL_EL* MasterLink;

	KOBJ_BASE Base;
} KOVL_HANDLE_INTERNAL, *PKOVL_HANDLE_INTERNAL;
typedef KOVL_HANDLE_INTERNAL KOVL_ITEM, *PKOVL_ITEM;

typedef struct _KOVL_EL
{
	PKOVL_HANDLE_INTERNAL Handle;

	struct _KOVL_EL* next;
	struct _KOVL_EL* prev;

} KOVL_EL, *PKOVL_EL;

#define Init_Handle_OvlPoolK(HandlePtr) do {	\
		(HandlePtr)->Flags = 0; 					\
		(HandlePtr)->UsbHandle = NULL; 				\
	}while(0)
typedef struct _KOVL_POOL_HANDLE_INTERNAL
{
	KOBJ_BASE Base;

	volatile long MasterListCount;

	PKOVL_EL MasterArray;
	PKOVL_EL AcquiredList;
	PKOVL_EL ReleasedList;

	KOVL_POOL_FLAG Flags;
	PKUSB_HANDLE_INTERNAL UsbHandle;
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

#define KSTM_THREADSTATE_STOPPED	(0)
#define KSTM_THREADSTATE_STARTED	(1 << 0)
#define KSTM_THREADSTATE_STARTING	(1 << 1)
#define KSTM_THREADSTATE_STOPPING	(1 << 2)

#define Init_Handle_StmK(HandlePtr) do {							\
		(HandlePtr)->Heap = NULL;										\
		(HandlePtr)->Info = NULL;										\
		(HandlePtr)->Flags = 0;											\
		(HandlePtr)->UserCB = NULL;										\
		(HandlePtr)->PendingTransfers = 0;								\
		(HandlePtr)->PendingIO = 0;										\
		(HandlePtr)->TimeoutCancelMS = 0;								\
		memset(&((HandlePtr)->Thread), 0, sizeof((HandlePtr)->Thread));	\
		memset(&((HandlePtr)->List), 0, sizeof((HandlePtr)->List));		\
	}while(0)
typedef struct _KSTM_HANDLE_INTERNAL
{
	KOBJ_BASE Base;

	HANDLE			Heap;
	PKSTM_INFO		Info;
	KSTM_FLAG		Flags;
	PKSTM_CALLBACK	UserCB;

	volatile long PendingTransfers;
	volatile long PendingIO;

	ULONG TimeoutCancelMS;
	HANDLE SemReady;
	INT WaitTimeout;

	struct
	{
		volatile long State;

		UINT Id;
		HANDLE Handle;
		HANDLE StartedEvent;
		HANDLE StoppedEvent;
	} Thread;

	struct
	{
		PKSTM_XFER_LIST Idle;
		PKSTM_XFER_LIST Finished;
		PKSTM_XFER_LINK_EL Queued;
		PKSTM_XFER_LINK_EL Master;

	} List;

} KSTM_HANDLE_INTERNAL;
typedef KSTM_HANDLE_INTERNAL* PKSTM_HANDLE_INTERNAL;

#endif

#define DEF_POOLED_HANDLE_STRUCT(AllKSection,HandleType,HandlePoolCount)	\
	struct  																\
	{   																	\
		volatile long Index; 												\
		HandleType Handles[HandlePoolCount];								\
	} AllKSection

// structure of all static libusbK handle pools.
typedef struct
{
	volatile BOOL Valid;
	volatile long InitLock;

	HANDLE Heap;

	BOOL (WINAPI* CancelIoEx)(HANDLE DeviceHandle, KOVL_HANDLE Overlapped);

	KDYN_PathMatchSpec* PathMatchSpec;

	DEF_POOLED_HANDLE_STRUCT(HotK,		KHOT_HANDLE_INTERNAL,			KHOT_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(LstK,		KLST_HANDLE_INTERNAL,			KLST_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(LstInfoK,	KLST_DEVINFO_HANDLE_INTERNAL,	KLST_DEVINFO_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(UsbK,		KUSB_HANDLE_INTERNAL,			KUSB_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(DevK,		KDEV_HANDLE_INTERNAL,			KDEV_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(OvlK,		KOVL_HANDLE_INTERNAL,			KOVL_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(OvlPoolK,	KOVL_POOL_HANDLE_INTERNAL,		KOVL_POOL_HANDLE_COUNT);
	DEF_POOLED_HANDLE_STRUCT(StmK,		KSTM_HANDLE_INTERNAL,			KSTM_HANDLE_COUNT);
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

PROTO_POOLHANDLE(HotK, KHOT_HANDLE_INTERNAL);
PROTO_POOLHANDLE(LstK, KLST_HANDLE_INTERNAL);
PROTO_POOLHANDLE(LstInfoK, KLST_DEVINFO_HANDLE_INTERNAL);
PROTO_POOLHANDLE(UsbK, KUSB_HANDLE_INTERNAL);
PROTO_POOLHANDLE(DevK, KDEV_HANDLE_INTERNAL);
PROTO_POOLHANDLE(OvlK, KOVL_HANDLE_INTERNAL);
PROTO_POOLHANDLE(OvlPoolK, KOVL_POOL_HANDLE_INTERNAL);
PROTO_POOLHANDLE(StmK, KSTM_HANDLE_INTERNAL);

#define PoolHandle_Live(KLib_Handle_Internal,AllKSection,KLib_Handle_Type) do {	\
		if (!(KLib_Handle_Internal)->Base.User.Valid)  								\
		{  																			\
			(KLib_Handle_Internal)->Base.User.Valid = TRUE;							\
		}  																			\
	}while(0)

#define PoolHandle_Dead(KLib_Handle_Internal,AllKSection,KLib_Handle_Type) do {	\
		if ((KLib_Handle_Internal)->Base.User.Valid)   								\
		{  																			\
			(KLib_Handle_Internal)->Base.User.Valid = FALSE;   						\
			if ((KLib_Handle_Internal)->Base.User.CleanupCB)  						\
			{  																		\
				(KLib_Handle_Internal)->Base.User.CleanupCB(   					\
				        (KLib_Handle_Internal),    										\
				        (KLib_Handle_Type),    											\
				        ((KLib_Handle_Internal)->Base.User.Context)); 					\
				\
				(KLib_Handle_Internal)->Base.User.CleanupCB = NULL;				\
			}  																		\
		}  																			\
	}while(0)


#define PoolHandle_Dead_HotK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,HotK,KLIB_HANDLE_TYPE_HOTK)
#define PoolHandle_Dead_LstK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,LstK,KLIB_HANDLE_TYPE_LSTK)
#define PoolHandle_Dead_LstInfoK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,LstInfoK,KLIB_HANDLE_TYPE_LSTINFOK)
#define PoolHandle_Dead_UsbK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,UsbK,KLIB_HANDLE_TYPE_USBK)
#define PoolHandle_Dead_DevK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,DevK,KLIB_HANDLE_TYPE_USBSHAREDK)
#define PoolHandle_Dead_OvlK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,OvlK,KLIB_HANDLE_TYPE_OVLK)
#define PoolHandle_Dead_OvlPoolK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,OvlPoolK,KLIB_HANDLE_TYPE_OVLPOOLK)
#define PoolHandle_Dead_StmK(KLib_Handle_Internal) PoolHandle_Dead(KLib_Handle_Internal,StmK,KLIB_HANDLE_TYPE_STMK)

#define PoolHandle_Live_HotK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,HotK,KLIB_HANDLE_TYPE_HOTK)
#define PoolHandle_Live_LstK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,LstK,KLIB_HANDLE_TYPE_LSTK)
#define PoolHandle_Live_LstInfoK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,LstInfoK,KLIB_HANDLE_TYPE_LSTINFOK)
#define PoolHandle_Live_UsbK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,UsbK,KLIB_HANDLE_TYPE_USBK)
#define PoolHandle_Live_DevK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,DevK,KLIB_HANDLE_TYPE_USBSHAREDK)
#define PoolHandle_Live_OvlK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,OvlK,KLIB_HANDLE_TYPE_OVLK)
#define PoolHandle_Live_OvlPoolK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,OvlPoolK,KLIB_HANDLE_TYPE_OVLPOOLK)
#define PoolHandle_Live_StmK(KLib_Handle_Internal) PoolHandle_Live(KLib_Handle_Internal,StmK,KLIB_HANDLE_TYPE_STMK)

#endif
