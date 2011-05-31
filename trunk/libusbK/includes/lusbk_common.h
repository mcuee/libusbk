/*! \file lusbk_common.h
* \brief General structs, typedefs, enums, defines, and functions used with the libusbK library.
*
* \note
* The libusbK library provides a subset of general usb/descriptor types.
* <BR>
* The general usb/descriptor types are also available as part of the WDK.
* In order to maximum compatiblity with all \ref usbk_drivers, the
* usb/descriptor types defined in libusbK are \b identical to those found
* in Microsoft WDK distributions.
*
* \attention
* If the subset of libusbK usb/descriptor types are inadequate, include the Microsoft WDK
* \c usb.h \b before including any libusbk header files.
*
*/

#ifndef __LUSBK_COMMON_H
#define __LUSBK_COMMON_H

#include <windows.h>
#include <stddef.h>
#include <objbase.h>


#include <specstrings.h>

#ifndef __deref_inout

#define __in				SAL__in
#define __in_opt			SAL__in_opt
#define __deref_in			SAL__deref_in

#define __out				SAL__out
#define __out_opt			SAL__out_opt
#define __deref_out			SAL__deref_out

#define __inout				SAL__inout
#define __inout_opt			SAL__inout_opt
#define __deref_inout		SAL__deref_inout

#endif

/*! \addtogroup genk
*  @{
*/

/*! \struct OVERLAPPED
* \brief
* A standard Windows \c OVERLAPPED structure.
* see \htmlonly
* <A href="http://msdn.microsoft.com/en-us/library/ms684342%28v=vs.85%29.aspx">OVERLAPPED Structure</A>
* \endhtmlonly
*/

/*! \typedef OVERLAPPED* LPOVERLAPPED
* \brief Pointer to a standard Windows \ref OVERLAPPED structure.
*/

//! Indicates the size in bytes of a \ref KUSB_USER_CONTEXT structure.
#define KUSB_CONTEXT_SIZE 32

typedef INT_PTR (FAR WINAPI* KPROC)();

//! Indicates that a function is an exported API call.
#if defined(DYNAMIC_DLL)
#define KUSB_EXP
#else
#define KUSB_EXP
#endif

//! Indicates the calling convention. This is always WINAPI (stdcall) by default.
#if !defined(KUSB_API)
#define KUSB_API WINAPI
#endif

#pragma warning(disable:4201)
#include <pshpack1.h>

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning (disable:4201)
#pragma warning(disable:4214) // named type definition in parentheses

//! Represents 32 bytes of user defined storage space.
/*!
* Most libusbK device objects contain additional stoarge space
*/
typedef struct _KUSB_USER_CONTEXT
{
	union
	{
		UCHAR		Byte[KUSB_CONTEXT_SIZE];
		CHAR		Char[KUSB_CONTEXT_SIZE];
		USHORT		Word[KUSB_CONTEXT_SIZE / sizeof(short)];
		SHORT		Short[KUSB_CONTEXT_SIZE / sizeof(short)];

		ULONG		ULong[KUSB_CONTEXT_SIZE / sizeof(int)];
		LONG		Long[KUSB_CONTEXT_SIZE / sizeof(int)];

		ULONG_PTR	Ptr[KUSB_CONTEXT_SIZE / sizeof(__int64)];
	};
} KUSB_USER_CONTEXT;
//! pointer to a \c KUSB_USER_CONTEXT
typedef KUSB_USER_CONTEXT* PKUSB_USER_CONTEXT;

typedef union _KUSB_BM_REQUEST_TYPE
{
	struct _BM
	{
		UCHAR   Recipient: 2;
		UCHAR   Reserved: 3;
		UCHAR   Type: 2;
		UCHAR   Dir: 1;
	} BM;
	UCHAR B;
} KUSB_BM_REQUEST_TYPE;
//! pointer to a \c KUSB_BM_REQUEST_TYPE
typedef KUSB_BM_REQUEST_TYPE* PKUSB_BM_REQUEST_TYPE;

//! USB control setup packet
/*!
* This structure is identical in size to a \ref WINUSB_SETUP_PACKET,
* but provides additional user friendly members.
*/
typedef struct _KUSB_SETUP_PACKET
{
	//! Request value
	KUSB_BM_REQUEST_TYPE bmRequestType;


	//! Request type value
	UCHAR bRequest;

	//! Unionized wValue
	union _wValue
	{
		//! wValue structure
		struct
		{
			//! Low byte of \c wValue
			UCHAR LowByte;
			//! High byte of \c wValue
			UCHAR HiByte;
		};

		//! wValue ushort value
		USHORT W;
	} wValue;

	//! Unionized wIndex
	union _wIndex
	{
		//! wIndex structure
		struct
		{
			//! Low byte of \c wIndex
			UCHAR LowByte;
			//! High byte of \c wIndex
			UCHAR HiByte;
		};

		//! wIndex ushort value
		USHORT W;
	} wIndex;

	//! wLength ushort value
	USHORT wLength;

} KUSB_SETUP_PACKET;
//! pointer to a \c KUSB_SETUP_PACKET
typedef KUSB_SETUP_PACKET* PKUSB_SETUP_PACKET;
// setup packet is eight bytes -- defined by spec
C_ASSERT(sizeof(KUSB_SETUP_PACKET) == 8);

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#include <poppack.h>

//! libusbK interfacing handle.
/*!
* This handle is required by other libusbK routines that perform
* operations on a device. The handle is opaque. To release this handle,
* call the \ref UsbK_Close or \ref UsbK_Free function.
*
* A \ref LIBUSBK_INTERFACE_HANDLE differs from a \c
* WINUSB_INTERFACE_HANDLE in the following ways:
* - libusbK handles have no "master" handle. All libusbK handles can operate
*   as a master handles. This means all handles work with all API functions.
*
* - libusbK handles encompass then entire set of interfaces available to a
*   usb device file handle. For most users there is no need to use the \ref
*   UsbK_GetAssociatedInterface function. Instead, they can use the \ref
*   UsbK_ClaimInterface and \ref UsbK_ReleaseInterface functions. libusbK
*   will maintain all interface settings on one (1) handle.
*
* - Legacy WinUSB functions (all are supported by the libusbK api) do
*   not provide an interface number/index parameter. (e.g. \ref
*   UsbK_QueryInterfaceSettings, \ref UsbK_SetCurrentAlternateSetting, etc.)
*   libusbK deals with these functions as follows:
*   - The most recent interface claimed with \ref UsbK_ClaimInterface becomes
*     the default interface.
*   - When \ref UsbK_ReleaseInterface is called the previously claimed
*     interface becomes the default interface.
*   - \ref UsbK_GetAssociatedInterface clones the source libusbK handle and
*     changes the default interface of the clones handle to the index
*     specified by the user.
*
*/
typedef PVOID LIBUSBK_INTERFACE_HANDLE;

//! Refrence to a \ref LIBUSBK_INTERFACE_HANDLE
typedef LIBUSBK_INTERFACE_HANDLE* PLIBUSBK_INTERFACE_HANDLE;

#ifndef   __USB_H__

#include <PSHPACK1.H>

//! bmRequest.Dir
enum BMREQUEST_DIR_ENUM
{
	BMREQUEST_HOST_TO_DEVICE = 0,
	BMREQUEST_DEVICE_TO_HOST = 1,
};

//! bmRequest.Type
enum BMREQUEST_TYPE_ENUM
{
	//! Standard request. See \ref USB_REQUEST_ENUM
	BMREQUEST_STANDARD = 0,

	//! Class-specific request.
	BMREQUEST_CLASS = 1,

	//! Vendor-specific request
	BMREQUEST_VENDOR = 2,
};

//! bmRequest.Recipient
enum BMREQUEST_RECIPIENT_ENUM
{
	//! Request is for a device.
	BMREQUEST_TO_DEVICE = 0,

	//! Request is for an interface of a device.
	BMREQUEST_TO_INTERFACE = 1,

	//! Request is for an endpoint of a device.
	BMREQUEST_TO_ENDPOINT = 2,

	//! Request is for a vendor-specific purpose.
	BMREQUEST_TO_OTHER = 3,
};

//! Maximum length (in bytes) of a usb string. USB strings are always stored in wide-char format.
#define MAXIMUM_USB_STRING_LENGTH 255

//! Values for the bits returned by the \ref USB_REQUEST_GET_STATUS request.
enum USB_GETSTATUS_ENUM
{
	//! Device is self powered
	USB_GETSTATUS_SELF_POWERED = 0x01,

	//! Device can wake the system from a low power/sleeping state.
	USB_GETSTATUS_REMOTE_WAKEUP_ENABLED = 0x02
};

//! Standard USB descriptor types. For more information, see section 9-5 of the USB 3.0 specifications.
enum USB_DESCRIPTOR_TYPE_ENUM
{
	//! Device descriptor type.
	USB_DEVICE_DESCRIPTOR_TYPE = 0x01,

	//! Configuration descriptor type.
	USB_CONFIGURATION_DESCRIPTOR_TYPE = 0x02,

	//! String descriptor type.
	USB_STRING_DESCRIPTOR_TYPE = 0x03,

	//! Interface descriptor type.
	USB_INTERFACE_DESCRIPTOR_TYPE = 0x04,

	//! Endpoint descriptor type.
	USB_ENDPOINT_DESCRIPTOR_TYPE = 0x05,

	//! Device qualifier descriptor type.
	USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE = 0x06,

	//! Config power descriptor type.
	USB_CONFIG_POWER_DESCRIPTOR_TYPE = 0x07,

	//! Interface power descriptor type.
	USB_INTERFACE_POWER_DESCRIPTOR_TYPE = 0x08,

	//! Interface association descriptor type.
	USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE = 0x0B,
};

//! Makes the \ref KUSB_SETUP_PACKET::_wValue for a \ref USB_REQUEST_GET_DESCRIPTOR or \ref USB_REQUEST_SET_DESCRIPTOR request.
#define USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(d, i)	\
	((USHORT)((USHORT)d<<8 | i))

//! Endpoint type mask for the \c bmAttributes field of a \ref USB_ENDPOINT_DESCRIPTOR
#define USB_ENDPOINT_TYPE_MASK                    0x03

//! Indicates a control endpoint
#define USB_ENDPOINT_TYPE_CONTROL                 0x00

//! Indicates an isochronous endpoint
#define USB_ENDPOINT_TYPE_ISOCHRONOUS             0x01

//! Indicates a bulk endpoint
#define USB_ENDPOINT_TYPE_BULK                    0x02

//! Indicates an interrupt endpoint
#define USB_ENDPOINT_TYPE_INTERRUPT               0x03

//! Values used in the \c bmAttributes field of a \ref USB_ENDPOINT_DESCRIPTOR
typedef enum _USBD_PIPE_TYPE
{
	//! Indicates a control endpoint
	UsbdPipeTypeControl,

	//! Indicates an isochronous endpoint
	UsbdPipeTypeIsochronous,

	//! Indicates a bulk endpoint
	UsbdPipeTypeBulk,

	//! Indicates an interrupt endpoint
	UsbdPipeTypeInterrupt,
} USBD_PIPE_TYPE;

//! Config power mask for the \c bmAttributes field of a \ref USB_CONFIGURATION_DESCRIPTOR
#define USB_CONFIG_POWERED_MASK                   0xc0

//! Values used in the \c bmAttributes field of a \ref USB_CONFIGURATION_DESCRIPTOR
enum USB_CONFIG_BM_ATTRIBUTE_ENUM
{
	//! The device is powered by it's host.
	USB_CONFIG_BUS_POWERED = 0x80,

	//! The device has an external power source.
	USB_CONFIG_SELF_POWERED = 0x40,

	//! The device is capable of waking the the host from a low power/sleeping state.
	USB_CONFIG_REMOTE_WAKEUP = 0x20,
};

//! Endpoint direction mask for the \c bEndpointAddress field of a \ref USB_ENDPOINT_DESCRIPTOR
#define USB_ENDPOINT_DIRECTION_MASK               0x80


//! Tests the \c bEndpointAddress direction bit. TRUE if the endpoint address is an OUT endpoint. (HostToDevice, PC Write)
/*!
* \param addr \c bEndpointAddress field of a \ref USB_ENDPOINT_DESCRIPTOR
*/
#define USB_ENDPOINT_DIRECTION_OUT(addr)          (!((addr) & USB_ENDPOINT_DIRECTION_MASK))

//!  Tests the \c bEndpointAddress direction bit. TRUE if the endpoint address is an IN endpoint. (DeviceToHost, PC Read)
/*!
* \param addr \c bEndpointAddress field of a \ref USB_ENDPOINT_DESCRIPTOR
*/
#define USB_ENDPOINT_DIRECTION_IN(addr)           ((addr) & USB_ENDPOINT_DIRECTION_MASK)

//! USB defined request codes
/*
* see Chapter 9 of the USB 2.0 specifcation for
* more information.
*
* These are the correct values based on the USB 2.0 specification.
*/
enum USB_REQUEST_ENUM
{
	//! Request status of the specific recipient
	USB_REQUEST_GET_STATUS = 0x00,

	//! Clear or disable a specific feature
	USB_REQUEST_CLEAR_FEATURE = 0x01,

	//! Set or enable a specific feature
	USB_REQUEST_SET_FEATURE = 0x03,

	//! Set device address for all future accesses
	USB_REQUEST_SET_ADDRESS = 0x05,

	//! Get the specified descriptor
	USB_REQUEST_GET_DESCRIPTOR = 0x06,

	//! Update existing descriptors or add new descriptors
	USB_REQUEST_SET_DESCRIPTOR = 0x07,

	//! Get the current device configuration value
	USB_REQUEST_GET_CONFIGURATION = 0x08,

	//! Set device configuration
	USB_REQUEST_SET_CONFIGURATION = 0x09,

	//! Return the selected alternate setting for the specified interface
	USB_REQUEST_GET_INTERFACE = 0x0A,

	//! Select an alternate interface for the specified interface
	USB_REQUEST_SET_INTERFACE = 0x0B,

	//! Set then report an endpoint's synchronization frame
	USB_REQUEST_SYNC_FRAME = 0x0C,
};

//! USB defined class codes
/*!
* see http://www.usb.org/developers/defined_class for more information.
*
*/
enum USB_DEVICE_CLASS_ENUM
{
	//! Reserved class
	USB_DEVICE_CLASS_RESERVED = 0x00,

	//! Audio class
	USB_DEVICE_CLASS_AUDIO = 0x01,

	//! Communications class
	USB_DEVICE_CLASS_COMMUNICATIONS = 0x02,

	//! Human Interface Device class
	USB_DEVICE_CLASS_HUMAN_INTERFACE = 0x03,

	//! Imaging class
	USB_DEVICE_CLASS_IMAGING = 0x06,

	//! Printer class
	USB_DEVICE_CLASS_PRINTER = 0x07,

	//! Mass storage class
	USB_DEVICE_CLASS_STORAGE = 0x08,

	//! Hub class
	USB_DEVICE_CLASS_HUB = 0x09,

	//! vendor-specific class
	USB_DEVICE_CLASS_VENDOR_SPECIFIC = 0xFF,
};

//! A structure representing the standard USB device descriptor.
/*!
 * This descriptor is documented in section 9.6.1 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
typedef struct _USB_DEVICE_DESCRIPTOR
{
	//! Size of this descriptor (in bytes)
	UCHAR bLength;

	//! Descriptor type
	UCHAR bDescriptorType;

	//! USB specification release number in binary-coded decimal.
	/*!
	 * A value of 0x0200 indicates USB 2.0, 0x0110 indicates USB 1.1, etc.
	 */
	USHORT bcdUSB;

	//! USB-IF class code for the device
	UCHAR bDeviceClass;

	//! USB-IF subclass code for the device
	UCHAR bDeviceSubClass;

	//! USB-IF protocol code for the device
	UCHAR bDeviceProtocol;

	//! Maximum packet size for control endpoint 0
	UCHAR bMaxPacketSize0;

	//! USB-IF vendor ID
	USHORT idVendor;

	//! USB-IF product ID
	USHORT idProduct;

	//! Device release number in binary-coded decimal
	USHORT bcdDevice;

	//! Index of string descriptor describing manufacturer
	UCHAR iManufacturer;

	//! Index of string descriptor describing product
	UCHAR iProduct;

	//! Index of string descriptor containing device serial number
	UCHAR iSerialNumber;

	//! Number of possible configurations
	UCHAR bNumConfigurations;

} USB_DEVICE_DESCRIPTOR;
//! pointer to a \c USB_DEVICE_DESCRIPTOR
typedef USB_DEVICE_DESCRIPTOR* PUSB_DEVICE_DESCRIPTOR;

//! A structure representing the standard USB endpoint descriptor.
/*!
 * This descriptor is documented in section 9.6.3 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
typedef struct _USB_ENDPOINT_DESCRIPTOR
{
	//! Size of this descriptor (in bytes)
	UCHAR bLength;

	//! Descriptor type
	UCHAR bDescriptorType;

	//! The address of the endpoint described by this descriptor.
	/*!
	 * - Bits 0:3 are the endpoint number
	 * - Bits 4:6 are reserved
	 * - Bit 7 indicates direction
	 */
	UCHAR bEndpointAddress;

	//! Attributes which apply to the endpoint when it is configured using the bConfigurationValue.
	/*!
	 * - Bits 0:1 determine the transfer type.
	 * - Bits 2:3 are only used for isochronous endpoints and refer to sync type.
	 * - Bits 4:5 are also only used for isochronous endpoints and refer to usage type.
	 * - Bits 6:7 are reserved.
	 */
	UCHAR bmAttributes;

	//! Maximum packet size this endpoint is capable of sending/receiving.
	USHORT wMaxPacketSize;

	//! Interval for polling endpoint for data transfers.
	UCHAR bInterval;

} USB_ENDPOINT_DESCRIPTOR;
//! pointer to a \c USB_ENDPOINT_DESCRIPTOR
typedef USB_ENDPOINT_DESCRIPTOR* PUSB_ENDPOINT_DESCRIPTOR;

/** A structure representing the standard USB configuration descriptor.
 * This descriptor is documented in section 9.6.3 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
typedef struct _USB_CONFIGURATION_DESCRIPTOR
{
	//! Size of this descriptor (in bytes)
	UCHAR bLength;

	//! Descriptor type
	UCHAR bDescriptorType;

	//! Total length of data returned for this configuration
	USHORT wTotalLength;

	//! Number of interfaces supported by this configuration
	UCHAR bNumInterfaces;

	//! Identifier value for this configuration
	UCHAR bConfigurationValue;

	//! Index of string descriptor describing this configuration
	UCHAR iConfiguration;

	//! Configuration characteristics
	UCHAR bmAttributes;

	//! Maximum power consumption of the USB device from this bus in this configuration when the device is fully opreation.
	/*!
	 * Expressed in units of 2 mA.
	 */
	UCHAR MaxPower;
} USB_CONFIGURATION_DESCRIPTOR;
//! pointer to a \c USB_CONFIGURATION_DESCRIPTOR
typedef USB_CONFIGURATION_DESCRIPTOR* PUSB_CONFIGURATION_DESCRIPTOR;

//! A structure representing the standard USB interface descriptor.
/*!
 * This descriptor is documented in section 9.6.5 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
typedef struct _USB_INTERFACE_DESCRIPTOR
{
	//! Size of this descriptor (in bytes)
	UCHAR bLength;

	//! Descriptor type
	UCHAR bDescriptorType;

	//! Number of this interface
	UCHAR bInterfaceNumber;

	//! Value used to select this alternate setting for this interface
	UCHAR bAlternateSetting;

	//! Number of endpoints used by this interface (excluding the control endpoint)
	UCHAR bNumEndpoints;

	//! USB-IF class code for this interface
	UCHAR bInterfaceClass;

	//! USB-IF subclass code for this interface
	UCHAR bInterfaceSubClass;

	//! USB-IF protocol code for this interface
	UCHAR bInterfaceProtocol;

	//! Index of string descriptor describing this interface
	UCHAR iInterface;

} USB_INTERFACE_DESCRIPTOR;
//! pointer to a \c USB_INTERFACE_DESCRIPTOR
typedef USB_INTERFACE_DESCRIPTOR* PUSB_INTERFACE_DESCRIPTOR;

//! A structure representing the standard USB string descriptor.
/*!
 * This descriptor is documented in section 9.6.5 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
typedef struct _USB_STRING_DESCRIPTOR
{
	//! Size of this descriptor (in bytes)
	UCHAR bLength;

	//! Descriptor type
	UCHAR bDescriptorType;

	//! Content of the string
	WCHAR bString[1];

} USB_STRING_DESCRIPTOR;
//! pointer to a \c USB_STRING_DESCRIPTOR
typedef USB_STRING_DESCRIPTOR* PUSB_STRING_DESCRIPTOR;

//! A structure representing the common USB descriptor.
typedef struct _USB_COMMON_DESCRIPTOR
{
	//! Size of this descriptor (in bytes)
	UCHAR bLength;

	//! Descriptor type
	UCHAR bDescriptorType;

} USB_COMMON_DESCRIPTOR;
//! pointer to a \c USB_COMMON_DESCRIPTOR
typedef USB_COMMON_DESCRIPTOR* PUSB_COMMON_DESCRIPTOR;

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning (disable:4201)
#pragma warning(disable:4214) // named type definition in parentheses

//! The ECN specifies a USB descriptor, called the Interface Association
//! Descriptor (IAD), that allows hardware manufacturers to define groupings
//! of interfaces.
/*!
* The Universal Serial Bus Specification, revision 2.0, does not support
* grouping more than one interface of a composite device within a single
* function. However, the USB Device Working Group (DWG) created USB device
* classes that allow for functions with multiple interfaces, and the USB
* Implementor's Forum issued an Engineering Change Notification (ECN) that
* defines a mechanism for grouping interfaces.
*/
typedef struct _USB_INTERFACE_ASSOCIATION_DESCRIPTOR
{
	//! Size of this descriptor (in bytes)
	UCHAR   bLength;

	//! Descriptor type
	UCHAR   bDescriptorType;

	//! First interface number of the set of interfaces that follow this descriptor
	UCHAR   bFirstInterface;

	//! The Number of interfaces follow this descriptor that are considered "associated"
	UCHAR   bInterfaceCount;

	//! \c bInterfaceClass used for this associated interfaces
	UCHAR   bFunctionClass;

	//! \c bInterfaceSubClass used for the associated interfaces
	UCHAR   bFunctionSubClass;

	//! \c bInterfaceProtocol used for the associated interfaces
	UCHAR   bFunctionProtocol;

	//! Index of string descriptor describing the associated interfaces
	UCHAR   iFunction;

} USB_INTERFACE_ASSOCIATION_DESCRIPTOR;
//! pointer to a \c USB_INTERFACE_ASSOCIATION_DESCRIPTOR
typedef USB_INTERFACE_ASSOCIATION_DESCRIPTOR* PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR;

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#include <POPPACK.H>

#if _MSC_VER >= 1200
#pragma warning(push)
#pragma warning(disable:4201)
#endif

// Microsoft OS Descriptor APIs
// supported in windows XP and later

#define OS_STRING_DESCRIPTOR_INDEX                  0xEE

#define MS_GENRE_DESCRIPTOR_INDEX                   0x0001
#define MS_POWER_DESCRIPTOR_INDEX                   0x0002

#define MS_OS_STRING_SIGNATURE                      L"MSFT100"

#define MS_OS_FLAGS_CONTAINERID                     0x02

typedef struct _OS_STRING
{
	UCHAR bLength;
	UCHAR bDescriptorType;
	WCHAR MicrosoftString[7];
	UCHAR bVendorCode;
	union
	{
		UCHAR bPad;
		UCHAR bFlags;
	};
} OS_STRING;
//! pointer to a \c OS_STRING
typedef OS_STRING* POS_STRING;

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#endif // __USB_H__

/*! @} */

#endif // __LUSBK_COMMON_H

