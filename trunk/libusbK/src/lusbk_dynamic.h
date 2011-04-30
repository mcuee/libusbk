/*! \file lusbk_dynamic.h
*/

#ifndef __LUSBK_DYNAMIC_H_
#define __LUSBK_DYNAMIC_H_

#include <windows.h>
#include <objbase.h>
#include "lusbk_common.h"
#include "lusbk_device_list.h"

typedef enum _KUSB_DRVID
{
	KUSB_DRVID_INVALID = -1L,

	KUSB_DRVID_LIBUSBK,
	KUSB_DRVID_LIBUSB0,
	KUSB_DRVID_WINUSB,
	KUSB_DRVID_LIBUSB0_FILTER,

	KUSB_DRVID_COUNT

} KUSB_DRVID;

typedef enum _KUSB_FNID
{
	KUSB_FNID_INVALID = -1L,
	KUSB_FNID_Initialize,
	KUSB_FNID_Free,
	KUSB_FNID_GetAssociatedInterface,
	KUSB_FNID_GetDescriptor,
	KUSB_FNID_QueryInterfaceSettings,
	KUSB_FNID_QueryDeviceInformation,
	KUSB_FNID_SetCurrentAlternateSetting,
	KUSB_FNID_GetCurrentAlternateSetting,
	KUSB_FNID_QueryPipe,
	KUSB_FNID_SetPipePolicy,
	KUSB_FNID_GetPipePolicy,
	KUSB_FNID_ReadPipe,
	KUSB_FNID_WritePipe,
	KUSB_FNID_ControlTransfer,
	KUSB_FNID_ResetPipe,
	KUSB_FNID_AbortPipe,
	KUSB_FNID_FlushPipe,
	KUSB_FNID_SetPowerPolicy,
	KUSB_FNID_GetPowerPolicy,
	KUSB_FNID_GetOverlappedResult,
	KUSB_FNID_ResetDevice,
	KUSB_FNID_Open,
	KUSB_FNID_Close,
	KUSB_FNID_SetConfiguration,
	KUSB_FNID_GetConfiguration,
	KUSB_FNID_ClaimInterface,
	KUSB_FNID_ReleaseInterface,
	KUSB_FNID_SetAltInterface,
	KUSB_FNID_GetAltInterface,
	KUSB_FNID_IsoReadPipe,
	KUSB_FNID_IsoWritePipe,

	KUSB_FNID_COUNT
} KUSB_FNID;

typedef struct _KUSB_DRIVER_API
{
	BOOL (KUSB_API* Initialize)				(__in HANDLE DeviceHandle, __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	BOOL (KUSB_API* Free)					(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	BOOL (KUSB_API* GetAssociatedInterface)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AssociatedInterfaceIndex, __out PLIBUSBK_INTERFACE_HANDLE AssociatedInterfaceHandle);
	BOOL (KUSB_API* GetDescriptor)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR DescriptorType, __in UCHAR Index, __in USHORT LanguageID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out PULONG LengthTransferred);
	BOOL (KUSB_API* QueryInterfaceSettings)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AlternateSettingNumber, __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);
	BOOL (KUSB_API* QueryDeviceInformation)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in ULONG InformationType, __inout PULONG BufferLength, __out PVOID Buffer);
	BOOL (KUSB_API* SetCurrentAlternateSetting)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR SettingNumber);
	BOOL (KUSB_API* GetCurrentAlternateSetting)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __out PUCHAR SettingNumber);
	BOOL (KUSB_API* QueryPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AlternateSettingNumber, __in UCHAR PipeIndex, __out PWINUSB_PIPE_INFORMATION PipeInformation);
	BOOL (KUSB_API* SetPipePolicy)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	BOOL (KUSB_API* GetPipePolicy)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
	BOOL (KUSB_API* ReadPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* WritePipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* ControlTransfer)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in WINUSB_SETUP_PACKET SetupPacket, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* ResetPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* AbortPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* FlushPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* SetPowerPolicy)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	BOOL (KUSB_API* GetPowerPolicy)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
	BOOL (KUSB_API* GetOverlappedResult)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in LPOVERLAPPED lpOverlapped, __out LPDWORD lpNumberOfBytesTransferred, __in BOOL bWait);
	BOOL (KUSB_API* ResetDevice)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	BOOL (KUSB_API* Open)					(__in PKUSB_DEV_LIST DeviceListItem, __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	BOOL (KUSB_API* Close)					(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	BOOL (KUSB_API* SetConfiguration)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR ConfigurationNumber);
	BOOL (KUSB_API* GetConfiguration)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __out PUCHAR ConfigurationNumber);
	BOOL (KUSB_API* ClaimInterface)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR InterfaceNumberOrIndex, __in BOOL IsIndex);
	BOOL (KUSB_API* ReleaseInterface)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR InterfaceNumberOrIndex, __in BOOL IsIndex);
	BOOL (KUSB_API* SetAltInterface)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR InterfaceNumberOrIndex, __in BOOL IsIndex, __in UCHAR AltInterfaceNumber);
	BOOL (KUSB_API* GetAltInterface)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR InterfaceNumberOrIndex, __in BOOL IsIndex, __out PUCHAR AltInterfaceNumber);
	BOOL (KUSB_API* IsoReadPipe)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __in ULONG IsoPacketSize, __in LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* IsoWritePipe)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in PUCHAR Buffer, __in ULONG BufferLength, __in ULONG IsoPacketSize, __in LPOVERLAPPED Overlapped);

}* PKUSB_DRIVER_API, KUSB_DRIVER_API;

typedef BOOL KUSB_API KUSB_Initialize (
    __in HANDLE DeviceHandle,
    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);

typedef BOOL KUSB_API KUSB_Free (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);

typedef BOOL KUSB_API KUSB_GetAssociatedInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AssociatedInterfaceIndex,
    __out PLIBUSBK_INTERFACE_HANDLE AssociatedInterfaceHandle);

typedef BOOL KUSB_API KUSB_GetDescriptor (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR DescriptorType,
    __in UCHAR Index,
    __in USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out PULONG LengthTransferred);

typedef BOOL KUSB_API KUSB_QueryInterfaceSettings (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AlternateSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);

typedef BOOL KUSB_API KUSB_QueryDeviceInformation (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG InformationType,
    __inout PULONG BufferLength,
    __out PVOID Buffer);

typedef BOOL KUSB_API KUSB_SetCurrentAlternateSetting (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR SettingNumber);

typedef BOOL KUSB_API KUSB_GetCurrentAlternateSetting (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR SettingNumber);

typedef BOOL KUSB_API KUSB_QueryPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AlternateSettingNumber,
    __in UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation);

typedef BOOL KUSB_API KUSB_SetPipePolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value);

typedef BOOL KUSB_API KUSB_GetPipePolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value);

typedef BOOL KUSB_API KUSB_ReadPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped);

typedef BOOL KUSB_API KUSB_WritePipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped);

typedef BOOL KUSB_API KUSB_ControlTransfer (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in WINUSB_SETUP_PACKET SetupPacket,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped);

typedef BOOL KUSB_API KUSB_ResetPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID);

typedef BOOL KUSB_API KUSB_AbortPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID);

typedef BOOL KUSB_API KUSB_FlushPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID);

typedef BOOL KUSB_API KUSB_SetPowerPolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value);

typedef BOOL KUSB_API KUSB_GetPowerPolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value);

typedef BOOL KUSB_API KUSB_GetOverlappedResult (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in BOOL bWait);

typedef BOOL KUSB_API KUSB_ResetDevice (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);

typedef BOOL KUSB_API KUSB_Open (
    __in PKUSB_DEV_LIST DeviceListItem,
    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);

typedef BOOL KUSB_API KUSB_Close (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);

typedef BOOL KUSB_API KUSB_SetConfiguration (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR ConfigurationNumber);

typedef BOOL KUSB_API KUSB_GetConfiguration (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR ConfigurationNumber);

typedef BOOL KUSB_API KUSB_ClaimInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex);

typedef BOOL KUSB_API KUSB_ReleaseInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex);

typedef BOOL KUSB_API KUSB_SetAltInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex,
    __in UCHAR AltInterfaceNumber);

typedef BOOL KUSB_API KUSB_GetAltInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex,
    __out PUCHAR AltInterfaceNumber);

typedef BOOL KUSB_API KUSB_IsoReadPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __in ULONG IsoPacketSize,
    __in LPOVERLAPPED Overlapped);

typedef BOOL KUSB_API KUSB_IsoWritePipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __in ULONG IsoPacketSize,
    __in LPOVERLAPPED Overlapped);

#ifdef __cplusplus
extern "C" {
#endif

//! Initialize a driver api set.
	/*!
	  \ingroup driverapi
	  \param DriverAPI A driver api structure to populate.
	  \param DriverID The driver id of the api to retrieve.
	*/
	KUSB_EXP BOOL KUSB_API DrvK_LoadDriverApi(
	    __inout PKUSB_DRIVER_API DriverAPI,
	    __in ULONG DriverID);

//! Initialize a driver api function.
	/*!
	  \ingroup driverapi
	  \param ProcAddress Pointer reference that will receive the API function.
	  \param DriverID The driver id of the api to use.
	  \param FunctionID The function id.
	*/
	KUSB_EXP BOOL KUSB_API DrvK_GetProcAddress(
	    __out KPROC* ProcAddress,
	    __in ULONG DriverID,
	    __in ULONG FunctionID);

#ifdef __cplusplus
}
#endif

#endif
