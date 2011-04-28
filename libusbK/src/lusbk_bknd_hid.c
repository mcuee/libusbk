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

KUSB_EXP BOOL KUSB_API Hid_Initialize (
    __in HANDLE DeviceHandle,
    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	UNREFERENCED_PARAMETER(DeviceHandle);
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_Free (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_GetAssociatedInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AssociatedInterfaceIndex,
    __out PLIBUSBK_INTERFACE_HANDLE AssociatedInterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AssociatedInterfaceIndex);
	UNREFERENCED_PARAMETER(AssociatedInterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_GetDescriptor (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR DescriptorType,
    __in UCHAR Index,
    __in USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out PULONG LengthTransferred)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(DescriptorType);
	UNREFERENCED_PARAMETER(Index);
	UNREFERENCED_PARAMETER(LanguageID);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(LengthTransferred);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_QueryInterfaceSettings (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AlternateSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AlternateSettingNumber);
	UNREFERENCED_PARAMETER(UsbAltInterfaceDescriptor);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_QueryDeviceInformation (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG InformationType,
    __inout PULONG BufferLength,
    __out PVOID Buffer)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(InformationType);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(Buffer);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_SetCurrentAlternateSetting (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR SettingNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(SettingNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_GetCurrentAlternateSetting (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR SettingNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(SettingNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_QueryPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AlternateSettingNumber,
    __in UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AlternateSettingNumber);
	UNREFERENCED_PARAMETER(PipeIndex);
	UNREFERENCED_PARAMETER(PipeInformation);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_SetPipePolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);
	UNREFERENCED_PARAMETER(PolicyType);
	UNREFERENCED_PARAMETER(ValueLength);
	UNREFERENCED_PARAMETER(Value);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_GetPipePolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);
	UNREFERENCED_PARAMETER(PolicyType);
	UNREFERENCED_PARAMETER(ValueLength);
	UNREFERENCED_PARAMETER(Value);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_ReadPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(LengthTransferred);
	UNREFERENCED_PARAMETER(Overlapped);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_WritePipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(LengthTransferred);
	UNREFERENCED_PARAMETER(Overlapped);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_ControlTransfer (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in WINUSB_SETUP_PACKET SetupPacket,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(SetupPacket);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(LengthTransferred);
	UNREFERENCED_PARAMETER(Overlapped);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_ResetPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_AbortPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_FlushPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_SetPowerPolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PolicyType);
	UNREFERENCED_PARAMETER(ValueLength);
	UNREFERENCED_PARAMETER(Value);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_GetPowerPolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PolicyType);
	UNREFERENCED_PARAMETER(ValueLength);
	UNREFERENCED_PARAMETER(Value);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_GetOverlappedResult (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in BOOL bWait)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(lpOverlapped);
	UNREFERENCED_PARAMETER(lpNumberOfBytesTransferred);
	UNREFERENCED_PARAMETER(bWait);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_ResetDevice (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_Open (
    __in PKUSB_DEV_LIST DeviceListItem,
    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	UNREFERENCED_PARAMETER(DeviceListItem);
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_Close (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_SetConfiguration (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR ConfigurationNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(ConfigurationNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_GetConfiguration (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR ConfigurationNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(ConfigurationNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_ClaimInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(InterfaceNumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_ReleaseInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(InterfaceNumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_SetAltInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex,
    __in UCHAR AltInterfaceNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(InterfaceNumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);
	UNREFERENCED_PARAMETER(AltInterfaceNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_GetAltInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex,
    __out PUCHAR AltInterfaceNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(InterfaceNumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);
	UNREFERENCED_PARAMETER(AltInterfaceNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_IsoReadPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __in ULONG IsoPacketSize,
    __in LPOVERLAPPED Overlapped)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(IsoPacketSize);
	UNREFERENCED_PARAMETER(Overlapped);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Hid_IsoWritePipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __in ULONG IsoPacketSize,
    __in LPOVERLAPPED Overlapped)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(IsoPacketSize);
	UNREFERENCED_PARAMETER(Overlapped);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}


BOOL GetProcAddress_Hid(__out KPROC* ProcAddress, __in ULONG FunctionID)
{
	DWORD rtn = ERROR_SUCCESS;

	switch(FunctionID)
	{
	case KUSB_FNID_Initialize:
		*ProcAddress = (KPROC)Hid_Initialize;
		break;
	case KUSB_FNID_Free:
		*ProcAddress = (KPROC)Hid_Free;
		break;
	case KUSB_FNID_GetAssociatedInterface:
		*ProcAddress = (KPROC)Hid_GetAssociatedInterface;
		break;
	case KUSB_FNID_GetDescriptor:
		*ProcAddress = (KPROC)Hid_GetDescriptor;
		break;
	case KUSB_FNID_QueryInterfaceSettings:
		*ProcAddress = (KPROC)Hid_QueryInterfaceSettings;
		break;
	case KUSB_FNID_QueryDeviceInformation:
		*ProcAddress = (KPROC)Hid_QueryDeviceInformation;
		break;
	case KUSB_FNID_SetCurrentAlternateSetting:
		*ProcAddress = (KPROC)Hid_SetCurrentAlternateSetting;
		break;
	case KUSB_FNID_GetCurrentAlternateSetting:
		*ProcAddress = (KPROC)Hid_GetCurrentAlternateSetting;
		break;
	case KUSB_FNID_QueryPipe:
		*ProcAddress = (KPROC)Hid_QueryPipe;
		break;
	case KUSB_FNID_SetPipePolicy:
		*ProcAddress = (KPROC)Hid_SetPipePolicy;
		break;
	case KUSB_FNID_GetPipePolicy:
		*ProcAddress = (KPROC)Hid_GetPipePolicy;
		break;
	case KUSB_FNID_ReadPipe:
		*ProcAddress = (KPROC)Hid_ReadPipe;
		break;
	case KUSB_FNID_WritePipe:
		*ProcAddress = (KPROC)Hid_WritePipe;
		break;
	case KUSB_FNID_ControlTransfer:
		*ProcAddress = (KPROC)Hid_ControlTransfer;
		break;
	case KUSB_FNID_ResetPipe:
		*ProcAddress = (KPROC)Hid_ResetPipe;
		break;
	case KUSB_FNID_AbortPipe:
		*ProcAddress = (KPROC)Hid_AbortPipe;
		break;
	case KUSB_FNID_FlushPipe:
		*ProcAddress = (KPROC)Hid_FlushPipe;
		break;
	case KUSB_FNID_SetPowerPolicy:
		*ProcAddress = (KPROC)Hid_SetPowerPolicy;
		break;
	case KUSB_FNID_GetPowerPolicy:
		*ProcAddress = (KPROC)Hid_GetPowerPolicy;
		break;
	case KUSB_FNID_GetOverlappedResult:
		*ProcAddress = (KPROC)Hid_GetOverlappedResult;
		break;
	case KUSB_FNID_ResetDevice:
		*ProcAddress = (KPROC)Hid_ResetDevice;
		break;
	case KUSB_FNID_Open:
		*ProcAddress = (KPROC)Hid_Open;
		break;
	case KUSB_FNID_Close:
		*ProcAddress = (KPROC)Hid_Close;
		break;
	case KUSB_FNID_SetConfiguration:
		*ProcAddress = (KPROC)Hid_SetConfiguration;
		break;
	case KUSB_FNID_GetConfiguration:
		*ProcAddress = (KPROC)Hid_GetConfiguration;
		break;
	case KUSB_FNID_ClaimInterface:
		*ProcAddress = (KPROC)Hid_ClaimInterface;
		break;
	case KUSB_FNID_ReleaseInterface:
		*ProcAddress = (KPROC)Hid_ReleaseInterface;
		break;
	case KUSB_FNID_SetAltInterface:
		*ProcAddress = (KPROC)Hid_SetAltInterface;
		break;
	case KUSB_FNID_GetAltInterface:
		*ProcAddress = (KPROC)Hid_GetAltInterface;
		break;
	case KUSB_FNID_IsoReadPipe:
		*ProcAddress = (KPROC)Hid_IsoReadPipe;
		break;
	case KUSB_FNID_IsoWritePipe:
		*ProcAddress = (KPROC)Hid_IsoWritePipe;
		break;


	default:
		rtn = ERROR_NOT_SUPPORTED;
		*ProcAddress = (KPROC)NULL;
		USBERR("Unrecognized function id! FunctionID=%u\n", FunctionID);
		break;

	}
	return LusbwError(rtn);
}
