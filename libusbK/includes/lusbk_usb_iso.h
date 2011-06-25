/*! \file lusbk_usb_iso.h
* \brief structs, typedefs, enums, defines, and functions for the \ref isok.
*/
#ifndef __LUSBK_USB_ISO_H_
#define __LUSBK_USB_ISO_H_

#if !defined(WDK_DRIVER)
#include <windows.h>
#include <objbase.h>
#include "lusbk_common.h"
#endif

#include <pshpack1.h>
/*! \addtogroup isok
*  @{
*/
//! Structure describing an isochronous transfer packet.
typedef struct _KISO_PACKET
{
	//! Specifies the offset, in bytes, of the buffer for this packet from the beginning of the entire isochronous transfer data buffer.
	/*!
	* \c Offset represents an absolute data offset from the start of the \c Buffer parameter \ref UsbK_IsoReadPipe or \ref UsbK_IsoWritePipe.
	*
	* \note This field is assgined by the user application only and used by the driver upon transfer submission and completion.
	*/
	ULONG Offset;

	//! Set by the host controller to indicate the actual number of bytes received by the device for isochronous IN transfers. Length not used for isochronous OUT transfers.
	/*!
	* \note This field is is not user assignable and is updated by the driver upon transfer completion.
	*/
	ULONG Length;

	//! Contains the USBD status, on return from the host controller driver, of this transfer packet.
	/*!
	* See MSDN for USBD status codes: <A href="http://msdn.microsoft.com/en-us/library/ff539136%28VS.85%29.aspx">USBD status code reference</A>
	*
	* \note This field is is not user assignable and is updated by the driver upon transfer completion.
	*/
	ULONG Status;

} KISO_PACKET;
//! pointer to a \c KISO_PACKET structure
typedef KISO_PACKET* PKISO_PACKET;

#pragma warning(disable:4200)

//! Structure describing a user defined isochronous transfer.
/*!
*
* The \ref KISO_CONTEXT::StartFrame member of the \ref
* KISO_CONTEXT specifies the starting USB frame number for the
* transaction. The driver can use \ref UsbK_GetCurrentFrameNumber to
* request the current frame number.
*
* In full-speed transmissions, the frame number for any particular packet
* will be the sum of the start frame number and the packet index. For
* instance, the fourth packet in the \ref KISO_CONTEXT has an index of
* 3, so its frame number will be StartFrame + 3. In a write transfer, the
* port driver loads this frame with the buffer data at the data buffer
* offset specified by IsoPacket[3].Offset.
*
* When the driver processes the \ref KISO_CONTEXT, it discards all
* packets in the \ref KISO_CONTEXT whose frame numbers are lower than
* the current frame number. The port driver sets the Status member of the
* packet descriptor for each discarded packet to
* USBD_STATUS_ISO_NA_LATE_USBPORT, USBD_STATUS_ISO_NOT_ACCESSED_BY_HW or
* USBD_STATUS_ISO_NOT_ACCESSED_LATE. Even if it discards some packets, the
* port driver attempts to transmit those packets in the \ref
* KISO_CONTEXT whose frame numbers are higher than the current frame
* number.
*
* The check for a valid StartFrame member is slightly more complicated in
* high-speed transmissions because the port driver loads each packet into
* a high-speed microframe; however, the value in StartFrame refers to the
* 1 millisecond (full-speed) frame number, not the microframe. For
* example, if the StartFrame value recorded in the \ref KISO_CONTEXT
* is one less than the current frame, the port driver will discard as many
* as eight packets. The exact number of packets that the port driver
* discards depends on the period associated with the isochronous pipe.
*
* High-speed isochronous pipes can have periods of 1, 2, 4, or 8. The
* period number specifies the frequency with which the port driver inserts
* packets into the data stream. If the period is 2, for example, the port
* driver will insert a packet into the data stream every two microframes.
* This means that it will only use four of the eight microframes available
* within each 1-millisecond frame for isochronous data transmission.
*
* In general, the higher the period, the fewer packets the port driver
* will discard when a \ref KISO_CONTEXT arrives late. Assume the
* period on an isochronous pipe is 2. With a period of 2, each
* 1-millisecond speed frame will carry four packets of isochronous data
* for that pipe. So, for example, if CurrentFrame - StartFrame = 3, the
* port driver will discard 3 * 4 = 12 packets. On the other hand, if the
* period is 4, each 1-millisecond frame carries only two packets of
* isochronous data for the pipe. Therefore, if the \ref KISO_CONTEXT
* arrives three 1-millisecond frames late, as in the previous example, the
* port driver will discard 3 * 2 = 6 packets, instead of 12 packets.
*
* For all types of isochronous pipe, the distance between the current
* frame and the StartFrame value specified in the \ref KISO_CONTEXT
* must be less than USBD_ISO_START_FRAME_RANGE. If StartFrame is not
* within the proper range, the driver sets the Status member of the \ref
* KISO_PACKET \c USBD_STATUS_BAD_START_FRAME and discards the entire
* \ref KISO_CONTEXT. The following code example shows the precise
* check that the port driver does on the \ref KISO_CONTEXT start
* frame:
* \code
* if (abs((CurrentFrame - StartFrame)) > USBD_ISO_START_FRAME_RANGE)
* {
* 	// discard the KISO_CONTEXT
* }
* \endcode
*
*/
typedef struct _KISO_CONTEXT
{
	//! An 8-bit value that consists of a 7-bit address and a direction bit.
	/*
	* This parameter corresponds to the bEndpointAddress field in the endpoint
	* descriptor.
	*
	* \note This field is assgined by the user application only and used by the driver upon transfer submission.
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
	* Under certain circumstances, the driver can specify 0 for \ref
	* KISO_CONTEXT::StartFrame, and the bus driver will begin the
	* transaction in the next available frame.
	*
	* Specifing \b 0 for \ref KISO_CONTEXT::StartFrame (start transfer
	* ASAP) is restricted to the first transaction on a newly opened or reset
	* pipe. Furthermore, the USB stack contains a bug in Microsoft Windows
	* Server 2003 and Windows XP that limits the use of this to an isochronous
	* context with 255 or fewer packets.
	*
	* For more information about resetting pipes, see \ref UsbK_ResetPipe.
	*
	* \note This field may be assgined by the user application and is updated by the driver upon transfer completion.
	*/
	ULONG StartFrame;

	//! Contains the number of packets that completed with an error condition on return from the host controller driver.
	/*!
	* \note This field is is not user assignable and is updated by the driver upon transfer completion.
	*/
	ULONG ErrorCount;

	//! Contains a unqiue per-pipe transfer counter.
	/*
	*
	* For each pipe, libusbK maintains a transfer counter which increments
	* each time a transfer is submitted. This 32-bit unsigned integer intially
	* starts with \c 1 and is reset to \c 1 when new alternate settings are
	* selected or a pipe reset occurs. This counter is \c 0 \b only when it
	* reaches a \b rollover condition.
	*
	* \note This field is is not user assignable and is updated by the driver upon transfer submission.
	*/
	ULONG TransferCounter;

	//! Specifies the number of packets that are described by the variable-length array member \c IsoPacket.
	/*
	* \note This field is assgined by the user application only and used by the driver upon transfer submission and completion.
	*/
	ULONG NumberOfPackets;

	//! Contains a variable-length array of \c KISO_PACKET structures that describe the isochronous transfer packets to be transferred on the USB bus.
	/*
	* \note This field is assgined by the user application, used by the driver upon transfer submission, and updated by the driver upon transfer completion.
	*/
	KISO_PACKET IsoPackets[0];

} KISO_CONTEXT;
//! pointer to a \c KISO_CONTEXT structure
typedef KISO_CONTEXT* PKISO_CONTEXT;

#pragma warning(default:4200)

#include <poppack.h>

#if !defined(WDK_DRIVER)
// [User application/library only section]

//! Callback function typedef for \ref IsoK_EnumPackets
typedef BOOL KUSB_API KISO_PACKETS_ENUM_CB (__in ULONG PacketIndex, __inout PKISO_PACKET IsoPacket, __inout PVOID UserContext);
//! Pointer to a \ref KISO_PACKETS_ENUM_CB.
typedef KISO_PACKETS_ENUM_CB* PKISO_PACKETS_ENUM_CB;

#ifdef __cplusplus
extern "C" {
#endif

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
	* -# Zero-initializes all iso context memory.
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
	* A pointer reference to an isochronous transfer context created with \ref IsoK_Init.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*/
	KUSB_EXP BOOL KUSB_API IsoK_Free(
	    __deref_inout PKISO_CONTEXT* IsoContext);

//! Convenience function for setting the offset of all iso packets of an isochronous transfer context.
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
	* the iso packet in the PKISO_CONTEXT::IsoPackets array at the specified
	* index.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*/
	KUSB_EXP BOOL KUSB_API IsoK_GetPacket(
	    __in PKISO_CONTEXT IsoContext,
	    __in ULONG PacketIndex,
	    __out PKISO_PACKET IsoPacket);

//! Convenience function for enumerating iso packets of an isochronous transfer context.
	/*!
	* \param IsoContext
	* A pointer to an isochronous transfer context.
	*
	* \param EnumPackets
	* Pointer to a user supplied callback function which is executed for all iso packets
	* in \c IsoContext or until the user supplied callback function returns \c FALSE.
	*
	* \param StartPacketIndex
	* The zero-based iso pakcet index to begin enumeration at.
	*
	* \param UserContext
	* A user defined value which is passed as a parameter to the user supplied callback function.
	*
	* \returns On success, TRUE. Otherwise FALSE. Use \c GetLastError() to get
	* extended error information.
	*/
	KUSB_EXP BOOL KUSB_API IsoK_EnumPackets(
	    __in PKISO_CONTEXT IsoContext,
	    __in PKISO_PACKETS_ENUM_CB EnumPackets,
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

#ifdef __cplusplus
}
#endif

/*! @} */
#endif // WDK_DRIVER

#endif // __LUSBK_USB_ISO_H_
