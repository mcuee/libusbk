/*! \file lusbk_common.h
* The libusbK library provides a subset of general usb/descriptor types.
* The general usb/descriptor types are also available as part of the WDK.
* In order to maximum compatiblity with all \ref usbk_drivers, the
* usb/descriptor types defined in libusbK are \b identical to those found
* in Microsoft WDK distributions.
*
* \attention
* If the libusbK usb/descriptor types are inadequate, include the Microsoft WDK
* \c usb.h \b before including any libusbk header files.
*
*/

#ifndef __LUSBK_COMMON_H
#define __LUSBK_COMMON_H

#include <windows.h>
#include <stddef.h>
#include <objbase.h>


#define KUSB_CONTEXT_SIZE 32

typedef INT_PTR (FAR WINAPI* KPROC)();

/*!
  \def KUSB_EXP
  Indicates that a function is an exported API call.
*/
#if defined(DYNAMIC_DLL)
#define KUSB_EXP
#else
#define KUSB_EXP
#endif

/*! Indicates the calling convention. This is always WINAPI (stdcall) by default.
*/
#if !defined(KUSB_API)
#define KUSB_API WINAPI
#endif

#pragma warning(disable:4201)
#include <pshpack1.h>
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
}* PKUSB_USER_CONTEXT, KUSB_USER_CONTEXT;
#include <poppack.h>
#pragma warning(default:4201)

/*! \addtogroup core_general
*  @{
*/
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
typedef PVOID LIBUSBK_INTERFACE_HANDLE, *PLIBUSBK_INTERFACE_HANDLE;

#ifndef   __USB_H__

#ifndef   __USB200_H__

#ifndef   __USB100_H__

#include <PSHPACK1.H>

//bmRequest.Dir
#define BMREQUEST_HOST_TO_DEVICE        0
#define BMREQUEST_DEVICE_TO_HOST        1

//bmRequest.Type
#define BMREQUEST_STANDARD              0
#define BMREQUEST_CLASS                 1
#define BMREQUEST_VENDOR                2

//bmRequest.Recipient
#define BMREQUEST_TO_DEVICE             0
#define BMREQUEST_TO_INTERFACE          1
#define BMREQUEST_TO_ENDPOINT           2
#define BMREQUEST_TO_OTHER              3


#define MAXIMUM_USB_STRING_LENGTH 255

// values for the bits returned by the USB GET_STATUS command
#define USB_GETSTATUS_SELF_POWERED                0x01
#define USB_GETSTATUS_REMOTE_WAKEUP_ENABLED       0x02


#define USB_DEVICE_DESCRIPTOR_TYPE                0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE         0x02
#define USB_STRING_DESCRIPTOR_TYPE                0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE             0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE              0x05

// descriptor types defined by DWG documents
#define USB_RESERVED_DESCRIPTOR_TYPE              0x06
#define USB_CONFIG_POWER_DESCRIPTOR_TYPE          0x07
#define USB_INTERFACE_POWER_DESCRIPTOR_TYPE       0x08

#define USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(d, i) ((USHORT)((USHORT)d<<8 | i))

//
// Values for bmAttributes field of an
// endpoint descriptor
//

#define USB_ENDPOINT_TYPE_MASK                    0x03

#define USB_ENDPOINT_TYPE_CONTROL                 0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS             0x01
#define USB_ENDPOINT_TYPE_BULK                    0x02
#define USB_ENDPOINT_TYPE_INTERRUPT               0x03


//
// definitions for bits in the bmAttributes field of a
// configuration descriptor.
//
#define USB_CONFIG_POWERED_MASK                   0xc0

#define USB_CONFIG_BUS_POWERED                    0x80
#define USB_CONFIG_SELF_POWERED                   0x40
#define USB_CONFIG_REMOTE_WAKEUP                  0x20

//
// Endpoint direction bit, stored in address
//

#define USB_ENDPOINT_DIRECTION_MASK               0x80

// test direction bit in the bEndpointAddress field of
// an endpoint descriptor.
#define USB_ENDPOINT_DIRECTION_OUT(addr)          (!((addr) & USB_ENDPOINT_DIRECTION_MASK))
#define USB_ENDPOINT_DIRECTION_IN(addr)           ((addr) & USB_ENDPOINT_DIRECTION_MASK)

//
// USB defined request codes
// see chapter 9 of the USB 1.0 specifcation for
// more information.
//

// These are the correct values based on the USB 1.0
// specification

#define USB_REQUEST_GET_STATUS                    0x00
#define USB_REQUEST_CLEAR_FEATURE                 0x01

#define USB_REQUEST_SET_FEATURE                   0x03

#define USB_REQUEST_SET_ADDRESS                   0x05
#define USB_REQUEST_GET_DESCRIPTOR                0x06
#define USB_REQUEST_SET_DESCRIPTOR                0x07
#define USB_REQUEST_GET_CONFIGURATION             0x08
#define USB_REQUEST_SET_CONFIGURATION             0x09
#define USB_REQUEST_GET_INTERFACE                 0x0A
#define USB_REQUEST_SET_INTERFACE                 0x0B
#define USB_REQUEST_SYNC_FRAME                    0x0C


//
// defined USB device classes
//


#define USB_DEVICE_CLASS_RESERVED           0x00
#define USB_DEVICE_CLASS_AUDIO              0x01
#define USB_DEVICE_CLASS_COMMUNICATIONS     0x02
#define USB_DEVICE_CLASS_HUMAN_INTERFACE    0x03
#define USB_DEVICE_CLASS_MONITOR            0x04
#define USB_DEVICE_CLASS_PHYSICAL_INTERFACE 0x05
#define USB_DEVICE_CLASS_POWER              0x06
#define USB_DEVICE_CLASS_PRINTER            0x07
#define USB_DEVICE_CLASS_STORAGE            0x08
#define USB_DEVICE_CLASS_HUB                0x09
#define USB_DEVICE_CLASS_VENDOR_SPECIFIC    0xFF

//
// USB Core defined Feature selectors
//

#define USB_FEATURE_ENDPOINT_STALL          0x0000
#define USB_FEATURE_REMOTE_WAKEUP           0x0001

//
// USB DWG defined Feature selectors
//

#define USB_FEATURE_INTERFACE_POWER_D0      0x0002
#define USB_FEATURE_INTERFACE_POWER_D1      0x0003
#define USB_FEATURE_INTERFACE_POWER_D2      0x0004
#define USB_FEATURE_INTERFACE_POWER_D3      0x0005

typedef struct _USB_DEVICE_DESCRIPTOR
{
	UCHAR bLength;
	UCHAR bDescriptorType;
	USHORT bcdUSB;
	UCHAR bDeviceClass;
	UCHAR bDeviceSubClass;
	UCHAR bDeviceProtocol;
	UCHAR bMaxPacketSize0;
	USHORT idVendor;
	USHORT idProduct;
	USHORT bcdDevice;
	UCHAR iManufacturer;
	UCHAR iProduct;
	UCHAR iSerialNumber;
	UCHAR bNumConfigurations;
}* PUSB_DEVICE_DESCRIPTOR, USB_DEVICE_DESCRIPTOR;

typedef struct _USB_ENDPOINT_DESCRIPTOR
{
	UCHAR bLength;
	UCHAR bDescriptorType;
	UCHAR bEndpointAddress;
	UCHAR bmAttributes;
	USHORT wMaxPacketSize;
	UCHAR bInterval;
}* PUSB_ENDPOINT_DESCRIPTOR, USB_ENDPOINT_DESCRIPTOR;

typedef struct _USB_CONFIGURATION_DESCRIPTOR
{
	UCHAR bLength;
	UCHAR bDescriptorType;
	USHORT wTotalLength;
	UCHAR bNumInterfaces;
	UCHAR bConfigurationValue;
	UCHAR iConfiguration;
	UCHAR bmAttributes;
	UCHAR MaxPower;
}* PUSB_CONFIGURATION_DESCRIPTOR, USB_CONFIGURATION_DESCRIPTOR;

typedef struct _USB_INTERFACE_DESCRIPTOR
{
	UCHAR bLength;
	UCHAR bDescriptorType;
	UCHAR bInterfaceNumber;
	UCHAR bAlternateSetting;
	UCHAR bNumEndpoints;
	UCHAR bInterfaceClass;
	UCHAR bInterfaceSubClass;
	UCHAR bInterfaceProtocol;
	UCHAR iInterface;
}* PUSB_INTERFACE_DESCRIPTOR, USB_INTERFACE_DESCRIPTOR;

typedef struct _USB_STRING_DESCRIPTOR
{
	UCHAR bLength;
	UCHAR bDescriptorType;
	WCHAR bString[1];
}* PUSB_STRING_DESCRIPTOR, USB_STRING_DESCRIPTOR;

typedef struct _USB_COMMON_DESCRIPTOR
{
	UCHAR bLength;
	UCHAR bDescriptorType;
}* PUSB_COMMON_DESCRIPTOR, USB_COMMON_DESCRIPTOR;

typedef struct _USB_CONFIGURATION_POWER_DESCRIPTOR
{
	UCHAR bLength;
	UCHAR bDescriptorType;
	UCHAR SelfPowerConsumedD0[3];
	UCHAR bPowerSummaryId;
	UCHAR bBusPowerSavingD1;
	UCHAR bSelfPowerSavingD1;
	UCHAR bBusPowerSavingD2;
	UCHAR bSelfPowerSavingD2;
	UCHAR bBusPowerSavingD3;
	UCHAR bSelfPowerSavingD3;
	USHORT TransitionTimeFromD1;
	USHORT TransitionTimeFromD2;
	USHORT TransitionTimeFromD3;
}* PUSB_CONFIGURATION_POWER_DESCRIPTOR, USB_CONFIGURATION_POWER_DESCRIPTOR;

typedef struct _USB_INTERFACE_POWER_DESCRIPTOR
{
	UCHAR bLength;
	UCHAR bDescriptorType;
	UCHAR bmCapabilitiesFlags;
	UCHAR bBusPowerSavingD1;
	UCHAR bSelfPowerSavingD1;
	UCHAR bBusPowerSavingD2;
	UCHAR bSelfPowerSavingD2;
	UCHAR bBusPowerSavingD3;
	UCHAR bSelfPowerSavingD3;
	USHORT TransitionTimeFromD1;
	USHORT TransitionTimeFromD2;
	USHORT TransitionTimeFromD3;
}* PUSB_INTERFACE_POWER_DESCRIPTOR, USB_INTERFACE_POWER_DESCRIPTOR;

#include <POPPACK.H>

#endif   // __USB100_H__

#include <PSHPACK1.H>

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning (disable:4201)
#pragma warning(disable:4214) // named type definition in parentheses

typedef enum _USB_DEVICE_SPEED
{
	UsbLowSpeed = 0,
	UsbFullSpeed,
	UsbHighSpeed
} USB_DEVICE_SPEED;

typedef enum _USB_DEVICE_TYPE
{
	Usb11Device = 0,
	Usb20Device
} USB_DEVICE_TYPE;


// standard definitions for the port status
// word of the HUB port register

#define USB_PORT_STATUS_CONNECT         0x0001
#define USB_PORT_STATUS_ENABLE          0x0002
#define USB_PORT_STATUS_SUSPEND         0x0004
#define USB_PORT_STATUS_OVER_CURRENT    0x0008
#define USB_PORT_STATUS_RESET           0x0010
#define USB_PORT_STATUS_POWER           0x0100
#define USB_PORT_STATUS_LOW_SPEED       0x0200
#define USB_PORT_STATUS_HIGH_SPEED      0x0400

typedef union _BM_REQUEST_TYPE
{
	struct _BM
	{
		UCHAR   Recipient: 2;
		UCHAR   Reserved: 3;
		UCHAR   Type: 2;
		UCHAR   Dir: 1;
	};
	UCHAR B;
}* PBM_REQUEST_TYPE, BM_REQUEST_TYPE;

typedef struct _USB_DEFAULT_PIPE_SETUP_PACKET
{
	BM_REQUEST_TYPE bmRequestType;
	UCHAR bRequest;

	union _wValue
	{
		struct
		{
			UCHAR LowByte;
			UCHAR HiByte;
		};
		USHORT W;
	} wValue;

	union _wIndex
	{
		struct
		{
			UCHAR LowByte;
			UCHAR HiByte;
		};
		USHORT W;
	} wIndex;
	USHORT wLength;
}* PUSB_DEFAULT_PIPE_SETUP_PACKET, USB_DEFAULT_PIPE_SETUP_PACKET;

// setup packet is eight bytes -- defined by spec
C_ASSERT(sizeof(USB_DEFAULT_PIPE_SETUP_PACKET) == 8);

#define USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE            0x06
#define USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_TYPE   0x07

typedef struct _USB_DEVICE_QUALIFIER_DESCRIPTOR
{
	UCHAR bLength;
	UCHAR bDescriptorType;
	USHORT bcdUSB;
	UCHAR bDeviceClass;
	UCHAR bDeviceSubClass;
	UCHAR bDeviceProtocol;
	UCHAR bMaxPacketSize0;
	UCHAR bNumConfigurations;
	UCHAR bReserved;
}* PUSB_DEVICE_QUALIFIER_DESCRIPTOR, USB_DEVICE_QUALIFIER_DESCRIPTOR;


typedef union _USB_HIGH_SPEED_MAXPACKET
{
	struct _MP
	{
		USHORT   MaxPacket: 11; /* 0..10 */
		USHORT   HSmux: 2;       /* 11..12 */
		USHORT   Reserved: 3;   /* 13..15 */
	};
	USHORT us;
}* PUSB_HIGH_SPEED_MAXPACKET, USB_HIGH_SPEED_MAXPACKET;

#define USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE 0x0B

typedef struct _USB_INTERFACE_ASSOCIATION_DESCRIPTOR
{

	UCHAR   bLength;
	UCHAR   bDescriptorType;
	UCHAR   bFirstInterface;
	UCHAR   bInterfaceCount;
	UCHAR   bFunctionClass;
	UCHAR   bFunctionSubClass;
	UCHAR   bFunctionProtocol;
	UCHAR   iFunction;

}* PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR, USB_INTERFACE_ASSOCIATION_DESCRIPTOR;

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#include <POPPACK.H>

#endif // __USB200_H__

typedef enum _USBD_PIPE_TYPE
{
	UsbdPipeTypeControl,
	UsbdPipeTypeIsochronous,
	UsbdPipeTypeBulk,
	UsbdPipeTypeInterrupt
} USBD_PIPE_TYPE;

#if (_WIN32_WINNT >= 0x0501)

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
}* POS_STRING, OS_STRING;

#if _MSC_VER >= 1200
#pragma warning(pop)
#endif

#endif // _WIN32_WINNT

#endif // __USB_H__

/*! @} */

#endif // __LUSBK_COMMON_H

