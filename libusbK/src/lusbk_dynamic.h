
#ifndef __LUSBK_DYNAMIC_H_
#define __LUSBK_DYNAMIC_H_

#include <windows.h>
#include <objbase.h>
#include "lusbk_common.h"

typedef INT_PTR (FAR WINAPI* KPROC)();

typedef enum _KUSB_DRVID
{
	KUSB_DRVID_INVALID = -1L,

	KUSB_DRVID_LIBUSBK,
	KUSB_DRVID_LIBUSB0,
	KUSB_DRVID_LIBUSB0_FILTER,
	KUSB_DRVID_WINUSB,

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

	KUSB_FNID_COUNT
} KUSB_FNID;

typedef struct _KUSB_DRIVER_API
{
	BOOL (KUSB_API* Initialize)					(__in HANDLE DeviceHandle, __out PWINUSB_INTERFACE_HANDLE InterfaceHandle);
	BOOL (KUSB_API* Free)						(__in WINUSB_INTERFACE_HANDLE InterfaceHandle);
	BOOL (KUSB_API* GetAssociatedInterface)		(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AssociatedInterfaceIndex, __out PWINUSB_INTERFACE_HANDLE AssociatedInterfaceHandle);
	BOOL (KUSB_API* GetDescriptor)				(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in UCHAR DescriptorType, __in UCHAR Index, __in USHORT LanguageID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out PULONG LengthTransferred);
	BOOL (KUSB_API* QueryInterfaceSettings)		(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AlternateInterfaceNumber, __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescripto);
	BOOL (KUSB_API* QueryDeviceInformation)		(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in ULONG InformationType, __inout PULONG BufferLength, __out PVOID Buffe);
	BOOL (KUSB_API* SetCurrentAlternateSetting)	(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in UCHAR SettingNumbe);
	BOOL (KUSB_API* GetCurrentAlternateSetting)	(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __out PUCHAR SettingNumbe);
	BOOL (KUSB_API* QueryPipe)					(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AlternateInterfaceNumber, __in UCHAR PipeIndex, __out PWINUSB_PIPE_INFORMATION PipeInformation);
	BOOL (KUSB_API* SetPipePolicy)				(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	BOOL (KUSB_API* GetPipePolicy)				(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
	BOOL (KUSB_API* ReadPipe)					(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* WritePipe)					(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* ControlTransfer)			(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in WINUSB_SETUP_PACKET SetupPacket, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* ResetPipe)					(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* AbortPipe)					(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* FlushPipe)					(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* SetPowerPolicy)				(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	BOOL (KUSB_API* GetPowerPolicy)				(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
	BOOL (KUSB_API* GetOverlappedResult)		(__in WINUSB_INTERFACE_HANDLE InterfaceHandle, __in LPOVERLAPPED lpOverlapped, __out LPDWORD lpNumberOfBytesTransferred, __in BOOL bWait);
	BOOL (KUSB_API* ResetDevice)				(__in WINUSB_INTERFACE_HANDLE InterfaceHandle);
} KUSB_DRIVER_API, *PKUSB_DRIVER_API;

typedef BOOL KUSB_API KUSB_Initialize (
    __in  HANDLE DeviceHandle,
    __out PWINUSB_INTERFACE_HANDLE InterfaceHandle
);

typedef BOOL KUSB_API KUSB_Free (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle
);

typedef BOOL KUSB_API KUSB_GetAssociatedInterface (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AssociatedInterfaceIndex,
    __out PWINUSB_INTERFACE_HANDLE AssociatedInterfaceHandle
);

typedef BOOL KUSB_API KUSB_GetDescriptor (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR DescriptorType,
    __in  UCHAR Index,
    __in  USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out PULONG LengthTransferred
);

typedef BOOL KUSB_API KUSB_QueryInterfaceSettings (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AlternateInterfaceNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor
);

typedef BOOL KUSB_API KUSB_QueryDeviceInformation (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG InformationType,
    __inout PULONG BufferLength,
    __out PVOID Buffer
);

typedef BOOL KUSB_API KUSB_SetCurrentAlternateSetting (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR SettingNumber
);

typedef BOOL KUSB_API KUSB_GetCurrentAlternateSetting (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR SettingNumber
);

typedef BOOL KUSB_API KUSB_QueryPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AlternateInterfaceNumber,
    __in  UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation
);

typedef BOOL KUSB_API KUSB_SetPipePolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in  ULONG PolicyType,
    __in  ULONG ValueLength,
    __in PVOID Value
);

typedef BOOL KUSB_API KUSB_GetPipePolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in  ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value
);

typedef BOOL KUSB_API KUSB_ReadPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped
);

typedef BOOL KUSB_API KUSB_WritePipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped
);

typedef BOOL KUSB_API KUSB_ControlTransfer (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  WINUSB_SETUP_PACKET SetupPacket,
    __out_opt PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt  LPOVERLAPPED Overlapped
);

typedef BOOL KUSB_API KUSB_ResetPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID
);

typedef BOOL KUSB_API KUSB_AbortPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID
);

typedef BOOL KUSB_API KUSB_FlushPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID
);

typedef BOOL KUSB_API KUSB_SetPowerPolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG PolicyType,
    __in  ULONG ValueLength,
    __in PVOID Value
);

typedef BOOL KUSB_API KUSB_GetPowerPolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value
);

typedef BOOL KUSB_API KUSB_GetOverlappedResult (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in  BOOL bWait
);

typedef BOOL KUSB_API KUSB_ResetDevice (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle);


#endif