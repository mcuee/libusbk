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

KUSB_EXP BOOL KUSB_API Unsupported_Initialize(
    _in HANDLE DeviceHandle,
    _out KUSB_HANDLE* InterfaceHandle)
{
	UNREFERENCED_PARAMETER(DeviceHandle);
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_GetDescriptor(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR DescriptorType,
    _in UCHAR Index,
    _in USHORT LanguageID,
    _out PUCHAR Buffer,
    _in ULONG BufferLength,
    _outopt PULONG LengthTransferred)
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

KUSB_EXP BOOL KUSB_API Unsupported_QueryDeviceInformation(
    _in KUSB_HANDLE InterfaceHandle,
    _in ULONG InformationType,
    _ref PULONG BufferLength,
    _ref PVOID Buffer)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(InformationType);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(Buffer);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_SetCurrentAlternateSetting(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AltSettingNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_GetCurrentAlternateSetting(
    _in KUSB_HANDLE InterfaceHandle,
    _out PUCHAR AltSettingNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AltSettingNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_SetPipePolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in ULONG PolicyType,
    _in ULONG ValueLength,
    _in PVOID Value)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);
	UNREFERENCED_PARAMETER(PolicyType);
	UNREFERENCED_PARAMETER(ValueLength);
	UNREFERENCED_PARAMETER(Value);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_GetPipePolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in ULONG PolicyType,
    _ref PULONG ValueLength,
    _out PVOID Value)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);
	UNREFERENCED_PARAMETER(PolicyType);
	UNREFERENCED_PARAMETER(ValueLength);
	UNREFERENCED_PARAMETER(Value);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_ReadPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _out PUCHAR Buffer,
    _in ULONG BufferLength,
    _outopt PULONG LengthTransferred,
    _inopt LPOVERLAPPED Overlapped)
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

KUSB_EXP BOOL KUSB_API Unsupported_WritePipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in PUCHAR Buffer,
    _in ULONG BufferLength,
    _outopt PULONG LengthTransferred,
    _inopt LPOVERLAPPED Overlapped)
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

KUSB_EXP BOOL KUSB_API Unsupported_ControlTransfer(
    _in KUSB_HANDLE InterfaceHandle,
    _in WINUSB_SETUP_PACKET SetupPacket,
    _refopt PUCHAR Buffer,
    _in ULONG BufferLength,
    _outopt PULONG LengthTransferred,
    _inopt LPOVERLAPPED Overlapped)
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

KUSB_EXP BOOL KUSB_API Unsupported_ResetPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_AbortPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_FlushPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_SetPowerPolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in ULONG PolicyType,
    _in ULONG ValueLength,
    _in PVOID Value)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PolicyType);
	UNREFERENCED_PARAMETER(ValueLength);
	UNREFERENCED_PARAMETER(Value);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_GetPowerPolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in ULONG PolicyType,
    _ref PULONG ValueLength,
    _out PVOID Value)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PolicyType);
	UNREFERENCED_PARAMETER(ValueLength);
	UNREFERENCED_PARAMETER(Value);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_GetOverlappedResult(
    _in KUSB_HANDLE InterfaceHandle,
    _in LPOVERLAPPED lpOverlapped,
    _out LPDWORD lpNumberOfBytesTransferred,
    _in BOOL bWait)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(lpOverlapped);
	UNREFERENCED_PARAMETER(lpNumberOfBytesTransferred);
	UNREFERENCED_PARAMETER(bWait);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_ResetDevice(
    _in KUSB_HANDLE InterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_Init(
    _out KUSB_HANDLE* InterfaceHandle,
    _in KLST_DEVINFO_HANDLE DevInfo)
{
	UNREFERENCED_PARAMETER(DevInfo);
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_SetConfiguration(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR ConfigurationNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(ConfigurationNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_GetConfiguration(
    _in KUSB_HANDLE InterfaceHandle,
    _out PUCHAR ConfigurationNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(ConfigurationNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_ClaimInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(NumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_ReleaseInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(NumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_SetAltInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex,
    _in UCHAR AltSettingNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(NumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);
	UNREFERENCED_PARAMETER(AltSettingNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_GetAltInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex,
    _out PUCHAR AltSettingNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(NumberOrIndex);
	UNREFERENCED_PARAMETER(IsIndex);
	UNREFERENCED_PARAMETER(AltSettingNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_IsoReadPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _out PUCHAR Buffer,
    _in ULONG BufferLength,
    _in LPOVERLAPPED Overlapped,
    _refopt PKISO_CONTEXT IsoContext)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);
	UNREFERENCED_PARAMETER(IsoContext);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(Overlapped);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_IsoWritePipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in PUCHAR Buffer,
    _in ULONG BufferLength,
    _in LPOVERLAPPED Overlapped,
    _refopt PKISO_CONTEXT IsoContext)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(PipeID);
	UNREFERENCED_PARAMETER(IsoContext);
	UNREFERENCED_PARAMETER(Buffer);
	UNREFERENCED_PARAMETER(BufferLength);
	UNREFERENCED_PARAMETER(Overlapped);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_GetCurrentFrameNumber(
    _in KUSB_HANDLE InterfaceHandle,
    _out PULONG FrameNumber)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(FrameNumber);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_Free(
    _in KUSB_HANDLE InterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_GetAssociatedInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AssociatedInterfaceIndex,
    _out KUSB_HANDLE* AssociatedInterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AssociatedInterfaceIndex);
	UNREFERENCED_PARAMETER(AssociatedInterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_Clone(
    _in KUSB_HANDLE InterfaceHandle,
    _out KUSB_HANDLE* DstInterfaceHandle)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(DstInterfaceHandle);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_QueryPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber,
    _in UCHAR PipeIndex,
    _out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AltSettingNumber);
	UNREFERENCED_PARAMETER(PipeIndex);
	UNREFERENCED_PARAMETER(PipeInformation);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API Unsupported_QueryInterfaceSettings(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber,
    _out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	UNREFERENCED_PARAMETER(InterfaceHandle);
	UNREFERENCED_PARAMETER(AltSettingNumber);
	UNREFERENCED_PARAMETER(UsbAltInterfaceDescriptor);

	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}


BOOL GetProcAddress_Unsupported(__out KPROC* ProcAddress, __in LONG FunctionID)
{
	switch(FunctionID)
	{
	case KUSB_FNID_Initialize:
		*ProcAddress = (KPROC)Unsupported_Initialize;
		break;
	case KUSB_FNID_GetDescriptor:
		*ProcAddress = (KPROC)Unsupported_GetDescriptor;
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
	case KUSB_FNID_Init:
		*ProcAddress = (KPROC)Unsupported_Init;
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

	default:
		*ProcAddress = (KPROC)NULL;
		USBERRN("Unrecognized function id! FunctionID=%u", FunctionID);
		break;

	}
	return LusbwError(ERROR_NOT_SUPPORTED);
}
