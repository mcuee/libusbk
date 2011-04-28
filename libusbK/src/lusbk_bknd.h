/*! \file lusbk_bknd.h
*/

#ifndef __LUSBK_BKND_
#define __LUSBK_BKND_

#include "lusbk_private.h"

long AquireDeviceActionPendingLock(__in PKUSB_SHARED_DEVICE SharedDevice);
long ReleaseDeviceActionPendingLock(__in PKUSB_SHARED_DEVICE SharedDevice);

#define GET_INTERNAL_HANDLE_EX(HandlePtrRtn, OnFailureReturnTrue)						\
{																						\
	HandlePtrRtn = PublicToPrivateHandle(InterfaceHandle);								\
	if (!(HandlePtrRtn))																\
	{																					\
		if ((OnFailureReturnTrue)==TRUE) return TRUE;									\
		USBERR("invalid interface handle.\n");											\
		return LusbwError(ERROR_INVALID_HANDLE);										\
	}																					\
}

#define GET_INTERNAL_HANDLE(HandlePtrRtn) GET_INTERNAL_HANDLE_EX(HandlePtrRtn, FALSE)

#define GET_BACKEND_CONTEXT(BackendPtrRtn, InternalHandle, ContextType)			\
{																				\
	BackendPtrRtn = (ContextType*)(InternalHandle)->BackendContext;				\
	if (!IsHandleValid(BackendPtrRtn))											\
	{																			\
		USBERR("invalid back-end context.\n");									\
		return LusbwError(ERROR_INVALID_HANDLE);								\
	}																			\
}

#define GET_BACKEND_CONTEXT_EJUMP(BackendPtrRtn, InternalHandle, ContextType, ErrorJump)	\
{																							\
	BackendPtrRtn = (ContextType*)(InternalHandle)->BackendContext;							\
	if (!IsHandleValid(BackendPtrRtn))														\
	{																						\
		USBERR("invalid back-end context.\n");												\
		LusbwError(ERROR_INVALID_HANDLE);													\
		goto ErrorJump;																		\
	}																						\
}

PKUSB_INTERFACE_HANDLE_INTERNAL GetInternalPoolHandle();
PKUSB_SHARED_DEVICE GetSharedDevicePoolHandle(LPCSTR DevicePath);

BOOL GetProcAddress_UsbK(__out KPROC* ProcAddress, __in ULONG FunctionID);
BOOL GetProcAddress_LUsb0(__out KPROC* ProcAddress, __in ULONG FunctionID);
BOOL GetProcAddress_WUsb(__out KPROC* ProcAddress, __in ULONG FunctionID);

#endif
