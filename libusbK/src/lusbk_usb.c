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
#include "lusbk_bknd_unsupported.h"

// Default loggging level
ULONG DebugLevel = 4;

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#define CASE_FNID_LOAD(FunctionName)															\
	case KUSB_FNID_##FunctionName:  															\
	if (LibK_GetProcAddress((KPROC*)&DriverAPI->FunctionName, DriverID, fnIdIndex) == FALSE)	\
	{   																						\
		USBWRNN("function id %u for driver id %u does not exist.",fnIdIndex,DriverID);  		\
	}   																						\
	else																						\
		fnIdCount++;																			\
	break

VOID WUsb_Init_Library();
BOOL GetProcAddress_Base(__out KPROC* ProcAddress, __in LONG FunctionID);
BOOL GetProcAddress_UsbK(__out KPROC* ProcAddress, __in LONG FunctionID);
BOOL GetProcAddress_WUsb(__out KPROC* ProcAddress, __in LONG FunctionID);

BOOL GetProcAddress_Base(__out KPROC* ProcAddress, __in LONG FunctionID)
{
	// some function are the same for all apis; they are set here.
	switch(FunctionID)
	{
	case KUSB_FNID_Free:
		*ProcAddress = (KPROC)UsbK_Free;
		break;
	case KUSB_FNID_GetAssociatedInterface:
		*ProcAddress = (KPROC)UsbK_GetAssociatedInterface;
		break;
	case KUSB_FNID_QueryInterfaceSettings:
		*ProcAddress = (KPROC)UsbK_QueryInterfaceSettings;
		break;
	case KUSB_FNID_QueryPipe:
		*ProcAddress = (KPROC)UsbK_QueryPipe;
		break;
	case KUSB_FNID_Clone:
		*ProcAddress = (KPROC)UsbK_Clone;
		break;
	case KUSB_FNID_SelectInterface:
		*ProcAddress = (KPROC)UsbK_SelectInterface;
		break;
	case KUSB_FNID_GetProperty:
		*ProcAddress = (KPROC)UsbK_GetProperty;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL GetProcAddress_UsbK(__out KPROC* ProcAddress, __in LONG FunctionID)
{
	switch(FunctionID)
	{
	case KUSB_FNID_Initialize:
		*ProcAddress = (KPROC)UsbK_Initialize;
		break;
	case KUSB_FNID_GetDescriptor:
		*ProcAddress = (KPROC)UsbK_GetDescriptor;
		break;
	case KUSB_FNID_QueryDeviceInformation:
		*ProcAddress = (KPROC)UsbK_QueryDeviceInformation;
		break;
	case KUSB_FNID_SetCurrentAlternateSetting:
		*ProcAddress = (KPROC)UsbK_SetCurrentAlternateSetting;
		break;
	case KUSB_FNID_GetCurrentAlternateSetting:
		*ProcAddress = (KPROC)UsbK_GetCurrentAlternateSetting;
		break;
	case KUSB_FNID_SetPipePolicy:
		*ProcAddress = (KPROC)UsbK_SetPipePolicy;
		break;
	case KUSB_FNID_GetPipePolicy:
		*ProcAddress = (KPROC)UsbK_GetPipePolicy;
		break;
	case KUSB_FNID_ReadPipe:
		*ProcAddress = (KPROC)UsbK_ReadPipe;
		break;
	case KUSB_FNID_WritePipe:
		*ProcAddress = (KPROC)UsbK_WritePipe;
		break;
	case KUSB_FNID_ControlTransfer:
		*ProcAddress = (KPROC)UsbK_ControlTransfer;
		break;
	case KUSB_FNID_ResetPipe:
		*ProcAddress = (KPROC)UsbK_ResetPipe;
		break;
	case KUSB_FNID_AbortPipe:
		*ProcAddress = (KPROC)UsbK_AbortPipe;
		break;
	case KUSB_FNID_FlushPipe:
		*ProcAddress = (KPROC)UsbK_FlushPipe;
		break;
	case KUSB_FNID_SetPowerPolicy:
		*ProcAddress = (KPROC)UsbK_SetPowerPolicy;
		break;
	case KUSB_FNID_GetPowerPolicy:
		*ProcAddress = (KPROC)UsbK_GetPowerPolicy;
		break;
	case KUSB_FNID_GetOverlappedResult:
		*ProcAddress = (KPROC)UsbK_GetOverlappedResult;
		break;
	case KUSB_FNID_ResetDevice:
		*ProcAddress = (KPROC)UsbK_ResetDevice;
		break;
	case KUSB_FNID_Init:
		*ProcAddress = (KPROC)UsbK_Init;
		break;
	case KUSB_FNID_SetConfiguration:
		*ProcAddress = (KPROC)UsbK_SetConfiguration;
		break;
	case KUSB_FNID_GetConfiguration:
		*ProcAddress = (KPROC)UsbK_GetConfiguration;
		break;
	case KUSB_FNID_ClaimInterface:
		*ProcAddress = (KPROC)UsbK_ClaimInterface;
		break;
	case KUSB_FNID_ReleaseInterface:
		*ProcAddress = (KPROC)UsbK_ReleaseInterface;
		break;
	case KUSB_FNID_SetAltInterface:
		*ProcAddress = (KPROC)UsbK_SetAltInterface;
		break;
	case KUSB_FNID_GetAltInterface:
		*ProcAddress = (KPROC)UsbK_GetAltInterface;
		break;
	case KUSB_FNID_IsoReadPipe:
		*ProcAddress = (KPROC)UsbK_IsoReadPipe;
		break;
	case KUSB_FNID_IsoWritePipe:
		*ProcAddress = (KPROC)UsbK_IsoWritePipe;
		break;
	case KUSB_FNID_GetCurrentFrameNumber:
		*ProcAddress = (KPROC)UsbK_GetCurrentFrameNumber;
		break;
	default:
		return FALSE;

	}
	return TRUE;
}

BOOL GetProcAddress_LUsb0(__out KPROC* ProcAddress, __in LONG FunctionID)
{
	switch(FunctionID)
	{
	case KUSB_FNID_IsoReadPipe:
		GetProcAddress_Unsupported(ProcAddress, FunctionID);
		return LusbwError(ERROR_NOT_SUPPORTED);
	case KUSB_FNID_IsoWritePipe:
		GetProcAddress_Unsupported(ProcAddress, FunctionID);
		return LusbwError(ERROR_NOT_SUPPORTED);
	default:
		return GetProcAddress_UsbK(ProcAddress, FunctionID);
	}
}

KUSB_EXP BOOL KUSB_API LibK_GetProcAddress(
    _out KPROC* ProcAddress,
    _in LONG DriverID,
    _in LONG FunctionID)
{
	CheckLibInit();

	*ProcAddress = (KPROC)NULL;

	if (GetProcAddress_Base(ProcAddress, FunctionID))
		return TRUE;

	switch(DriverID)
	{
	case KUSB_DRVID_LIBUSBK:
		if (GetProcAddress_UsbK(ProcAddress, FunctionID)) return TRUE;
		break;

	case KUSB_DRVID_LIBUSB0_FILTER:
	case KUSB_DRVID_LIBUSB0:
		if (GetProcAddress_LUsb0(ProcAddress, FunctionID)) return TRUE;
		break;

	case KUSB_DRVID_WINUSB:
		if (GetProcAddress_WUsb(ProcAddress, FunctionID)) return TRUE;
		break;
	}

	GetProcAddress_Unsupported(ProcAddress, FunctionID);
	return LusbwError(ERROR_NOT_SUPPORTED);
}

KUSB_EXP BOOL KUSB_API LibK_CopyDriverAPI(
    _out PKUSB_DRIVER_API DriverAPI,
    _in KUSB_HANDLE UsbHandle)
{
	PKUSB_HANDLE_INTERNAL handle;

	Pub_To_Priv_UsbK(UsbHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	memcpy(DriverAPI, handle->Device->DriverAPI, sizeof(*DriverAPI));

	PoolHandle_Dec_UsbK(handle);
	return TRUE;
}

KUSB_EXP BOOL KUSB_API LibK_LoadDriverAPI(
    _out PKUSB_DRIVER_API DriverAPI,
    _in LONG DriverID)
{
	int fnIdIndex;
	int fnIdCount = 0;

	CheckLibInit();

	if (!IsHandleValid(DriverAPI))
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	Mem_Zero(DriverAPI, sizeof(*DriverAPI));

	for (fnIdIndex = 0; fnIdIndex < KUSB_FNID_COUNT; fnIdIndex++)
	{
		switch(fnIdIndex)
		{
			CASE_FNID_LOAD(Init);
			CASE_FNID_LOAD(Free);
			CASE_FNID_LOAD(ClaimInterface);
			CASE_FNID_LOAD(ReleaseInterface);
			CASE_FNID_LOAD(SetAltInterface);
			CASE_FNID_LOAD(GetAltInterface);
			CASE_FNID_LOAD(GetDescriptor);
			CASE_FNID_LOAD(ControlTransfer);
			CASE_FNID_LOAD(SetPowerPolicy);
			CASE_FNID_LOAD(GetPowerPolicy);
			CASE_FNID_LOAD(SetConfiguration);
			CASE_FNID_LOAD(GetConfiguration);
			CASE_FNID_LOAD(ResetDevice);
			CASE_FNID_LOAD(Initialize);
			CASE_FNID_LOAD(SelectInterface);
			CASE_FNID_LOAD(GetAssociatedInterface);
			CASE_FNID_LOAD(Clone);
			CASE_FNID_LOAD(QueryInterfaceSettings);
			CASE_FNID_LOAD(QueryDeviceInformation);
			CASE_FNID_LOAD(SetCurrentAlternateSetting);
			CASE_FNID_LOAD(GetCurrentAlternateSetting);
			CASE_FNID_LOAD(QueryPipe);
			CASE_FNID_LOAD(SetPipePolicy);
			CASE_FNID_LOAD(GetPipePolicy);
			CASE_FNID_LOAD(ReadPipe);
			CASE_FNID_LOAD(WritePipe);
			CASE_FNID_LOAD(ResetPipe);
			CASE_FNID_LOAD(AbortPipe);
			CASE_FNID_LOAD(FlushPipe);
			CASE_FNID_LOAD(IsoReadPipe);
			CASE_FNID_LOAD(IsoWritePipe);
			CASE_FNID_LOAD(GetCurrentFrameNumber);
			CASE_FNID_LOAD(GetOverlappedResult);
			CASE_FNID_LOAD(GetProperty);

		default:
			USBERRN("undeclared api function %u!", fnIdIndex);
			return LusbwError(ERROR_NOT_SUPPORTED);
		}
	}

	DriverAPI->Info.FunctionCount = fnIdCount;
	DriverAPI->Info.DriverID = (KUSB_DRVID)DriverID;

	return fnIdCount > 0 ? (BOOL)fnIdCount : FALSE;
}

static PKOBJ_BASE Lib_GetBase(KLIB_HANDLE Handle, KLIB_HANDLE_TYPE HandleType)
{
	PKOBJ_BASE base = NULL;
	switch (HandleType)
	{
	case KLIB_HANDLE_TYPE_HOTK:
		base = (PKOBJ_BASE)Handle;
		break;

	case KLIB_HANDLE_TYPE_USBK:
		base = (PKOBJ_BASE)Handle;
		break;

	case KLIB_HANDLE_TYPE_USBSHAREDK:
		base = (PKOBJ_BASE) & (((PKUSB_HANDLE_INTERNAL)Handle)->Device->Base);
		break;

	case KLIB_HANDLE_TYPE_LSTK:
		base = (PKOBJ_BASE)Handle;
		break;

	case KLIB_HANDLE_TYPE_LSTINFOK:
		base = (PKOBJ_BASE) & (((PKLST_DEVINFO_EL)Handle)->DevInfoHandle->Base);
		break;

	case KLIB_HANDLE_TYPE_OVLK:
		base = (PKOBJ_BASE) & (((PKOVL_HANDLE_INTERNAL)Handle)->Base);
		break;

	case KLIB_HANDLE_TYPE_OVLPOOLK:
		base = (PKOBJ_BASE)Handle;
		break;

	case KLIB_HANDLE_TYPE_STMK:
		base = (PKOBJ_BASE)Handle;
		break;
	}
	return base;
}

KUSB_EXP KLIB_USER_CONTEXT KUSB_API LibK_GetContext(
    _in KLIB_HANDLE Handle,
    _in KLIB_HANDLE_TYPE HandleType)
{
	PKOBJ_BASE base = Lib_GetBase(Handle, HandleType);

	if (base != NULL && base->Count.Ref >= 1 && base->Count.Use == SPINLOCK_HELD)
		return base->User.Context;

	LusbwError(ERROR_INVALID_HANDLE);
	return 0;
}

KUSB_EXP BOOL KUSB_API LibK_SetContext(
    _in KLIB_HANDLE Handle,
    _in KLIB_HANDLE_TYPE HandleType,
    _in KLIB_USER_CONTEXT ContextValue)
{
	PKOBJ_BASE base = Lib_GetBase(Handle, HandleType);

	if (base != NULL && base->Count.Ref >= 1 && base->Count.Use == SPINLOCK_HELD)
	{
		base->User.Context = ContextValue;
		return TRUE;
	}
	LusbwError(ERROR_INVALID_HANDLE);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API LibK_SetCleanupCallback(
    _in KLIB_HANDLE Handle,
    _in KLIB_HANDLE_TYPE HandleType,
    _in KLIB_HANDLE_CLEANUP_CB* CleanupCB)
{
	PKOBJ_BASE base = Lib_GetBase(Handle, HandleType);

	if (base != NULL && base->Count.Ref >= 1 && base->Count.Use == SPINLOCK_HELD)
	{
		base->User.CleanupCB = CleanupCB;
		return TRUE;
	}
	LusbwError(ERROR_INVALID_HANDLE);
	return FALSE;
}

KUSB_EXP VOID KUSB_API LibK_GetVersion(
    _out PKLIB_VERSION Version)
{
	ErrorParamAction(!IsHandleValid(Version), "Version", return);
	Version->Major = VERSION_MAJOR;
	Version->Minor = VERSION_MINOR;
	Version->Micro = VERSION_MICRO;
	Version->Nano = VERSION_NANO;
}