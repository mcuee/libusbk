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

KUSB_EXP BOOL KUSB_API WinUsb_Initialize (
    __in HANDLE DeviceHandle,
    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	return UsbK_Initialize (DeviceHandle, InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_Free (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	return UsbK_Free (InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetAssociatedInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AssociatedInterfaceIndex,
    __out PLIBUSBK_INTERFACE_HANDLE AssociatedInterfaceHandle)
{
	return UsbK_GetAssociatedInterface (InterfaceHandle, AssociatedInterfaceIndex, AssociatedInterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetDescriptor (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
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
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AlternateSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	return UsbK_QueryInterfaceSettings (InterfaceHandle, AlternateSettingNumber, UsbAltInterfaceDescriptor);
}

KUSB_EXP BOOL KUSB_API WinUsb_QueryDeviceInformation (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG InformationType,
    __inout PULONG BufferLength,
    __out PVOID Buffer)
{
	return UsbK_QueryDeviceInformation (InterfaceHandle, InformationType, BufferLength, Buffer);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetCurrentAlternateSetting (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AlternateSettingNumber)
{
	return UsbK_SetCurrentAlternateSetting (InterfaceHandle, AlternateSettingNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetCurrentAlternateSetting (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR AlternateSettingNumber)
{
	return UsbK_GetCurrentAlternateSetting (InterfaceHandle, AlternateSettingNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_QueryPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AlternateSettingNumber,
    __in UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	return UsbK_QueryPipe (InterfaceHandle, AlternateSettingNumber, PipeIndex, PipeInformation);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetPipePolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value)
{
	return UsbK_SetPipePolicy (InterfaceHandle, PipeID, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetPipePolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	return UsbK_GetPipePolicy (InterfaceHandle, PipeID, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_ReadPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	return UsbK_ReadPipe (InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_WritePipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	return UsbK_WritePipe (InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_ControlTransfer (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in WINUSB_SETUP_PACKET SetupPacket,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	return UsbK_ControlTransfer (InterfaceHandle, SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_ResetPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	return UsbK_ResetPipe (InterfaceHandle, PipeID);
}

KUSB_EXP BOOL KUSB_API WinUsb_AbortPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	return UsbK_AbortPipe (InterfaceHandle, PipeID);
}

KUSB_EXP BOOL KUSB_API WinUsb_FlushPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	return UsbK_FlushPipe (InterfaceHandle, PipeID);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetPowerPolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value)
{
	return UsbK_SetPowerPolicy (InterfaceHandle, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetPowerPolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	return UsbK_GetPowerPolicy (InterfaceHandle, PolicyType, ValueLength, Value);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetOverlappedResult (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in BOOL bWait)
{
	return UsbK_GetOverlappedResult (InterfaceHandle, lpOverlapped, lpNumberOfBytesTransferred, bWait);
}

KUSB_EXP BOOL KUSB_API WinUsb_ResetDevice (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	return UsbK_ResetDevice (InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_Open (
    __in PKLST_DEV_INFO DeviceListItem,
    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	return UsbK_Open (DeviceListItem, InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_Close (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	return UsbK_Close (InterfaceHandle);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetConfiguration (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR ConfigurationNumber)
{
	return UsbK_SetConfiguration (InterfaceHandle, ConfigurationNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetConfiguration (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR ConfigurationNumber)
{
	return UsbK_GetConfiguration (InterfaceHandle, ConfigurationNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_ClaimInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex)
{
	return UsbK_ClaimInterface (InterfaceHandle, InterfaceNumberOrIndex, IsIndex);
}

KUSB_EXP BOOL KUSB_API WinUsb_ReleaseInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex)
{
	return UsbK_ReleaseInterface (InterfaceHandle, InterfaceNumberOrIndex, IsIndex);
}

KUSB_EXP BOOL KUSB_API WinUsb_SetAltInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex,
    __in UCHAR AltInterfaceNumber)
{
	return UsbK_SetAltInterface (InterfaceHandle, InterfaceNumberOrIndex, IsIndex, AltInterfaceNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetAltInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex,
    __out PUCHAR AltInterfaceNumber)
{
	return UsbK_GetAltInterface (InterfaceHandle, InterfaceNumberOrIndex, IsIndex, AltInterfaceNumber);
}

KUSB_EXP BOOL KUSB_API WinUsb_IsoReadPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __inout PKISO_CONTEXT IsoContext,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __in LPOVERLAPPED Overlapped)
{
	return UsbK_IsoReadPipe (InterfaceHandle, IsoContext, Buffer, BufferLength, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_IsoWritePipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __inout PKISO_CONTEXT IsoContext,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __in LPOVERLAPPED Overlapped)
{
	return UsbK_IsoWritePipe (InterfaceHandle, IsoContext, Buffer, BufferLength, Overlapped);
}

KUSB_EXP BOOL KUSB_API WinUsb_GetCurrentFrameNumber (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PULONG FrameNumber)
{
	return UsbK_GetCurrentFrameNumber (InterfaceHandle, FrameNumber);
}

#endif // EXCLUDE_WINUSB_WRAPPER
