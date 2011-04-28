/*! \file lusbk_usb.h
*/

#ifndef _LUSBK_USB_H__
#define _LUSBK_USB_H__

#include <windows.h>
#include <objbase.h>
#include "lusbk_common.h"
#include "lusbk_dynamic.h"
#include "lusbk_linked_list.h"
#include "lusbk_device_list.h"
#include "lusbk_overlapped.h"

#ifdef __cplusplus
extern "C" {
#endif
	/*! \addtogroup core UsbK Core API
	* Core API functions are available in two forms:
	* - As normal exported functions for use with libusbK.sys or libusb0.sys. (not WinUSB)
	* - As dynamically loaded functions based on the devices driver type. (all supported drivers)
	*   For more information on dynamic function loading, see \ref DrvK_LoadDriverApi and \ref DrvK_GetProcAddress
	*
	*  @{
	*/

	/*! \addtogroup core_functions Functions
	*  @{
	*/

//! Creates a libusbK handle for the device specified by a file handle.
	/*!
	*
	* When \ref UsbK_Initialize is called, the policy settings of the interface
	* are reset to the default values.
	*
	* The \ref UsbK_Initialize call queries the underlying USB stack for various
	* descriptors and allocates enough memory to store the retrieved
	* descriptor data.
	*
	* \ref UsbK_Initialize first retrieves the device descriptor and then gets
	* the associated configuration descriptor. From the configuration
	* descriptor, the call derives the associated interface descriptors and
	* stores them in an array. The interfaces in the array are identified by
	* zero-based indexes. An index value of 0 indicates the first interface
	* (the default interface), a value of 1 indicates the second associated
	* interface, and so on. \ref UsbK_Initialize parses the default interface
	* descriptor for the endpoint descriptors and caches information such as
	* the associated pipes or state specific data. The handle received in the
	* InterfaceHandle parameter is a pointer to the memory block allocated for
	* the first interface in the array.
	*
	* If an application wants to use another interface on the device, it must
	* call \ref UsbK_GetAssociatedInterface, specify the index of the interface,
	* and retrieve a handle to the memory block allocated for the specified
	* interface.
	*
	* \param DeviceHandle
	* The handle to the device that CreateFile returned.
	* libusbK uses overlapped I/O, so FILE_FLAG_OVERLAPPED must be specified in the
	* dwFlagsAndAttributes parameter of CreateFile call for DeviceHandle to have the
	* characteristics necessary for this to function properly.
	*
	* \param InterfaceHandle
	* Receives a handle to the first (default) interface on the device.
	* This handle is required by other libusbK routines that perform operations
	* on the default interface. The handle is opaque. To release this handle,
	* call the \ref UsbK_Free function.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_Initialize (
	    __in HANDLE DeviceHandle,
	    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);

//! Frees a libusbK interface handle.
	/*!
	*
	* The \ref UsbK_Free function releases all of the resources that
	* \ref UsbK_Initialize or \ref UsbK_Open allocated. This is a synchronous
	* operation.
	*
	* \param InterfaceHandle
	* Handle to an interface on the device. This handle must be created by a previous call to:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns TRUE.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_Free (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);

//! Retrieves a handle for an associated interface.
	/*!
	*
	* The \ref UsbK_GetAssociatedInterface function retrieves a handle for an
	* associated interface. This is a synchronous operation.
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	*
	* \param AssociatedInterfaceIndex
	* An index that specifies the associated interface to retrieve. A value of
	* 0 indicates the first associated interface, a value of 1 indicates the
	* second associated interface, and so on.
	*
	* \param AssociatedInterfaceHandle
	* A handle for the associated interface. Callers must pass this interface
	* handle to libusbK Functions exposed by libusbK.dll. To close this handle,
	* call \ref UsbK_Free.
	*
	* The \ref UsbK_GetAssociatedInterface routine retrieves an opaque handle.
	*
	* The first associated interface is the interface that immediately follows
	* the interface whose handle the \ref UsbK_Initialize or \ref UsbK_Open routine retrieves.
	*
	* The handle that \ref UsbK_GetAssociatedInterface returns must be released
	* by calling \ref UsbK_Free.
	*
	* Callers of \ref UsbK_GetAssociatedInterface can retrieve only one handle for
	* each interface. If a caller attempts to retrieve more than one handle
	* for the same interface, the routine will fail with an error of
	* ERROR_ALREADY_EXISTS.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetAssociatedInterface (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR AssociatedInterfaceIndex,
	    __out PLIBUSBK_INTERFACE_HANDLE AssociatedInterfaceHandle);

//! Gets the requested descriptor. This is a synchronous operation.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param DescriptorType
	* A value that specifies the type of descriptor to return. This
	* parameter corresponds to the bDescriptorType field of a standard device
	* descriptor, whose values are described in the Universal Serial Bus
	* specification.
	*
	* \param Index
	* The descriptor index. For an explanation of the descriptor index, see
	* the Universal Serial Bus specification (www.usb.org).
	*
	* \param LanguageID
	* A value that specifies the language identifier, if the requested
	* descriptor is a string descriptor.
	*
	* \param Buffer
	* A caller-allocated buffer that receives the requested descriptor.
	*
	* \param BufferLength
	* The length, in bytes, of Buffer.
	*
	* \param LengthTransferred
	*
	* The number of bytes that were copied into Buffer.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetDescriptor (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR DescriptorType,
	    __in UCHAR Index,
	    __in USHORT LanguageID,
	    __out_opt PUCHAR Buffer,
	    __in ULONG BufferLength,
	    __out PULONG LengthTransferred);

//! Retrieves the interface descriptor for the specified alternate interface settings for a particular interface handle.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param AlternateSettingNumber
	* A value that indicates which alternate settings to return. A value of 0
	* indicates the first alternate setting, a value of 1 indicates the second
	* alternate setting, and so on.
	*
	* \param UsbAltInterfaceDescriptor
	* A pointer to a caller-allocated \ref USB_INTERFACE_DESCRIPTOR structure that
	* contains information about the interface that AlternateSettingNumber
	* specified.
	*
	* \ref UsbK_QueryInterfaceSettings parses the configuration descriptor
	* previously retrieved by \ref UsbK_Initialize or \ref UsbK_Open. For more information, see
	* the Remarks section for \ref UsbK_Initialize or \ref UsbK_Open.
	*
	* The \ref UsbK_QueryInterfaceSettings call searches the interface array
	* for the alternate interface specified by the interface index passed by
	* the caller in the AlternateSettingNumber. If the specified interface is
	* found, the function populates the caller-allocated
	* USB_INTERFACE_DESCRIPTOR structure. If the specified interface is not
	* found, then the call fails with the ERROR_NO_MORE_ITEMS code.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_QueryInterfaceSettings (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR AlternateSettingNumber,
	    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);

//! Retrieves information about the physical device that is associated with a libusbK handle.
	/*!
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param InformationType
	* A value that specifies which interface information value to retrieve.
	* On input, InformationType must have the following value: \ref DEVICE_SPEED
	* (0x01).
	*
	* \param BufferLength
	* The maximum number of bytes to read. This number must be less than or
	* equal to the size, in bytes, of Buffer. On output, BufferLength is set
	* to the actual number of bytes that were copied into Buffer.
	*
	* \param Buffer
	* A caller-allocated buffer that receives the requested value.
	* On output, Buffer indicates the device speed. The function returns
	* \ref LowSpeed (0x01) or \ref HighSpeed (0x03).
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_QueryDeviceInformation (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in ULONG InformationType,
	    __inout PULONG BufferLength,
	    __out PVOID Buffer);

//! Sets the alternate setting of an interface.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Initialize
	* - \ref UsbK_Open
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param AlternateSetting
	* The value that is contained in the bAlternateSetting member of the
	* \ref USB_INTERFACE_DESCRIPTOR structure. This structure can be populated by the
	* \ref UsbK_QueryInterfaceSettings routine.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_SetCurrentAlternateSetting (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR SettingNumber);

//! Gets the current alternate interface setting for an interface.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param AlternateSetting
	* A pointer to an unsigned character that receives an integer that indicates the current alternate setting.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetCurrentAlternateSetting (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __out PUCHAR SettingNumber);

//! Retrieves information about a pipe that is associated with an interface.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_QueryPipe (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR AlternateSettingNumber,
	    __in UCHAR PipeIndex,
	    __out PWINUSB_PIPE_INFORMATION PipeInformation);

//! Sets the policy for a specific pipe (endpoint).
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_SetPipePolicy (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR PipeID,
	    __in ULONG PolicyType,
	    __in ULONG ValueLength,
	    __in PVOID Value);

//! Gets the policy for a specific pipe (endpoint).
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetPipePolicy (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR PipeID,
	    __in ULONG PolicyType,
	    __inout PULONG ValueLength,
	    __out PVOID Value);

//! Reads data from the specified pipe.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_ReadPipe (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR PipeID,
	    __out_opt PUCHAR Buffer,
	    __in ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt LPOVERLAPPED Overlapped);

//! Writes data to a pipe.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_WritePipe (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR PipeID,
	    __in PUCHAR Buffer,
	    __in ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt LPOVERLAPPED Overlapped);

//! Transmits control data over a default control endpoint.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_ControlTransfer (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in WINUSB_SETUP_PACKET SetupPacket,
	    __out_opt PUCHAR Buffer,
	    __in ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt LPOVERLAPPED Overlapped);

//! Resets the data toggle and clears the stall condition on a pipe.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_ResetPipe (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR PipeID);

//! Aborts all of the pending transfers for a pipe.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_AbortPipe (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR PipeID);

//! Discards any data that is cached in a pipe.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_FlushPipe (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR PipeID);

//! Sets the power policy for a device.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_SetPowerPolicy (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in ULONG PolicyType,
	    __in ULONG ValueLength,
	    __in PVOID Value);

//! Gets the power policy for a device.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetPowerPolicy (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in ULONG PolicyType,
	    __inout PULONG ValueLength,
	    __out PVOID Value);

//! Retrieves the results of an overlapped operation on the specified libusbK handle.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetOverlappedResult (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in LPOVERLAPPED lpOverlapped,
	    __out LPDWORD lpNumberOfBytesTransferred,
	    __in BOOL bWait);

//! Resest the usb device of the specified interface handle. (port cycle).
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_ResetDevice (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);

//! Creates/opens a libusbK interface handle from the device list. This is a perferred method.
	/*!
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_Open (
	    __in PKUSB_DEV_LIST DeviceListItem,
	    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);

//! Closes a libusbK interface handle opened by \ref UsbK_Open or \ref UsbK_Initialize. This is a perferred method.
	/*!
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_Close (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);

//! Sets the device configuration number.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_SetConfiguration (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR ConfigurationNumber);

//! Gets the device current configuration number.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetConfiguration (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __out PUCHAR ConfigurationNumber);

//! Claims the specified interface by number or index.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_ClaimInterface (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR InterfaceNumberOrIndex,
	    __in BOOL IsIndex);

//! Releases the specified interface by number or index.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_ReleaseInterface (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR InterfaceNumberOrIndex,
	    __in BOOL IsIndex);

//! Sets the alternate setting of the specified interface.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_SetAltInterface (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR InterfaceNumberOrIndex,
	    __in BOOL IsIndex,
	    __in UCHAR AltInterfaceNumber);

//! Gets the alternate setting for the specified interface.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetAltInterface (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR InterfaceNumberOrIndex,
	    __in BOOL IsIndex,
	    __out PUCHAR AltInterfaceNumber);

//! Reads from an isochronous pipe.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_IsoReadPipe (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR PipeID,
	    __out_opt PUCHAR Buffer,
	    __in ULONG BufferLength,
	    __in ULONG IsoPacketSize,
	    __in LPOVERLAPPED Overlapped);

//! Writes to an isochronous pipe.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_IsoWritePipe (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR PipeID,
	    __in PUCHAR Buffer,
	    __in ULONG BufferLength,
	    __in ULONG IsoPacketSize,
	    __in LPOVERLAPPED Overlapped);

	/*! @} */

	/*! @} */

#ifdef __cplusplus
}
#endif

#endif // _LUSBK_USB_H__
