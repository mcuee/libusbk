/*! \file drv_api.h
*/
#include "..\includes\lusbk_shared.h"

#ifndef __LIBUSBK_KUSB_DRIVER_API_H__
#define __LIBUSBK_KUSB_DRIVER_API_H__

enum
{
    LIBUSB_DEBUG_OFF,
    LIBUSB_DEBUG_ERR,
    LIBUSB_DEBUG_WRN,
    LIBUSB_DEBUG_MSG,

    LIBUSB_DEBUG_MAX = 0xff,
};


/* 64k */
#define LIBUSB_MAX_READ_WRITE 0x10000

#define LIBUSB_MAX_NUMBER_OF_DEVICES 256
#define LIBUSB_MAX_NUMBER_OF_CHILDREN 32

#define LIBUSB_IOCTL_SET_CONFIGURATION CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_GET_CONFIGURATION CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_SET_INTERFACE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_GET_INTERFACE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_SET_FEATURE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_CLEAR_FEATURE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_GET_STATUS CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_SET_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_GET_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x80A, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x80B, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_VENDOR_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x80C, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_VENDOR_READ CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x80D, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_RESET_ENDPOINT CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x80E, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_ABORT_ENDPOINT CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x80F, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_RESET_DEVICE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x810, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_SET_DEBUG_LEVEL CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x811, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_GET_VERSION CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x812, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_ISOCHRONOUS_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x813, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_ISOCHRONOUS_READ CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x814, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_CLAIM_INTERFACE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x815, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_RELEASE_INTERFACE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x816, METHOD_BUFFERED, FILE_ANY_ACCESS)

/////////////////////////////////////////////////////////////////////////////
// supported after 0.1.12.2
/////////////////////////////////////////////////////////////////////////////

// [trobinso] adds support for querying device properties
#define LIBUSB_IOCTL_GET_DEVICE_PROPERTY CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_GET_CUSTOM_REG_PROPERTY CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// supported after 1.2.0.0
/////////////////////////////////////////////////////////////////////////////
#define LIBUSB_IOCTL_GET_CACHED_CONFIGURATION CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// supported in libusbK (3.0.0.0 and up)
/////////////////////////////////////////////////////////////////////////////
#define LIBUSB_IOCTL_QUERY_DEVICE_INFORMATION CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x904, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_SET_PIPE_POLICY CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x906, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_GET_PIPE_POLICY CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x907, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_SET_POWER_POLICY CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x908, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_GET_POWER_POLICY CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x909, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_CONTROL_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x90A, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_CONTROL_READ CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x90B, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define LIBUSB_IOCTL_FLUSH_PIPE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x90C, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSBK_IOCTL_CLAIM_INTERFACE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x90D, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSBK_IOCTL_RELEASE_INTERFACE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x90E, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSBK_IOCTL_RELEASE_ALL_INTERFACES CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x90F, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSBK_IOCTL_SET_INTERFACE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x910, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSBK_IOCTL_GET_INTERFACE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x911, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSBK_IOCTL_GET_CURRENTFRAME_NUMBER CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x912, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LIBUSBK_IOCTL_ISOEX_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x913, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

#define LIBUSBK_IOCTL_ISOEX_READ CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x914, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define LIBUSBK_IOCTL_AUTOISOEX_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x915, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

#define LIBUSBK_IOCTL_AUTOISOEX_READ CTL_CODE(FILE_DEVICE_UNKNOWN,\
        0x916, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

/////////////////////////////////////////////////////////////////////////////

#include <pshpack1.h>

enum LIBUSB0_TRANSFER_FLAGS
{
    TRANSFER_FLAGS_SHORT_NOT_OK = 1 << 0,
    TRANSFER_FLAGS_ISO_SET_START_FRAME = 1 << 30,
    TRANSFER_FLAGS_ISO_ADD_LATENCY = 1 << 31,
};

#pragma warning(disable:4201)

#pragma warning(disable:4214)
typedef struct
{
	unsigned int interface_number;
	unsigned int altsetting_number;

	unsigned char intf_use_index: 1;	// libusbK Only
	unsigned char altf_use_index: 1;	// libusbK Only
	unsigned char: 6;

	short interface_index;		// libusbK Only
	short altsetting_index;		// libusbK Only
} interface_request_t;
#pragma warning(default:4214)

typedef struct
{
	unsigned int major;
	unsigned int minor;
	unsigned int micro;
	unsigned int nano;
	unsigned int mod_value;
} version_t;

typedef struct
{
	unsigned int timeout;
	union
	{
		struct
		{
			unsigned int configuration;
		} configuration;

		interface_request_t intf; // libusbK Only

		version_t version;

		struct
		{
			unsigned int endpoint;
			unsigned int packet_size;

			unsigned int unused; // max_transfer_size is deprecated; (see pipe policies)
			unsigned int transfer_flags;
			unsigned int iso_start_frame_latency;
		} endpoint;

		struct
		{
			UCHAR PipeID;
			ULONG IsoContextSize;
			PKISO_CONTEXT IsoContext;
		} IsoEx;
		struct
		{
			UCHAR PipeID;
		} AutoIsoEx;
		struct
		{
			unsigned int type;
			unsigned int recipient;
			unsigned int request;
			unsigned int value;
			unsigned int index;
		} vendor;
		struct
		{
			unsigned int recipient;
			unsigned int feature;
			unsigned int index;
		} feature;
		struct
		{
			unsigned int recipient;
			unsigned int index;
			unsigned int status;
		} status;
		struct
		{
			unsigned int type;
			unsigned int index;
			unsigned int language_id;
			unsigned int recipient;
		} descriptor;
		struct
		{
			unsigned int level;
		} debug;

		struct
		{
			unsigned int property;
		} device_property;
		struct
		{
			unsigned int key_type;
			unsigned int name_offset;
			unsigned int value_offset;
			unsigned int value_length;
		} device_registry_key;
		struct
		{
			ULONG information_type;
		} query_device;
		struct
		{
			unsigned int interface_index;
			unsigned int pipe_id;
			unsigned int policy_type;
		} pipe_policy;
		struct
		{
			unsigned int policy_type;
		} power_policy;

		// WDF_USB_CONTROL_SETUP_PACKET control;
		union
		{
			struct
			{
				UCHAR   RequestType;
				UCHAR   Request;
				USHORT  Value;
				USHORT  Index;
				USHORT  Length;
			} control;
		};
	};
} libusb_request;
#pragma warning(default:4201)
#include <poppack.h>

#endif
