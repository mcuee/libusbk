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

#include "lusbk_bknd_unsupported.h"

BOOL KUSB_API Unsupported_Initialize (
    __in HANDLE DeviceHandle,
    __out KUSB_HANDLE* InterfaceHandle)
{
	UNREFERENCED_PARAMETER(DeviceHandle);
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_Free (
    __in KUSB_HANDLE InterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_GetAssociatedInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AssociatedInterfaceIndex,
    __out KUSB_HANDLE* AssociatedInterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AssociatedInterfaceIndex);
	UNREFERENCED_PARAMETER(AssociatedInterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_Clone (
    __in KUSB_HANDLE InterfaceHandle,
    __out KUSB_HANDLE* DstInterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(DstInterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_GetDescriptor (
    __in KUSB_HANDLE InterfaceHandle,
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

BOOL KUSB_API Unsupported_QueryInterfaceSettings (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AltSettingNumber);
	UNREFERENCED_PARAMETER(UsbAltInterfaceDescriptor);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_QueryDeviceInformation (
    __in KUSB_HANDLE InterfaceHandle,
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

BOOL KUSB_API Unsupported_SetCurrentAlternateSetting (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AltSettingNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_GetCurrentAlternateSetting (
    __in KUSB_HANDLE InterfaceHandle,
    __out PUCHAR AltSettingNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AltSettingNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_QueryPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber,
    __in UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AltSettingNumber);
	UNREFERENCED_PARAMETER(PipeIndex);
	UNREFERENCED_PARAMETER(PipeInformation);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_SetPipePolicy (
    __in KUSB_HANDLE InterfaceHandle,
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

BOOL KUSB_API Unsupported_GetPipePolicy (
    __in KUSB_HANDLE InterfaceHandle,
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

BOOL KUSB_API Unsupported_ReadPipe (
    __in KUSB_HANDLE InterfaceHandle,
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

BOOL KUSB_API Unsupported_WritePipe (
    __in KUSB_HANDLE InterfaceHandle,
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

BOOL KUSB_API Unsupported_ControlTransfer (
    __in KUSB_HANDLE InterfaceHandle,
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

BOOL KUSB_API Unsupported_ResetPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_AbortPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_FlushPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_SetPowerPolicy (
    __in KUSB_HANDLE InterfaceHandle,
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

BOOL KUSB_API Unsupported_GetPowerPolicy (
    __in KUSB_HANDLE InterfaceHandle,
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

BOOL KUSB_API Unsupported_GetOverlappedResult (
    __in KUSB_HANDLE InterfaceHandle,
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

BOOL KUSB_API Unsupported_ResetDevice (
    __in KUSB_HANDLE InterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_Open (
    __in KLST_DEVINFO* DevInfo,
    __out KUSB_HANDLE* InterfaceHandle)
{
	UNREFERENCED_PARAMETER(DevInfo);
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_Close (
    __in KUSB_HANDLE InterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_SetConfiguration (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR ConfigurationNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(ConfigurationNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_GetConfiguration (
    __in KUSB_HANDLE InterfaceHandle,
    __out PUCHAR ConfigurationNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(ConfigurationNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_ClaimInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(NumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_SelectInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(NumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_ReleaseInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(NumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_SetAltInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex,
    __in UCHAR AltSettingNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(NumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);
	UNREFERENCED_PARAMETER(AltSettingNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_GetAltInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex,
    __out PUCHAR AltSettingNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(NumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);
	UNREFERENCED_PARAMETER(AltSettingNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_IsoReadPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __inout PKISO_CONTEXT IsoContext,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __in LPOVERLAPPED Overlapped)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(IsoContext);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(Overlapped);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_IsoWritePipe (
    __in KUSB_HANDLE InterfaceHandle,
    __inout PKISO_CONTEXT IsoContext,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __in LPOVERLAPPED Overlapped)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(IsoContext);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(Overlapped);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

BOOL KUSB_API Unsupported_GetCurrentFrameNumber (
    __in KUSB_HANDLE InterfaceHandle,
    __out PULONG FrameNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(FrameNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}


BOOL GetProcAddress_Unsupported(__out KPROC* ProcAddress, __in ULONG FunctionID)
{
	DWORD rtn = ERROR_SUCCESS;

	switch(FunctionID)
	{
	case KUSB_FNID_Initialize:
		*ProcAddress = (KPROC)Unsupported_Initialize;
		break;
	case KUSB_FNID_Free:
		*ProcAddress = (KPROC)Unsupported_Free;
		break;
	case KUSB_FNID_GetAssociatedInterface:
		*ProcAddress = (KPROC)Unsupported_GetAssociatedInterface;
		break;
	case KUSB_FNID_GetDescriptor:
		*ProcAddress = (KPROC)Unsupported_GetDescriptor;
		break;
	case KUSB_FNID_QueryInterfaceSettings:
		*ProcAddress = (KPROC)Unsupported_QueryInterfaceSettings;
		break;
	case KUSB_FNID_QueryDeviceInformation:
		*ProcAddress = (KPROC)Unsupported_QueryDeviceInformation;
		break;
	case KUSB_FNID_SetCurrentAlternateSetting:
		*ProcAddress = (KPROC)Unsupported_SetCurrentAlternateSetting;
		break;
	case KUSB_FNID_GetCurrentAlternateSetting:
		*ProcAddress = (KPROC)Unsupported_GetCurrentAlternateSetting;
		break;
	case KUSB_FNID_QueryPipe:
		*ProcAddress = (KPROC)Unsupported_QueryPipe;
		break;
	case KUSB_FNID_SetPipePolicy:
		*ProcAddress = (KPROC)Unsupported_SetPipePolicy;
		break;
	case KUSB_FNID_GetPipePolicy:
		*ProcAddress = (KPROC)Unsupported_GetPipePolicy;
		break;
	case KUSB_FNID_ReadPipe:
		*ProcAddress = (KPROC)Unsupported_ReadPipe;
		break;
	case KUSB_FNID_WritePipe:
		*ProcAddress = (KPROC)Unsupported_WritePipe;
		break;
	case KUSB_FNID_ControlTransfer:
		*ProcAddress = (KPROC)Unsupported_ControlTransfer;
		break;
	case KUSB_FNID_ResetPipe:
		*ProcAddress = (KPROC)Unsupported_ResetPipe;
		break;
	case KUSB_FNID_AbortPipe:
		*ProcAddress = (KPROC)Unsupported_AbortPipe;
		break;
	case KUSB_FNID_FlushPipe:
		*ProcAddress = (KPROC)Unsupported_FlushPipe;
		break;
	case KUSB_FNID_SetPowerPolicy:
		*ProcAddress = (KPROC)Unsupported_SetPowerPolicy;
		break;
	case KUSB_FNID_GetPowerPolicy:
		*ProcAddress = (KPROC)Unsupported_GetPowerPolicy;
		break;
	case KUSB_FNID_GetOverlappedResult:
		*ProcAddress = (KPROC)Unsupported_GetOverlappedResult;
		break;
	case KUSB_FNID_ResetDevice:
		*ProcAddress = (KPROC)Unsupported_ResetDevice;
		break;
	case KUSB_FNID_Open:
		*ProcAddress = (KPROC)Unsupported_Open;
		break;
	case KUSB_FNID_Close:
		*ProcAddress = (KPROC)Unsupported_Close;
		break;
	case KUSB_FNID_SetConfiguration:
		*ProcAddress = (KPROC)Unsupported_SetConfiguration;
		break;
	case KUSB_FNID_GetConfiguration:
		*ProcAddress = (KPROC)Unsupported_GetConfiguration;
		break;
	case KUSB_FNID_ClaimInterface:
		*ProcAddress = (KPROC)Unsupported_ClaimInterface;
		break;
	case KUSB_FNID_ReleaseInterface:
		*ProcAddress = (KPROC)Unsupported_ReleaseInterface;
		break;
	case KUSB_FNID_SetAltInterface:
		*ProcAddress = (KPROC)Unsupported_SetAltInterface;
		break;
	case KUSB_FNID_GetAltInterface:
		*ProcAddress = (KPROC)Unsupported_GetAltInterface;
		break;
	case KUSB_FNID_IsoReadPipe:
		*ProcAddress = (KPROC)Unsupported_IsoReadPipe;
		break;
	case KUSB_FNID_IsoWritePipe:
		*ProcAddress = (KPROC)Unsupported_IsoWritePipe;
		break;
	case KUSB_FNID_GetCurrentFrameNumber:
		*ProcAddress = (KPROC)Unsupported_GetCurrentFrameNumber;
		break;
	case KUSB_FNID_Clone:
		*ProcAddress = (KPROC)Unsupported_Clone;
		break;
	case KUSB_FNID_SelectInterface:
		*ProcAddress = (KPROC)Unsupported_SelectInterface;
		break;


	default:
		rtn = ERROR_NOT_SUPPORTED;
		*ProcAddress = (KPROC)NULL;
		USBERRN("Unrecognized function id! FunctionID=%u", FunctionID);
		break;

	}
	return LusbwError(rtn);
}
