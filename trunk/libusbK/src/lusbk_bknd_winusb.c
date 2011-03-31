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

extern HMODULE winusb_dll;

BOOL GetProcAddress_winusb(__out KPROC* ProcAddress, __in ULONG FunctionID)
{
	HMODULE dll = winusb_dll;

	if (dll == NULL)
		return LusbwError(ERROR_DLL_NOT_FOUND);

	switch(FunctionID)
	{
	case KUSB_FNID_Initialize:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_Initialize")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_Free:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_Free")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_GetAssociatedInterface:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_GetAssociatedInterface")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_GetDescriptor:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_GetDescriptor")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_QueryInterfaceSettings:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_QueryInterfaceSettings")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_QueryDeviceInformation:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_QueryDeviceInformation")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_SetCurrentAlternateSetting:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_SetCurrentAlternateSetting")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_GetCurrentAlternateSetting:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_GetCurrentAlternateSetting")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_QueryPipe:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_QueryPipe")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_SetPipePolicy:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_SetPipePolicy")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_GetPipePolicy:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_GetPipePolicy")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_ReadPipe:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_ReadPipe")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_WritePipe:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_WritePipe")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_ControlTransfer:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_ControlTransfer")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_ResetPipe:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_ResetPipe")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_AbortPipe:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_AbortPipe")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_FlushPipe:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_FlushPipe")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_SetPowerPolicy:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_SetPowerPolicy")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_GetPowerPolicy:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_GetPowerPolicy")) == NULL)
			return FALSE;
		break;

	case KUSB_FNID_GetOverlappedResult:
		if ((*ProcAddress = GetProcAddress(dll, "WinUsb_GetOverlappedResult")) == NULL)
			return FALSE;
		break;

	default:
		return LusbwError(ERROR_NOT_SUPPORTED);
	}

	return TRUE;
}
