/*! \file libusbk.h
* \brief functions for usb device communication.
*
* \note
* This is the \b main libusbK USB user include file.
*/

#ifndef _LIBUSBK_H__
#define _LIBUSBK_H__

#include "lusbk_shared.h"

#ifndef __deref_inout

#define __in
#define __in_opt
#define __deref_in

#define __out
#define __out_opt
#define __deref_out

#define __inout
#define __inout_opt
#define __deref_inout

#define __deref_out_opt

#endif

///////////////////////////////////////////////////////////////////////
// L I B U S B K  PUBLIC STRUCTS, DEFINES, AND ENUMS //////////////////
///////////////////////////////////////////////////////////////////////

#ifndef _LIBUSBK_LIBK_TYPES

/*! \addtogroup libk
* @{
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

//! Indicates the size in bytes of a \ref KLIB_USER_CONTEXT structure.
#define KLIB_CONTEXT_SIZE 8

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

//! Managed user defined storage space.
/*!
* By default, All libusbK handles contain 8 bytes of user-defined storage space.
*
*/
typedef union _KLIB_USER_CONTEXT
{
	PVOID	Custom;
	UINT64	Value;
	UCHAR	ValueBytes[8];
} KLIB_USER_CONTEXT;
C_ASSERT(sizeof(KLIB_USER_CONTEXT) == 8);
typedef KLIB_USER_CONTEXT* PKLIB_USER_CONTEXT;

//! USB control setup packet
/*!
* This structure is identical in size to a \ref WINUSB_SETUP_PACKET,
* but provides additional user friendly members.
*/
typedef union _KUSB_SETUP_PACKET
{
	UCHAR	Bytes[8];
	USHORT	Words[4];
	struct
	{
		//! Request value
		struct
		{
			UCHAR Recipient: 2;
			UCHAR Reserved: 3;
			UCHAR Type: 2;
			UCHAR Dir: 1;
		} BmRequest;

		//! Request type value
		UCHAR Request;

		//! wValue
		USHORT Value;

		//! wIndex
		USHORT Index;

		//! wLength ushort value
		USHORT Length;
	};
	struct
	{
		struct
		{
			UCHAR b0: 1;
			UCHAR b1: 1;
			UCHAR b2: 1;
			UCHAR b3: 1;
			UCHAR b4: 1;
			UCHAR b5: 1;
			UCHAR b6: 1;
			UCHAR b7: 1;
		} BmRequestBits;

		struct
		{
			UCHAR b0: 1;
			UCHAR b1: 1;
			UCHAR b2: 1;
			UCHAR b3: 1;
			UCHAR b4: 1;
			UCHAR b5: 1;
			UCHAR b6: 1;
			UCHAR b7: 1;
		} RequestBits;

		UCHAR ValueLo;
		UCHAR ValueHi;
		UCHAR IndexLo;
		UCHAR IndexHi;
		UCHAR LengthLo;
		UCHAR LengthHi;
	};
} KUSB_SETUP_PACKET;
// setup packet is eight bytes -- defined by spec
C_ASSERT(sizeof(KUSB_SETUP_PACKET) == 8);

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#include <poppack.h>

typedef void* KLIB_HANDLE;
#define DECLARE_KLIB_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
#define DECLARE_KLIB_SEMI_OPAQUE_HANDLE(Pub_Type_Def_Name) typedef Pub_Type_Def_Name*Pub_Type_Def_Name##_HANDLE

//! libusbK interfacing handle.
/*!
* This handle is required by other libusbK routines that perform
* operations on a device. The handle is opaque. To release this handle,
* call the \ref UsbK_Close or \ref UsbK_Free function.
*
* A \ref KUSB_HANDLE differs from a \c
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
DECLARE_KLIB_HANDLE(KUSB_HANDLE);

//! Pointer to a device list handle.
/*!
* \attention This is an opaque pointer.
*/
DECLARE_KLIB_HANDLE(KLST_HANDLE);

//! Pointer to a hot-plug handle.
/*!
* \attention This is an opaque pointer.
*/
DECLARE_KLIB_HANDLE(KHOT_HANDLE);

//! pointer to an OverlappedK structure.
/*!
*
* To acquire an OverlappedK for use, call \ref OvlK_Acquire. To release an
* OverlappedK back to its associated pool, call \ref OvlK_Release.
*
* \remarks
* OverlappedK structures contain a "standard" windows \c OVERLAPPED
* structure as their first member. This makes them compatible with windows
* API function which take a \ref LPOVERLAPPED as a parameter. However,
* in-order to make use of the OverlappedK functions (such as \ref
* OvlK_Wait and \ref OvlK_IsComplete) the \ref KOVL_HANDLE must pass
* through one of the libusbK \ref usbk transfer functions. e.g. \ref
* UsbK_ReadPipe and \ref UsbK_WritePipe
*
*/
DECLARE_KLIB_HANDLE(KOVL_HANDLE);


//! pointer to an OverlappedK pool structure.
/*!
* An OverlappedK pool encompasses an array of OverlappedK structures.
*
*/
DECLARE_KLIB_HANDLE(KOVL_POOL_HANDLE);

typedef enum _KLIB_HANDLE_TYPE
{
    KLIB_HANDLE_TYPE_HOTK,
    KLIB_HANDLE_TYPE_USBK,
    KLIB_HANDLE_TYPE_USBSHAREDK,
    KLIB_HANDLE_TYPE_LSTK,
    KLIB_HANDLE_TYPE_LSTINFOK,
    KLIB_HANDLE_TYPE_OVLK,
    KLIB_HANDLE_TYPE_OVLPOOLK,

    KLIB_HANDLE_TYPE_COUNT
} KLIB_HANDLE_TYPE;

typedef LONG KUSB_API KLIB_INIT_HANDLE_CB (
    __in KLIB_HANDLE Handle,
    __in KLIB_HANDLE_TYPE HandleType,
    __in PKLIB_USER_CONTEXT UserContext);

typedef LONG KUSB_API KLIB_FREE_HANDLE_CB (
    __in KLIB_HANDLE Handle,
    __in KLIB_HANDLE_TYPE HandleType,
    __in PKLIB_USER_CONTEXT UserContext);

/*! @} */

#endif

#ifndef _LIBUSBK_ISOK_TYPES

//! Callback function typedef for \ref IsoK_EnumPackets
typedef BOOL KUSB_API KISO_ENUM_PACKETS_CB (__in ULONG PacketIndex, __inout PKISO_PACKET IsoPacket, __inout PVOID UserContext);
//! Pointer to a \ref KISO_ENUM_PACKETS_CB.
typedef KISO_ENUM_PACKETS_CB* PKISO_ENUM_PACKETS_CB;

#endif

#ifndef _LIBUSBK_LSTK_TYPES

#include <pshpack1.h>

/*! \addtogroup lstk
* @{
*/

//!  Allocated length for all strings in a \ref KLST_DEVINFO structure.
#define KLST_STRING_MAX_LEN 256

typedef enum _KLST_SYNC_FLAG
{
    SYNC_FLAG_NONE				= 0,
    SYNC_FLAG_UNCHANGED			= 1 << 0,
    SYNC_FLAG_ADDED				= 1 << 1,
    SYNC_FLAG_REMOVED			= 1 << 2,
    SYNC_FLAG_CONNECT_CHANGE	= 1 << 3,
    SYNC_FLAG_MASK				= (SYNC_FLAG_CONNECT_CHANGE - 1) | SYNC_FLAG_CONNECT_CHANGE,
} KLST_SYNC_FLAG;

typedef struct _KLST_SYNC_PARAMS
{
	ULONG _ununsed;
} KLST_SYNC_PARAMS;
typedef KLST_SYNC_PARAMS* PKLST_SYNC_PARAMS;

//! Common usb device information structure
typedef struct _KLST_DEV_COMMON_INFO
{
	//! VendorID parsed from \ref KLST_DEVINFO::InstanceID
	UINT Vid;

	//! ProductID parsed from \ref KLST_DEVINFO::InstanceID
	UINT Pid;

	//! Interface number (valid for composite devices only) parsed from \ref KLST_DEVINFO::InstanceID
	UINT MI;

	// An ID that uniquely identifies a USB device.
	CHAR InstanceID[KLST_STRING_MAX_LEN];

} KLST_DEV_COMMON_INFO;
//! Pointer to a \c KLST_DEV_COMMON_INFO structure.
typedef KLST_DEV_COMMON_INFO* PKLST_DEV_COMMON_INFO;

//! USB device information element of a \ref KLST_DEV_LIST collection.
/*!
* Contains information about a USB device retrieved from the windows
*
* \attention This structure is semi-opaque.
*
* All \ref KLST_DEVINFO elements contain a \ref KLIB_USER_CONTEXT.
* This 32 bytes of user context space can be used by you, the developer, for any desired purpose.
*/
typedef struct _KLST_DEVINFO
{
	//! Common usb device information
	KLST_DEV_COMMON_INFO Common;

	//! Driver id this device element is using
	LONG DrvId;

	//! Device interface GUID
	CHAR DeviceInterfaceGUID[KLST_STRING_MAX_LEN];

	//! Device instance ID
	/*!
	* A Device instance ID has the following format:
	* [enumerator]\[enumerator-specific-device-ID]\[instance-specific-ID]
	* - [enumerator]
	*   - For USB device, the enumerator is always \c USB
	* - [enumerator-specific-device-ID]
	*   - Contains the vendor and product id (VID_xxxx&PID_xxxx)
	*   - If present, contains the usbccgp (windows composite device layer) interface number (MI_xx)
	* - [instance-specific-ID]
	*   - If the device is composite, contains a unique interface ID generated by Windows.
	*   - If the device is not composite and has a serial number, contains the devices serial number.
	*   - If the device does not have a serial number, contains a unique ID generated by Windows.
	*/
	CHAR InstanceID[KLST_STRING_MAX_LEN];

	//! Class GUID
	CHAR ClassGUID[KLST_STRING_MAX_LEN];

	//! Manufacturer name as specified in the INF file
	CHAR Mfg[KLST_STRING_MAX_LEN];

	//! Device description as specified in the INF file
	CHAR DeviceDesc[KLST_STRING_MAX_LEN];

	//! Driver service name
	CHAR Service[KLST_STRING_MAX_LEN];

	//! Unique symbolic link identifier
	/*!
	* The \c SymbolicLink can be used to uniquely distinguish between device list elements.
	*/
	CHAR SymbolicLink[KLST_STRING_MAX_LEN];

	//! physical device filename.
	/*!
	* This path is used with the Windows \c CreateFile() function to obtain on opened device handle.
	*/
	CHAR DevicePath[KLST_STRING_MAX_LEN];

	//! libusb-win32 filter index id.
	DWORD LUsb0FilterIndex;

	//! Indicates the devices connection state.
	BOOL Connected;

	union
	{
		struct
		{
			KLST_SYNC_FLAG SyncFlags;
			ULONG UserFlags;
		};
		struct
		{
			unsigned Unchanged: 1;
			unsigned Added: 1;
			unsigned Removed: 1;
			unsigned ConnectChange: 1;
		};
	} SyncResults;
} KLST_DEVINFO;

//! pointer to a \ref KLST_DEVINFO.
//! Pointer to a device information handle.
/*!
* \attention This is a  semi-opaque pointer.
*/
DECLARE_KLIB_SEMI_OPAQUE_HANDLE(KLST_DEVINFO);

//! Initialization parameters for \ref LstK_Init
typedef struct _KLST_INIT_PARAMS
{
	//! Enable listings for the raw device interface GUID.{A5DCBF10-6530-11D2-901F-00C04FB951ED}
	BOOL EnableRawDeviceInterfaceGuid;

	BOOL ShowDisconnectedDevices;

} KLST_INIT_PARAMS, *PKLST_INIT_PARAMS;

#include <POPPACK.H>


//! Enumeration callback typedef (or delegate).
/*!
* Use this typedef as a prototype for an enumeration function in \ref LstK_Enumerate.
* \param DeviceList
* The device list \c DeviceInfo belongs to
*
* \param DeviceInfo
* Device information
*
* \param Context
* User context that was passed into \ref LstK_Enumerate
*
*/
typedef BOOL KUSB_API KLST_ENUM_DEVINFO_CB (
    __in KLST_HANDLE DeviceList,
    __in KLST_DEVINFO_HANDLE DeviceInfo,
    __in PVOID Context);

//! Pointer to a \c KLST_ENUM_DEVINFO_CB
typedef KLST_ENUM_DEVINFO_CB* PKLST_ENUM_DEVINFO_CB;

/*! @} */

#endif

#ifndef   __USB_H__

//! Maximum value that can be added to the current start frame.
#define USBD_ISO_START_FRAME_RANGE 1024

#include <pshpack1.h>

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

//! Endpoint address mask for the \c bEndpointAddress field of a \ref USB_ENDPOINT_DESCRIPTOR
#define USB_ENDPOINT_ADDRESS_MASK 0x0F

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
* see Chapter 9 of the USB 2.0 specification for
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

	//! Maximum power consumption of the USB device from this bus in this configuration when the device is fully operation.
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

#ifndef _LIBUSBK_LIBK_TYPES

/*! \addtogroup libk
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

    //! \ref UsbK_Clone function id
    KUSB_FNID_Clone,

     //! \ref UsbK_SelectInterface function id
    KUSB_FNID_SelectInterface,

   //! Supported function count
    KUSB_FNID_COUNT
} KUSB_FNID;

typedef BOOL KUSB_API KUSB_Initialize (
    __in HANDLE DeviceHandle,
    __out KUSB_HANDLE* InterfaceHandle);

typedef BOOL KUSB_API KUSB_Free (
    __in KUSB_HANDLE InterfaceHandle);

typedef BOOL KUSB_API KUSB_GetAssociatedInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AssociatedInterfaceIndex,
    __out KUSB_HANDLE* AssociatedInterfaceHandle);

typedef BOOL KUSB_API KUSB_GetDescriptor (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR DescriptorType,
    __in UCHAR Index,
    __in USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out PULONG LengthTransferred);

typedef BOOL KUSB_API KUSB_QueryInterfaceSettings (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);

typedef BOOL KUSB_API KUSB_QueryDeviceInformation (
    __in KUSB_HANDLE InterfaceHandle,
    __in ULONG InformationType,
    __inout PULONG BufferLength,
    __out PVOID Buffer);

typedef BOOL KUSB_API KUSB_SetCurrentAlternateSetting (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber);

typedef BOOL KUSB_API KUSB_GetCurrentAlternateSetting (
    __in KUSB_HANDLE InterfaceHandle,
    __out PUCHAR AltSettingNumber);

typedef BOOL KUSB_API KUSB_QueryPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR AltSettingNumber,
    __in UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation);

typedef BOOL KUSB_API KUSB_SetPipePolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value);

typedef BOOL KUSB_API KUSB_GetPipePolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value);

typedef BOOL KUSB_API KUSB_ReadPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped);

typedef BOOL KUSB_API KUSB_WritePipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped);

typedef BOOL KUSB_API KUSB_ControlTransfer (
    __in KUSB_HANDLE InterfaceHandle,
    __in WINUSB_SETUP_PACKET SetupPacket,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped);

typedef BOOL KUSB_API KUSB_ResetPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID);

typedef BOOL KUSB_API KUSB_AbortPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID);

typedef BOOL KUSB_API KUSB_FlushPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR PipeID);

typedef BOOL KUSB_API KUSB_SetPowerPolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value);

typedef BOOL KUSB_API KUSB_GetPowerPolicy (
    __in KUSB_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value);

typedef BOOL KUSB_API KUSB_GetOverlappedResult (
    __in KUSB_HANDLE InterfaceHandle,
    __in LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in BOOL bWait);

typedef BOOL KUSB_API KUSB_ResetDevice (
    __in KUSB_HANDLE InterfaceHandle);

typedef BOOL KUSB_API KUSB_Open (
    __in KLST_DEVINFO* DevInfo,
    __out KUSB_HANDLE* InterfaceHandle);

typedef BOOL KUSB_API KUSB_Close (
    __in KUSB_HANDLE InterfaceHandle);

typedef BOOL KUSB_API KUSB_SetConfiguration (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR ConfigurationNumber);

typedef BOOL KUSB_API KUSB_GetConfiguration (
    __in KUSB_HANDLE InterfaceHandle,
    __out PUCHAR ConfigurationNumber);

typedef BOOL KUSB_API KUSB_ClaimInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex);

typedef BOOL KUSB_API KUSB_ReleaseInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex);

typedef BOOL KUSB_API KUSB_SetAltInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex,
    __in UCHAR AltSettingNumber);

typedef BOOL KUSB_API KUSB_GetAltInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR NumberOrIndex,
    __in BOOL IsIndex,
    __out PUCHAR AltSettingNumber);

typedef BOOL KUSB_API KUSB_IsoReadPipe (
    __in KUSB_HANDLE InterfaceHandle,
    __inout PKISO_CONTEXT IsoContext,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __in LPOVERLAPPED Overlapped);

typedef BOOL KUSB_API KUSB_IsoWritePipe (
    __in KUSB_HANDLE InterfaceHandle,
    __inout PKISO_CONTEXT IsoContext,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __in LPOVERLAPPED Overlapped);

typedef BOOL KUSB_API KUSB_GetCurrentFrameNumber (
    __in KUSB_HANDLE InterfaceHandle,
    __out PULONG FrameNumber);

typedef BOOL KUSB_API KUSB_Clone (
    __in KUSB_HANDLE InterfaceHandle,
    __out KUSB_HANDLE* DstInterfaceHandle);

typedef BOOL KUSB_API KUSB_SelectInterface (
    __in KUSB_HANDLE InterfaceHandle,
    __in UCHAR IndexOrNumber,
    __in BOOL IsIndex);

//! Driver API function set structure.
typedef struct _KUSB_DRIVER_API
{
	BOOL (KUSB_API* Initialize)				(__in HANDLE DeviceHandle, __out KUSB_HANDLE* InterfaceHandle);
	BOOL (KUSB_API* Free)					(__in KUSB_HANDLE InterfaceHandle);
	BOOL (KUSB_API* GetAssociatedInterface)	(__in KUSB_HANDLE InterfaceHandle, __in UCHAR AssociatedInterfaceIndex, __out KUSB_HANDLE* AssociatedInterfaceHandle);
	BOOL (KUSB_API* GetDescriptor)			(__in KUSB_HANDLE InterfaceHandle, __in UCHAR DescriptorType, __in UCHAR Index, __in USHORT LanguageID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out PULONG LengthTransferred);
	BOOL (KUSB_API* QueryInterfaceSettings)	(__in KUSB_HANDLE InterfaceHandle, __in UCHAR AltSettingNumber, __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);
	BOOL (KUSB_API* QueryDeviceInformation)	(__in KUSB_HANDLE InterfaceHandle, __in ULONG InformationType, __inout PULONG BufferLength, __out PVOID Buffer);
	BOOL (KUSB_API* SetCurrentAlternateSetting)	(__in KUSB_HANDLE InterfaceHandle, __in UCHAR AltSettingNumber);
	BOOL (KUSB_API* GetCurrentAlternateSetting)	(__in KUSB_HANDLE InterfaceHandle, __out PUCHAR AltSettingNumber);
	BOOL (KUSB_API* QueryPipe)				(__in KUSB_HANDLE InterfaceHandle, __in UCHAR AltSettingNumber, __in UCHAR PipeIndex, __out PWINUSB_PIPE_INFORMATION PipeInformation);
	BOOL (KUSB_API* SetPipePolicy)			(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	BOOL (KUSB_API* GetPipePolicy)			(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
	BOOL (KUSB_API* ReadPipe)				(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* WritePipe)				(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID, __in PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* ControlTransfer)		(__in KUSB_HANDLE InterfaceHandle, __in WINUSB_SETUP_PACKET SetupPacket, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __out_opt PULONG LengthTransferred, __in_opt LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* ResetPipe)				(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* AbortPipe)				(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* FlushPipe)				(__in KUSB_HANDLE InterfaceHandle, __in UCHAR PipeID);
	BOOL (KUSB_API* SetPowerPolicy)			(__in KUSB_HANDLE InterfaceHandle, __in ULONG PolicyType, __in ULONG ValueLength, __in PVOID Value);
	BOOL (KUSB_API* GetPowerPolicy)			(__in KUSB_HANDLE InterfaceHandle, __in ULONG PolicyType, __inout PULONG ValueLength, __out PVOID Value);
	BOOL (KUSB_API* GetOverlappedResult)	(__in KUSB_HANDLE InterfaceHandle, __in LPOVERLAPPED lpOverlapped, __out LPDWORD lpNumberOfBytesTransferred, __in BOOL bWait);
	BOOL (KUSB_API* ResetDevice)			(__in KUSB_HANDLE InterfaceHandle);
	BOOL (KUSB_API* Open)					(__in KLST_DEVINFO* DevInfo, __out KUSB_HANDLE* InterfaceHandle);
	BOOL (KUSB_API* Close)					(__in KUSB_HANDLE InterfaceHandle);
	BOOL (KUSB_API* SetConfiguration)		(__in KUSB_HANDLE InterfaceHandle, __in UCHAR ConfigurationNumber);
	BOOL (KUSB_API* GetConfiguration)		(__in KUSB_HANDLE InterfaceHandle, __out PUCHAR ConfigurationNumber);
	BOOL (KUSB_API* ClaimInterface)			(__in KUSB_HANDLE InterfaceHandle, __in UCHAR NumberOrIndex, __in BOOL IsIndex);
	BOOL (KUSB_API* ReleaseInterface)		(__in KUSB_HANDLE InterfaceHandle, __in UCHAR NumberOrIndex, __in BOOL IsIndex);
	BOOL (KUSB_API* SetAltInterface)		(__in KUSB_HANDLE InterfaceHandle, __in UCHAR NumberOrIndex, __in BOOL IsIndex, __in UCHAR AltSettingNumber);
	BOOL (KUSB_API* GetAltInterface)		(__in KUSB_HANDLE InterfaceHandle, __in UCHAR NumberOrIndex, __in BOOL IsIndex, __out PUCHAR AltSettingNumber);
	BOOL (KUSB_API* IsoReadPipe)			(__in KUSB_HANDLE InterfaceHandle, __inout PKISO_CONTEXT IsoContext, __out_opt PUCHAR Buffer, __in ULONG BufferLength, __in LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* IsoWritePipe)			(__in KUSB_HANDLE InterfaceHandle, __inout PKISO_CONTEXT IsoContext, __in PUCHAR Buffer, __in ULONG BufferLength, __in LPOVERLAPPED Overlapped);
	BOOL (KUSB_API* GetCurrentFrameNumber)	(__in KUSB_HANDLE InterfaceHandle, __out PULONG FrameNumber);
	BOOL (KUSB_API* Clone)					(__in KUSB_HANDLE InterfaceHandle, __out KUSB_HANDLE* DstInterfaceHandle);
	BOOL (KUSB_API* SelectInterface)		(__in KUSB_HANDLE InterfaceHandle, __out KUSB_HANDLE* DstInterfaceHandle, __in UCHAR NumberOrIndex, __in BOOL IsIndex);
} KUSB_DRIVER_API;

/**@}*/

#endif

#ifndef _LIBUSBK_HOTK_TYPES

/*! \addtogroup hotk
* @{
*/
#include <pshpack1.h>

#define HOTK_PLUG_EVENT VOID

typedef struct _KHOT_PARAMS
{
	PVOID Context;

	HOTK_PLUG_EVENT (KUSB_API* OnHotPlug)(
	    KHOT_HANDLE HotHandle,
	    struct _KHOT_PARAMS* HotParams,
	    KLST_DEVINFO_HANDLE DeviceInfo,
	    KLST_SYNC_FLAG PlugType);

	HWND UserHwnd;
	UINT UserMessage;

	struct
	{
		CHAR InstanceID[KLST_STRING_MAX_LEN];
		CHAR DeviceInterfaceGUID[KLST_STRING_MAX_LEN];
		CHAR DevicePath[KLST_STRING_MAX_LEN];
	} PatternMatch;

	union
	{
		ULONG Value;
		struct
		{
			unsigned PlugAllOnInit: 1;
			unsigned AllowDupeInstanceIDs: 1;
			unsigned PostUserMessage: 1;
		};
	} Flags;

	KLST_DEVINFO_HANDLE MatchedInfo;

} KHOT_PARAMS;

typedef VOID KUSB_API KHOT_PLUG_EVENT(
    KHOT_HANDLE HotHandle,
    KHOT_PARAMS* HotParams,
    KLST_DEVINFO_HANDLE DeviceInfo,
    KLST_SYNC_FLAG PlugType);

#include <poppack.h>

/**@}*/

#endif

#ifndef _LIBUSBK_OVLK_TYPES

/*! \addtogroup ovlk
*  @{
*/

//! \c WaitFlags used by \ref OvlK_Wait.
/*!
*
*/
typedef enum _KOVL_WAIT_FLAGS
{
    //! Do not perform any additional actions upon exiting \ref OvlK_Wait.
    WAIT_FLAGS_NONE							= 0,

    //! If the i/o operation completes successfully, release the OverlappedK back to it's pool.
    WAIT_FLAGS_RELEASE_ON_SUCCESS			= 0x0001,

    //! If the i/o operation fails, release the OverlappedK back to it's pool.
    WAIT_FLAGS_RELEASE_ON_FAIL				= 0x0002,

    //! If the i/o operation fails or completes successfully, release the OverlappedK back to its pool. Perform no actions if it times-out.
    WAIT_FLAGS_RELEASE_ON_SUCCESS_FAIL		= 0x0003,

    //! If the i/o operation times-out cancel it, but do not release the OverlappedK back to its pool.
    WAIT_FLAGS_CANCEL_ON_TIMEOUT			= 0x0004,

    //! If the i/o operation times-out, cancel it and release the OverlappedK back to its pool.
    WAIT_FLAGS_RELEASE_ON_TIMEOUT			= WAIT_FLAGS_CANCEL_ON_TIMEOUT | 0x0008,

    //! Always release the OverlappedK back to its pool.  If the operation timed-out, cancel it before releasing back to its pool.
    WAIT_FLAGS_RELEASE_ALWAYS				= WAIT_FLAGS_RELEASE_ON_SUCCESS_FAIL | WAIT_FLAGS_RELEASE_ON_TIMEOUT,

} KOVL_WAIT_FLAGS;

/**@}*/

#endif

///////////////////////////////////////////////////////////////////////
// L I B U S B K  PUBLIC FUNCTIONS ////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _LIBUSBK_ISOK_FUNCTIONS
	/*! \addtogroup isok
	*  @{
	*/

	//! Creates a new isochronous transfer context.
	/*!
	*
	* \param IsoContext
	* Receives a new isochronous transfer context.
	*
	* \param NumberOfPackets
	* The number of \ref KISO_PACKET structures allocated to \c
	* IsoContext. Assigned to \ref KISO_CONTEXT::NumberOfPackets. The \ref
	* KISO_CONTEXT::NumberOfPackets field is assignable by \c IsoK_Init
	* only and must not be changed by the user.
	*
	* \param PipeID
	* The USB endpoint address assigned to \ref KISO_CONTEXT::PipeID. The
	* driver uses this field to determine which pipe will receive the transfer
	* request. The \ref KISO_CONTEXT::PipeID may be chamged by the user in
	* subsequent request.
	*
	* \param StartFrame
	* The USB frame number this request must start on (or \b 0 for ASAP) and
	* assigned to \ref KISO_CONTEXT::StartFrame. The \ref
	* KISO_CONTEXT::StartFrame may be chamged by the user in subsequent
	* request. For more information, see \ref KISO_CONTEXT::StartFrame.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* \c IsoK_Init is performs the following tasks in order:
	* -# Allocates the \c IsoContext and the required \ref KISO_PACKET structures.
	* -# Zero-initializes all ISO context memory.
	* -# Assigns \b NumberOfPackets, \b PipeID, and \b StartFrame to \c IsoContext.
	*
	*/
	KUSB_EXP BOOL KUSB_API IsoK_Init (
	    __deref_out PKISO_CONTEXT* IsoContext,
	    __in ULONG NumberOfPackets,
	    __in_opt UCHAR PipeID,
	    __in_opt ULONG StartFrame);

	//! Destroys an isochronous transfer context.
	/*!
	* \param IsoContext
	* A pointer to an isochronous transfer context created with \ref IsoK_Init.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*/
	KUSB_EXP BOOL KUSB_API IsoK_Free(
	    __in PKISO_CONTEXT IsoContext);

	//! Convenience function for setting the offset of all ISO packets of an isochronous transfer context.
	/*!
	* \param IsoContext
	* A pointer to an isochronous transfer context.
	*
	* \param PacketSize
	* The packet size used to calculate and assign the absolute data offset for each \ref KISO_PACKET in \c IsoContext.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* \c IsoK_SetPackets updates all \ref KISO_PACKET::Offset fields in a \ref KISO_CONTEXT
	* so all offset are \c PacketSize apart.
	* For example:
	* - The offset of the first  (0-index) packet is 0.
	* - The offset of the second (1-index) packet is PacketSize.
	* - The offset of the third  (2-index) packet is PacketSize*2.
	*
	*/
	KUSB_EXP BOOL KUSB_API IsoK_SetPackets(
	    __inout PKISO_CONTEXT IsoContext,
	    __in ULONG PacketSize);

	//! Convenience function for setting all fields of a \ref KISO_PACKET.
	/*!
	* \param IsoContext
	* A pointer to an isochronous transfer context.
	*
	* \param PacketIndex
	* The packet index to set.
	*
	* \param IsoPacket
	* Pointer to a user allocated \c KISO_PACKET which is copied into
	* the PKISO_CONTEXT::IsoPackets array at the specified index.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*/
	KUSB_EXP BOOL KUSB_API IsoK_SetPacket(
	    __in PKISO_CONTEXT IsoContext,
	    __in ULONG PacketIndex,
	    __in PKISO_PACKET IsoPacket);

	//! Convenience function for getting all fields of a \ref KISO_PACKET.
	/*!
	* \param IsoContext
	* A pointer to an isochronous transfer context.
	*
	* \param PacketIndex
	* The packet index to get.
	*
	* \param IsoPacket
	* Pointer to a user allocated \c KISO_PACKET which receives a copy of
	* the ISO packet in the PKISO_CONTEXT::IsoPackets array at the specified
	* index.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*/
	KUSB_EXP BOOL KUSB_API IsoK_GetPacket(
	    __in PKISO_CONTEXT IsoContext,
	    __in ULONG PacketIndex,
	    __out PKISO_PACKET IsoPacket);

	//! Convenience function for enumerating ISO packets of an isochronous transfer context.
	/*!
	* \param IsoContext
	* A pointer to an isochronous transfer context.
	*
	* \param EnumPackets
	* Pointer to a user supplied callback function which is executed for all ISO packets
	* in \c IsoContext or until the user supplied callback function returns \c FALSE.
	*
	* \param StartPacketIndex
	* The zero-based ISO packet index to begin enumeration at.
	*
	* \param UserContext
	* A user defined value which is passed as a parameter to the user supplied callback function.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*/
	KUSB_EXP BOOL KUSB_API IsoK_EnumPackets(
	    __in PKISO_CONTEXT IsoContext,
	    __in PKISO_ENUM_PACKETS_CB EnumPackets,
	    __in_opt ULONG StartPacketIndex,
	    __in_opt PVOID UserContext);

	//! Convenience function for re-using an isochronous transfer context in a subsequent request.
	/*!
	* \param IsoContext
	* A pointer to an isochronous transfer context.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* \c IsoK_ReUse is performs the following tasks in order:
	* -# Zero-initializes the \b Length and \b Status fields of all \ref KISO_PACKET structures.
	* -# Zero-initializes the \b StartFrame and \b ErrorCount of the \ref KISO_CONTEXT.
	*
	*/
	KUSB_EXP BOOL KUSB_API IsoK_ReUse(
	    __inout PKISO_CONTEXT IsoContext);

	/*! @} */

#endif

#ifndef _LIBUSBK_LIBK_FUNCTIONS
	/*! \addtogroup libk
	* @{
	*/

	KUSB_EXP PKLIB_USER_CONTEXT KUSB_API LibK_GetContext(
	    KLIB_HANDLE Handle,
	    KLIB_HANDLE_TYPE HandleType);

	KUSB_EXP LONG KUSB_API LibK_GetContextSize(
	    KLIB_HANDLE Handle,
	    KLIB_HANDLE_TYPE HandleType);

	KUSB_EXP BOOL KUSB_API LibK_SetContextSize(
	    KLIB_HANDLE Handle,
	    KLIB_HANDLE_TYPE HandleType,
	    ULONG ContextSize);

	KUSB_EXP BOOL KUSB_API LibK_SetHandleCallbacks(
	    KLIB_HANDLE_TYPE HandleType,
	    KLIB_INIT_HANDLE_CB* InitHandleCB,
	    KLIB_FREE_HANDLE_CB* FreeHandleCB);

	KUSB_EXP LONG KUSB_API LibK_GetDefaultContextSize(
	    KLIB_HANDLE_TYPE HandleType);

	KUSB_EXP BOOL KUSB_API LibK_SetDefaultContextSize(
	    KLIB_HANDLE_TYPE HandleType,
	    ULONG ContextSize);

	//! Initialize a driver API set.
	/*!
	*
	* \param DriverAPI
	* A driver API structure to populate.
	*
	* \param DriverID
	* The driver id of the API set to retrieve. See \ref KUSB_DRVID
	*
	* \param SizeofDriverAPI
	* Should always be set to the \b sizeof the driver API struct \ref KUSB_DRIVER_API
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	*/
	KUSB_EXP BOOL KUSB_API LibK_LoadDriverApi(
	    __inout KUSB_DRIVER_API* DriverAPI,
	    __in ULONG DriverID,
	    __in ULONG SizeofDriverAPI);

	//! Initialize a driver API function.
	/*!
	* \param ProcAddress
	* Reference to a function pointer that will receive the API function pointer.
	*
	* \param DriverID
	* The driver id of the API to use. See \ref KUSB_DRVID
	*
	* \param FunctionID
	* The function id. See \ref KUSB_FNID
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	*/
	KUSB_EXP BOOL KUSB_API LibK_GetProcAddress(
	    __out KPROC* ProcAddress,
	    __in ULONG DriverID,
	    __in ULONG FunctionID);

	/**@}*/
#endif

#ifndef _LIBUSBK_USBK_FUNCTIONS
	/*! \addtogroup usbk
	*  @{
	*/

	//! Creates/opens a libusbK interface handle from the device list. This is a preferred method.
	/*!
	*
	* \param DevInfo
	* The device list element to open.<BR>
	* To obtain a \c DevInfo:
	* - Get a list of device elements using \ref LstK_Init.
	* - Use the linked list macros in \ref lusbk_linked_list.h to iterate/search the list for the device element of interest.
	* - Once \c UsbK_Open returns, the device list can be freed at the users discretion.
	*
	* \param InterfaceHandle
	* Receives a handle configured to the first (default) interface on the device.
	* This handle is required by other libusbK routines that perform operations
	* on the default interface. The handle is opaque. To release this handle,
	* call the \ref UsbK_Close function.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* \ref UsbK_Open performs the same tasks as \ref UsbK_Initialize with the following exceptions:
	* - Uses a \ref KLST_DEVINFO instead of a file handle created with the Windows CreateFile() API function.
	* - File handles are managed internally and are closed when the last \ref KUSB_HANDLE is
	*   closed with \ref UsbK_Close.
	* - If \c DevInfo is a composite device, multiple device file handles are managed as one.
	*
	*/
	KUSB_EXP BOOL KUSB_API UsbK_Open (
	    __in KLST_DEVINFO* DevInfo,
	    __out KUSB_HANDLE* InterfaceHandle);

	//! Closes a libusbK interface handle opened by \ref UsbK_Open or \ref UsbK_Initialize. This is a preferred method.
	/*!
	*
	* \param InterfaceHandle
	* Handle to an interface on the device. This handle must be created by a previous call to:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* The \ref UsbK_Close function releases all of the resources that
	* \ref UsbK_Initialize, \ref UsbK_Open, or \ref UsbK_GetAssociatedInterface allocated. This is a synchronous
	* operation.
	*
	* \note \ref UsbK_Close and \ref UsbK_Free perform the same tasks.  The difference is in the return code only.
	* - \ref UsbK_Free always returns TRUE.
	* - \ref UsbK_Close will return FALSE in the hande is already closed/free.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_Close (
	    __in KUSB_HANDLE InterfaceHandle);

	//! Claims the specified interface by number or index.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param NumberOrIndex
	* Interfaces can be claimed or released by a interface index or \c bInterfaceNumber.
	* - Interface indexes always start from 0 and continue sequentially for all interfaces of the device.
	* - An interface number always represents the actual \ref USB_INTERFACE_DESCRIPTOR::bInterfaceNumber.
	*   Interface numbers are not guaranteed to be zero based or sequential.
	*
	* \param IsIndex
	* If TRUE, \c NumberOrIndex represents an interface index.\n
	* if FALSE \c NumberOrIndex represents a \c bInterfaceNumber.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* Claiming an interface allows applications a way to prevent other applications
	* or multiple instances of the same application from using an interface at the same time.
	*
	* When an interface is claimed with \ref UsbK_ClaimInterface it performs the following actions:
	* - Checks if the interface exists. If it does not, returns FALSE and sets last error to ERROR_NO_MORE_ITEMS.
	* - The default (or current) interface for the device is changed to \c NumberOrIndex.
	* - libusb0.sys and libusbK.sys:
	*   - A request to claim the interface is sent to the driver.
	*     If the interface is not claimed or already claimed by the application the request succeeds.
	*     If the interface is claimed by another application, \ref UsbK_ClaimInterface returns FALSE
	*     and sets last error to \c ERROR_BUSY.  In this case the
	*     The default (or current) interface for the device is \b still changed to \c NumberOrIndex.
	* - WinUSB.sys:
	*   All WinUSB device interfaces are claimed when the device is opened.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_ClaimInterface (
	    __in KUSB_HANDLE InterfaceHandle,
	    __in UCHAR NumberOrIndex,
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
	* \param NumberOrIndex
	* Interfaces can be claimed or released by a interface index or \c bInterfaceNumber.
	* - Interface indexes always start from 0 and continue sequentially for all interfaces of the device.
	* - An interface number always represents the actual \ref USB_INTERFACE_DESCRIPTOR::bInterfaceNumber.
	*   Interface numbers are not guaranteed to be zero based or sequential.
	*
	* \param IsIndex
	* If TRUE, \c NumberOrIndex represents an interface index.\n
	* if FALSE \c NumberOrIndex represents a \c bInterfaceNumber.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* When an interface is release with \ref UsbK_ReleaseInterface it performs the following actions:
	* - Checks if the interface exists. If it does not, returns FALSE and sets last error to ERROR_NO_MORE_ITEMS.
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
	*/
	KUSB_EXP BOOL KUSB_API UsbK_ReleaseInterface (
	    __in KUSB_HANDLE InterfaceHandle,
	    __in UCHAR NumberOrIndex,
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
	* \param NumberOrIndex
	* Interfaces can be specified by a interface index or \c bInterfaceNumber.
	* - Interface indexes always start from 0 and continue sequentially for all interfaces of the device.
	* - An interface number always represents the actual \ref USB_INTERFACE_DESCRIPTOR::bInterfaceNumber.
	*   Interface numbers are not guaranteed to be zero based or sequential.
	*
	* \param IsIndex
	* If TRUE, \c NumberOrIndex represents an interface index.\n
	* if FALSE \c NumberOrIndex represents a \c bInterfaceNumber.
	*
	* \param AltSettingNumber
	* The bAlternateSetting to activate.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* \ref UsbK_SetAltInterface performs the same task as \ref UsbK_SetCurrentAlternateSetting except it provides
	* the option of specifying which interfaces alternate setting to activate.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_SetAltInterface (
	    __in KUSB_HANDLE InterfaceHandle,
	    __in UCHAR NumberOrIndex,
	    __in BOOL IsIndex,
	    __in UCHAR AltSettingNumber);

	//! Gets the alternate setting for the specified interface.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param NumberOrIndex
	* Interfaces can be specified by a interface index or \c bInterfaceNumber.
	* - Interface indexes always start from 0 and continue sequentially for all interfaces of the device.
	* - An interface number always represents the actual \ref USB_INTERFACE_DESCRIPTOR::bInterfaceNumber.
	*   Interface numbers are not guaranteed to be zero based or sequential.
	*
	* \param IsIndex
	* If TRUE, \c NumberOrIndex represents an interface index.\n
	* if FALSE \c NumberOrIndex represents a \c bInterfaceNumber.
	*
	* \param AltSettingNumber
	* On success, returns the active bAlternateSetting.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* \ref UsbK_GetAltInterface performs the same task as \ref UsbK_GetCurrentAlternateSetting except it provides
	* the option of specifying which interfaces alternate setting is to be retrieved.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetAltInterface (
	    __in KUSB_HANDLE InterfaceHandle,
	    __in UCHAR NumberOrIndex,
	    __in BOOL IsIndex,
	    __out PUCHAR AltSettingNumber);

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
	*
	* If the device descriptor or active config descriptor is requested,
	* \ref UsbK_GetDescriptor retrieves cached data and this becomes a non-blocking, non I/O request.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetDescriptor (
	    __in KUSB_HANDLE InterfaceHandle,
	    __in UCHAR DescriptorType,
	    __in UCHAR Index,
	    __in USHORT LanguageID,
	    __out_opt PUCHAR Buffer,
	    __in ULONG BufferLength,
	    __out PULONG LengthTransferred);

	//! Transmits control data over a default control endpoint.
	/*!
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
	* A \ref UsbK_ControlTransfer is never cached.  These requests always go directly to the usb device.
	*
	* \attention
	* This function should not be used for operations supported by the library.\n
	* e.g. \ref UsbK_SetConfiguration, \ref UsbK_SetAltInterface, etc..
	*/
	KUSB_EXP BOOL KUSB_API UsbK_ControlTransfer (
	    __in KUSB_HANDLE InterfaceHandle,
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
	* describes symbolic constants that are defined in \ref lusbk_shared.h.
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
	* The new value for the power policy parameter. Data type and value for
	* Value depends on the type of power policy passed in PolicyType. For more
	* information, see PolicyType.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
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
	*/
	KUSB_EXP BOOL KUSB_API UsbK_SetPowerPolicy (
	    __in KUSB_HANDLE InterfaceHandle,
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
	* \ref lusbk_shared.h.
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
	    __in KUSB_HANDLE InterfaceHandle,
	    __in ULONG PolicyType,
	    __inout PULONG ValueLength,
	    __out PVOID Value);

	//! Sets the device configuration number.
	/*!
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
	*
	* \ref UsbK_SetConfiguration is only supported with libusb0.sys.
	* If the driver in not libusb0.sys, this function performs the following emulation actions:
	* - If the requested configuration number is the current configuration number, returns TRUE.
	* - If the requested configuration number is one other than the current configuration number,
	*   returns FALSE and set last error to \c ERROR_NO_MORE_ITEMS.
	*
	* This function will fail if there are pending I/O operations or there are other libusbK interface
	* handles referencing the device.
	* \sa UsbK_Free
	*/
	KUSB_EXP BOOL KUSB_API UsbK_SetConfiguration (
	    __in KUSB_HANDLE InterfaceHandle,
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
	    __in KUSB_HANDLE InterfaceHandle,
	    __out PUCHAR ConfigurationNumber);

	//! Resets the usb device of the specified interface handle. (port cycle).
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
	    __in KUSB_HANDLE InterfaceHandle);

	//! Creates a libusbK handle for the device specified by a file handle.
	/*!
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
	*/
	KUSB_EXP BOOL KUSB_API UsbK_Initialize (
	    __in HANDLE DeviceHandle,
	    __out KUSB_HANDLE* InterfaceHandle);

	//! Frees a libusbK interface handle.
	/*!
	*
	* \param InterfaceHandle
	* Handle to an interface on the device. This handle must be created by a previous call to:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \returns TRUE.
	*
	* The \ref UsbK_Free function releases all of the resources that
	* \ref UsbK_Initialize or \ref UsbK_Open allocated. This is a synchronous
	* operation.
	*
	* \note \ref UsbK_Close and \ref UsbK_Free perform the same tasks.  The difference is in the return code only.
	* - \ref UsbK_Free always returns TRUE.
	* - \ref UsbK_Close will return FALSE in the hande is already closed/free.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_Free (
	    __in KUSB_HANDLE InterfaceHandle);

	KUSB_EXP BOOL KUSB_API UsbK_SelectInterface (
	    __in KUSB_HANDLE InterfaceHandle,
	    __in UCHAR NumberOrIndex,
	    __in BOOL IsIndex);

	//! Retrieves a handle for an associated interface.
	/*!
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
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* The \ref UsbK_GetAssociatedInterface function retrieves an opaque handle for an
	* associated interface. This is a synchronous operation.
	*
	* The first associated interface is the interface that immediately follows
	* the current (or default) interface of the specified /c InterfaceHandle.
	*
	* The handle that \ref UsbK_GetAssociatedInterface returns must be released
	* by calling \ref UsbK_Free.
	*
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetAssociatedInterface (
	    __in KUSB_HANDLE InterfaceHandle,
	    __in UCHAR AssociatedInterfaceIndex,
	    __out KUSB_HANDLE* AssociatedInterfaceHandle);

	KUSB_EXP BOOL KUSB_API UsbK_Clone (
	    __in KUSB_HANDLE InterfaceHandle,
	    __out KUSB_HANDLE* DstInterfaceHandle);

	//! Retrieves the interface descriptor for the specified alternate interface settings for a particular interface handle.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param AltSettingNumber
	* A value that indicates which alternate settings to return. A value of 0
	* indicates the first alternate setting, a value of 1 indicates the second
	* alternate setting, and so on.
	*
	* \param UsbAltInterfaceDescriptor
	* A pointer to a caller-allocated \ref USB_INTERFACE_DESCRIPTOR structure that
	* contains information about the interface that AltSettingNumber
	* specified.
	*
	* The \ref UsbK_QueryInterfaceSettings call searches the current/default interface array
	* for the alternate interface specified by the caller in the AltSettingNumber.
	* If the specified alternate interface is found, the function populates the caller-allocated
	* USB_INTERFACE_DESCRIPTOR structure. If the specified alternate interface is not
	* found, then the call fails with the ERROR_NO_MORE_ITEMS code.
	*
	* To change the current/default interface, see \ref UsbK_ClaimInterface and \ref UsbK_ReleaseInterface
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_QueryInterfaceSettings (
	    __in KUSB_HANDLE InterfaceHandle,
	    __in UCHAR AltSettingNumber,
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
	    __in KUSB_HANDLE InterfaceHandle,
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
	* \param AltSettingNumber
	* The value that is contained in the bAlternateSetting member of the
	* \ref USB_INTERFACE_DESCRIPTOR structure. This structure can be populated by the
	* \ref UsbK_QueryInterfaceSettings routine.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* Sets the active bAlternateSetting for the current/default interface.
	*
	* To change the default/current interface see \ref UsbK_ClaimInterface and \ref UsbK_ReleaseInterface
	* \sa UsbK_SetAltInterface
	*/
	KUSB_EXP BOOL KUSB_API UsbK_SetCurrentAlternateSetting (
	    __in KUSB_HANDLE InterfaceHandle,
	    __in UCHAR AltSettingNumber);

	//! Gets the current alternate interface setting for an interface.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param AltSettingNumber
	* A pointer to an unsigned character that receives an integer that indicates the current alternate setting.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* Gets the active bAlternateSetting for the current/default interface.
	*
	* To change the default/current interface see \ref UsbK_ClaimInterface and \ref UsbK_ReleaseInterface
	* \sa UsbK_GetAltInterface
	*
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetCurrentAlternateSetting (
	    __in KUSB_HANDLE InterfaceHandle,
	    __out PUCHAR AltSettingNumber);

	//! Retrieves information about a pipe that is associated with an interface.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param AltSettingNumber
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
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
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
	*/
	KUSB_EXP BOOL KUSB_API UsbK_QueryPipe (
	    __in KUSB_HANDLE InterfaceHandle,
	    __in UCHAR AltSettingNumber,
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
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* The following list describes symbolic constants that are defined in \ref lusbk_shared.h
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
	*/
	KUSB_EXP BOOL KUSB_API UsbK_SetPipePolicy (
	    __in KUSB_HANDLE InterfaceHandle,
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
	    __in KUSB_HANDLE InterfaceHandle,
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
	* operations. This can be a \ref KOVL_HANDLE or a pointer to a standard
	* windows OVERLAPPED structure. If this parameter is specified, \c
	* UsbK_ReadPipe returns immediately rather than waiting synchronously for
	* the operation to complete before returning. An event is signaled when
	* the operation is complete.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_ReadPipe (
	    __in KUSB_HANDLE InterfaceHandle,
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
	* operations. This can be a \ref KOVL_HANDLE or a pointer to a standard
	* windows OVERLAPPED structure. If this parameter is specified, \c
	* UsbK_WritePipe returns immediately rather than waiting synchronously for
	* the operation to complete before returning. An event is signaled when
	* the operation is complete.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_WritePipe (
	    __in KUSB_HANDLE InterfaceHandle,
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
	    __in KUSB_HANDLE InterfaceHandle,
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
	    __in KUSB_HANDLE InterfaceHandle,
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
	    __in KUSB_HANDLE InterfaceHandle,
	    __in UCHAR PipeID);

	//! Reads from an isochronous pipe.
	/*!
	*
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param IsoContext
	* Pointer to an isochronous transfer context created with \ref IsoK_Init
	*
	* \param Buffer
	* A caller-allocated buffer that receives the data that is read.
	*
	* \param BufferLength
	* The maximum number of bytes to read. This number must be less than or
	* equal to the size, in bytes, of Buffer.
	*
	* \param Overlapped
	* A \b required pointer to an overlapped structure for asynchronous
	* operations. This can be a \ref KOVL_HANDLE or a pointer to a standard
	* windows OVERLAPPED structure. If this parameter is specified, \c
	* UsbK_IsoReadPipe returns immediately rather than waiting synchronously for
	* the operation to complete before returning. An event is signaled when
	* the operation is complete.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	*
	* \par Overlapped I/O considerations
	* If an \c Overlapped parameter is specified and the transfer is submitted
	* successfully, the function returns \b FALSE and sets last error to \c
	* ERROR_IO_PENDING. When using overlapped I/O, users may ignore the return
	* results of this function and instead use the return results from one of
	* the \ref ovlk wait functions or from \ref UsbK_GetOverlappedResult.
	*
	*/
	KUSB_EXP BOOL KUSB_API UsbK_IsoReadPipe (
	    __in KUSB_HANDLE InterfaceHandle,
	    __inout PKISO_CONTEXT IsoContext,
	    __out_opt PUCHAR Buffer,
	    __in ULONG BufferLength,
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
	* \param IsoContext
	* Pointer to an isochronous transfer context created with \ref IsoK_Init. See remarks below.
	*
	* \param Buffer
	* A caller-allocated buffer that receives the data that is read.
	*
	* \param BufferLength
	* The maximum number of bytes to write. This number must be less than or
	* equal to the size, in bytes, of Buffer.
	*
	* \param Overlapped
	* An optional pointer to an overlapped structure for asynchronous
	* operations. This can be a \ref KOVL_HANDLE or a pointer to a standard
	* windows OVERLAPPED structure. If this parameter is specified, \c
	* UsbK_IsoWritePipe returns immediately rather than waiting synchronously for
	* the operation to complete before returning. An event is signaled when
	* the operation is complete.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* \remarks
	*
	*/
	KUSB_EXP BOOL KUSB_API UsbK_IsoWritePipe (
	    __in KUSB_HANDLE InterfaceHandle,
	    __inout PKISO_CONTEXT IsoContext,
	    __in PUCHAR Buffer,
	    __in ULONG BufferLength,
	    __in LPOVERLAPPED Overlapped);

	//! Retrieves the current USB frame number.
	/*!
	* \param InterfaceHandle
	* A libusbK interface handle which is returned by:
	* - \ref UsbK_Open
	* - \ref UsbK_Initialize
	* - \ref UsbK_GetAssociatedInterface
	*
	* \param FrameNumber
	* A pointer to a location that receives the current 32-bit frame number on
	* the USB bus (from the host controller driver).
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API UsbK_GetCurrentFrameNumber (
	    __in KUSB_HANDLE InterfaceHandle,
	    __out PULONG FrameNumber);

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
	* This function is like the Win32 API routine, GetOverlappedResult, with
	* one difference; instead of passing a file handle that is returned from
	* CreateFile, the caller passes an interface handle that is returned from
	* \ref UsbK_Initialize, \ref UsbK_Open, or \ref UsbK_GetAssociatedInterface.
	* The caller can use either API routine, if the
	* appropriate handle is passed. The \ref UsbK_GetOverlappedResult function
	* extracts the file handle from the interface handle and then calls
	* GetOverlappedResult. \n
	*
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
	* If the bWait parameter is TRUE, \ref UsbK_GetOverlappedResult determines
	* whether the pending operation has been completed by waiting for the
	* event object to be in the signaled state.
	*
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
	    __in KUSB_HANDLE InterfaceHandle,
	    __in LPOVERLAPPED lpOverlapped,
	    __out LPDWORD lpNumberOfBytesTransferred,
	    __in BOOL bWait);

	/*! @} */


#endif

#ifndef _LIBUSBK_LSTK_FUNCTIONS
	/*! \addtogroup lstk
	* @{
	*/

	//! Initializes a new usb device list.
	/*!
	*
	* \c LstK_Init populates \c DeviceList with connected usb devices that can be used by libusbK.
	*
	* \note if \ref LstK_Init returns TRUE, the device list must be freed with \ref LstK_Free when it is no longer needed.
	*
	* \param DeviceList
	* Pointer reference that will receive a populated device list.
	*
	* \param InitParameters
	* Search, filter, and listing options. see \c KLST_INIT_PARAMS
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API LstK_Init(
	    __deref_inout KLST_HANDLE* DeviceList,
	    __in PKLST_INIT_PARAMS InitParameters);

	//! Frees a usb device list.
	/*!
	* Frees all resources that were allocated to \c DeviceList by \ref LstK_Init.
	*
	* \note if \ref LstK_Init returns TRUE, the device list must be freed with \ref LstK_Free when it is no longer needed.
	*
	* \param DeviceList
	* The \c DeviceList to free.
	*
	* \returns NONE
	*/
	KUSB_EXP BOOL KUSB_API LstK_Free(
	    __in KLST_HANDLE DeviceList);

	//! Enumerates \ref KLST_DEVINFO elements of a \ref KLST_DEV_LIST.
	/*!
	*
	* \param DeviceList
	* The \c DeviceList to enumerate.
	*
	* \param EnumDevListCB
	* Function to call for each iteration.
	*
	* \param Context
	* Optional user context pointer.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* Calls \c EnumDevListCB for each element in the device list or until \c EnumDevListCB returns FALSE.
	*/
	KUSB_EXP BOOL KUSB_API LstK_Enumerate(
	    __in KLST_HANDLE DeviceList,
	    __in PKLST_ENUM_DEVINFO_CB EnumDevListCB,
	    __in_opt PVOID Context);

	//! Gets the \ref KLST_DEVINFO element for the current position.
	/*!
	*
	* \param DeviceList
	* The \c DeviceList to retrieve a current \ref KLST_DEVINFO for.
	*
	* \param DeviceInfo
	* The device information.
	*
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* After a \c DeviceList is created or after the \ref LstK_MoveReset method is
	* called, the \c LstK_MoveNext method must be called to advance the device
	* list enumerator to the first element of the \c DeviceList before calling
	* \c LstK_Current otherwise, \c DeviceInfo is undefined.
	*
	* \c LstK_Current returns \c FALSE and sets last error to \c
	* ERROR_NO_MORE_ITEMS if the last call to \c LstK_MoveNext returned \c
	* FALSE, which indicates the end of the \c DeviceList.
	*
	* \c LstK_Current does not move the position of the device list
	* enumerator, and consecutive calls to \c LstK_Current return the same
	* object until either \c LstK_MoveNext or \ref LstK_MoveReset is called.
	*/
	KUSB_EXP BOOL KUSB_API LstK_Current(
	    __in KLST_HANDLE DeviceList,
	    __deref_out KLST_DEVINFO_HANDLE* DeviceInfo);

	//! Advances the device list current \ref KLST_DEVINFO position.
	/*!
	* \param DeviceList
	* A usb device list returned by \ref LstK_Init
	*
	* \param DeviceInfo [OPTIONAL]
	* On success, contains a pointer to the device information for the current enumerators position.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*
	* After a \c DeviceList is created or after \ref LstK_MoveReset is called, an
	* enumerator is positioned before the first element of the \c DeviceList
	* and the \b first call to \c LstK_MoveNext moves the enumerator over the
	* first element of the \c DeviceList.
	*
	* If \c LstK_MoveNext passes the end of the \c DeviceList, the enumerator
	* is positioned after the last element in the \c DeviceList and \c
	* LstK_MoveNext returns \c FALSE. When the enumerator is at this position,
	* a subsequent call to \c LstK_MoveNext will reset the enumerator and it
	* continues from the beginning.
	*
	*/
	KUSB_EXP BOOL KUSB_API LstK_MoveNext(
	    __inout KLST_HANDLE DeviceList,
	    __deref_out_opt KLST_DEVINFO_HANDLE* DeviceInfo);

	//! Sets the device list to its initial position, which is before the first element in the list.
	/*!
	*
	* \param DeviceList
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP VOID KUSB_API LstK_MoveReset(
	    __inout KLST_HANDLE DeviceList);

	//! Find a device by vendor and product id
	/*!
	*
	* \param DeviceList
	* The \c DeviceList to retrieve a current \ref KLST_DEVINFO for.
	*
	* \param Vid
	* ID is used in conjunction with the \c Pid to uniquely identify USB
	* devices, providing traceability to the OEM.
	*
	* \param Pid
	* ID is used in conjunction with the \c Pid to uniquely identify USB
	* devices, providing traceability to the OEM.
	*
	* \param DeviceInfo
	* On success, the device information pointer, otherwise NULL.
	*
	* \returns
	* - TRUE if the device was found
	* - FALSE if the device was \b not found or an error occurred.
	*   - Sets last error to \c ERROR_NO_MORE_ITEMS if the device was \b not found.
	*
	* Searches all elements in \c DeviceList for usb device matching the specified.
	*/
	KUSB_EXP BOOL KUSB_API LstK_FindByVidPid(
	    __in KLST_HANDLE DeviceList,
	    __in UINT Vid,
	    __in UINT Pid,
	    __deref_out KLST_DEVINFO_HANDLE* DeviceInfo);

	KUSB_EXP BOOL KUSB_API LstK_Count(
	    __in KLST_HANDLE DeviceList,
	    __inout PULONG Count);

	KUSB_EXP BOOL KUSB_API LstK_Sync(
	    __inout KLST_HANDLE MasterList,
	    __in KLST_HANDLE SlaveList,
	    __in_opt PKLST_SYNC_PARAMS SyncParams);

	//! Creates a copy of an existing device list.
	/*!
	*
	* \param SrcList
	* The device list to copy.
	*
	* \param DstList
	* Reference to a pointer that receives the cloned device list.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get extended error information.
	*/
	KUSB_EXP BOOL KUSB_API LstK_Clone(
	    __in KLST_HANDLE SrcList,
	    __out KLST_HANDLE* DstList);

	KUSB_EXP BOOL KUSB_API LstK_CloneInfo(
	    __in KLST_DEVINFO_HANDLE SrcInfo,
	    __deref_inout KLST_DEVINFO_HANDLE* DstInfo);

	KUSB_EXP BOOL KUSB_API LstK_DetachInfo(
	    __inout KLST_HANDLE DeviceList,
	    __in KLST_DEVINFO_HANDLE DeviceInfo);

	KUSB_EXP BOOL KUSB_API LstK_AttachInfo(
	    __inout KLST_HANDLE DeviceList,
	    __in KLST_DEVINFO_HANDLE DeviceInfo);

	KUSB_EXP BOOL KUSB_API LstK_FreeInfo(
	    __deref_inout KLST_DEVINFO_HANDLE DeviceInfo);

	/*! @} */

#endif

#ifndef _LIBUSBK_HOTK_FUNCTIONS
	/*! \addtogroup hotk
	* @{
	*/

	//! Creates a new hot-plug handle for USB device arrival/removal event monitoring.
	/*!
	*
	* \param Handle
	* Reference to a handle pointer that will receive the initialized hot-plug handle.
	*
	* \param InitParams
	* Hot plug handle initialization structure.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	*
	*/
	KUSB_EXP BOOL KUSB_API HotK_Init(
	    __deref_out KHOT_HANDLE* Handle,
	    __in KHOT_PARAMS* InitParams);

	//! Frees the specified hot-plug handle.
	/*!
	*
	* \param Handle
	* hot-plug handle pointer to free.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	*
	*/
	KUSB_EXP BOOL KUSB_API HotK_Free(
	    __in KHOT_HANDLE Handle);

	//! Frees all hot-plug handles initialized with \ref HotK_Init.
	/*!
	*
	*/
	KUSB_EXP VOID KUSB_API HotK_FreeAll(VOID);

	/**@}*/

#endif

#ifndef _LIBUSBK_OVLK_FUNCTIONS
	/*! \addtogroup ovlk
	*  @{
	*/

	//! Gets a preallocated \c OverlappedK structure from the specified/default pool.
	/*!
	*
	* \param Pool
	* The overlapped pool used to retrieve the next available \c OverlappedK,
	* or NULL for the default pool.
	*
	* \c Pool parameter.
	*
	* \returns On success, the next unused overlappedK available in the pool.
	* Otherwise NULL. Use \c GetLastError() to get extended error information.
	*
	* After calling \ref OvlK_Acquire or \ref OvlK_ReUse the \c OverlappedK is
	* ready to be used in an I/O operation. See one of the \c UsbK core
	* transfer functions such as \ref UsbK_ReadPipe or \ref UsbK_WritePipe for
	* more information.
	*
	* If the pools internal refurbished list (a re-usable list of \c
	* OverlappedK structures) is not empty, the \ref OvlK_Acquire function
	* will choose an overlapped from the refurbished list.
	*
	*/
	KUSB_EXP BOOL KUSB_API OvlK_Acquire(
	    __out KOVL_HANDLE* OverlappedK,
	    __in KOVL_POOL_HANDLE Pool);

	//! Returns an \c OverlappedK structure to it's pool.
	/*!
	*
	* \param OverlappedK
	* The overlappedK to release.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* When an overlapped is returned to pool, it resources are \b not freed.
	* Instead, it is added to an internal refurbished list (a re-usable list of \c
	* OverlappedK structures).
	*
	* \warning
	* This function must not be called when the OverlappedK is in-use. If
	* unsure, consider using \ref OvlK_WaitAndRelease instead.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_Release(
	    __in KOVL_HANDLE OverlappedK);


	//! Creates a new overlapped pool.
	/*!
	*
	* \param MaxOverlappedCount
	* Maximum number of overkappedK handles allowed in the pool.
	*
	* \returns On success, the newly created overlapped pool.  Otherwise NULL.
	* Use \c GetLastError() to get extended error information.
	*
	* \c OverlappedK pools use a spin-lock to achieve thread safety.
	* Some of the \c OvlK function hold this spin-lock for a brief period to protect internal data.
	*
	*/
	KUSB_EXP BOOL KUSB_API OvlK_InitPool(
	    __out KOVL_POOL_HANDLE* PoolHandle,
	    __in USHORT MaxOverlappedCount);

	//! Destroys the specified pool and all resources it created.
	/*!
	*
	*
	* \param Pool
	* The overlapped pool to destroy. Once destroyed, the pool and all
	* resources which belong to it can no longer be used.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* \warning
	* A pool should not be destroyed until all OverlappedKs acquired from it
	* are no longer in-use. For more information see \ref OvlK_WaitAndRelease or \ref OvlK_Release.
	*
	*/
	KUSB_EXP BOOL KUSB_API OvlK_FreePool(
	    __in KOVL_POOL_HANDLE Pool);


	//! Returns the internal event handle used to signal IO operations.
	/*!
	*
	* \param OverlappedK
	* The overlappedK used to return the internal event handle.
	*
	* \returns On success, The manual reset event handle being used by this
	* overlappedK. Otherwise NULL. Use \c GetLastError() to get extended error
	* information.
	*
	* \ref OvlK_GetEventHandle is useful for applications that must to their
	* own event handling. It exposes the windows \c OVERLAPPED \c hEvent used
	* for i/o completion signaling. This event handle can be used by the
	* standard event wait functions; /c WaitForMultipleObjectsEx for example.
	*
	* \warning Use \ref OvlK_GetEventHandle with caution. Event handles
	* returned by this function should never be used unless the OverlappedK
	* has been \b acquired by the application.
	*
	*/
	KUSB_EXP HANDLE KUSB_API OvlK_GetEventHandle(
	    __in KOVL_HANDLE OverlappedK);

	//! Waits for an OverlappedK i/o operation to complete.
	/*!
	*
	* \param OverlappedK
	* The overlappedK to wait on.
	*
	* \param TimeoutMS
	* Number of milliseconds to wait for overlapped completion.
	*
	* \param WaitFlags
	* See /ref KOVL_WAIT_FLAGS
	*
	* \param TransferredLength
	* On success, returns the number of bytes transferred by this overlappedK.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* \c OvlK_Wait waits the the time interval specified by \c TimeoutMS for
	* an overlapped result. If the transfer does not complete in time, on
	* operation specified by \c WaitFlags can be performed on the \c
	* OverlappedK.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_Wait(
	    __in KOVL_HANDLE OverlappedK,
	    __in_opt DWORD TimeoutMS,
	    __in_opt KOVL_WAIT_FLAGS WaitFlags,
	    __out PULONG TransferredLength);

	//! Waits for an OverlappedK i/o operation to complete; cancels if it fails to complete within the specified time.
	/*!
	*
	* \note
	* When \c OvlK_WaitOrCancel returns, the \c OverlappedK is ready for
	* re-use or release.
	*
	* \param OverlappedK
	* The overlappedK to wait on.
	*
	* \param TimeoutMS
	* Number of milliseconds to wait for overlapped completion.
	*
	* \param TransferredLength
	* On success, returns the number of bytes transferred by this overlappedK.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* \c OvlK_WaitOrCancel waits the the time interval specified by \c
	* TimeoutMS for an overlapped result. If the transfer does not complete in
	* time, the overlapped operation is canceled.
	*
	* This convenience function calls \ref OvlK_Wait with \ref
	* WAIT_FLAGS_CANCEL_ON_TIMEOUT
	*
	*/
	KUSB_EXP BOOL KUSB_API OvlK_WaitOrCancel(
	    __in KOVL_HANDLE OverlappedK,
	    __in_opt DWORD TimeoutMS,
	    __out PULONG TransferredLength);

	//! Waits for completion, cancels the transfer if it fails to complete within the specified time. Always releases the \c OverlappedK back to it pool.
	/*!
	*
	*
	* \param OverlappedK
	* The overlappedK to wait on.
	*
	* \param TimeoutMS
	* Number of milliseconds to wait for overlapped completion.
	*
	* \param TransferredLength
	* On success, returns the number of bytes transferred by this overlappedK.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	* \c OvlK_WaitAndRelease waits the the time interval specified by \c
	* TimeoutMS for an overlapped result. If the transfer does not complete in
	* time, the overlapped operation is canceled. \c OverlappedK is always
	* released back to its pool.
	*
	* This convenience function calls \ref OvlK_Wait with \ref
	* WAIT_FLAGS_RELEASE_ALWAYS
	*
	* \note
	* When \c OvlK_WaitOrCancel returns, the i/o operation has either been
	* completed or canceled and \c OverlappedK has been released back to the
	* pool.
	*
	*/
	KUSB_EXP BOOL KUSB_API OvlK_WaitAndRelease(
	    __in KOVL_HANDLE OverlappedK,
	    __in_opt DWORD TimeoutMS,
	    __out PULONG TransferredLength);

	//! Checks for i/o completion; returns immediately. (polling)
	/*!
	*
	* \param OverlappedK
	* The overlappedK to check for completion.
	*
	* \warning
	* \ref OvlK_IsComplete does \b no validation on the OverlappedK.
	* It's purpose is to check the event signal state as fast as possible.
	*
	* \returns TRUE if the \c OverlappedK has completed, otherwise FALSE.
	*
	* \c OvlK_IsComplete quickly checks if the \c OverlappedK i/o operation has completed.
	*/
	KUSB_EXP BOOL KUSB_API OvlK_IsComplete(
	    __in KOVL_HANDLE OverlappedK);

	//! Initializes an overlappedK for re-use. The overlappedK is not return to its pool.
	/*!
	*
	* \param OverlappedK
	* The overlappedK to re-use.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*
	*  This function performs the following actions:
	*  - Resets the overlapped event to non-signaled via ResetEvent().
	*  - Clears the internal overlapped information.
	*  - Clears the 'Internal' and 'InternalHigh' members of the windows
	*    overlapped structure.
	*
	* \note
	* Re-using OverlappedKs is the most efficient means of OverlappedK
	* management. When an OverlappedK is "re-used" it is not returned to the
	* pool. Instead, the application retains ownership for use in another i/o
	* operation.
	*
	*/
	KUSB_EXP BOOL KUSB_API KUSB_API OvlK_ReUse(
	    __in KOVL_HANDLE OverlappedK);

	/**@}*/

#endif

#ifdef __cplusplus
}
#endif

#endif // _LIBUSBK_H__
