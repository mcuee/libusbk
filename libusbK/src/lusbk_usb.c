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
#include "lusbk_handles.h"

// Default loggging level
ULONG DebugLevel = 4;

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

/*
#define RET_DEF_USER_CONTEXT_SIZE(AllKSection)	\
	return AllK.AllKSection.DefUserContextSize > sizeof(KLIB_USER_CONTEXT) ? AllK.AllKSection.DefUserContextSize : sizeof(KLIB_USER_CONTEXT)
*/

#define RET_DEF_USER_CONTEXT_SIZE(AllKSection)	return (AllK.AllKSection.DefUserContextSize)

#define KLIB_SET_USER_CONTEXT_CB(AllKSection,mInit_User_ContextCB,mFree_User_ContextCB) do {		\
	AllK.AllKSection.InitHandleCB = mInit_User_ContextCB;											\
	AllK.AllKSection.FreeHandleCB = mFree_User_ContextCB;											\
}while(0)

#define GET_CONTEXT_CASE(AllKSection,Handle_Suffix)  						\
	case KLIB_HANDLE_TYPE_##Handle_Suffix:   								\
	Pub_To_Priv_##AllKSection(Handle,priv.AllKSection.Handle,return NULL)	\
 
#define GETRET_CONTEXT_CASE(AllKSection,Handle_Suffix)   					\
	GET_CONTEXT_CASE(AllKSection,Handle_Suffix); 							\
	return &priv.AllKSection.Handle->Base.User.Context   					\
 
#define GET_CONTEXTSIZE_CASE(AllKSection,Handle_Suffix)  					\
	case KLIB_HANDLE_TYPE_##Handle_Suffix:   								\
	Pub_To_Priv_##AllKSection(Handle,priv.AllKSection.Handle,return -1)		\
 
#define GETRET_CONTEXTSIZE_CASE(AllKSection,Handle_Suffix)   				\
	GET_CONTEXTSIZE_CASE(AllKSection,Handle_Suffix); 						\
	return (LONG)priv.AllKSection.Handle->Base.User.ContextSize


typedef union  _KLIB_ALLK_PRIVATE_HANDLE
{
	struct
	{
		PKHOT_HANDLE_INTERNAL Handle;
	} HotK;
	struct
	{
		PKLST_HANDLE_INTERNAL Handle;
	} LstK;
	struct
	{
		PKLST_DEVINFO_HANDLE_INTERNAL Handle;
	} LstInfoK;
	struct
	{
		PKUSB_HANDLE_INTERNAL Handle;
	} UsbK;
	struct
	{
		PKOVL_HANDLE_INTERNAL Handle;
	} OvlK;
	struct
	{
		PKOVL_POOL_HANDLE_INTERNAL Handle;
	} OvlPoolK;
} KLIB_ALLK_PRIVATE_HANDLE;

KUSB_EXP BOOL KUSB_API LibK_GetProcAddress(__out KPROC* ProcAddress, __in KUSB_DRVID DriverID, __in KUSB_FNID FunctionID)
{
	CheckLibInit();

	*ProcAddress = (KPROC)NULL;

	switch(DriverID)
	{
	case KUSB_DRVID_LIBUSBK:
		if (GetProcAddress_UsbK(ProcAddress, FunctionID) && *ProcAddress != NULL)
			return TRUE;
		break;

	case KUSB_DRVID_LIBUSB0_FILTER:
	case KUSB_DRVID_LIBUSB0:
		if (GetProcAddress_LUsb0(ProcAddress, FunctionID) && *ProcAddress != NULL)
			return TRUE;
		break;

	case KUSB_DRVID_WINUSB:
		if (GetProcAddress_WUsb(ProcAddress, FunctionID) && *ProcAddress != NULL)
			return TRUE;

	default:
		if (GetProcAddress_Unsupported(ProcAddress, FunctionID) && *ProcAddress != NULL)
			return TRUE;
		break;
	}

	return LusbwError(ERROR_NOT_SUPPORTED);
}


KUSB_EXP BOOL KUSB_API LibK_LoadDriverApi(
    __inout KUSB_DRIVER_API* DriverAPI,
    __in ULONG DriverID,
    __in ULONG SizeofDriverAPI)
{

#define CASE_FNID_LOAD(FunctionName)																\
		case KUSB_FNID_##FunctionName:																\
		if (LibK_GetProcAddress((KPROC*)&tempDriverAPI.FunctionName, DriverID, fnIdIndex) == FALSE)	\
		{																							\
			USBWRNN("function id %u for driver id %u does not exist.",fnIdIndex,DriverID);			\
		}else {fnIdCount++;}																		\
		break

	KUSB_DRIVER_API tempDriverAPI;
	int fnIdIndex;
	int fnIdCount = 0;

	CheckLibInit();

	Mem_Zero(&tempDriverAPI, sizeof(tempDriverAPI));

	if (!IsHandleValid(DriverAPI))
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	for (fnIdIndex = 0; fnIdIndex < KUSB_FNID_COUNT; fnIdIndex++)
	{
		switch(fnIdIndex)
		{
			CASE_FNID_LOAD(Initialize);
			CASE_FNID_LOAD(Free);
			CASE_FNID_LOAD(GetAssociatedInterface);
			CASE_FNID_LOAD(GetDescriptor);
			CASE_FNID_LOAD(QueryInterfaceSettings);
			CASE_FNID_LOAD(QueryDeviceInformation);
			CASE_FNID_LOAD(SetCurrentAlternateSetting);
			CASE_FNID_LOAD(GetCurrentAlternateSetting);
			CASE_FNID_LOAD(QueryPipe);
			CASE_FNID_LOAD(SetPipePolicy);
			CASE_FNID_LOAD(GetPipePolicy);
			CASE_FNID_LOAD(ReadPipe);
			CASE_FNID_LOAD(WritePipe);
			CASE_FNID_LOAD(ControlTransfer);
			CASE_FNID_LOAD(ResetPipe);
			CASE_FNID_LOAD(AbortPipe);
			CASE_FNID_LOAD(FlushPipe);
			CASE_FNID_LOAD(SetPowerPolicy);
			CASE_FNID_LOAD(GetPowerPolicy);
			CASE_FNID_LOAD(GetOverlappedResult);
			CASE_FNID_LOAD(ResetDevice);
			CASE_FNID_LOAD(Open);
			CASE_FNID_LOAD(Close);
			CASE_FNID_LOAD(SetConfiguration);
			CASE_FNID_LOAD(GetConfiguration);
			CASE_FNID_LOAD(ClaimInterface);
			CASE_FNID_LOAD(ReleaseInterface);
			CASE_FNID_LOAD(SetAltInterface);
			CASE_FNID_LOAD(GetAltInterface);
			CASE_FNID_LOAD(IsoReadPipe);
			CASE_FNID_LOAD(IsoWritePipe);
			CASE_FNID_LOAD(GetCurrentFrameNumber);
			CASE_FNID_LOAD(Clone);
			CASE_FNID_LOAD(SelectInterface);

		default:
			USBERRN("undeclared api function %u!", fnIdIndex);
			return LusbwError(ERROR_NOT_SUPPORTED);
		}
	}

	memcpy(DriverAPI, &tempDriverAPI, SizeofDriverAPI);
	return fnIdCount > 0 ? (BOOL)fnIdCount : FALSE;
}

KUSB_EXP PKLIB_USER_CONTEXT KUSB_API LibK_GetContext(KLIB_HANDLE Handle, KLIB_HANDLE_TYPE HandleType)
{
	KLIB_ALLK_PRIVATE_HANDLE priv;

	switch (HandleType)
	{
		GETRET_CONTEXT_CASE(HotK, HOTK);

		GETRET_CONTEXT_CASE(UsbK, USBK);

		GET_CONTEXT_CASE(UsbK, USBSHAREDK);
		return &priv.UsbK.Handle->Device->Base.User.Context;

		GETRET_CONTEXT_CASE(LstK, LSTK);

		GETRET_CONTEXT_CASE(LstInfoK, LSTINFOK);

		GETRET_CONTEXT_CASE(OvlK, OVLK);

		GETRET_CONTEXT_CASE(OvlPoolK, OVLPOOLK);
	}

	LusbwError(ERROR_INVALID_HANDLE);
	return NULL;
}


KUSB_EXP LONG KUSB_API LibK_GetContextSize(KLIB_HANDLE Handle, KLIB_HANDLE_TYPE HandleType)
{
	KLIB_ALLK_PRIVATE_HANDLE priv;

	switch (HandleType)
	{
		GETRET_CONTEXTSIZE_CASE(HotK, HOTK);

		GETRET_CONTEXTSIZE_CASE(UsbK, USBK);

		GET_CONTEXTSIZE_CASE(UsbK, USBSHAREDK);
		return priv.UsbK.Handle->Device->Base.User.ContextSize;

		GETRET_CONTEXTSIZE_CASE(LstK, LSTK);

		GETRET_CONTEXTSIZE_CASE(LstInfoK, LSTINFOK);

		GETRET_CONTEXTSIZE_CASE(OvlK, OVLK);

		GETRET_CONTEXTSIZE_CASE(OvlPoolK, OVLPOOLK);
	}

	LusbwError(ERROR_INVALID_HANDLE);
	return -1;
}

KUSB_EXP BOOL KUSB_API LibK_SetHandleCallbacks(
    KLIB_HANDLE_TYPE HandleType,
    KLIB_INIT_HANDLE_CB* InitHandleCB,
    KLIB_FREE_HANDLE_CB* FreeHandleCB)
{
	switch(HandleType)
	{
	case KLIB_HANDLE_TYPE_HOTK:
		KLIB_SET_USER_CONTEXT_CB(HotK, InitHandleCB, FreeHandleCB);
		return TRUE;
	case KLIB_HANDLE_TYPE_USBK:
		KLIB_SET_USER_CONTEXT_CB(UsbK, InitHandleCB, FreeHandleCB);
		return TRUE;
	case KLIB_HANDLE_TYPE_USBSHAREDK:
		KLIB_SET_USER_CONTEXT_CB(DevK, InitHandleCB, FreeHandleCB);
		return TRUE;
	case KLIB_HANDLE_TYPE_LSTK:
		KLIB_SET_USER_CONTEXT_CB(LstK, InitHandleCB, FreeHandleCB);
		return TRUE;
	case KLIB_HANDLE_TYPE_LSTINFOK:
		KLIB_SET_USER_CONTEXT_CB(LstInfoK, InitHandleCB, FreeHandleCB);
		return TRUE;
	case KLIB_HANDLE_TYPE_OVLK:
		KLIB_SET_USER_CONTEXT_CB(OvlK, InitHandleCB, FreeHandleCB);
		return TRUE;
	case KLIB_HANDLE_TYPE_OVLPOOLK:
		KLIB_SET_USER_CONTEXT_CB(OvlPoolK, InitHandleCB, FreeHandleCB);
		return TRUE;
	}
	LusbwError(ERROR_INVALID_PARAMETER);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LibK_SetContextSize(KLIB_HANDLE Handle, KLIB_HANDLE_TYPE HandleType, ULONG ContextSize)
{
	KLIB_ALLK_PRIVATE_HANDLE priv;
	PKOBJ_BASE base = NULL;

	ErrorParamAction(ContextSize != 0 || ContextSize <= sizeof(KLIB_USER_CONTEXT), "ContextSize", return FALSE);


	switch (HandleType)
	{
		GET_CONTEXTSIZE_CASE(HotK, HOTK);
		base = &priv.HotK.Handle->Base;
		break;

		GET_CONTEXTSIZE_CASE(UsbK, USBK);
		base = &priv.UsbK.Handle->Base;
		break;
		GET_CONTEXTSIZE_CASE(UsbK, USBSHAREDK);
		base = &priv.UsbK.Handle->Device->Base;
		break;

		GET_CONTEXTSIZE_CASE(LstK, LSTK);
		base = &priv.LstK.Handle->Base;
		break;

		GET_CONTEXTSIZE_CASE(LstInfoK, LSTINFOK);
		base = &priv.LstInfoK.Handle->Base;
		break;

		GET_CONTEXTSIZE_CASE(OvlK, OVLK);
		base = &priv.OvlK.Handle->Base;
		break;

		GET_CONTEXTSIZE_CASE(OvlPoolK, OVLPOOLK);
		base = &priv.OvlPoolK.Handle->Base;
		break;
	}

	if (!base)
	{
		LusbwError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	ErrorHandleAction(base->Disposing || !base->Count.Use || base->Count.Ref < 1, "Handle", return FALSE);

	if (base->User.ContextAllocated && ContextSize != base->User.ContextSize)
	{
		Mem_Free(&base->User.Context.Custom);
		base->User.ContextAllocated = FALSE;
		base->User.ContextSize = 0;
		base->User.Context.Custom = NULL;
	}
	if (!base->User.ContextAllocated && ContextSize)
	{
		base->User.ContextAllocated = TRUE;
		base->User.ContextSize = ContextSize;
		base->User.Context.Custom = Mem_Alloc(ContextSize);
	}

	return TRUE;
}

KUSB_EXP LONG KUSB_API LibK_GetDefaultContextSize(KLIB_HANDLE_TYPE HandleType)
{

	switch(HandleType)
	{
	case KLIB_HANDLE_TYPE_HOTK:
		RET_DEF_USER_CONTEXT_SIZE(HotK);
	case KLIB_HANDLE_TYPE_USBK:
		RET_DEF_USER_CONTEXT_SIZE(UsbK);
	case KLIB_HANDLE_TYPE_USBSHAREDK:
		RET_DEF_USER_CONTEXT_SIZE(DevK);
	case KLIB_HANDLE_TYPE_LSTK:
		RET_DEF_USER_CONTEXT_SIZE(LstK);
	case KLIB_HANDLE_TYPE_LSTINFOK:
		RET_DEF_USER_CONTEXT_SIZE(LstInfoK);
	case KLIB_HANDLE_TYPE_OVLK:
		RET_DEF_USER_CONTEXT_SIZE(OvlK);
	case KLIB_HANDLE_TYPE_OVLPOOLK:
		RET_DEF_USER_CONTEXT_SIZE(OvlPoolK);
	}

	LusbwError(ERROR_INVALID_PARAMETER);
	return 0;
}

KUSB_EXP BOOL KUSB_API LibK_SetDefaultContextSize(KLIB_HANDLE_TYPE HandleType, ULONG ContextSize)
{
	ErrorParamAction(ContextSize != 0 || ContextSize <= sizeof(KLIB_USER_CONTEXT), "ContextSize", return FALSE);

	switch(HandleType)
	{
	case KLIB_HANDLE_TYPE_HOTK:
		AllK.HotK.DefUserContextSize = ContextSize;
		return TRUE;
	case KLIB_HANDLE_TYPE_USBK:
		AllK.UsbK.DefUserContextSize = ContextSize;
		return TRUE;
	case KLIB_HANDLE_TYPE_USBSHAREDK:
		AllK.DevK.DefUserContextSize = ContextSize;
		return TRUE;
	case KLIB_HANDLE_TYPE_LSTK:
		AllK.LstK.DefUserContextSize = ContextSize;
		return TRUE;
	case KLIB_HANDLE_TYPE_LSTINFOK:
		AllK.LstInfoK.DefUserContextSize = ContextSize;
		return TRUE;
	case KLIB_HANDLE_TYPE_OVLK:
		AllK.OvlK.DefUserContextSize = ContextSize;
		return TRUE;
	case KLIB_HANDLE_TYPE_OVLPOOLK:
		AllK.OvlPoolK.DefUserContextSize = ContextSize;
		return TRUE;
	}
	LusbwError(ERROR_INVALID_PARAMETER);
	return FALSE;
}

