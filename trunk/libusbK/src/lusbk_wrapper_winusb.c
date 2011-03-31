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

#include "lusbk_private.h"

#ifndef EXCLUDE_WINUSB_WRAPPER

KUSB_EXP BOOL KUSB_API WinUsb_Initialize (
    __in  HANDLE DeviceHandle,
    __out PWINUSB_INTERFACE_HANDLE InterfaceHandle)
{
	return LUsbK_Initialize(DeviceHandle, InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_Free (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle)
{
	return LUsbK_Free(InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetAssociatedInterface (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AssociatedInterfaceIndex,
    __out PWINUSB_INTERFACE_HANDLE AssociatedInterfaceHandle)
{
	return LUsbK_GetAssociatedInterface(InterfaceHandle, AssociatedInterfaceIndex, AssociatedInterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetDescriptor (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR DescriptorType,
    __in  UCHAR Index,
    __in  USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out PULONG LengthTransferred)
{
	return LUsbK_GetDescriptor(InterfaceHandle, DescriptorType, Index, LanguageID, Buffer, BufferLength, LengthTransferred);
}

KUSB_EXP BOOL KUSB_API WinUsb_QueryInterfaceSettings (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AlternateInterfaceNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	return LUsbK_QueryInterfaceSettings(InterfaceHandle, AlternateInterfaceNumber, UsbAltInterfaceDescriptor);
}

KUSB_EXP BOOL KUSB_API WinUsb_QueryDeviceInformation (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG InformationType,
    __inout PULONG BufferLength,
    __out PVOID Buffer)
{
	return LUsbK_QueryDeviceInformation(InterfaceHandle, InformationType, BufferLength, Buffer);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetCurrentAlternateSetting (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR SettingNumber)
{
	return LUsbK_SetCurrentAlternateSetting(InterfaceHandle, SettingNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetCurrentAlternateSetting (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR SettingNumber)
{
	return LUsbK_GetCurrentAlternateSetting(InterfaceHandle, SettingNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_QueryPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AlternateInterfaceNumber,
    __in  UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	return LUsbK_QueryPipe(InterfaceHandle, AlternateInterfaceNumber, PipeIndex, PipeInformation);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetPipePolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in  ULONG PolicyType,
    __in  ULONG ValueLength,
    __in PVOID Value)
{
	return LUsbK_SetPipePolicy(InterfaceHandle, PipeID, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetPipePolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in  ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	return LUsbK_GetPipePolicy(InterfaceHandle, PipeID, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_ReadPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	return LUsbK_ReadPipe(InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_WritePipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	return LUsbK_WritePipe(InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_ControlTransfer (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  WINUSB_SETUP_PACKET SetupPacket,
    __out_opt PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt  LPOVERLAPPED Overlapped)
{
	return LUsbK_ControlTransfer(InterfaceHandle, SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_ResetPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID)
{
	return LUsbK_ResetPipe(InterfaceHandle, PipeID);
}

KUSB_EXP BOOL KUSB_API WinUsb_AbortPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID)
{
	return LUsbK_AbortPipe(InterfaceHandle, PipeID);
}

KUSB_EXP BOOL KUSB_API WinUsb_FlushPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID)
{
	return LUsbK_FlushPipe(InterfaceHandle, PipeID);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetPowerPolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG PolicyType,
    __in  ULONG ValueLength,
    __in PVOID Value)
{
	return LUsbK_SetPowerPolicy(InterfaceHandle, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetPowerPolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	return LUsbK_GetPowerPolicy(InterfaceHandle, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetOverlappedResult (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in  BOOL bWait)
{
	return LUsbK_GetOverlappedResult(InterfaceHandle, lpOverlapped, lpNumberOfBytesTransferred, bWait);
}


#endif // EXCLUDE_WINUSB_WRAPPER
