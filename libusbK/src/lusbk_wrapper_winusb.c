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

#ifndef EXCLUDE_WINUSB_WRAPPER

#include "lusbk_private.h"

KUSB_EXP BOOL KUSB_API WinUsb_Initialize (
    __in HANDLE DeviceHandle,
    __out KUSB_HANDLE* InterfaceHandle)
{
	return UsbK_Initialize (DeviceHandle, InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_Free (
    __in KUSB_HANDLE InterfaceHandle)
{
	return UsbK_Free (InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetAssociatedInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AssociatedInterfaceIndex,
    __out KUSB_HANDLE* AssociatedInterfaceHandle)
{
	return UsbK_GetAssociatedInterface (InterfaceHandle, AssociatedInterfaceIndex, AssociatedInterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetDescriptor (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR DescriptorType,
    __in UCHAR Index,
    __in USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out PULONG LengthTransferred)
{
	return UsbK_GetDescriptor (InterfaceHandle, DescriptorType, Index, LanguageID, Buffer, BufferLength, LengthTransferred);
}

KUSB_EXP BOOL KUSB_API WinUsb_QueryInterfaceSettings (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	return UsbK_QueryInterfaceSettings (InterfaceHandle, AltSettingNumber, UsbAltInterfaceDescriptor);
}

KUSB_EXP BOOL KUSB_API WinUsb_QueryDeviceInformation (
    __in KUSB_HANDLE InterfaceHandle,
    __in ULONG InformationType,
    __inout PULONG BufferLength,
    __out PVOID Buffer)
{
	return UsbK_QueryDeviceInformation (InterfaceHandle, InformationType, BufferLength, Buffer);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetCurrentAlternateSetting (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber)
{
	return UsbK_SetCurrentAlternateSetting (InterfaceHandle, AltSettingNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetCurrentAlternateSetting (
    __in KUSB_HANDLE InterfaceHandle,
    __out PUCHAR AltSettingNumber)
{
	return UsbK_GetCurrentAlternateSetting (InterfaceHandle, AltSettingNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_QueryPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber,
    __in UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	return UsbK_QueryPipe (InterfaceHandle, AltSettingNumber, PipeIndex, PipeInformation);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetPipePolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value)
{
	return UsbK_SetPipePolicy (InterfaceHandle, PipeID, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetPipePolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	return UsbK_GetPipePolicy (InterfaceHandle, PipeID, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_ReadPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	return UsbK_ReadPipe (InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_WritePipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	return UsbK_WritePipe (InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_ControlTransfer (
    __in KUSB_HANDLE InterfaceHandle,
    __in WINUSB_SETUP_PACKET SetupPacket,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	return UsbK_ControlTransfer (InterfaceHandle, SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_ResetPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	return UsbK_ResetPipe (InterfaceHandle, PipeID);
}

KUSB_EXP BOOL KUSB_API WinUsb_AbortPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	return UsbK_AbortPipe (InterfaceHandle, PipeID);
}

KUSB_EXP BOOL KUSB_API WinUsb_FlushPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	return UsbK_FlushPipe (InterfaceHandle, PipeID);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetPowerPolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value)
{
	return UsbK_SetPowerPolicy (InterfaceHandle, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetPowerPolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	return UsbK_GetPowerPolicy (InterfaceHandle, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetOverlappedResult (
    __in KUSB_HANDLE InterfaceHandle,
    __in LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in BOOL bWait)
{
	return UsbK_GetOverlappedResult (InterfaceHandle, lpOverlapped, lpNumberOfBytesTransferred, bWait);
}

KUSB_EXP BOOL KUSB_API WinUsb_ResetDevice (
    __in KUSB_HANDLE InterfaceHandle)
{
	return UsbK_ResetDevice (InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_Open (
    __in KLST_DEVINFO* DevInfo,
    __out KUSB_HANDLE* InterfaceHandle)
{
	return UsbK_Open (DevInfo, InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_Close (
    __in KUSB_HANDLE InterfaceHandle)
{
	return UsbK_Close (InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetConfiguration (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR ConfigurationNumber)
{
	return UsbK_SetConfiguration (InterfaceHandle, ConfigurationNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetConfiguration (
    __in KUSB_HANDLE InterfaceHandle,
    __out PUCHAR ConfigurationNumber)
{
	return UsbK_GetConfiguration (InterfaceHandle, ConfigurationNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_ClaimInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex)
{
	return UsbK_ClaimInterface (InterfaceHandle, NumberOrIndex, IsIndex);
}

KUSB_EXP BOOL KUSB_API WinUsb_ReleaseInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex)
{
	return UsbK_ReleaseInterface (InterfaceHandle, NumberOrIndex, IsIndex);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetAltInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex,
    __in UCHAR AltSettingNumber)
{
	return UsbK_SetAltInterface (InterfaceHandle, NumberOrIndex, IsIndex, AltSettingNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetAltInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex,
    __out PUCHAR AltSettingNumber)
{
	return UsbK_GetAltInterface (InterfaceHandle, NumberOrIndex, IsIndex, AltSettingNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_IsoReadPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __inout PKISO_CONTEXT IsoContext,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __in LPOVERLAPPED Overlapped)
{
	return UsbK_IsoReadPipe (InterfaceHandle, IsoContext, Buffer, BufferLength, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_IsoWritePipe (
    __in KUSB_HANDLE InterfaceHandle,
    __inout PKISO_CONTEXT IsoContext,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __in LPOVERLAPPED Overlapped)
{
	return UsbK_IsoWritePipe (InterfaceHandle, IsoContext, Buffer, BufferLength, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetCurrentFrameNumber (
    __in KUSB_HANDLE InterfaceHandle,
    __out PULONG FrameNumber)
{
	return UsbK_GetCurrentFrameNumber (InterfaceHandle, FrameNumber);
}


KUSB_EXP BOOL KUSB_API WinUsb_Clone (
    __in KUSB_HANDLE InterfaceHandle,
    __out KUSB_HANDLE* DstInterfaceHandle)
{
	return UsbK_Clone (InterfaceHandle, DstInterfaceHandle);
}


KUSB_EXP BOOL KUSB_API WinUsb_SelectInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex)
{
	return UsbK_SelectInterface (InterfaceHandle, NumberOrIndex, IsIndex);
}

#endif // EXCLUDE_WINUSB_WRAPPER
