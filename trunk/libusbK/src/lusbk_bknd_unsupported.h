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

#ifndef _LUSBK_BKND_UNSUPPORTED__
#define _LUSBK_BKND_UNSUPPORTED__

BOOL KUSB_API Unsupported_Initialize (
    __in HANDLE DeviceHandle,
    __out KUSB_HANDLE* InterfaceHandle);

BOOL KUSB_API Unsupported_Free (
    __in KUSB_HANDLE InterfaceHandle);

BOOL KUSB_API Unsupported_GetAssociatedInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AssociatedInterfaceIndex,
    __out KUSB_HANDLE* AssociatedInterfaceHandle);

BOOL KUSB_API Unsupported_SelectInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR IndexOrNumber,
    __in BOOL IsIndex);

BOOL KUSB_API Unsupported_GetDescriptor (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR DescriptorType,
    __in UCHAR Index,
    __in USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out PULONG LengthTransferred);

BOOL KUSB_API Unsupported_QueryInterfaceSettings (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);

BOOL KUSB_API Unsupported_QueryDeviceInformation (
    __in KUSB_HANDLE InterfaceHandle,
    __in ULONG InformationType,
    __inout PULONG BufferLength,
    __out PVOID Buffer);

BOOL KUSB_API Unsupported_SetCurrentAlternateSetting (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber);

BOOL KUSB_API Unsupported_GetCurrentAlternateSetting (
    __in KUSB_HANDLE InterfaceHandle,
    __out PUCHAR AltSettingNumber);

BOOL KUSB_API Unsupported_QueryPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber,
    __in UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation);

BOOL KUSB_API Unsupported_SetPipePolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value);

BOOL KUSB_API Unsupported_GetPipePolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value);

BOOL KUSB_API Unsupported_ReadPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped);

BOOL KUSB_API Unsupported_WritePipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped);

BOOL KUSB_API Unsupported_ControlTransfer (
    __in KUSB_HANDLE InterfaceHandle,
    __in WINUSB_SETUP_PACKET SetupPacket,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped);

BOOL KUSB_API Unsupported_ResetPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID);

BOOL KUSB_API Unsupported_AbortPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID);

BOOL KUSB_API Unsupported_FlushPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID);

BOOL KUSB_API Unsupported_SetPowerPolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value);

BOOL KUSB_API Unsupported_GetPowerPolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value);

BOOL KUSB_API Unsupported_GetOverlappedResult (
    __in KUSB_HANDLE InterfaceHandle,
    __in LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in BOOL bWait);

BOOL KUSB_API Unsupported_ResetDevice (
    __in KUSB_HANDLE InterfaceHandle);

BOOL KUSB_API Unsupported_Open (
    __in KLST_DEVINFO* DevInfo,
    __out KUSB_HANDLE* InterfaceHandle);

BOOL KUSB_API Unsupported_Close (
    __in KUSB_HANDLE InterfaceHandle);

BOOL KUSB_API Unsupported_SetConfiguration (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR ConfigurationNumber);

BOOL KUSB_API Unsupported_GetConfiguration (
    __in KUSB_HANDLE InterfaceHandle,
    __out PUCHAR ConfigurationNumber);

BOOL KUSB_API Unsupported_ClaimInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex);

BOOL KUSB_API Unsupported_ReleaseInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex);

BOOL KUSB_API Unsupported_SetAltInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex,
    __in UCHAR AltSettingNumber);

BOOL KUSB_API Unsupported_GetAltInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex,
    __out PUCHAR AltSettingNumber);

BOOL KUSB_API Unsupported_IsoReadPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __inout PKISO_CONTEXT IsoContext,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __in LPOVERLAPPED Overlapped);

BOOL KUSB_API Unsupported_IsoWritePipe (
    __in KUSB_HANDLE InterfaceHandle,
    __inout PKISO_CONTEXT IsoContext,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __in LPOVERLAPPED Overlapped);

BOOL KUSB_API Unsupported_GetCurrentFrameNumber (
    __in KUSB_HANDLE InterfaceHandle,
    __out PULONG FrameNumber);


BOOL GetProcAddress_Unsupported(__out KPROC* ProcAddress, __in ULONG FunctionID);

#endif
