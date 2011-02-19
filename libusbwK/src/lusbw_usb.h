/* libusb-win32 WDF, Generic KMDF Windows USB Driver
 * Copyright (c) 2010-2011 Travis Robinson <libusbdotnet@gmail.com>
 * Copyright (c) 2002-2005 Stephan Meyer <ste_meyer@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LUSBW_USER_API_H__
#define __LUSBW_USER_API_H__

#if defined(DYNAMIC_DLL)
#define LUSBW_EXP
#else
#define LUSBW_EXP
#endif

#if !defined(LUSBW_API)
#define LUSBW_API WINAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __WUSB_H__

#include "lusbw_winusb_compat_io.h"

	typedef PVOID WINUSB_INTERFACE_HANDLE, *PWINUSB_INTERFACE_HANDLE;

#pragma pack(1)

	typedef struct _WINUSB_SETUP_PACKET
	{
		UCHAR   RequestType;
		UCHAR   Request;
		USHORT  Value;
		USHORT  Index;
		USHORT  Length;
	} WINUSB_SETUP_PACKET, *PWINUSB_SETUP_PACKET;

#pragma pack()

#else

#ifndef EXCLUDE_WINUSB_WRAPPER
#define EXCLUDE_WINUSB_WRAPPER 1
#endif

#endif // // __WUSB_H__

	LUSBW_EXP BOOL LUSBW_API LUsbW_Initialize (
	    __in  HANDLE DeviceHandle,
	    __out PWINUSB_INTERFACE_HANDLE InterfaceHandle
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_Free (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_GetAssociatedInterface (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR AssociatedInterfaceIndex,
	    __out PWINUSB_INTERFACE_HANDLE AssociatedInterfaceHandle
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_GetDescriptor (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR DescriptorType,
	    __in  UCHAR Index,
	    __in  USHORT LanguageID,
	    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
	    __in  ULONG BufferLength,
	    __out PULONG LengthTransferred
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_QueryInterfaceSettings (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR AlternateInterfaceNumber,
	    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_QueryDeviceInformation (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  ULONG InformationType,
	    __inout PULONG BufferLength,
	    __out_bcount(*BufferLength) PVOID Buffer
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_SetCurrentAlternateSetting (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR SettingNumber
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_GetCurrentAlternateSetting (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __out PUCHAR SettingNumber
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_QueryPipe (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR AlternateInterfaceNumber,
	    __in  UCHAR PipeIndex,
	    __out PWINUSB_PIPE_INFORMATION PipeInformation
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_SetPipePolicy (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID,
	    __in  ULONG PolicyType,
	    __in  ULONG ValueLength,
	    __in_bcount(ValueLength) PVOID Value
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_GetPipePolicy (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID,
	    __in  ULONG PolicyType,
	    __inout PULONG ValueLength,
	    __out_bcount(*ValueLength) PVOID Value
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_ReadPipe (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID,
	    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
	    __in  ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt LPOVERLAPPED Overlapped
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_WritePipe (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID,
	    __in_bcount(BufferLength) PUCHAR Buffer,
	    __in  ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt LPOVERLAPPED Overlapped
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_ControlTransfer (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  WINUSB_SETUP_PACKET SetupPacket,
	    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
	    __in  ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt  LPOVERLAPPED Overlapped
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_ResetPipe (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_AbortPipe (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_FlushPipe (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_SetPowerPolicy (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  ULONG PolicyType,
	    __in  ULONG ValueLength,
	    __in_bcount(ValueLength) PVOID Value
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_GetPowerPolicy (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  ULONG PolicyType,
	    __inout PULONG ValueLength,
	    __out_bcount(*ValueLength) PVOID Value
	);

	LUSBW_EXP BOOL LUSBW_API LUsbW_GetOverlappedResult (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  LPOVERLAPPED lpOverlapped,
	    __out LPDWORD lpNumberOfBytesTransferred,
	    __in  BOOL bWait
	);

	LUSBW_EXP PUSB_INTERFACE_DESCRIPTOR LUsbW_ParseConfigurationDescriptor (
	    __in  PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
	    __in  PVOID StartPosition,
	    __in  LONG InterfaceNumber,
	    __in  LONG AlternateSetting,
	    __in  LONG InterfaceClass,
	    __in  LONG InterfaceSubClass,
	    __in  LONG InterfaceProtocol
	);

	LUSBW_EXP PUSB_COMMON_DESCRIPTOR LUsbW_ParseDescriptors (
	    __in_bcount(TotalLength) PVOID    DescriptorBuffer,
	    __in  ULONG    TotalLength,
	    __in  PVOID    StartPosition,
	    __in  LONG     DescriptorType
	);

#ifndef EXCLUDE_WINUSB_WRAPPER

	LUSBW_EXP BOOL LUSBW_API WinUsb_Initialize (
	    __in  HANDLE DeviceHandle,
	    __out PWINUSB_INTERFACE_HANDLE InterfaceHandle
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_Free (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_GetAssociatedInterface (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR AssociatedInterfaceIndex,
	    __out PWINUSB_INTERFACE_HANDLE AssociatedInterfaceHandle
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_GetDescriptor (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR DescriptorType,
	    __in  UCHAR Index,
	    __in  USHORT LanguageID,
	    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
	    __in  ULONG BufferLength,
	    __out PULONG LengthTransferred
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_QueryInterfaceSettings (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR AlternateInterfaceNumber,
	    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_QueryDeviceInformation (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  ULONG InformationType,
	    __inout PULONG BufferLength,
	    __out_bcount(*BufferLength) PVOID Buffer
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_SetCurrentAlternateSetting (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR SettingNumber
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_GetCurrentAlternateSetting (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __out PUCHAR SettingNumber
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_QueryPipe (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR AlternateInterfaceNumber,
	    __in  UCHAR PipeIndex,
	    __out PWINUSB_PIPE_INFORMATION PipeInformation
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_SetPipePolicy (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID,
	    __in  ULONG PolicyType,
	    __in  ULONG ValueLength,
	    __in_bcount(ValueLength) PVOID Value
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_GetPipePolicy (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID,
	    __in  ULONG PolicyType,
	    __inout PULONG ValueLength,
	    __out_bcount(*ValueLength) PVOID Value
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_ReadPipe (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID,
	    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
	    __in  ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt LPOVERLAPPED Overlapped
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_WritePipe (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID,
	    __in_bcount(BufferLength) PUCHAR Buffer,
	    __in  ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt LPOVERLAPPED Overlapped
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_ControlTransfer (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  WINUSB_SETUP_PACKET SetupPacket,
	    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
	    __in  ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt  LPOVERLAPPED Overlapped
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_ResetPipe (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_AbortPipe (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_FlushPipe (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_SetPowerPolicy (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  ULONG PolicyType,
	    __in  ULONG ValueLength,
	    __in_bcount(ValueLength) PVOID Value
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_GetPowerPolicy (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  ULONG PolicyType,
	    __inout PULONG ValueLength,
	    __out_bcount(*ValueLength) PVOID Value
	);

	LUSBW_EXP BOOL LUSBW_API WinUsb_GetOverlappedResult (
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  LPOVERLAPPED lpOverlapped,
	    __out LPDWORD lpNumberOfBytesTransferred,
	    __in  BOOL bWait
	);

	LUSBW_EXP PUSB_INTERFACE_DESCRIPTOR WinUsb_ParseConfigurationDescriptor (
	    __in  PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
	    __in  PVOID StartPosition,
	    __in  LONG InterfaceNumber,
	    __in  LONG AlternateSetting,
	    __in  LONG InterfaceClass,
	    __in  LONG InterfaceSubClass,
	    __in  LONG InterfaceProtocol
	);

	LUSBW_EXP PUSB_COMMON_DESCRIPTOR WinUsb_ParseDescriptors (
	    __in_bcount(TotalLength) PVOID    DescriptorBuffer,
	    __in  ULONG    TotalLength,
	    __in  PVOID    StartPosition,
	    __in  LONG     DescriptorType
	);

#endif // EXCLUDE_WINUSB_WRAPPER


#ifdef __cplusplus
}
#endif

#endif // __LUSBW_USER_API_H__
