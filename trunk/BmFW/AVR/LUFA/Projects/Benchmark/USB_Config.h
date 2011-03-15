/*
             LUFA Library
     Copyright (C) Dean Camera, 2010.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2011  Pete Batard (pbatard [at] gmail [dot] com)
  Copyright 2010-2011 Travis Robinson (libusb.win32.support [at] gmail [dot] com)
  Copyright 2010  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortuous action,
  arising out of or in connection with the use or performance of
  this software.
*/ 


#ifndef USB_CONFIG_H_
#define USB_CONFIG_H_

	/* Descriptor configuration */

	/* DUAL_INTERFACE Selection:
	 * if defined, creates a dual interfaces device each interface will be 
	 * exposed as a separate device (windows). */

	// #define DUAL_INTERFACE

	/* DUAL_INTERFACE_WITH_ASSOCIATION Selection:
	 * If defined, creates a single device with two interfaces. */

	// #define DUAL_INTERFACE_WITH_ASSOCIATION

	/* SINGLE_INTERFACE_WITH_ALTSETTINGS Selection:
	 * If defined, creates a single interface device with an additional alt 
	 * setting. By default, the first alt setting will have either 0 or 
	 * USBGEN_EP_SIZE_INTF0/2 for its endpoints wMaxPacketSize's (0 for ISO 
	 * endpoints). The second alt setting exposes the interface with the actual 
	 * wMaxPacketSize setting. */

	#define SINGLE_INTERFACE_WITH_ALTSETTINGS

	/* ENABLE_VENDOR_BUFFER_AND_SET_DESCRIPTOR Selection:
	 * Enables additional control requests and an 8 byte buffer for storing and 
	 * retrieving data. */

	#define ENABLE_VENDOR_BUFFER_AND_SET_DESCRIPTOR

	#if defined(DUAL_INTERFACE_WITH_ASSOCIATION) && !defined(DUAL_INTERFACE)
	#define DUAL_INTERFACE
	#endif

	#if defined(SINGLE_INTERFACE_WITH_ALTSETTINGS) && defined(DUAL_INTERFACE)
	#error "Dual interface and single interface with altsettings defines cannot be combined."
	#endif

	/* Hardware ID configuration */
	#define VENDOR_ID					0x1234
	#define PRODUCT_ID					0x0001
	#define RELEASE_NUMBER				00.01

	/* Descriptor String configuration */
	#define MANUFACTURER_STRING_LENGTH	15
	#define MANUFACTURER_STRING			L"Travis Robinson"

	#if !defined(DUAL_INTERFACE)
		#define SERIAL_NUMBER_LENGTH	6
		#define SERIAL_NUMBER			L"BMD001"

		#define PRODUCT_STRING_LENGTH	16
		#define PRODUCT_STRING			L"Benchmark Device"
	#else
		#define SERIAL_NUMBER_LENGTH	6
		#define SERIAL_NUMBER			L"BMD002"

		#define PRODUCT_STRING_LENGTH	21
		#define PRODUCT_STRING			L"Dual Benchmark Device"

		#define INTF0_STRING_LENGTH		13
		#define INTF0_STRING			L"Benchmark One"

		#define INTF1_STRING_LENGTH		13
		#define INTF1_STRING			L"Benchmark Two"
	#endif

	/* Interface & endpoint configuration */

	/* Interface number to use in interface descriptor(s) */
	#define INTF0_NUMBER				1
	#define INTF1_NUMBER				2

	/* Endpoint #1 (in, out) size & type */
	#define USBGEN_EP_SIZE_INTF0		64
	//#define EP_INTF0					EP_TYPE_ISOCHRONOUS
	#define EP_INTF0					EP_TYPE_BULK
	//#define EP_INTF0					EP_TYPE_INTERRUPT

	/* Endpoint #2 (in, out) size & type */
	#define USBGEN_EP_SIZE_INTF1		64
	//#define EP_INTF1					EP_TYPE_ISOCHRONOUS
	#define EP_INTF1					EP_TYPE_BULK
	//#define EP_INTF1					EP_TYPE_INTERRUPT

	/* USB Service Mode */
	//#define USB_POLLING
	#define USB_INTERRUPT

	/* Interface & endpoint internal setup */
	#if (EP_INTF0 == EP_TYPE_BULK)
		#define USBGEN_EP_ATTRIBUTES_INTF0		(EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA)
		#define USBGEN_EP_HANDSHAKE_INTF0		USB_HANDSHAKE_ENABLED
		#define USBGEN_EP_INTERVAL_INTF0		0
	#elif (EP_INTF0 == EP_TYPE_INTERRUPT)
		#define USBGEN_EP_ATTRIBUTES_INTF0		(EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA)
		#define USBGEN_EP_HANDSHAKE_INTF0		USB_HANDSHAKE_ENABLED
		#define USBGEN_EP_INTERVAL_INTF0		1
	#elif (EP_INTF0 == EP_TYPE_ISOCHRONOUS)
		#define USBGEN_EP_ATTRIBUTES_INTF0		(EP_TYPE_ISOCHRONOUS | ENDPOINT_ATTR_ASYNC | ENDPOINT_USAGE_DATA)
		#define USBGEN_EP_HANDSHAKE_INTF0		0
		#define USBGEN_EP_INTERVAL_INTF0		1
	#endif

	#if (EP_INTF1 == EP_TYPE_BULK)
		#define USBGEN_EP_ATTRIBUTES_INTF1		(EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA)
		#define USBGEN_EP_HANDSHAKE_INTF1		USB_HANDSHAKE_ENABLED
		#define USBGEN_EP_INTERVAL_INTF1		0
	#elif (EP_INTF1 == EP_TYPE_INTERRUPT)
		#define USBGEN_EP_ATTRIBUTES_INTF1		(EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA)
		#define USBGEN_EP_HANDSHAKE_INTF1		USB_HANDSHAKE_ENABLED
		#define USBGEN_EP_INTERVAL_INTF1		1
	#elif (EP_INTF1 == EP_TYPE_ISOCHRONOUS)
		#define USBGEN_EP_ATTRIBUTES_INTF1		(EP_TYPE_ISOCHRONOUS | ENDPOINT_ATTR_ASYNC | ENDPOINT_USAGE_DATA)
		#define USBGEN_EP_HANDSHAKE_INTF1		0
		#define USBGEN_EP_INTERVAL_INTF1		1
	#endif

	#if (EP_INTF0 == EP_TYPE_ISOCHRONOUS)
		#define USBGEN_EP_SIZE_INTF0_ALT0		0
	#else
		#define USBGEN_EP_SIZE_INTF0_ALT0		(USBGEN_EP_SIZE_INTF0/2)
	#endif
	#define USBGEN_EP_SIZE_INTF0_ALT1			USBGEN_EP_SIZE_INTF0

	#define PP_COUNT						2		// Ping Pong Count
	#define USB_NEXT_PING_PONG				4
	
	#define mBDT_MaskAndToggleDTS(BdtPtr)  (BdtPtr->STAT.Val & _DTSMASK)
	#define mBDT_IsOdd(BdtPtr)             ((((BYTE_VAL*)&BdtPtr)->Val & USB_NEXT_PING_PONG)?1:0)
	#define mBDT_TogglePP(BdtPtr)          ((BYTE_VAL*)&BdtPtr)->Val ^= USB_NEXT_PING_PONG	

	#define USBGEN_EP_NUM_INTF0				1
	#define USBGEN_EP_NUM_INTF1				2

	#define USB_EP0_BUFF_SIZE				8
	
	#if defined(SINGLE_INTERFACE_WITH_ALTSETTINGS)
		#if (EP_INTF0 == EP_TYPE_ISOCHRONOUS)
			#define USBGEN_EP_SIZE_INTF0_ALT0 (0)
		#else
			#define USBGEN_EP_SIZE_INTF0_ALT0 (USBGEN_EP_SIZE_INTF0/2)
		#endif
		#define USBGEN_EP_SIZE_INTF0_ALT1 USBGEN_EP_SIZE_INTF0
	#endif	

	#if defined(DUAL_INTERFACE)
		#define USB_MAX_NUM_INT				(INTF1_NUMBER + 1)	// For tracking Alternate Setting
		#define USB_MAX_EP_NUMBER			2
		#define USB_NUM_STRING_DESCRIPTORS	6
	#else
		#define USB_MAX_NUM_INT				(INTF0_NUMBER + 1)	// For tracking Alternate Setting
		#define USB_MAX_EP_NUMBER			1
		#define USB_NUM_STRING_DESCRIPTORS	4
	#endif


	/* Make sure only one of the below "#define USB_PING_PONG_MODE"
	 * is uncommented. */
	//#define USB_PING_PONG_MODE USB_PING_PONG__NO_PING_PONG
	#define USB_PING_PONG_MODE USB_PING_PONG__FULL_PING_PONG
	//#define USB_PING_PONG_MODE USB_PING_PONG__EP0_OUT_ONLY
	//#define USB_PING_PONG_MODE USB_PING_PONG__ALL_BUT_EP0		//NOTE: This mode is not supported in PIC18F4550 family rev A3 devices

#endif /* USB_CONFIG_H_ */