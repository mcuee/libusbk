/*!********************************************************************
libusbK - Multi-driver USB library.
Copyright (C) 2011 All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen     (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/

#include "lusbk_bknd.h"

// Default loggging level
ULONG DebugLevel = 4;

KUSB_EXP BOOL KUSB_API DrvK_GetProcAddress(__out KPROC* ProcAddress, __in ULONG DriverID, __in ULONG FunctionID)
{
	CheckLibInitialized();

	switch(DriverID)
	{
	case KUSB_DRVID_LIBUSBK:
		return GetProcAddress_UsbK(ProcAddress, FunctionID);
	case KUSB_DRVID_LIBUSB0_FILTER:
	case KUSB_DRVID_LIBUSB0:
		return GetProcAddress_LUsb0(ProcAddress, FunctionID);
	case KUSB_DRVID_WINUSB:
		return GetProcAddress_WUsb(ProcAddress, FunctionID);
	}

	return LusbwError(ERROR_NOT_SUPPORTED);
}


KUSB_EXP BOOL KUSB_API DrvK_LoadDriverApi(
    __inout PKUSB_DRIVER_API DriverAPI,
    __in ULONG DriverID)
{
#define CASE_FNID_LOAD(FunctionName)															\
		case KUSB_FNID_##FunctionName:																\
		if (DrvK_GetProcAddress((KPROC*)&DriverAPI->FunctionName, DriverID, fnIdIndex) == FALSE)	\
		{																							\
			USBWRN("function id %u for driver id %u does not exist.\n",fnIdIndex,DriverID);			\
		}																							\
		break

	int fnIdIndex;
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

		default:
			USBERR("undeclared api function %u!\n", fnIdIndex);
			return LusbwError(ERROR_NOT_SUPPORTED);
		}
	}

	return TRUE;
}

