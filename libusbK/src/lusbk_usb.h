/*! \file lusbk_usb.h
* Main libusbK USB user include file.
*/

#ifndef _LUSBK_USB_H__
#define _LUSBK_USB_H__

#include <windows.h>
#include <objbase.h>
#include "lusbk_common.h"
#include "lusbk_usbio.h"
#include "lusbk_dynamic.h"
#include "lusbk_linked_list.h"
#include "lusbk_device_list.h"
#include "lusbk_overlapped.h"

#ifdef __cplusplus
extern "C" {
#endif
	/*! \addtogroup core
	*  @{
	*/

//! Creates/opens a libusbK interface handle from the device list. This is a perferred method.
	/*!
	*
	* \ref UsbK_Open performs the same tasks as \ref UsbK_Initialize with the following exceptions:
	* - Uses a \ref KUSB_DEV_LIST instead of a file handle created with the Windows CreateFile() API function.
	* - File handles are managed internally and are closed when the last \ref LIBUSBK_INTERFACE_HANDLE is
	*   closed with \ref UsbK_Close.
	* - If \c DeviceListItem is a composite device, multiple device file handles are managed as one.
	*
	* \param DeviceListItem
	* The device list element to open.
	*
	* \param InterfaceHandle
	* Receives a handle configured to the first (default) interface on the device.
	* This handle is required by other libusbK routines that perform operations
	* on the default interface. The handle is opaque. To release this handle,
	* call the \ref UsbK_Close function.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_Open (
	    __in PKUSB_DEV_LIST DeviceListItem,
	    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle);

//! Closes a libusbK interface handle opened by \ref UsbK_Open or \ref UsbK_Initialize. This is a perferred method.
	/*!
	*
	* The \ref UsbK_Close function releases all of the resources that
	* \ref UsbK_Initialize, \ref UsbK_Open, or \ref UsbK_GetAssociatedInterface allocated. This is a synchronous
	* operation.
	*
	* \note \ref UsbK_Close and \ref UsbK_Free perform the same tasks.  The difference is in the return code only.
	* - \ref UsbK_Free always returns TRUE.
	* - \ref UsbK_Close will return FALSE in the hande is already closed/free.
	*
	* \param InterfaceHandle
	* Handle to an interface on the device. This handle must be created by a previous call to:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_Close (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle);

//! Claims the specified interface by number or index.
	/*!
	* Claiming an interface allows applications a way to prevent other applications
	* or multiple instances of the same application from using an interface at the same time.
	*
	* When an interface is claimed with \ref UsbK_ClaimInterface it performs the following actions:
	* - Checks if the inteface exists. If it does not, returns FALSE and sets last error to ERROR_NO_MORE_ITEMS.
	* - The default (or current) interface for the device is changed to \c InterfaceNumberOrIndex.
	* - libusb0.sys and libusbK.sys:
	*   - A request to claim the interface is sent to the driver.
	*     If the interface is not claimed or already claimed by the application the request succeeds.
	*     If the interface is claimed by another application, \ref UsbK_ClaimInterface returns FALSE
	*     and sets last error to \c ERROR_BUSY.  In this case the
	*     The default (or current) interface for the device is \b still changed to \c InterfaceNumberOrIndex.
	* - WinUSB.sys:
	*   All WinUSB device interfaces are claimed when the device is opened.
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param InterfaceNumberOrIndex
	* Interfaces can be claimed or released by a interface index or \c bInterfaceNumber.
	* - Interface indexes always start from 0 and continue sequentially for all interfaces of the device.
	* - An interface number always represents the actual \ref USB_INTERFACE_DESCRIPTOR::bInterfaceNumber.
	*   Interface numbers are not guaranteed to be zero based or sequential.
	*
	* \param IsIndex
	* If TRUE, \c InterfaceNumberOrIndex represents an interface index.\n
	* if FALSE \c InterfaceNumberOrIndex represents a \c bInterfaceNumber.
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
	* When an interface is release with \ref UsbK_ReleaseInterface it performs the following actions:
	* - Checks if the inteface exists. If it does not, returns FALSE and sets last error to ERROR_NO_MORE_ITEMS.
	* - The default (or current) interface for the device is changed to the previously claimed interface.
	* - libusb0.sys and libusbK.sys:
	*   - A request to release the interface is sent to the driver.
	*     If the interface is not claimed by a different application the request succeeds.
	*     If the interface is claimed by another application, \ref UsbK_ReleaseInterface returns FALSE
	*     and sets last error to \c ERROR_BUSY.  In this case, the default/current interface for the device
	*     is \b still changed to the previously claimed interface.
	* - WinUSB.sys:
	*   No other action needed, returns TRUE.
	*
	* \note
	* When an interface is released, it is moved to the bottom if an interface stack making a previously
	* claimed interface the current.  This will continue to occur regardless of whether the interface is claimed.
	* For this reason, \ref UsbK_ReleaseInterface can be used as a means to change the current/default interface
	* of an \c InterfaceHandle without claiming the interface.
	*
	* \param InterfaceNumberOrIndex
	* Interfaces can be claimed or released by a interface index or \c bInterfaceNumber.
	* - Interface indexes always start from 0 and continue sequentially for all interfaces of the device.
	* - An interface number always represents the actual \ref USB_INTERFACE_DESCRIPTOR::bInterfaceNumber.
	*   Interface numbers are not guaranteed to be zero based or sequential.
	*
	* \param IsIndex
	* If TRUE, \c InterfaceNumberOrIndex represents an interface index.\n
	* if FALSE \c InterfaceNumberOrIndex represents a \c bInterfaceNumber.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_ReleaseInterface (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR InterfaceNumberOrIndex,
	    __in BOOL IsIndex);

//! Sets the alternate setting of the specified interface.
	/*!
	* \ref UsbK_SetAltInterface performs the same task as \ref UsbK_SetCurrentAlternateSetting except it provides
	* the option of specifying which interfaces alternate setting to activate.
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param InterfaceNumberOrIndex
	* Interfaces can be specified by a interface index or \c bInterfaceNumber.
	* - Interface indexes always start from 0 and continue sequentially for all interfaces of the device.
	* - An interface number always represents the actual \ref USB_INTERFACE_DESCRIPTOR::bInterfaceNumber.
	*   Interface numbers are not guaranteed to be zero based or sequential.
	*
	* \param IsIndex
	* If TRUE, \c InterfaceNumberOrIndex represents an interface index.\n
	* if FALSE \c InterfaceNumberOrIndex represents a \c bInterfaceNumber.
	*
	* \param AltInterfaceNumber
	* The bAlternateSetting to activate.
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
	* \ref UsbK_GetAltInterface performs the same task as \ref UsbK_GetCurrentAlternateSetting except it provides
	* the option of specifying which interfaces alternate setting is to be retrieved.
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param InterfaceNumberOrIndex
	* Interfaces can be specified by a interface index or \c bInterfaceNumber.
	* - Interface indexes always start from 0 and continue sequentially for all interfaces of the device.
	* - An interface number always represents the actual \ref USB_INTERFACE_DESCRIPTOR::bInterfaceNumber.
	*   Interface numbers are not guaranteed to be zero based or sequential.
	*
	* \param IsIndex
	* If TRUE, \c InterfaceNumberOrIndex represents an interface index.\n
	* if FALSE \c InterfaceNumberOrIndex represents a \c bInterfaceNumber.
	*
	* \param AltInterfaceNumber
	* On success, returns the active bAlternateSetting.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetAltInterface (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR InterfaceNumberOrIndex,
	    __in BOOL IsIndex,
	    __out PUCHAR AltInterfaceNumber);

//! Gets the requested descriptor. This is a synchronous operation.
	/*!
	*
	* If the device descriptor or active config descriptor is requested,
	* \ref UsbK_GetDescriptor retrieves cached data and this becomes a non-blocking, non I/O request.
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

//! Transmits control data over a default control endpoint.
	/*!
	*
	* A \ref UsbK_ControlTransfer is never cached.  These requests always go directly to the usb device.
	*
	* \attention
	* This function should not be used for operations supported by the library.\n
	* e.g. \ref UsbK_SetConfiguration, \ref UsbK_SetAltInterface, etc..
	*
	* \param InterfaceHandle
	* A valid libusbK interface handle returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param SetupPacket
	*  The 8-byte setup packet of type WINUSB_SETUP_PACKET.
	*
	* \param Buffer
	* A caller-allocated buffer that contains the data to transfer.
	*
	* \param BufferLength
	* The number of bytes to transfer, not including the setup packet. This
	* number must be less than or equal to the size, in bytes, of Buffer.
	*
	* \param LengthTransferred
	* A pointer to a ULONG variable that receives the actual number of
	* transferred bytes. If the application does not expect any data to be
	* transferred during the data phase (BufferLength is zero),
	* LengthTransferred can be NULL.
	*
	* \param Overlapped
	* An optional pointer to an OVERLAPPED structure, which is used for
	* asynchronous operations. If this parameter is specified, \ref
	* UsbK_ControlTransfer immediately returns, and the event is signaled when
	* the operation is complete. If Overlapped is not supplied, the \ref
	* UsbK_ControlTransfer function transfers data synchronously.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	* If an \c Overlapped member is supplied and the operation succeeds this function returns FALSE
	* and sets last error to ERROR_IO_PENDING.
	*
	*/
	KUSB_EXP BOOL KUSB_API UsbK_ControlTransfer (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in WINUSB_SETUP_PACKET SetupPacket,
	    __out_opt PUCHAR Buffer,
	    __in ULONG BufferLength,
	    __out_opt PULONG LengthTransferred,
	    __in_opt LPOVERLAPPED Overlapped);

//! Sets the power policy for a device.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param PolicyType
	* A value that specifies the power policy to set. The following table
	* describes symbolic constants that are defined in \ref lusbk_usbio.h.
	*
	* - AUTO_SUSPEND (0x81)
	*   - Specifies the auto-suspend policy type; the power policy parameter must
	*     be specified by the caller in the Value parameter.
	*   - For auto-suspend, the Value parameter must point to a UCHAR variable.
	*   - If Value is TRUE (nonzero), the USB stack suspends the device if the
	*     device is idle. A device is idle if there are no transfers pending, or
	*     if the only pending transfers are IN transfers to interrupt or bulk
	*     endpoints.
	*   - The default value is determined by the value set in the DefaultIdleState
	*     registry setting. By default, this value is TRUE.
	*
	* - SUSPEND_DELAY (0x83)
	*   - Specifies the suspend-delay policy type; the power policy parameter must
	*     be specified by the caller in the Value parameter.
	*   - For suspend-delay, Value must point to a ULONG variable.
	*   - Value specifies the minimum amount of time, in milliseconds, that the
	*     driver must wait post transfer before it can suspend the device.
	*   - The default value is determined by the value set in the
	*     DefaultIdleTimeout registry setting. By default, this value is five
	*     seconds.
	*
	* \param ValueLength
	* The size, in bytes, of the buffer at Value.
	*
	* \param Value
	* The new value for the power policy parameter. Datatype and value for
	* Value depends on the type of power policy passed in PolicyType. For more
	* information, see PolicyType.
	*
	* The following list summarizes the effects of changes to power management
	* states:
	* - All pipe handles, interface handles, locks, and alternate settings are
	*   preserved across power management events.
	* - Any transfers that are in progress are suspended when a device transfers
	*   to a low power state, and they are resumed when the device is restored
	*   to a working state.
	* - The device and system must be in a working state before the client can
	*   restore a device-specific configuration. Clients can determine whether
	*   the device and system are in a working state from the WM_POWERBROADCAST
	*   message.
	* - The client can indicate that an interface is idle by calling \ref
	*   UsbK_SetPowerPolicy.
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
	* \param PolicyType
	* A value that specifies the power policy parameter to retrieve in Value.
	* The following table describes symbolic constants that are defined in
	* \ref lusbk_usbio.h.
	*
	* - AUTO_SUSPEND (0x81)
	*   - If the caller specifies a power policy of AUTO_SUSPEND, \ref
	*     UsbK_GetPowerPolicy returns the value of the auto suspend policy
	*     parameter in the Value parameter.
	*   - If Value is TRUE (that is, nonzero), the USB stack suspends the device
	*     when no transfers are pending or the only transfers pending are IN
	*     transfers on an interrupt or bulk endpoint.
	*   - The value of the DefaultIdleState registry value determines the default
	*     value of the auto suspend policy parameter.
	*   - The Value parameter must point to a UCHAR variable.
	*
	* - SUSPEND_DELAY (0x83)
	*   - If the caller specifies a power policy of SUSPEND_DELAY, \ref
	*     UsbK_GetPowerPolicy returns the value of the suspend delay policy
	*     parameter in Value.
	*   - The suspend delay policy parameter specifies the minimum amount of time,
	*     in milliseconds, that the driver must wait after any transfer before it
	*     can suspend the device.
	*   - Value must point to a ULONG variable.
	*
	* \param ValueLength
	* A pointer to the size of the buffer that Value. On output, ValueLength
	* receives the size of the data that was copied into the Value buffer.
	*
	* \param Value
	* A buffer that receives the specified power policy parameter. For more
	* information, see PolicyType.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetPowerPolicy (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in ULONG PolicyType,
	    __inout PULONG ValueLength,
	    __out PVOID Value);

//! Sets the device configuration number.
	/*!
	* \ref UsbK_SetConfiguration is only supported with libusb0.sys.
	* If the driver in not libusb0.sys, this function performs the following emulation actions:
	* - If the requested configuration number is the current configuration number, returns TRUE.
	* - If the requested configuration number is one other than the current configuration number,
	*   returns FALSE and set last error to \c ERROR_NO_MORE_ITEMS.
	*
	* This function will fail if there are pending I/O operations or there are other libusbK interface
	* handles referencing the device.
	* \sa UsbK_Free
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param ConfigurationNumber
	* The configuration number to activate.
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
	* \param ConfigurationNumber
	* On success, receives the active configuration number.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetConfiguration (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __out PUCHAR ConfigurationNumber);

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
	* InterfaceHandle parameter will have its default interface configured to
	* the first interface in the array.
	*
	* If an application wants to use another interface on the device, it can
	* call \ref UsbK_GetAssociatedInterface, or \ref UsbK_ClaimInterface.
	*
	* \param DeviceHandle
	* The handle to the device that CreateFile returned.
	* libusbK uses overlapped I/O, so FILE_FLAG_OVERLAPPED must be specified in the
	* dwFlagsAndAttributes parameter of CreateFile call for DeviceHandle to have the
	* characteristics necessary for this to function properly.
	*
	* \param InterfaceHandle
	* Receives a handle configured to the first (default) interface on the device.
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
	* \note \ref UsbK_Close and \ref UsbK_Free perform the same tasks.  The difference is in the return code only.
	* - \ref UsbK_Free always returns TRUE.
	* - \ref UsbK_Close will return FALSE in the hande is already closed/free.
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
	* the current (or default) interface of the specified /c InterfaceHandle.
	*
	* The handle that \ref UsbK_GetAssociatedInterface returns must be released
	* by calling \ref UsbK_Free.
	*
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetAssociatedInterface (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR AssociatedInterfaceIndex,
	    __out PLIBUSBK_INTERFACE_HANDLE AssociatedInterfaceHandle);

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
	* The \ref UsbK_QueryInterfaceSettings call searches the current/default interface array
	* for the alternate interface specified by the caller in the AlternateSettingNumber.
	* If the specified alternate interface is found, the function populates the caller-allocated
	* USB_INTERFACE_DESCRIPTOR structure. If the specified alternate interface is not
	* found, then the call fails with the ERROR_NO_MORE_ITEMS code.
	*
	* To change the current/default interface, see \ref UsbK_ClaimInterface and \ref UsbK_ReleaseInterface
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
	* On input, InformationType must have the following value: \c DEVICE_SPEED
	* (0x01).
	*
	* \param BufferLength
	* The maximum number of bytes to read. This number must be less than or
	* equal to the size, in bytes, of Buffer. On output, BufferLength is set
	* to the actual number of bytes that were copied into Buffer.
	*
	* \param Buffer
	* A caller-allocated buffer that receives the requested value.
	* On output, Buffer indicates the device speed:
	* - (0x01) low/full speed device.
	* - (0x03) high speed device.
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
	* Sets the active bAlternateSetting for the current/default interface.
	*
	* To change the default/current interface see \ref UsbK_ClaimInterface and \ref UsbK_ReleaseInterface
	* \sa UsbK_SetAltInterface
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Initialize
	* - \ref UsbK_Open
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param AlternateSettingNumber
	* The value that is contained in the bAlternateSetting member of the
	* \ref USB_INTERFACE_DESCRIPTOR structure. This structure can be populated by the
	* \ref UsbK_QueryInterfaceSettings routine.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_SetCurrentAlternateSetting (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR AlternateSettingNumber);

//! Gets the current alternate interface setting for an interface.
	/*!
	* Gets the active bAlternateSetting for the current/default interface.
	*
	* To change the default/current interface see \ref UsbK_ClaimInterface and \ref UsbK_ReleaseInterface
	* \sa UsbK_GetAltInterface
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param AlternateSettingNumber
	* A pointer to an unsigned character that receives an integer that indicates the current alternate setting.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetCurrentAlternateSetting (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __out PUCHAR AlternateSettingNumber);

//! Retrieves information about a pipe that is associated with an interface.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param AlternateSettingNumber
	* A value that specifies the alternate interface to return the
	* information for.
	*
	* \param PipeIndex
	* A value that specifies the pipe to return information about. This value
	* is not the same as the bEndpointAddress field in the endpoint
	* descriptor. A PipeIndex value of 0 signifies the first endpoint that is
	* associated with the interface, a value of 1 signifies the second
	* endpoint, and so on. PipeIndex must be less than the value in the
	* bNumEndpoints field of the interface descriptor.
	*
	* \param PipeInformation
	* A pointer, on output, to a caller-allocated \ref WINUSB_PIPE_INFORMATION
	* structure that contains pipe information.
	*
	* The \ref UsbK_QueryPipe function does not retrieve information about the
	* control pipe.
	*
	* Each interface on the USB device can have multiple endpoints. To
	* communicate with each of these endpoints, the bus driver creates pipes
	* for each endpoint on the interface. The pipe indices are zero-based.
	* Therefore for n number of endpoints, the pipes' indices are set from
	* n-1. \ref UsbK_QueryPipe parses the configuration descriptor to get the
	* interface specified by the caller. It searches the interface descriptor
	* for the endpoint descriptor associated with the caller-specified pipe.
	* If the endpoint is found, the function populates the caller-allocated
	* \ref WINUSB_PIPE_INFORMATION structure with information from the endpoint
	* descriptor.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_QueryPipe (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR AlternateSettingNumber,
	    __in UCHAR PipeIndex,
	    __out PWINUSB_PIPE_INFORMATION PipeInformation);

//! Sets the policy for a specific pipe associated with an endpoint on the device. This is a synchronous operation.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param PipeID
	* An 8-bit value that consists of a 7-bit address and a direction bit.
	* This parameter corresponds to the bEndpointAddress field in the endpoint
	* descriptor.
	*
	* \param PolicyType
	*  A ULONG variable that specifies the policy parameter to change. The
	* Value parameter contains the new value for the policy parameter.
	* See the remarks section for information about each of the pipe policies
	* and the resulting behavior.
	*
	*
	* \param ValueLength
	* The size, in bytes, of the buffer at Value.
	*
	* \param Value
	* The new value for the policy parameter that PolicyType specifies. The
	* size of this input parameter depends on the policy to change. For
	* information about the size of this parameter, see the description of the
	* PolicyType parameter.
	*
	* \remarks
	* The following list describes symbolic constants that are defined in \ref lusbk_usbio.h
	*
	* - \c SHORT_PACKET_TERMINATE (0x01)
	*   - The default value is \c FALSE.
	*   - To enable \c SHORT_PACKET_TERMINATE, in Value pass the address of a
	*     caller-allocated \c UCHAR variable set to \c TRUE (nonzero).
	*   - Enabling \c SHORT_PACKET_TERMINATE causes the driver to send a zero-length
	*     packet at the end of every write request to the host controller.
	*
	* - \c AUTO_CLEAR_STALL (0x02)
	*   - The default value is \c FALSE. To enable \c AUTO_CLEAR_STALL, in Value pass
	*     the address of a caller-allocated \c UCHAR variable set to \c TRUE (nonzero).
	*   - Enabling \c AUTO_CLEAR_STALL causes libusbK to reset the pipe in order to
	*     automatically clear the stall condition. Data continues to flow on the
	*     bulk and interrupt \c IN endpoints again as soon as a new or a queued
	*     transfer arrives on the endpoint. This policy parameter does not affect
	*     control pipes.
	*   - Disabling \c AUTO_CLEAR_STALL causes all transfers (that arrive to the
	*     endpoint after the stalled transfer) to fail until the caller manually
	*     resets the endpoint's pipe by calling \ref UsbK_ResetPipe.
	*
	* - \c PIPE_TRANSFER_TIMEOUT (0x03)
	*   - The default value is zero. To set a time-out value, in Value pass the
	*     address of a caller-allocated \c ULONG variable that contains the time-out
	*     interval.
	*   - The \c PIPE_TRANSFER_TIMEOUT value specifies the time-out interval, in
	*     milliseconds. The host controller cancels transfers that do not complete
	*     within the specified time-out interval.
	*   - A value of zero (default) indicates that transfers do not time out
	*     because the host controller never cancels the transfer.
	*
	* - \c IGNORE_SHORT_PACKETS (0x04)
	*   - The default value is \c FALSE. To enable \c IGNORE_SHORT_PACKETS, in Value
	*     pass the address of a caller-allocated \c UCHAR variable set to \c TRUE
	*     (nonzero).
	*   - Enabling \c IGNORE_SHORT_PACKETS causes the host controller to not complete
	*     a read operation after it receives a short packet. Instead, the host
	*     controller completes the operation only after the host has read the
	*     specified number of bytes.
	*   - Disabling \c IGNORE_SHORT_PACKETS causes the host controller to complete a
	*     read operation when either the host has read the specified number of
	*     bytes or the host has received a short packet.
	*
	* - \c ALLOW_PARTIAL_READS (0x05)
	*   - The default value is \c TRUE (nonzero). To disable \c ALLOW_PARTIAL_READS, in
	*     Value pass the address of a caller-allocated \c UCHAR variable set to \c FALSE
	*     (zero).
	*   - Disabling \c ALLOW_PARTIAL_READS causes the read requests to fail whenever
	*     the device returns more data (on bulk and interrupt \c IN endpoints) than
	*     the caller requested.
	*   - Enabling \c ALLOW_PARTIAL_READS causes libusbK to save or discard the extra
	*     data when the device returns more data (on bulk and interrupt \c IN
	*     endpoints) than the caller requested. This behavior is defined by
	*     setting the \c AUTO_FLUSH value.
	*
	* - \c AUTO_FLUSH (0x06)
	*   - The default value is \c FALSE (zero). To enable \c AUTO_FLUSH, in Value pass
	*     the address of a caller-allocated \c UCHAR variable set to \c TRUE (nonzero).
	*   - \c AUTO_FLUSH must be used with \c ALLOW_PARTIAL_READS enabled. If
	*     \c ALLOW_PARTIAL_READS is \c TRUE, the value of \c AUTO_FLUSH determines the
	*     action taken by libusbK when the device returns more data than the caller
	*     requested.
	*   - Disabling \c ALLOW_PARTIAL_READS causes libusbK to ignore the \c AUTO_FLUSH
	*     value.
	*   - Disabling \c AUTO_FLUSH with \c ALLOW_PARTIAL_READS enabled causes libusbK to
	*     save the extra data, add the data to the beginning of the caller's next
	*     read request, and send it to the caller in the next read operation.
	*   - Enabling \c AUTO_FLUSH with \c ALLOW_PARTIAL_READS enabled causes libusbK to
	*     discard the extra data remaining from the read request.
	*
	* - \c RAW_IO (0x07)
	*   - The default value is \c FALSE (zero). To enable \c RAW_IO, in Value pass the
	*     address of a caller-allocated \c UCHAR variable set to \c TRUE (nonzero).
	*   - Enabling \c RAW_IO causes libusbK to send data directly to the \c USB driver
	*     stack, bypassing libusbK's queuing and error handling mechanism.
	*   - The buffers that are passed to \ref UsbK_ReadPipe must be configured by the
	*     caller as follows:
	*     - The buffer length must be a multiple of the maximum endpoint packet size.
	*     - The length must be less than or equal to the value of
	*       \c MAXIMUM_TRANSFER_SIZE retrieved by \ref UsbK_GetPipePolicy.
	*   - Disabling \c RAW_IO (\c FALSE) does not impose any restriction on the buffers
	*     that are passed to \ref UsbK_ReadPipe.
	*
	* - \c RESET_PIPE_ON_RESUME (0x09)
	*   - The default value is \c FALSE (zero). To enable \c RESET_PIPE_ON_RESUME, in
	*     Value pass the address of a caller-allocated \c UCHAR variable set to \c TRUE
	*     (nonzero).
	*   - \c TRUE (or a nonzero value) indicates that on resume from suspend, libusbK
	*     resets the endpoint before it allows the caller to send new requests to
	*     the endpoint.
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
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param PipeID
	* An 8-bit value that consists of a 7-bit address and a direction bit.
	* This parameter corresponds to the bEndpointAddress field in the endpoint
	* descriptor.
	*
	* \param PolicyType
	* A ULONG variable that specifies the policy parameter to retrieve. The
	* current value for the policy parameter is retrieved the Value parameter.
	*
	* \param ValueLength
	* A pointer to the size, in bytes, of the buffer that Value points to. On
	* output, ValueLength receives the size, in bytes, of the data that was
	* copied into the Value buffer.
	*
	* \param Value
	* A pointer to a buffer that receives the specified pipe policy value.
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
	* \param PipeID
	* An 8-bit value that consists of a 7-bit address and a direction bit.
	* This parameter corresponds to the bEndpointAddress field in the endpoint
	* descriptor.
	*
	* \param Buffer
	* A caller-allocated buffer that receives the data that is read.
	*
	* \param BufferLength
	* The maximum number of bytes to read. This number must be less than or
	* equal to the size, in bytes, of Buffer.
	*
	* \param LengthTransferred
	* A pointer to a ULONG variable that receives the actual number of bytes
	* that were copied into Buffer. For more information, see Remarks.
	*
	* \param Overlapped
	* An optional pointer to an overlapped structure for asynchronous
	* operations. This can be a \ref POVERLAPPED_K or a pointer to a standard
	* windows OVERLAPPED structure. If this parameter is specified, \c
	* UsbK_ReadPipe returns immediately rather than waiting synchronously for
	* the operation to complete before returning. An event is signaled when
	* the operation is complete.
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
	* \param PipeID
	* An 8-bit value that consists of a 7-bit address and a direction bit.
	* This parameter corresponds to the bEndpointAddress field in the endpoint
	* descriptor.
	*
	* \param Buffer
	* A caller-allocated buffer that receives the data that is read.
	*
	* \param BufferLength
	* The maximum number of bytes to write. This number must be less than or
	* equal to the size, in bytes, of Buffer.
	*
	* \param LengthTransferred
	* A pointer to a ULONG variable that receives the actual number of bytes
	* that were transferred from Buffer. For more information, see Remarks.
	*
	* \param Overlapped
	* An optional pointer to an overlapped structure for asynchronous
	* operations. This can be a \ref POVERLAPPED_K or a pointer to a standard
	* windows OVERLAPPED structure. If this parameter is specified, \c
	* UsbK_WritePipe returns immediately rather than waiting synchronously for
	* the operation to complete before returning. An event is signaled when
	* the operation is complete.
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

//! Resets the data toggle and clears the stall condition on a pipe.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param PipeID
	* An 8-bit value that consists of a 7-bit address and a direction bit.
	* This parameter corresponds to the bEndpointAddress field in the endpoint
	* descriptor.
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
	* \param PipeID
	* An 8-bit value that consists of a 7-bit address and a direction bit.
	* This parameter corresponds to the bEndpointAddress field in the endpoint
	* descriptor.
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
	*
	* \param PipeID
	* An 8-bit value that consists of a 7-bit address and a direction bit.
	* This parameter corresponds to the bEndpointAddress field in the endpoint
	* descriptor.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_FlushPipe (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in UCHAR PipeID);

//! Reads from an isochronous pipe.
	/*!
	*
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param PipeID
	* An 8-bit value that consists of a 7-bit address and a direction bit.
	* This parameter corresponds to the bEndpointAddress field in the endpoint
	* descriptor.
	*
	* \param Buffer
	* A caller-allocated buffer that receives the data that is read.
	*
	* \param BufferLength
	* The maximum number of bytes to read. This number must be less than or
	* equal to the size, in bytes, of Buffer.
	*
	* \param IsoPacketSize
	* Data size in bytes for each iso packet.
	*
	* \param Overlapped
	* A \b required pointer to an overlapped structure for asynchronous
	* operations. This can be a \ref POVERLAPPED_K or a pointer to a standard
	* windows OVERLAPPED structure. If this parameter is specified, \c
	* UsbK_IsoReadPipe returns immediately rather than waiting synchronously for
	* the operation to complete before returning. An event is signaled when
	* the operation is complete.
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
	* \param PipeID
	* An 8-bit value that consists of a 7-bit address and a direction bit.
	* This parameter corresponds to the bEndpointAddress field in the endpoint
	* descriptor.
	*
	* \param Buffer
	* A caller-allocated buffer that receives the data that is read.
	*
	* \param BufferLength
	* The maximum number of bytes to write. This number must be less than or
	* equal to the size, in bytes, of Buffer.
	*
	* \param IsoPacketSize
	* Data size in bytes for each iso packet.
	*
	* \param Overlapped
	* An optional pointer to an overlapped structure for asynchronous
	* operations. This can be a \ref POVERLAPPED_K or a pointer to a standard
	* windows OVERLAPPED structure. If this parameter is specified, \c
	* UsbK_IsoWritePipe returns immediately rather than waiting synchronously for
	* the operation to complete before returning. An event is signaled when
	* the operation is complete.
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

//! Retrieves the results of an overlapped operation on the specified libusbK handle.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param lpOverlapped
	* A pointer to a standard windows OVERLAPPED structure that was specified
	* when the overlapped operation was started.
	*
	* \param lpNumberOfBytesTransferred
	* A pointer to a variable that receives the number of bytes that were
	* actually transferred by a read or write operation.
	*
	* \param bWait
	* If this parameter is TRUE, the function does not return until the
	* operation has been completed. If this parameter is FALSE and the
	* operation is still pending, the function returns FALSE and the
	* GetLastError function returns ERROR_IO_INCOMPLETE.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* \remarks
	* This function is like the Win32 API routine, GetOverlappedResult, with
	* one difference; instead of passing a file handle that is returned from
	* CreateFile, the caller passes an interface handle that is returned from
	* \ref UsbK_Initialize, \ref UsbK_Open, or \ref UsbK_GetAssociatedInterface.
	* The caller can use either API routine, if the
	* appropriate handle is passed. The \ref UsbK_GetOverlappedResult function
	* extracts the file handle from the interface handle and then calls
	* GetOverlappedResult. \n
	*
	* \remarks
	* \par
	* The results that are reported by the \ref UsbK_GetOverlappedResult
	* function are those from the specified handle's last overlapped operation
	* to which the specified standard windows OVERLAPPED structure was
	* provided, and for which the operation's results were pending. A pending
	* operation is indicated when the function that started the operation
	* returns FALSE, and the GetLastError routine returns ERROR_IO_PENDING.
	* When an I/O operation is pending, the function that started the
	* operation resets the hEvent member of the standard windows OVERLAPPED
	* structure to the nonsignaled state. Then when the pending operation has
	* been completed, the system sets the event object to the signaled state. \n
	*
	* \par
	* The caller can specify that an event object is manually reset in the
	* standard windows OVERLAPPED structure. If an automatic reset event
	* object is used, the event handle must not be specified in any other wait
	* operation in the interval between starting the overlapped operation and
	* the call to \ref UsbK_GetOverlappedResult. For example, the event object
	* is sometimes specified in one of the wait routines to wait for the
	* operation to be completed. When the wait routine returns, the system
	* sets an auto-reset event's state to nonsignaled, and a successive call
	* to \ref UsbK_GetOverlappedResult with the bWait parameter set to TRUE
	* causes the function to be blocked indefinitely.
	*
	* \par
	* If the bWait parameter is TRUE, \ref UsbK_GetOverlappedResult determines
	* whether the pending operation has been completed by waiting for the
	* event object to be in the signaled state.
	*
	* \par
	* If the hEvent member of the standard windows OVERLAPPED structure is
	* NULL, the system uses the state of the file handle to signal when the
	* operation has been completed. Do not use file handles for this purpose.
	* It is better to use an event object because of the confusion that can
	* occur when multiple concurrent overlapped operations are performed on
	* the same file. In this situation, you cannot know which operation caused
	* the state of the object to be signaled.
	*
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetOverlappedResult (
	    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
	    __in LPOVERLAPPED lpOverlapped,
	    __out LPDWORD lpNumberOfBytesTransferred,
	    __in BOOL bWait);

	/*! @} */

#ifdef __cplusplus
}
#endif

#endif // _LUSBK_USB_H__
