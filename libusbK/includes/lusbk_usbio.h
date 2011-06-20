

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
	//! A \c USBD_PIPE_TYPE enumeration value that specifies the pipe type
	USBD_PIPE_TYPE	PipeType;

	//! The pipe identifier (ID)
	UCHAR PipeId;

	//! The maximum size, in bytes, of the packets that are transmitted on the pipe
	USHORT MaximumPacketSize;

	//! The pipe interval
	UCHAR Interval;

} WINUSB_PIPE_INFORMATION;
//! Pointer to a \ref WINUSB_PIPE_INFORMATION structure
typedef WINUSB_PIPE_INFORMATION* PWINUSB_PIPE_INFORMATION;
C_ASSERT(sizeof(WINUSB_PIPE_INFORMATION) == 12);

#include <pshpack1.h>

///! The \c WINUSB_SETUP_PACKET structure describes a USB setup packet.
/*!
* It is often more convient to use this structure in combination with a \ref KUSB_SETUP_PACKET.
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

} WINUSB_SETUP_PACKET;
//! pointer to a \c WINUSB_SETUP_PACKET structure
typedef WINUSB_SETUP_PACKET* PWINUSB_SETUP_PACKET;
C_ASSERT(sizeof(WINUSB_SETUP_PACKET) == 8);

//! Structure describing an isochronous transfer packet.
typedef struct _KUSB_ISO_PACKET
{
	//! Specifies the offset, in bytes, of the buffer for this packet from the beginning of the entire isochronous transfer data buffer.
	ULONG Offset;

	//! Set by the host controller to indicate the actual number of bytes received by the device for isochronous IN transfers. Length not used for isochronous OUT transfers.
	ULONG Length;

	//! Contains the USBD status, on return from the host controller driver, of this transfer packet.
	/*!
	* See MSDN for USBD status codes: <A href="http://msdn.microsoft.com/en-us/library/ff539136%28VS.85%29.aspx">USBD status code reference</A>
	*/
	ULONG Status;

} KUSB_ISO_PACKET;
//! pointer to a \c KUSB_ISO_PACKET structure
typedef KUSB_ISO_PACKET* PKUSB_ISO_PACKET;

#pragma warning(disable:4200)

//! Structure describing a user defined isochronous transfer.
typedef struct _KUSB_ISO_CONTEXT
{
	//! An 8-bit value that consists of a 7-bit address and a direction bit.
	/*
	* This parameter corresponds to the bEndpointAddress field in the endpoint
	* descriptor.
	*/
	UCHAR PipeID;

	//! Specifies the frame number that the transfer should begin on (0 for ASAP).
	/*!
	* This variable must be within a system-defined range of the current
	* frame. The range is specified by the constant
	* \ref USBD_ISO_START_FRAME_RANGE.
	*
	* If 0 was specified (start ASAP), this member contains the frame number that the
	* transfer began on when the request is returned by the host controller
	* driver. Otherwise, this member must contain the frame number that this
	* transfer begins on.
	*
	*/
	ULONG StartFrame;

	//! Contains the number of packets that completed with an error condition on return from the host controller driver.
	ULONG ErrorCount;

	//! Specifies the number of packets that are described by the variable-length array member \c IsoPacket.
	ULONG NumberOfPackets;

	//! Contains a variable-length array of \c KUSB_ISO_PACKET structures that describe the isochronous transfer packets to be transferred on the USB bus.
	KUSB_ISO_PACKET IsoPackets[0];

} KUSB_ISO_CONTEXT;
//! pointer to a \c KUSB_ISO_CONTEXT structure
typedef KUSB_ISO_CONTEXT* PKUSB_ISO_CONTEXT;

#pragma warning(default:4200)

#include <poppack.h>

#endif // __WUSBIO_H__

#endif // __WINUSB_COMPAT_IO_H__

#endif

