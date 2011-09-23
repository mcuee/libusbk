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
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  USB Device Descriptors, for library use when in USB device mode. Descriptors are special
 *  computer-readable structures which the host requests upon device enumeration, to determine
 *  the device's capabilities and functions.
 */

#include "Descriptors.h"
#include "USB_Config.h"

/** Device descriptor structure. This descriptor, located in FLASH memory, describes the overall
 *  device characteristics, including the supported USB version, control endpoint size and the
 *  number of device configurations. The descriptor is read out by the USB host when the enumeration
 *  process begins.
 */
USB_Descriptor_Device_t PROGMEM Benchmark_DeviceDescriptor =
{
	.Header                 = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

	.USBSpecification       = VERSION_BCD(02.00),		// NB: if you change this, you must reinstall the Windows driver
	.Class                  = USB_CSCP_NoDeviceClass,
	.SubClass               = USB_CSCP_NoDeviceSubclass,
	.Protocol               = USB_CSCP_NoDeviceProtocol,

	.Endpoint0Size          = USB_EP0_BUFF_SIZE,

	.VendorID               = VENDOR_ID,
	.ProductID              = PRODUCT_ID,
	.ReleaseNumber          = VERSION_BCD(RELEASE_NUMBER),

	.ManufacturerStrIndex   = 0x01,
	.ProductStrIndex        = 0x02,
	.SerialNumStrIndex      = 0x03,

	.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

/** Configuration descriptor structure. This descriptor, located in FLASH memory, describes the usage
 *  of the device in one of its supported configurations, including information about any device interfaces
 *  and endpoints. The descriptor is read out by the USB host during the enumeration process when selecting
 *  a configuration so that the host may correctly communicate with the USB device.
 */
Benchmark_USB_Descriptor_Configuration_t PROGMEM Benchmark_ConfigurationDescriptor =
{
	.Config =
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

			.TotalConfigurationSize = sizeof(Benchmark_USB_Descriptor_Configuration_t),
#if defined(SINGLE_INTERFACE_WITH_ALTSETTINGS)
			.TotalInterfaces        = 1,
#else
			.TotalInterfaces        = 2,	// TODO: !!
#endif

			.ConfigurationNumber    = 1,
			.ConfigurationStrIndex  = NO_DESCRIPTOR,

			.ConfigAttributes       = USB_CONFIG_ATTR_BUSPOWERED | USB_CONFIG_ATTR_SELFPOWERED, // USB_CONFIG_ATTR_BUSPOWERED,

			.MaxPowerConsumption    = USB_CONFIG_POWER_MA(100)
		},

	.Benchmark_Interface_Alt0 =
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

			.InterfaceNumber        = INTF0_NUMBER,
			.AlternateSetting       = 0,

			.TotalEndpoints         = 2,

			.Class                  = 0x00,
			.SubClass               = 0x00,
			.Protocol               = 0x00,

			.InterfaceStrIndex      = NO_DESCRIPTOR
		},

	.Benchmark_DataOutEndpoint_Alt0 =
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

			.EndpointAddress        = (ENDPOINT_DIR_OUT | USBGEN_EP_NUM_INTF0),
			.Attributes             = USBGEN_EP_ATTRIBUTES_INTF0,
			.EndpointSize           = USBGEN_EP_SIZE_INTF0_ALT0,
			.PollingIntervalMS      = USBGEN_EP_INTERVAL_INTF0
		},

	.Benchmark_DataInEndpoint_Alt0 =
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

			.EndpointAddress        = (ENDPOINT_DIR_IN | USBGEN_EP_NUM_INTF0),
			.Attributes             = USBGEN_EP_ATTRIBUTES_INTF0,
			.EndpointSize           = USBGEN_EP_SIZE_INTF0_ALT0,
			.PollingIntervalMS      = USBGEN_EP_INTERVAL_INTF0
		},
		
#if defined(SINGLE_INTERFACE_WITH_ALTSETTINGS)		
	.Benchmark_Interface_Alt1 =
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

			.InterfaceNumber        = INTF0_NUMBER,
			.AlternateSetting       = 1,

			.TotalEndpoints         = 2,

			.Class                  = 0x00,
			.SubClass               = 0x00,
			.Protocol               = 0x00,

			.InterfaceStrIndex      = NO_DESCRIPTOR
		},

	.Benchmark_DataOutEndpoint_Alt1 =
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

			.EndpointAddress        = (ENDPOINT_DIR_OUT | USBGEN_EP_NUM_INTF0),
			.Attributes             = USBGEN_EP_ATTRIBUTES_INTF0,
			.EndpointSize           = USBGEN_EP_SIZE_INTF0_ALT1,
			.PollingIntervalMS      = USBGEN_EP_INTERVAL_INTF0
		},

	.Benchmark_DataInEndpoint_Alt1 =
		{
			.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

			.EndpointAddress        = (ENDPOINT_DIR_IN | USBGEN_EP_NUM_INTF0),
			.Attributes             = USBGEN_EP_ATTRIBUTES_INTF0,
			.EndpointSize           = USBGEN_EP_SIZE_INTF0_ALT1,
			.PollingIntervalMS      = USBGEN_EP_INTERVAL_INTF0
		}		
#endif	
};

/** Language descriptor structure. This descriptor, located in FLASH memory, is returned when the host requests
 *  the string descriptor with index 0 (the first index). It is actually an array of 16-bit integers, which indicate
 *  via the language ID table available at USB.org what languages the device supports for its string descriptors.
 */
USB_Descriptor_String_t PROGMEM Benchmark_LanguageString =
{
	.Header                 = {.Size = USB_STRING_LEN(1), .Type = DTYPE_String},

	.UnicodeString          = {LANGUAGE_ID_ENG}
};

/** Manufacturer descriptor string. This is a Unicode string containing the manufacturer's details in human readable
 *  form, and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
USB_Descriptor_String_t PROGMEM Benchmark_ManufacturerString =
{
	.Header                 = {.Size = USB_STRING_LEN(MANUFACTURER_STRING_LENGTH), .Type = DTYPE_String},

	.UnicodeString          = MANUFACTURER_STRING
};

/** Product descriptor string. This is a Unicode string containing the product's details in human readable form,
 *  and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
USB_Descriptor_String_t PROGMEM Benchmark_ProductString =
{
	.Header                 = {.Size = USB_STRING_LEN(PRODUCT_STRING_LENGTH), .Type = DTYPE_String},

	.UnicodeString          = PRODUCT_STRING
};

/** Serial number string. This is a Unicode string containing the device's unique serial number, expressed as a
 *  series of uppercase hexadecimal digits.
 */
USB_Descriptor_String_t PROGMEM Benchmark_SerialString =
{
	.Header                 = {.Size = USB_STRING_LEN(SERIAL_NUMBER_LENGTH), .Type = DTYPE_String},

	.UnicodeString          = SERIAL_NUMBER
};

/** This function is called by the library when in device mode, and must be overridden (see library "USB Descriptors"
 *  documentation) by the application code so that the address and size of a requested descriptor can be given
 *  to the USB library. When the device receives a Get Descriptor request on the control endpoint, this function
 *  is called so that the descriptor details can be passed back and the appropriate descriptor sent back to the
 *  USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint8_t wIndex,
                                    const void** const DescriptorAddress)
{
	const uint8_t  DescriptorType   = (wValue >> 8);
	const uint8_t  DescriptorNumber = (wValue & 0xFF);

	const void* Address = NULL;
	uint16_t    Size    = NO_DESCRIPTOR;

	switch (DescriptorType)
	{
		case DTYPE_Device:
			Address = &Benchmark_DeviceDescriptor;
			Size    = sizeof(USB_Descriptor_Device_t);
			break;
		case DTYPE_Configuration:
			Address = &Benchmark_ConfigurationDescriptor;
			Size    = sizeof(Benchmark_USB_Descriptor_Configuration_t);
			break;
		case DTYPE_String:
			switch (DescriptorNumber)
			{
				case 0x00:
					Address = &Benchmark_LanguageString;
					Size    = pgm_read_byte(&Benchmark_LanguageString.Header.Size);
					break;
				case 0x01:
					Address = &Benchmark_ManufacturerString;
					Size    = pgm_read_byte(&Benchmark_ManufacturerString.Header.Size);
					break;
				case 0x02:
					Address = &Benchmark_ProductString;
					Size    = pgm_read_byte(&Benchmark_ProductString.Header.Size);
					break;
				case 0x03:
					Address = &Benchmark_SerialString;
					Size    = pgm_read_byte(&Benchmark_SerialString.Header.Size);
					break;
			}

			break;
	}

	*DescriptorAddress = Address;
	return Size;
}

