/*! \file lusbk_usb.h
*/
#ifndef __KUSB_USER_API_H__
#define __KUSB_USER_API_H__

#include <windows.h>
#include <objbase.h>
#include "lusbk_common.h"
#include "lusbk_dynamic.h"
#include "lusbk_device_list.h"

#ifdef __cplusplus
extern "C" {
#endif

	//! Gets a usb device list.
	/*!
	  \param DeviceList Pointer reference that will receive a a populated device list.
	  \param SearchParameters search/filtering options.
	*/
	KUSB_EXP LONG KUSB_API LUsbK_GetDeviceList(
	    __deref_inout PKUSB_DEV_LIST* DeviceList,
	    __in PKUSB_DEV_LIST_SEARCH SearchParameters);

	//! Frees a usb device list.
	/*!
	  \param DeviceList The list to free.
	*/
	KUSB_EXP VOID KUSB_API LUsbK_FreeDeviceList(
	    __deref_inout PKUSB_DEV_LIST* DeviceList);

	//! Loads a dynamic driver api.
	/*!
	  \param DriverAPI A driver api structure to populate.
	  \param DriverID The driver id of the api to retrieve.
	*/
	KUSB_EXP BOOL KUSB_API LUsbK_LoadDriverApi(
	    __inout PKUSB_DRIVER_API DriverAPI,
	    __in ULONG DriverID);

	//! Loads a dynamic driver api.
	/*!
	  \param ProcAddress Pointer reference that will receive the API function.
	  \param DriverID The driver id of the api to use.
	  \param FunctionID The function id.
	*/
	KUSB_EXP BOOL KUSB_API LUsbK_GetProcAddress(
	    __out KPROC* ProcAddress,
	    __in ULONG DriverID,
	    __in ULONG FunctionID);

	/// <summary>Prepares the first interface of the device for use.</summary>
	KUSB_EXP BOOL KUSB_API LUsbK_Initialize(
	    __in  HANDLE DeviceHandle,
	    __out PWINUSB_INTERFACE_HANDLE InterfaceHandle
	);

	KUSB_EXP BOOL KUSB_API LUsbK_Free(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle
	);

	KUSB_EXP BOOL KUSB_API LUsbK_GetAssociatedInterface(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR AssociatedInterfaceIndex,
	    __out PWINUSB_INTERFACE_HANDLE AssociatedInterfaceHandle
	);

	KUSB_EXP BOOL KUSB_API LUsbK_GetDescriptor(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR DescriptorType,
	    __in  UCHAR Index,
	    __in  USHORT LanguageID,
	    __out_opt PUCHAR Buffer,
	    __in  ULONG BufferLength,
	    __out PULONG LengthTransferred
	);

	KUSB_EXP BOOL KUSB_API LUsbK_QueryInterfaceSettings(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR AlternateInterfaceNumber,
	    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor
	);

	KUSB_EXP BOOL KUSB_API LUsbK_QueryDeviceInformation(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  ULONG InformationType,
	    __inout PULONG BufferLength,
	    __out PVOID Buffer
	);

	KUSB_EXP BOOL KUSB_API LUsbK_SetCurrentAlternateSetting(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR SettingNumber
	);

	KUSB_EXP BOOL KUSB_API LUsbK_GetCurrentAlternateSetting(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __out PUCHAR SettingNumber
	);

	KUSB_EXP BOOL KUSB_API LUsbK_QueryPipe(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR AlternateInterfaceNumber,
	    __in  UCHAR PipeIndex,
	    __out PWINUSB_PIPE_INFORMATION PipeInformation
	);

	KUSB_EXP BOOL KUSB_API LUsbK_SetPipePolicy(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID,
	    __in  ULONG PolicyType,
	    __in  ULONG ValueLength,
	    __in PVOID Value
	);

	KUSB_EXP BOOL KUSB_API LUsbK_GetPipePolicy(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID,
	    __in  ULONG PolicyType,
	    __inout PULONG ValueLength,
	    __out PVOID Value
	);

	KUSB_EXP BOOL KUSB_API LUsbK_ReadPipe(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID,
	    __out_opt PUCHAR Buffer,
	    __in  ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt LPOVERLAPPED Overlapped
	);

	KUSB_EXP BOOL KUSB_API LUsbK_WritePipe(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID,
	    __in PUCHAR Buffer,
	    __in  ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt LPOVERLAPPED Overlapped
	);

	KUSB_EXP BOOL KUSB_API LUsbK_ControlTransfer(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  WINUSB_SETUP_PACKET SetupPacket,
	    __out_opt PUCHAR Buffer,
	    __in  ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt  LPOVERLAPPED Overlapped
	);

	KUSB_EXP BOOL KUSB_API LUsbK_ResetPipe(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID
	);

	KUSB_EXP BOOL KUSB_API LUsbK_AbortPipe(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID
	);

	KUSB_EXP BOOL KUSB_API LUsbK_FlushPipe(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  UCHAR PipeID
	);

	KUSB_EXP BOOL KUSB_API LUsbK_SetPowerPolicy(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  ULONG PolicyType,
	    __in  ULONG ValueLength,
	    __in PVOID Value
	);

	KUSB_EXP BOOL KUSB_API LUsbK_GetPowerPolicy(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  ULONG PolicyType,
	    __inout PULONG ValueLength,
	    __out PVOID Value
	);

	KUSB_EXP BOOL KUSB_API LUsbK_GetOverlappedResult(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
	    __in  LPOVERLAPPED lpOverlapped,
	    __out LPDWORD lpNumberOfBytesTransferred,
	    __in  BOOL bWait
	);

	KUSB_EXP BOOL KUSB_API LUsbK_ResetDevice(
	    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle);

#ifndef EXCLUDE_WINUSB_WRAPPER

	KUSB_EXP KUSB_Initialize						WinUsb_Initialize;
	KUSB_EXP KUSB_Free							WinUsb_Free;
	KUSB_EXP KUSB_GetAssociatedInterface			WinUsb_GetAssociatedInterface;
	KUSB_EXP KUSB_GetDescriptor					WinUsb_GetDescriptor;
	KUSB_EXP KUSB_QueryInterfaceSettings			WinUsb_QueryInterfaceSettings;
	KUSB_EXP KUSB_QueryDeviceInformation			WinUsb_QueryDeviceInformation;
	KUSB_EXP KUSB_SetCurrentAlternateSetting		WinUsb_SetCurrentAlternateSetting;
	KUSB_EXP KUSB_GetCurrentAlternateSetting		WinUsb_GetCurrentAlternateSetting;
	KUSB_EXP KUSB_QueryPipe						WinUsb_QueryPipe;
	KUSB_EXP KUSB_SetPipePolicy					WinUsb_SetPipePolicy;
	KUSB_EXP KUSB_GetPipePolicy					WinUsb_GetPipePolicy;
	KUSB_EXP KUSB_ReadPipe						WinUsb_ReadPipe;
	KUSB_EXP KUSB_WritePipe						WinUsb_WritePipe;
	KUSB_EXP KUSB_ControlTransfer				WinUsb_ControlTransfer;
	KUSB_EXP KUSB_ResetPipe						WinUsb_ResetPipe;
	KUSB_EXP KUSB_AbortPipe						WinUsb_AbortPipe;
	KUSB_EXP KUSB_FlushPipe						WinUsb_FlushPipe;
	KUSB_EXP KUSB_SetPowerPolicy					WinUsb_SetPowerPolicy;
	KUSB_EXP KUSB_GetPowerPolicy					WinUsb_GetPowerPolicy;
	KUSB_EXP KUSB_GetOverlappedResult			WinUsb_GetOverlappedResult;

#endif // EXCLUDE_WINUSB_WRAPPER


#ifdef __cplusplus
}
#endif

#endif // __KUSB_USER_API_H__
