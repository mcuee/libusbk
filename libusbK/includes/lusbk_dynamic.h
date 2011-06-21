/*! \file lusbk_dynamic.h
* \brief structs, typedefs, enums, defines, and functions for loading libusbK \ref usbk functions dynamically for a specific driver.
*/

#ifndef __LUSBK_DYNAMIC_H_
#define __LUSBK_DYNAMIC_H_

#include <windows.h>
#include <objbase.h>
#include "lusbk_common.h"
#include "lusbk_device_list.h"

/*! \addtogroup drvk
* @{
*/

//! Supported driver id enumeration.
typedef enum _KUSB_DRVID
{
    //! Invalid driver ID
    KUSB_DRVID_INVALID = -1L,

    //! libusbK.sys driver ID
    KUSB_DRVID_LIBUSBK,

    //! libusb0.sys driver ID
    KUSB_DRVID_LIBUSB0,

    //! WinUSB.sys driver ID
    KUSB_DRVID_WINUSB,

    //! libusb0.sys filter driver ID
    KUSB_DRVID_LIBUSB0_FILTER,

    //! Supported driver count
    KUSB_DRVID_COUNT

} KUSB_DRVID;

//! Supported function id enumeration.
typedef enum _KUSB_FNID
{
    //! Invalid function ID
    KUSB_FNID_INVALID = -1L,

    //! \ref UsbK_Initialize function id
    KUSB_FNID_Initialize,

    //! \ref UsbK_Free function id
    KUSB_FNID_Free,

    //! \ref UsbK_GetAssociatedInterface function id
    KUSB_FNID_GetAssociatedInterface,

    //! \ref UsbK_GetDescriptor function id
    KUSB_FNID_GetDescriptor,

    //! \ref UsbK_QueryInterfaceSettings function id
    KUSB_FNID_QueryInterfaceSettings,

    //! \ref UsbK_QueryDeviceInformation function id
    KUSB_FNID_QueryDeviceInformation,

    //! \ref UsbK_SetCurrentAlternateSetting function id
    KUSB_FNID_SetCurrentAlternateSetting,

    //! \ref UsbK_GetCurrentAlternateSetting function id
    KUSB_FNID_GetCurrentAlternateSetting,

    //! \ref UsbK_QueryPipe function id
    KUSB_FNID_QueryPipe,

    //! \ref UsbK_SetPipePolicy function id
    KUSB_FNID_SetPipePolicy,

    //! \ref UsbK_GetPipePolicy function id
    KUSB_FNID_GetPipePolicy,

    //! \ref UsbK_ReadPipe function id
    KUSB_FNID_ReadPipe,

    //! \ref UsbK_WritePipe function id
    KUSB_FNID_WritePipe,

    //! \ref UsbK_ControlTransfer function id
    KUSB_FNID_ControlTransfer,

    //! \ref UsbK_ResetPipe function id
    KUSB_FNID_ResetPipe,

    //! \ref UsbK_AbortPipe function id
    KUSB_FNID_AbortPipe,

    //! \ref UsbK_FlushPipe function id
    KUSB_FNID_FlushPipe,

    //! \ref UsbK_SetPowerPolicy function id
    KUSB_FNID_SetPowerPolicy,

    //! \ref UsbK_GetPowerPolicy function id
    KUSB_FNID_GetPowerPolicy,

    //! \ref UsbK_GetOverlappedResult function id
    KUSB_FNID_GetOverlappedResult,

    //! \ref UsbK_ResetDevice function id
    KUSB_FNID_ResetDevice,

    //! \ref UsbK_Open function id
    KUSB_FNID_Open,

    //! \ref UsbK_Close function id
    KUSB_FNID_Close,

    //! \ref UsbK_SetConfiguration function id
    KUSB_FNID_SetConfiguration,

    //! \ref UsbK_GetConfiguration function id
    KUSB_FNID_GetConfiguration,

    //! \ref UsbK_ClaimInterface function id
    KUSB_FNID_ClaimInterface,

    //! \ref UsbK_ReleaseInterface function id
    KUSB_FNID_ReleaseInterface,

    //! \ref UsbK_SetAltInterface function id
    KUSB_FNID_SetAltInterface,

    //! \ref UsbK_GetAltInterface function id
    KUSB_FNID_GetAltInterface,

    //! \ref UsbK_IsoReadPipe function id
    KUSB_FNID_IsoReadPipe,

    //! \ref UsbK_IsoWritePipe function id
    KUSB_FNID_IsoWritePipe,

    //! \ref UsbK_GetCurrentFrameNumber function id
    KUSB_FNID_GetCurrentFrameNumber,

    //! Supported function count
    KUSB_FNID_COUNT
} KUSB_FNID;

typedef BOOL KUSB_API KUSB_Initialize (
    __in HANDLE DeviceHandle,
    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);

//! Driver api function set structure.
typedef struct _KUSB_DRIVER_API
{
	/*!
	* \fn Initialize(__in HANDLE DeviceHandle, __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	* \brief Driver specific function pointer to \ref UsbK_Initialize
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* Initialize)				(__in HANDLE DeviceHandle, __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	/*!
	* \fn Free(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	* \brief Driver specific function pointer to \ref UsbK_Free
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* Free)					(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	/*!
	* \fn GetAssociatedInterface(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AssociatedInterfaceIndex, __out PLIBUSBK_INTERFACE_HANDLE AssociatedInterfaceHandle);
	* \brief Driver specific function pointer to \ref UsbK_GetAssociatedInterface
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* GetAssociatedInterface)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AssociatedInterfaceIndex, __out PLIBUSBK_INTERFACE_HANDLE AssociatedInterfaceHandle);
	/*!
	* \fn GetDescriptor(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR DescriptorType, __in UCHAR Index, __in USHORT LanguageID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out PULONG LengthTransferred);
	* \brief Driver specific function pointer to \ref UsbK_GetDescriptor
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* GetDescriptor)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR DescriptorType, __in UCHAR Index, __in USHORT LanguageID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out PULONG LengthTransferred);
	/*!
	* \fn QueryInterfaceSettings(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AlternateSettingNumber, __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);
	* \brief Driver specific function pointer to \ref UsbK_QueryInterfaceSettings
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* QueryInterfaceSettings)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AlternateSettingNumber, __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);
	/*!
	* \fn QueryDeviceInformation(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in ULONG InformationType, __inout PULONG BufferLength, __out PVOID Buffer);
	* \brief Driver specific function pointer to \ref UsbK_QueryDeviceInformation
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* QueryDeviceInformation)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in ULONG InformationType, __inout PULONG BufferLength, __out PVOID Buffer);
	/*!
	* \fn SetCurrentAlternateSetting(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AlternateSettingNumber);
	* \brief Driver specific function pointer to \ref UsbK_SetCurrentAlternateSetting
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* SetCurrentAlternateSetting)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AlternateSettingNumber);
	/*!
	* \fn GetCurrentAlternateSetting(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __out PUCHAR AlternateSettingNumber);
	* \brief Driver specific function pointer to \ref UsbK_GetCurrentAlternateSetting
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* GetCurrentAlternateSetting)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __out PUCHAR AlternateSettingNumber);
	/*!
	* \fn QueryPipe(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AlternateSettingNumber, __in UCHAR PipeIndex, __out PWINUSB_PIPE_INFORMATION PipeInformation);
	* \brief Driver specific function pointer to \ref UsbK_QueryPipe
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* QueryPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR AlternateSettingNumber, __in UCHAR PipeIndex, __out PWINUSB_PIPE_INFORMATION PipeInformation);
	/*!
	* \fn SetPipePolicy(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	* \brief Driver specific function pointer to \ref UsbK_SetPipePolicy
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* SetPipePolicy)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	/*!
	* \fn GetPipePolicy(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
	* \brief Driver specific function pointer to \ref UsbK_GetPipePolicy
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* GetPipePolicy)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
	/*!
	* \fn ReadPipe(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	* \brief Driver specific function pointer to \ref UsbK_ReadPipe
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* ReadPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	/*!
	* \fn WritePipe(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	* \brief Driver specific function pointer to \ref UsbK_WritePipe
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* WritePipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID, __in PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	/*!
	* \fn ControlTransfer(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in WINUSB_SETUP_PACKET SetupPacket, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	* \brief Driver specific function pointer to \ref UsbK_ControlTransfer
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* ControlTransfer)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in WINUSB_SETUP_PACKET SetupPacket, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	/*!
	* \fn ResetPipe(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	* \brief Driver specific function pointer to \ref UsbK_ResetPipe
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* ResetPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	/*!
	* \fn AbortPipe(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	* \brief Driver specific function pointer to \ref UsbK_AbortPipe
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* AbortPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	/*!
	* \fn FlushPipe(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	* \brief Driver specific function pointer to \ref UsbK_FlushPipe
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* FlushPipe)				(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR PipeID);
	/*!
	* \fn SetPowerPolicy(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	* \brief Driver specific function pointer to \ref UsbK_SetPowerPolicy
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* SetPowerPolicy)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	/*!
	* \fn GetPowerPolicy(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
	* \brief Driver specific function pointer to \ref UsbK_GetPowerPolicy
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* GetPowerPolicy)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
	/*!
	* \fn GetOverlappedResult(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in LPOVERLAPPED lpOverlapped, __out LPDWORD lpNumberOfBytesTransferred, __in BOOL bWait);
	* \brief Driver specific function pointer to \ref UsbK_GetOverlappedResult
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* GetOverlappedResult)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in LPOVERLAPPED lpOverlapped, __out LPDWORD lpNumberOfBytesTransferred, __in BOOL bWait);
	/*!
	* \fn ResetDevice(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	* \brief Driver specific function pointer to \ref UsbK_ResetDevice
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* ResetDevice)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	/*!
	* \fn Open(__in PKUSB_DEV_INFO DeviceListItem, __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	* \brief Driver specific function pointer to \ref UsbK_Open
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* Open)					(__in PKUSB_DEV_INFO DeviceListItem, __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	/*!
	* \fn Close(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	* \brief Driver specific function pointer to \ref UsbK_Close
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* Close)					(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);
	/*!
	* \fn SetConfiguration(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR ConfigurationNumber);
	* \brief Driver specific function pointer to \ref UsbK_SetConfiguration
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* SetConfiguration)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR ConfigurationNumber);
	/*!
	* \fn GetConfiguration(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __out PUCHAR ConfigurationNumber);
	* \brief Driver specific function pointer to \ref UsbK_GetConfiguration
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* GetConfiguration)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __out PUCHAR ConfigurationNumber);
	/*!
	* \fn ClaimInterface(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR InterfaceNumberOrIndex, __in BOOL IsIndex);
	* \brief Driver specific function pointer to \ref UsbK_ClaimInterface
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* ClaimInterface)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR InterfaceNumberOrIndex, __in BOOL IsIndex);
	/*!
	* \fn ReleaseInterface(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR InterfaceNumberOrIndex, __in BOOL IsIndex);
	* \brief Driver specific function pointer to \ref UsbK_ReleaseInterface
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* ReleaseInterface)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR InterfaceNumberOrIndex, __in BOOL IsIndex);
	/*!
	* \fn SetAltInterface(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR InterfaceNumberOrIndex, __in BOOL IsIndex, __in UCHAR AltInterfaceNumber);
	* \brief Driver specific function pointer to \ref UsbK_SetAltInterface
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* SetAltInterface)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR InterfaceNumberOrIndex, __in BOOL IsIndex, __in UCHAR AltInterfaceNumber);
	/*!
	* \fn GetAltInterface(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR InterfaceNumberOrIndex, __in BOOL IsIndex, __out PUCHAR AltInterfaceNumber);
	* \brief Driver specific function pointer to \ref UsbK_GetAltInterface
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* GetAltInterface)		(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __in UCHAR InterfaceNumberOrIndex, __in BOOL IsIndex, __out PUCHAR AltInterfaceNumber);
	/*!
	* \fn IsoReadPipe(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __inout PKUSB_ISO_CONTEXT IsoContext, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __in LPOVERLAPPED Overlapped);
	* \brief Driver specific function pointer to \ref UsbK_IsoReadPipe
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* IsoReadPipe)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __inout PKUSB_ISO_CONTEXT IsoContext, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __in LPOVERLAPPED Overlapped);
	/*!
	* \fn IsoWritePipe(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __inout PKUSB_ISO_CONTEXT IsoContext, __in PUCHAR Buffer, __in ULONG BufferLength, __in LPOVERLAPPED Overlapped);
	* \brief Driver specific function pointer to \ref UsbK_IsoWritePipe
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* IsoWritePipe)			(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __inout PKUSB_ISO_CONTEXT IsoContext, __in PUCHAR Buffer, __in ULONG BufferLength, __in LPOVERLAPPED Overlapped);
	/*!
	* \fn IsoWritePipe(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __inout PKUSB_ISO_CONTEXT IsoContext, __in PUCHAR Buffer, __in ULONG BufferLength, __in LPOVERLAPPED Overlapped);
	* \brief Driver specific function pointer to \ref UsbK_IsoWritePipe
	* \memberof KUSB_DRIVER_API
	*/
	BOOL (KUSB_API* GetCurrentFrameNumber)	(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle, __out PULONG FrameNumber);

} KUSB_DRIVER_API;
//! Pointer to a \ref KUSB_DRIVER_API structure
typedef KUSB_DRIVER_API* PKUSB_DRIVER_API;



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
    __in UCHAR AlternateSettingNumber);

typedef BOOL KUSB_API KUSB_GetCurrentAlternateSetting (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR AlternateSettingNumber);

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
    __in PKUSB_DEV_INFO DeviceListItem,
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
    __inout PKUSB_ISO_CONTEXT IsoContext,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __in LPOVERLAPPED Overlapped);

typedef BOOL KUSB_API KUSB_IsoWritePipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __inout PKUSB_ISO_CONTEXT IsoContext,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __in LPOVERLAPPED Overlapped);

typedef BOOL KUSB_API KUSB_GetCurrentFrameNumber (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PULONG FrameNumber);

#ifdef __cplusplus
extern "C" {
#endif

	//! Initialize a driver api set.
	/*!
	*
	* \param DriverAPI
	* A driver api structure to populate.
	*
	* \param DriverID
	* The driver id of the api set to retrieve. See \ref KUSB_DRVID
	*
	* \param SizeofDriverAPI
	* Should always be set to the \b sizeof the driver api struct \ref KUSB_DRIVER_API
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	*/
	KUSB_EXP BOOL KUSB_API DrvK_LoadDriverApi(
	    __inout PKUSB_DRIVER_API DriverAPI,
	    __in ULONG DriverID,
	    __in ULONG SizeofDriverAPI);

	//! Initialize a driver api function.
	/*!
	* \param ProcAddress
	* Reference to a function pointer that will receive the API function pointer.
	*
	* \param DriverID
	* The driver id of the api to use. See \ref KUSB_DRVID
	*
	* \param FunctionID
	* The function id. See \ref KUSB_FNID
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	*/
	KUSB_EXP BOOL KUSB_API DrvK_GetProcAddress(
	    __out KPROC* ProcAddress,
	    __in ULONG DriverID,
	    __in ULONG FunctionID);

	/**@}*/

#ifdef __cplusplus
}
#endif

#endif
