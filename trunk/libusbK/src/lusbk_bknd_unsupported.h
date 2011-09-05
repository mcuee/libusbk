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

KUSB_EXP BOOL KUSB_API Unsupported_Initialize(
    _in HANDLE DeviceHandle,
    _out KUSB_HANDLE* InterfaceHandle);

KUSB_EXP BOOL KUSB_API Unsupported_SelectInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex);

KUSB_EXP BOOL KUSB_API Unsupported_GetDescriptor(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR DescriptorType,
    _in UCHAR Index,
    _in USHORT LanguageID,
    _out PUCHAR Buffer,
    _in ULONG BufferLength,
    _outopt PULONG LengthTransferred);

KUSB_EXP BOOL KUSB_API Unsupported_QueryDeviceInformation(
    _in KUSB_HANDLE InterfaceHandle,
    _in ULONG InformationType,
    _ref PULONG BufferLength,
    _ref PVOID Buffer);

KUSB_EXP BOOL KUSB_API Unsupported_SetCurrentAlternateSetting(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber);

KUSB_EXP BOOL KUSB_API Unsupported_GetCurrentAlternateSetting(
    _in KUSB_HANDLE InterfaceHandle,
    _out PUCHAR AltSettingNumber);

KUSB_EXP BOOL KUSB_API Unsupported_SetPipePolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in ULONG PolicyType,
    _in ULONG ValueLength,
    _in PVOID Value);

KUSB_EXP BOOL KUSB_API Unsupported_GetPipePolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in ULONG PolicyType,
    _ref PULONG ValueLength,
    _out PVOID Value);

KUSB_EXP BOOL KUSB_API Unsupported_ReadPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _out PUCHAR Buffer,
    _in ULONG BufferLength,
    _outopt PULONG LengthTransferred,
    _inopt LPOVERLAPPED Overlapped);

KUSB_EXP BOOL KUSB_API Unsupported_WritePipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID,
    _in PUCHAR Buffer,
    _in ULONG BufferLength,
    _outopt PULONG LengthTransferred,
    _inopt LPOVERLAPPED Overlapped);

KUSB_EXP BOOL KUSB_API Unsupported_ControlTransfer(
    _in KUSB_HANDLE InterfaceHandle,
    _in WINUSB_SETUP_PACKET SetupPacket,
    _refopt PUCHAR Buffer,
    _in ULONG BufferLength,
    _outopt PULONG LengthTransferred,
    _inopt LPOVERLAPPED Overlapped);

KUSB_EXP BOOL KUSB_API Unsupported_ResetPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID);

KUSB_EXP BOOL KUSB_API Unsupported_AbortPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID);

KUSB_EXP BOOL KUSB_API Unsupported_FlushPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR PipeID);

KUSB_EXP BOOL KUSB_API Unsupported_SetPowerPolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in ULONG PolicyType,
    _in ULONG ValueLength,
    _in PVOID Value);

KUSB_EXP BOOL KUSB_API Unsupported_GetPowerPolicy(
    _in KUSB_HANDLE InterfaceHandle,
    _in ULONG PolicyType,
    _ref PULONG ValueLength,
    _out PVOID Value);

KUSB_EXP BOOL KUSB_API Unsupported_GetOverlappedResult(
    _in KUSB_HANDLE InterfaceHandle,
    _in LPOVERLAPPED lpOverlapped,
    _out LPDWORD lpNumberOfBytesTransferred,
    _in BOOL bWait);

KUSB_EXP BOOL KUSB_API Unsupported_ResetDevice(
    _in KUSB_HANDLE InterfaceHandle);

KUSB_EXP BOOL KUSB_API Unsupported_Init(
    _out KUSB_HANDLE* InterfaceHandle,
    _in KLST_DEVINFO_HANDLE DevInfo);

KUSB_EXP BOOL KUSB_API Unsupported_SetConfiguration(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR ConfigurationNumber);

KUSB_EXP BOOL KUSB_API Unsupported_GetConfiguration(
    _in KUSB_HANDLE InterfaceHandle,
    _out PUCHAR ConfigurationNumber);

KUSB_EXP BOOL KUSB_API Unsupported_ClaimInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex);

KUSB_EXP BOOL KUSB_API Unsupported_ReleaseInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex);

KUSB_EXP BOOL KUSB_API Unsupported_SetAltInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex,
    _in UCHAR AltSettingNumber);

KUSB_EXP BOOL KUSB_API Unsupported_GetAltInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR NumberOrIndex,
    _in BOOL IsIndex,
    _out PUCHAR AltSettingNumber);

KUSB_EXP BOOL KUSB_API Unsupported_IsoReadPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _ref PKISO_CONTEXT IsoContext,
    _out PUCHAR Buffer,
    _in ULONG BufferLength,
    _in LPOVERLAPPED Overlapped);

KUSB_EXP BOOL KUSB_API Unsupported_IsoWritePipe(
    _in KUSB_HANDLE InterfaceHandle,
    _ref PKISO_CONTEXT IsoContext,
    _in PUCHAR Buffer,
    _in ULONG BufferLength,
    _in LPOVERLAPPED Overlapped);

KUSB_EXP BOOL KUSB_API Unsupported_GetCurrentFrameNumber(
    _in KUSB_HANDLE InterfaceHandle,
    _out PULONG FrameNumber);

KUSB_EXP BOOL KUSB_API Unsupported_Free(
    _in KUSB_HANDLE InterfaceHandle);

KUSB_EXP BOOL KUSB_API Unsupported_GetAssociatedInterface(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AssociatedInterfaceIndex,
    _out KUSB_HANDLE* AssociatedInterfaceHandle);

KUSB_EXP BOOL KUSB_API Unsupported_Clone(
    _in KUSB_HANDLE InterfaceHandle,
    _out KUSB_HANDLE* DstInterfaceHandle);

KUSB_EXP BOOL KUSB_API Unsupported_QueryPipe(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber,
    _in UCHAR PipeIndex,
    _out PWINUSB_PIPE_INFORMATION PipeInformation);

KUSB_EXP BOOL KUSB_API Unsupported_QueryInterfaceSettings(
    _in KUSB_HANDLE InterfaceHandle,
    _in UCHAR AltSettingNumber,
    _out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);

BOOL GetProcAddress_Unsupported(__out KPROC* ProcAddress, __in LONG FunctionID);

#endif
