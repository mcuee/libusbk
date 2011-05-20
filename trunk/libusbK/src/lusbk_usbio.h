

#ifndef __LUSBK_USBIO_H_
#define __LUSBK_USBIO_H_

#ifndef __WINUSB_COMPAT_IO_H__
#define __WINUSB_COMPAT_IO_H__

#ifndef __WUSBIO_H__

// pipe policy types ///////////////
#define SHORT_PACKET_TERMINATE  0x01
#define AUTO_CLEAR_STALL        0x02
#define PIPE_TRANSFER_TIMEOUT   0x03
#define IGNORE_SHORT_PACKETS    0x04
#define ALLOW_PARTIAL_READS     0x05
#define AUTO_FLUSH              0x06
#define RAW_IO                  0x07
#define MAXIMUM_TRANSFER_SIZE   0x08
#define RESET_PIPE_ON_RESUME    0x09
// [tr] !NEW! (mainly for testing)
#define MAX_TRANSFER_STAGE_SIZE    0x0F

// Power policy types //////////////
#define AUTO_SUSPEND            0x81
#define SUSPEND_DELAY           0x83

// Device Information types ////////
#define DEVICE_SPEED            0x01

// Device Speeds
#define LowSpeed                0x01
#define FullSpeed               0x02
#define HighSpeed               0x03

///! The \c WINUSB_PIPE_INFORMATION structure contains pipe information that the \ref UsbK_QueryPipe routine retrieves.
typedef struct _WINUSB_PIPE_INFORMATION
{
	//! A \c USB_ENDPOINT_TYPE enumeration value that specifies the pipe type
	USB_ENDPOINT_TYPE	PipeType;

	//! The pipe identifier (ID)
	UCHAR				PipeId;

	//! The maximum size, in bytes, of the packets that are transmitted on the pipe
	USHORT				MaximumPacketSize;

	//! The pipe interval
	UCHAR				Interval;
}WINUSB_PIPE_INFORMATION;
//! Pointer to a \ref WINUSB_PIPE_INFORMATION structure
typedef struct WINUSB_PIPE_INFORMATION *PWINUSB_PIPE_INFORMATION;

C_ASSERT(sizeof(WINUSB_PIPE_INFORMATION) == 12);

#include <pshpack1.h>

///! The \c WINUSB_SETUP_PACKET structure describes a USB setup packet.
/*!
* It is often more convient to use this structure in combination with a \ref USB_DEFAULT_PIPE_SETUP_PACKET.
* For example:
* \code

* \endcode
*/
typedef struct _WINUSB_SETUP_PACKET
{
	//! The request type. The values that are assigned to this member are defined in Table 9.2 of section 9.3 of the Universal Serial Bus (USB) specification (www.usb.org). 
	UCHAR   RequestType;

	//! The device request. The values that are assigned to this member are defined in Table 9.3 of section 9.4 of the Universal Serial Bus (USB) specification.
	UCHAR   Request;

	//! The meaning of this member varies according to the request. For an explanation of this member, see the Universal Serial Bus (USB) specification.
	USHORT  Value;

	//! The meaning of this member varies according to the request. For an explanation of this member, see the Universal Serial Bus (USB) specification.
	USHORT  Index;

	//! The number of bytes to transfer. (not including the \c WINUSB_SETUP_PACKET itself)
	USHORT  Length;

}WINUSB_SETUP_PACKET;

//! pointer to a \c WINUSB_SETUP_PACKET structure
typedef WINUSB_SETUP_PACKET *PWINUSB_SETUP_PACKET;

C_ASSERT(sizeof(WINUSB_SETUP_PACKET) == 8);

#include <poppack.h>

#endif // __WUSBIO_H__

#endif // __WINUSB_COMPAT_IO_H__

#endif

