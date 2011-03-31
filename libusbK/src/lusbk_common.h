/*! \file lusbk_common.h
*/

#ifndef __LUSBK_COMMON_H
#define __LUSBK_COMMON_H

#ifndef __USB_H__
#define __USB_H__

#include <windows.h>
#include <stddef.h>
#include <objbase.h>
#include <PSHPACK1.H>

#ifndef __USB200_H__
#define __USB200_H__

#ifndef __USB100_H__
#define __USB100_H__

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

typedef enum _USB_DESCRIPTOR_TYPE
{
	USB_DEVICE_DESCRIPTOR_TYPE = 1,
	USB_CONFIGURATION_DESCRIPTOR_TYPE = 2,
	USB_STRING_DESCRIPTOR_TYPE = 3,
	USB_INTERFACE_DESCRIPTOR_TYPE = 4,
	USB_ENDPOINT_DESCRIPTOR_TYPE = 5,
	USB_DEVICEQUALIFIER_DESCRIPTOR_TYPE = 6,
	USB_OTHERSPEEDCONFIGURATION_DESCRIPTOR_TYPE = 7,
	USB_INTERFACEPOWER_DESCRIPTOR_TYPE = 8,
	USB_ONTHEGO_DESCRIPTOR_TYPE = 9,
	USB_DEBUG_DESCRIPTOR_TYPE = 10,
	USB_INTERFACEASSOCIATION_DESCRIPTOR_TYPE = 11,
	USB_HID_DESCRIPTOR_TYPE = 0x21,
	USB_HID_REPORT_DESCRIPTOR_TYPE = 0x22,
	USB_HID_PHYSICAL_DESCRIPTOR_TYPE = 0x23,
	USB_HUB_DESCRIPTOR_TYPE = 0x29
} USB_DESCRIPTOR_TYPE;

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
} USB_DEVICE_DESCRIPTOR, *PUSB_DEVICE_DESCRIPTOR;

typedef struct _USB_ENDPOINT_DESCRIPTOR
{
	UCHAR bLength;
	UCHAR bDescriptorType;
	UCHAR bEndpointAddress;
	UCHAR bmAttributes;
	USHORT wMaxPacketSize;
	UCHAR bInterval;
} USB_ENDPOINT_DESCRIPTOR, *PUSB_ENDPOINT_DESCRIPTOR;

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
} USB_CONFIGURATION_DESCRIPTOR, *PUSB_CONFIGURATION_DESCRIPTOR;

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
} USB_INTERFACE_DESCRIPTOR, *PUSB_INTERFACE_DESCRIPTOR;

typedef struct _USB_STRING_DESCRIPTOR
{
	UCHAR bLength;
	UCHAR bDescriptorType;
	WCHAR bString[1];
} USB_STRING_DESCRIPTOR, *PUSB_STRING_DESCRIPTOR;

typedef struct _USB_INTERFACEASSOCIATION_DESCRIPTOR
{
	UCHAR  bLength;
	UCHAR  bDescriptorType;
	UCHAR  bFirstInterface;
	UCHAR  bInterfaceCount;
	UCHAR  bFunctionClass;
	UCHAR  bFunctionSubClass;
	UCHAR  bFunctionProtocol;
	UCHAR  iFunction;
} USB_INTERFACEASSOCIATION_DESCRIPTOR, *PUSB_INTERFACEASSOCIATION_DESCRIPTOR;

typedef struct _USB_COMMON_DESCRIPTOR
{
	UCHAR bLength;
	UCHAR bDescriptorType;
} USB_COMMON_DESCRIPTOR, *PUSB_COMMON_DESCRIPTOR;

#endif // __USB100_H__

#pragma warning(disable:4214)
#pragma warning(disable:4201)
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
} BM_REQUEST_TYPE, *PBM_REQUEST_TYPE;

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
} USB_DEFAULT_PIPE_SETUP_PACKET, *PUSB_DEFAULT_PIPE_SETUP_PACKET;
C_ASSERT(sizeof(USB_DEFAULT_PIPE_SETUP_PACKET) == 8);

#pragma warning(default:4214)
#pragma warning(default:4201)

#endif // __USB200_H__


typedef enum _USBD_PIPE_TYPE
{
	UsbdPipeTypeControl,
	UsbdPipeTypeIsochronous,
	UsbdPipeTypeBulk,
	UsbdPipeTypeInterrupt
} USBD_PIPE_TYPE;
C_ASSERT(sizeof(USBD_PIPE_TYPE) == 4);

#include <POPPACK.H>

#endif // __USB_H__

#ifndef __WINUSB_COMPAT_IO_H__
#define __WINUSB_COMPAT_IO_H__

#ifndef __WUSBIO_H__

//#include <usb.h>
// Pipe policy types

// The default value is FALSE. To enable SHORT_PACKET_TERMINATE, in Value pass the address of a caller-allocated UCHAR variable set to TRUE (nonzero).
// Enabling SHORT_PACKET_TERMINATE causes the driver to send a zero-length packet at the end of every write request to the host controller.
#define SHORT_PACKET_TERMINATE  0x01

// The default value is FALSE. To enable AUTO_CLEAR_STALL, in Value pass the address of a caller-allocated UCHAR variable set to TRUE (nonzero).
// Enabling AUTO_CLEAR_STALL causes WinUSB to reset the pipe in order to automatically clear the stall condition. Data continues to flow on the bulk and interrupt IN endpoints again as soon as a new or a queued transfer arrives on the endpoint. This policy parameter does not affect control pipes.
// Disabling AUTO_CLEAR_STALL causes all transfers (that arrive to the endpoint after the stalled transfer) to fail until the caller manually resets the endpoint's pipe by calling WinUsb_ResetPipe.
#define AUTO_CLEAR_STALL        0x02

// The default value is zero. To set a time-out value, in Value pass the address of a caller-allocated ULONG variable that contains the time-out interval.
// The PIPE_TRANSFER_TIMEOUT value specifies the time-out interval, in milliseconds. The host controller cancels transfers that do not complete within the specified time-out interval.
// A value of zero (default) indicates that transfers do not time out because the host controller never cancels the transfer.
#define PIPE_TRANSFER_TIMEOUT   0x03

// The default value is FALSE. To enable IGNORE_SHORT_PACKETS, in Value pass the address of a caller-allocated UCHAR variable set to TRUE (nonzero).
// Enabling IGNORE_SHORT_PACKETS causes the host controller to not complete a read operation after it receives a short packet. Instead, the host controller completes the operation only after the host has read the specified number of bytes.
// Disabling IGNORE_SHORT_PACKETS causes the host controller to complete a read operation when either the host has read the specified number of bytes or the host has received a short packet.
#define IGNORE_SHORT_PACKETS    0x04

// The default value is TRUE (nonzero). To disable ALLOW_PARTIAL_READS, in Value pass the address of a caller-allocated UCHAR variable set to FALSE (zero).
// Disabling ALLOW_PARTIAL_READS causes the read requests to fail whenever the device returns more data (on bulk and interrupt IN endpoints) than the caller requested.
// Enabling ALLOW_PARTIAL_READS causes WinUSB to save or discard the extra data when the device returns more data (on bulk and interrupt IN endpoints) than the caller requested. This behavior is defined by setting the AUTO_FLUSH value.
#define ALLOW_PARTIAL_READS     0x05

// The default value is FALSE (zero). To enable AUTO_FLUSH, in Value pass the address of a caller-allocated UCHAR variable set to TRUE (nonzero).
// AUTO_FLUSH must be used with ALLOW_PARTIAL_READS enabled. If ALLOW_PARTIAL_READS is TRUE, the value of AUTO_FLUSH determines the action taken by WinUSB when the device returns more data than the caller requested.
// Disabling ALLOW_PARTIAL_READS causes WinUSB to ignore the AUTO_FLUSH value.
// Disabling AUTO_FLUSH with ALLOW_PARTIAL_READS enabled causes WinUSB to save the extra data, add the data to the beginning of the caller's next read request, and send it to the caller in the next read operation.
// Enabling AUTO_FLUSH with ALLOW_PARTIAL_READS enabled causes WinUSB to discard the extra data remaining from the read request.
#define AUTO_FLUSH              0x06

// The default value is FALSE (zero). To enable RAW_IO, in Value pass the address of a caller-allocated UCHAR variable set to TRUE (nonzero).
// Enabling RAW_IO causes WinUSB to send data directly to the USB driver stack, bypassing WinUSB's queuing and error handling mechanism.
// The buffers that are passed to WinUsb_ReadPipe must be configured by the caller as follows:
//   * The buffer length must be a multiple of the maximum endpoint packet size.
//   * The length must be less than or equal to the value of MAXIMUM_TRANSFER_SIZE retrieved by WinUsb_GetPipePolicy.
// Disabling RAW_IO (FALSE) does not impose any restriction on the buffers that are passed to WinUsb_ReadPipe.
#define RAW_IO                  0x07


// To get the value of the MAXIMUM_TRANSFER_SIZE policy, receive the value in a caller-allocated ULONG variable pointed to by Value.
// The retrieved value indicates the maximum size of a USB transfer supported by WinUSB.
// GET ONLY
#define MAXIMUM_TRANSFER_SIZE   0x08

// The default value is FALSE (zero). To enable RESET_PIPE_ON_RESUME, in Value pass the address of a caller-allocated UCHAR variable set to TRUE (nonzero).
// TRUE (or a nonzero value) indicates that on resume from suspend, WinUSB resets the endpoint before it allows the caller to send new requests to the endpoint.
#define RESET_PIPE_ON_RESUME    0x09

// TODO:	This cannot go in this file if we keep it.
//			This file must remain identical to the original winusb_io.h file.
// [tr] !NEW! (mainly for testing)
#define MAX_TRANSFER_STAGE_SIZE    0x0F

// Power policy types
//
// Add 0x80 for Power policy types in order to prevent overlap with
// Pipe policy types to prevent "accidentally" setting the wrong value for the
// wrong type.
//

// Specifies the auto-suspend policy type; the power policy parameter must be specified by the caller in the Value parameter.
// For auto-suspend, the Value parameter must point to a UCHAR variable.
// If Value is TRUE (nonzero), the USB stack suspends the device if the device is idle. A device is idle if there are no transfers pending, or if the only pending transfers are IN transfers to interrupt or bulk endpoints.
// The default value is determined by the value set in the DefaultIdleState registry setting. By default, this value is TRUE.
#define AUTO_SUSPEND            0x81

// Specifies the suspend-delay policy type; the power policy parameter must be specified by the caller in the Value parameter.
// For suspend-delay, Value must point to a ULONG variable.
// Value specifies the minimum amount of time, in milliseconds, that the WinUSB driver must wait post transfer before it can suspend the device.
// The default value is determined by the value set in the DefaultIdleTimeout registry setting. By default, this value is five seconds.
#define SUSPEND_DELAY           0x83

// Device Information types
#define DEVICE_SPEED            0x01

// Device Speeds
#define LowSpeed                0x01
#define FullSpeed               0x02
#define HighSpeed               0x03

typedef struct _WINUSB_PIPE_INFORMATION
{
	USBD_PIPE_TYPE  PipeType;
	UCHAR           PipeId;
	USHORT          MaximumPacketSize;
	UCHAR           Interval;
} WINUSB_PIPE_INFORMATION, *PWINUSB_PIPE_INFORMATION;
C_ASSERT(sizeof(WINUSB_PIPE_INFORMATION) == 12);

#endif // __WUSBIO_H__

#endif // __WINUSB_COMPAT_IO_H__

#ifndef __WUSB_H__

typedef PVOID WINUSB_INTERFACE_HANDLE, *PWINUSB_INTERFACE_HANDLE;

#include <PSHPACK1.H>

typedef struct _WINUSB_SETUP_PACKET
{
	UCHAR   RequestType;
	UCHAR   Request;
	USHORT  Value;
	USHORT  Index;
	USHORT  Length;
} WINUSB_SETUP_PACKET, *PWINUSB_SETUP_PACKET;
C_ASSERT(sizeof(WINUSB_SETUP_PACKET) == 8);

#ifndef __HIDPORT_H__

typedef struct _HID_DESCRIPTOR
{
	UCHAR   bLength;
	UCHAR   bDescriptorType;
	USHORT  bcdHID;
	UCHAR   bCountry;
	UCHAR   bNumDescriptors;

	/*
	 *  This is an array of one OR MORE descriptors.
	 */
	struct _HID_DESCRIPTOR_DESC_LIST
	{
		UCHAR   bReportType;
		USHORT  wReportLength;
	} DescriptorList [1];

} HID_DESCRIPTOR, * PHID_DESCRIPTOR;

#endif

#include <POPPACK.H>

#else // NDEF __WUSB_H__

#ifndef EXCLUDE_WINUSB_WRAPPER
#define EXCLUDE_WINUSB_WRAPPER 1
#endif // EXCLUDE_WINUSB_WRAPPER

#endif // __WUSB_H__

#endif // __LUSBK_COMMON_H
