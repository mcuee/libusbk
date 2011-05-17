

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

typedef struct _WINUSB_PIPE_INFORMATION
{
	USB_ENDPOINT_TYPE	PipeType;
	UCHAR				PipeId;
	USHORT				MaximumPacketSize;
	UCHAR				Interval;
}* PWINUSB_PIPE_INFORMATION, WINUSB_PIPE_INFORMATION;
C_ASSERT(sizeof(WINUSB_PIPE_INFORMATION) == 12);

#include <pshpack1.h>
typedef struct _WINUSB_SETUP_PACKET
{
	UCHAR   RequestType;
	UCHAR   Request;
	USHORT  Value;
	USHORT  Index;
	USHORT  Length;
}* PWINUSB_SETUP_PACKET, WINUSB_SETUP_PACKET;
C_ASSERT(sizeof(WINUSB_SETUP_PACKET) == 8);
#include <poppack.h>

#endif // __WUSBIO_H__

#endif // __WINUSB_COMPAT_IO_H__

#endif

