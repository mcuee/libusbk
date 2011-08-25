/*! \file usb_config.h
* \brief Benchmark USB device configuration file.
*
* The benchmark firmware has many options that can be configured at compile-time.
* Some options are not supported by all MCP USB chips, others cannot be achieved
* because of memory requirements.  If the firmware fails to build, it is most 
* likely because of one of these two reasons.
*
*/

/*! \mainpage 
* - \ref config_device
* - \ref config_interface
*/

#ifndef USBCFG_H
#define USBCFG_H

#define EP_ISO        0x01
#define EP_BULK       0x02
#define EP_INT        0x03

#include "usb_config_external.h"

/*! \addtogroup config_device Device Setup
@{
*/

/*! \def DUAL_INTERFACE
* \brief Configure for two non-associated interfaces.
*
* If defined, creates a dual interfaces device where each interface is exposed as a seperate device under windows.
*
*/
#ifndef USBCFG_H
#define DUAL_INTERFACE
#endif

/*! \def DUAL_INTERFACE_WITH_ASSOCIATION
* \brief Configure for two associated interfaces.
*
* If defined, creates a dual interfaces device where both interfaces are exposed as one device under windows.
*
*/
#ifndef USBCFG_H
#define DUAL_INTERFACE_WITH_ASSOCIATION
#endif

/*! \def SINGLE_INTERFACE_WITH_ALTSETTINGS
* \brief Configure for one interface with two alt settings.
*
* If defined, creates a single interface device with two alternate settings.
*
* If \ref INTF0 is configured for bulk or interrupt endpoints:
* - Alt setting 0 will have endpoints with \b wMaxPacketSize = USBGEN_EP_SIZE_INTF0 / 2.
* - Alt setting 1 will have endpoints with \b wMaxPacketSize = USBGEN_EP_SIZE_INTF0.
*
* If \ref INTF0 is configured for ISO endpoints:
* - Alt setting 0 will have endpoints with \b wMaxPacketSize = 0.
* - Alt setting 1 will have endpoints with \b wMaxPacketSize = USBGEN_EP_SIZE_INTF0.
*
*/
#ifndef USBCFG_H
#define SINGLE_INTERFACE_WITH_ALTSETTINGS
#endif

/*! \def VENDOR_BUFFER_ENABLED
* \brief Enables EP0 get/set vendor buffer.
*
* If defined, enables additional control requests and an 8 byte buffer for storing and 
* retrieving data.
*
*/
#ifndef USBCFG_H
#define VENDOR_BUFFER_ENABLED
#endif

/*! \def DESCRIPTOR_COUNTING_ENABLED
* \brief Enables descriptor request counting to vendor buffer.
*
* If defined, uses the vendor buffer to store descriptor request count information.
*
*/
#ifndef USBCFG_H
#define DESCRIPTOR_COUNTING_ENABLED
#endif

#if defined(DESCRIPTOR_COUNTING_ENABLED) && !defined(VENDOR_BUFFER_ENABLED)
#define VENDOR_BUFFER_ENABLED
#endif

#if defined(DUAL_INTERFACE_WITH_ASSOCIATION) && !defined(DUAL_INTERFACE)
#define DUAL_INTERFACE
#endif

#if defined(SINGLE_INTERFACE_WITH_ALTSETTINGS) && defined(DUAL_INTERFACE)
#error "Dual interface and single interface with altsettings defines cannot be combined."
#endif

// HARDWARE ID CONFIGURATION ////////////////////////////////////////
#define VENDOR_ID			0x04D8
#define PRODUCT_ID			0xFA2E
#define BCD_RELEASE_NUMBER	0x0001

// DESCRIPTOR STRING CONFIGURATION //////////////////////////////////
#define MANUFACTURER_STRING_LENGTH 15
#define MANUFACTURER_STRING 'T','r','a','v','i','s',' ','R','o','b','i','n','s','o','n'

#if !defined(DUAL_INTERFACE)
	#define SERIAL_NUMBER_LENGTH 6
	#define SERIAL_NUMBER 'L','U','S','B','W','1'

	#define PRODUCT_STRING_LENGTH 16
	#define PRODUCT_STRING 'B','e','n','c','h','m','a','r','k',' ','D','e','v','i','c','e'
#else
	#define SERIAL_NUMBER_LENGTH 6
	#define SERIAL_NUMBER 'L','U','S','B','W','2'

	#define PRODUCT_STRING_LENGTH 21
	#define PRODUCT_STRING 'D','u','a','l',' ','B','e','n','c','h','m','a','r','k',' ','D','e','v','i','c','e'

	#define INTF0_STRING_LENGTH 13
	#define INTF0_STRING 'B','e','n','c','h','m','a','r','k',' ','O','n','e'

	#define INTF1_STRING_LENGTH 13
	#define INTF1_STRING 'B','e','n','c','h','m','a','r','k',' ','T','w','o'
#endif

/*!@}*/

/*! \addtogroup config_interface Interface and Endpoint Setup
@{
*/

// Interface number to use in interface descriptor(s)
#define INTF0_NUMBER 0
#define INTF1_NUMBER 1

/*! \def USBGEN_EP_SIZE_INTF0
* \brief wMaxPacketSize for IN and OUT endpoints on interface 0.
*
*/
#ifndef USBGEN_EP_SIZE_INTF0
	#define USBGEN_EP_SIZE_INTF0	32
#endif

/*! \def INTF0
* \brief Endpoint type for IN and OUT endpoints on interface 0.
*
* Valid values are \c EP_ISO, \c EP_BULK, and \c EP_INT.
*/
#ifndef INTF0
	#define INTF0 EP_BULK
#endif

/*! \def USBGEN_EP_SIZE_INTF1
* \brief wMaxPacketSize for IN and OUT endpoints on interface 1.
*
*/
#ifndef USBGEN_EP_SIZE_INTF1
	#define USBGEN_EP_SIZE_INTF1	32
#endif

/*! \def INTF1
* \brief Endpoint type for IN and OUT endpoints on interface 1.
* 
* Valid values are \c EP_ISO, \c EP_BULK, and \c EP_INT.
*/
#ifndef INTF1
	#define INTF1 EP_BULK
#endif

/*!@}*/

/////////////////////////////////////////////////////////////////////
// USB SERVICE MODE
#if !defined(USB_POLLING) && !defined(USB_INTERRUPT)
#define USB_INTERRUPT
#endif
/////////////////////////////////////////////////////////////////////

// INTERFACE & ENDPOINT INTERNAL SETUP //////////////////////////////
#if (INTF0==EP_BULK)
	#define USBGEN_EP_ATTRIBUTES_INTF0		EP_BULK
	#define USBGEN_EP_HANDSHAKE_INTF0		USB_HANDSHAKE_ENABLED
	#define USBGEN_EP_INTERVAL_INTF0		0
#elif (INTF0==EP_INT)
	#define USBGEN_EP_ATTRIBUTES_INTF0		EP_INT
	#define USBGEN_EP_HANDSHAKE_INTF0		USB_HANDSHAKE_ENABLED
	#define USBGEN_EP_INTERVAL_INTF0		1
#elif (INTF0==EP_ISO)
	#define USBGEN_EP_ATTRIBUTES_INTF0		EP_ISO|_AS|_DE
	#define USBGEN_EP_HANDSHAKE_INTF0		0
	#define USBGEN_EP_INTERVAL_INTF0		1
#endif

#if (INTF1==EP_BULK)
	#define USBGEN_EP_ATTRIBUTES_INTF1		EP_BULK
	#define USBGEN_EP_HANDSHAKE_INTF1		USB_HANDSHAKE_ENABLED
	#define USBGEN_EP_INTERVAL_INTF1		0
#elif (INTF1==EP_INT)
	#define USBGEN_EP_ATTRIBUTES_INTF1		EP_INT
	#define USBGEN_EP_HANDSHAKE_INTF1		USB_HANDSHAKE_ENABLED
	#define USBGEN_EP_INTERVAL_INTF1		1
#elif (INTF1==EP_ISO)
	#define USBGEN_EP_ATTRIBUTES_INTF1		EP_ISO|_AS|_DE
	#define USBGEN_EP_HANDSHAKE_INTF1		0
	#define USBGEN_EP_INTERVAL_INTF1		1
#endif

#if defined(SINGLE_INTERFACE_WITH_ALTSETTINGS)
	#if (INTF0==EP_ISO)
		#define USBGEN_EP_SIZE_INTF0_ALT0 (0)
	#else
		#define USBGEN_EP_SIZE_INTF0_ALT0 (USBGEN_EP_SIZE_INTF0/2)
	#endif
	#define USBGEN_EP_SIZE_INTF0_ALT1 USBGEN_EP_SIZE_INTF0
#endif

/////////////////////////////////////////////////////////////////////


/* DEFINITIONS ****************************************************/
#define USBGEN_EP_NUM_INTF0		1
#define USBGEN_EP_NUM_INTF1		2

#define USB_EP0_BUFF_SIZE		8	// Valid Options: 8, 16, 32, or 64 bytes.
								// Using larger options take more SRAM, but
								// does not provide much advantage in most types
								// of applications.  Exceptions to this, are applications
								// that use EP0 IN or OUT for sending large amounts of
								// application related data.

#if defined(DUAL_INTERFACE)
	#define USB_MAX_NUM_INT     		(INTF1_NUMBER+1)   // For tracking Alternate Setting
	#define USB_MAX_EP_NUMBER			2
	#define USB_NUM_STRING_DESCRIPTORS	6
#else
	#define USB_MAX_NUM_INT     		(INTF0_NUMBER+1)   // For tracking Alternate Setting
	#define USB_MAX_EP_NUMBER			1
	#define USB_NUM_STRING_DESCRIPTORS	4
#endif

//Device descriptor - if these two definitions are not defined then
//  a ROM USB_DEVICE_DESCRIPTOR variable by the exact name of device_dsc
//  must exist.
#define USB_USER_DEVICE_DESCRIPTOR &device_dsc
#define USB_USER_DEVICE_DESCRIPTOR_INCLUDE extern ROM USB_DEVICE_DESCRIPTOR device_dsc

//Configuration descriptors - if these two definitions do not exist then
//  a ROM BYTE *ROM variable named exactly USB_CD_Ptr[] must exist.
#define USB_USER_CONFIG_DESCRIPTOR USB_CD_Ptr
#define USB_USER_CONFIG_DESCRIPTOR_INCLUDE extern ROM BYTE *ROM USB_CD_Ptr[]

//Make sure only one of the below "#define USB_PING_PONG_MODE"
//is uncommented.

#if defined(__C32__)
  #define USB_PING_PONG_MODE USB_PING_PONG__FULL_PING_PONG
#else
// #define USB_PING_PONG_MODE USB_PING_PONG__NO_PING_PONG
#define USB_PING_PONG_MODE USB_PING_PONG__FULL_PING_PONG
// #define USB_PING_PONG_MODE USB_PING_PONG__EP0_OUT_ONLY
// #define USB_PING_PONG_MODE USB_PING_PONG__ALL_BUT_EP0		//NOTE: This mode is not supported in PIC18F4550 family rev A3 devices
#endif


/* Parameter definitions are defined in usb_device.h */
#define USB_PULLUP_OPTION USB_PULLUP_ENABLE
//#define USB_PULLUP_OPTION USB_PULLUP_DISABLED

#define USB_TRANSCEIVER_OPTION USB_INTERNAL_TRANSCEIVER
//External Transceiver support is not available on all product families.  Please
//  refer to the product family datasheet for more information if this feature
//  is available on the target processor.
//#define USB_TRANSCEIVER_OPTION USB_EXTERNAL_TRANSCEIVER

#define USB_SPEED_OPTION USB_FULL_SPEED
//#define USB_SPEED_OPTION USB_LOW_SPEED //(not valid option for PIC24F devices)

#define USB_SUPPORT_DEVICE

// The USB_ENABLE_xx are not implemented in the MCP 2.7 stack.
#define USB_ENABLE_ALL_HANDLERS


/* DEVICE CLASS USAGE *********************************************/
#define USB_USE_GEN
#define	EVN	0
#define	ODD	1

#endif //USBCFG_H
