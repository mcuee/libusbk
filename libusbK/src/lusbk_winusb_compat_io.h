/* libusb-win32 WDF, Generic KMDF Windows USB Driver
 * Copyright (c) 2010-2011 Travis Robinson <libusbdotnet@gmail.com>
 * Copyright (c) 2002-2005 Stephan Meyer <ste_meyer@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINUSB_COMPAT_IO_H__
#define __WINUSB_COMPAT_IO_H__

#ifndef __WUSBIO_H__

#include <usb.h>
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

#endif // __WUSBIO_H__

#endif // __WINUSB_COMPAT_IO_H__
