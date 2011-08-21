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

KUSB_EXP BOOL KUSB_API WinUsb_Initialize(
    _in HANDLE DeviceHandle,
    _out KUSB_HANDLE* InterfaceHandle)
{
	return UsbK_Initialize (DeviceHandle, InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_Free(
    _in KUSB_HANDLE InterfaceHandle)
{
	return UsbK_Free (InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetAssociatedInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AssociatedInterfaceIndex,
    _out KUSB_HANDLE* AssociatedInterfaceHandle)
{
	return UsbK_GetAssociatedInterface (InterfaceHandle, AssociatedInterfaceIndex, AssociatedInterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetDescriptor(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR DescriptorType,
    _in UCHAR Index,
    _in USHORT LanguageID,
    _out PUCHAR Buffer,
    _in ULONG BufferLength,
    _outopt PULONG LengthTransferred)
{
	return UsbK_GetDescriptor (InterfaceHandle, DescriptorType, Index, LanguageID, Buffer, BufferLength, LengthTransferred);
}

KUSB_EXP BOOL KUSB_API WinUsb_QueryInterfaceSettings(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber,
    _out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	return UsbK_QueryInterfaceSettings (InterfaceHandle, AltSettingNumber, UsbAltInterfaceDescriptor);
}

KUSB_EXP BOOL KUSB_API WinUsb_QueryDeviceInformation(
    _in KUSB_HANDLE InterfaceHandle,
    _in ULONG InformationType,
    _ref PULONG BufferLength,
    _ref PVOID Buffer)
{
	return UsbK_QueryDeviceInformation (InterfaceHandle, InformationType, BufferLength, Buffer);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetCurrentAlternateSetting(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber)
{
	return UsbK_SetCurrentAlternateSetting (InterfaceHandle, AltSettingNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetCurrentAlternateSetting(
    _in KUSB_HANDLE InterfaceHandle,
    _out PUCHAR AltSettingNumber)
{
	return UsbK_GetCurrentAlternateSetting (InterfaceHandle, AltSettingNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_QueryPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber,
    _in UCHAR PipeIndex,
    _out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	return UsbK_QueryPipe (InterfaceHandle, AltSettingNumber, PipeIndex, PipeInformation);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetPipePolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in ULONG PolicyType,
    _in ULONG ValueLength,
    _in PVOID Value)
{
	return UsbK_SetPipePolicy (InterfaceHandle, PipeID, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetPipePolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in ULONG PolicyType,
    _ref PULONG ValueLength,
    _out PVOID Value)
{
	return UsbK_GetPipePolicy (InterfaceHandle, PipeID, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_ReadPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _out PUCHAR Buffer,
    _in ULONG BufferLength,
    _outopt PULONG LengthTransferred,
    _inopt LPOVERLAPPED Overlapped)
{
	return UsbK_ReadPipe (InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_WritePipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in PUCHAR Buffer,
    _in ULONG BufferLength,
    _outopt PULONG LengthTransferred,
    _inopt LPOVERLAPPED Overlapped)
{
	return UsbK_WritePipe (InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_ControlTransfer(
    _in KUSB_HANDLE InterfaceHandle,
    _in WINUSB_SETUP_PACKET SetupPacket,
    _refopt PUCHAR Buffer,
    _in ULONG BufferLength,
    _outopt PULONG LengthTransferred,
    _inopt LPOVERLAPPED Overlapped)
{
	return UsbK_ControlTransfer (InterfaceHandle, SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_ResetPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID)
{
	return UsbK_ResetPipe (InterfaceHandle, PipeID);
}

KUSB_EXP BOOL KUSB_API WinUsb_AbortPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID)
{
	return UsbK_AbortPipe (InterfaceHandle, PipeID);
}

KUSB_EXP BOOL KUSB_API WinUsb_FlushPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID)
{
	return UsbK_FlushPipe (InterfaceHandle, PipeID);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetPowerPolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in ULONG PolicyType,
    _in ULONG ValueLength,
    _in PVOID Value)
{
	return UsbK_SetPowerPolicy (InterfaceHandle, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetPowerPolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in ULONG PolicyType,
    _ref PULONG ValueLength,
    _out PVOID Value)
{
	return UsbK_GetPowerPolicy (InterfaceHandle, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetOverlappedResult(
    _in KUSB_HANDLE InterfaceHandle,
    _in LPOVERLAPPED lpOverlapped,
    _out LPDWORD lpNumberOfBytesTransferred,
    _in BOOL bWait)
{
	return UsbK_GetOverlappedResult (InterfaceHandle, lpOverlapped, lpNumberOfBytesTransferred, bWait);
}

#endif // EXCLUDE_WINUSB_WRAPPER
