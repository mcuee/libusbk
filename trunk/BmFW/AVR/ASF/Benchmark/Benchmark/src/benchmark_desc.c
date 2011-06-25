/*! benchmark_desc.c
 Created: 6/8/2011 10:27:18 PM
 Author : Travis

 - Copyright (c) 2011, Travis Lee Robinson
 - All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Travis Lee Robinson nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS ROBINSON BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "conf_usb.h"
#include "usb_protocol.h"
#include "udd.h"
#include "udc.h"
#include "udc_desc.h"
#include "benchmark_desc.h"
#include <string.h>

//! Status of USBK interface
volatile bool udi_usbk_running=false;

//! USB Device Descriptor
COMPILER_WORD_ALIGNED UDC_DESC_STORAGE usb_dev_desc_t udc_device_desc =
{
	.bLength                   = sizeof(usb_dev_desc_t),
	.bDescriptorType           = USB_DT_DEVICE,
	.bcdUSB                    = LE16(USB_V2_0),
	.bDeviceClass              = 0,
	.bDeviceSubClass           = 0,
	.bDeviceProtocol           = 0,
	.bMaxPacketSize0           = USB_DEVICE_EP_CTRL_SIZE,
	.idVendor                  = LE16(USB_DEVICE_VENDOR_ID),
	.idProduct                 = LE16(USB_DEVICE_PRODUCT_ID),
	.bcdDevice                 = LE16((USB_DEVICE_MAJOR_VERSION << 8) | USB_DEVICE_MINOR_VERSION),
	.iManufacturer             = 1,
	.iProduct                  = 2,
	.iSerialNumber             = 3,
	.bNumConfigurations        = 1
};


//! USB Device Qualifier Descriptor for HS
#ifdef USB_DEVICE_HS_SUPPORT
COMPILER_WORD_ALIGNED UDC_DESC_STORAGE usb_dev_qual_desc_t udc_device_qual =
{
	.bLength                   = sizeof(usb_dev_qual_desc_t),
	.bDescriptorType           = USB_DT_DEVICE_QUALIFIER,
	.bcdUSB                    = LE16(USB_V2_0),
	.bDeviceClass              = 0,
	.bDeviceSubClass           = 0,
	.bDeviceProtocol           = 0,
	.bMaxPacketSize0           = USB_DEVICE_EP_CTRL_SIZE,
	.bNumConfigurations        = 1
};
#endif

//! Structure for USB Device Configuration Descriptor
COMPILER_PACK_SET(1);
typedef struct
{
	usb_conf_desc_t conf;
	udi_usbk_data_desc_t udi_usbk_data;
} udc_desc_t;
COMPILER_PACK_RESET();

//! USB Device Configuration Descriptor filled for full and high speed
COMPILER_WORD_ALIGNED UDC_DESC_STORAGE udc_desc_t udc_desc =
{
	.conf.bLength              = sizeof(usb_conf_desc_t),
	.conf.bDescriptorType      = USB_DT_CONFIGURATION,
	.conf.wTotalLength         = LE16(sizeof(udc_desc_t)),
	.conf.bNumInterfaces       = 1,
	.conf.bConfigurationValue  = 1,
	.conf.iConfiguration       = 0,
	.conf.bmAttributes         = USB_CONFIG_ATTR_MUST_SET | USB_DEVICE_ATTR,
	.conf.bMaxPower            = USB_CONFIG_MAX_POWER(USB_DEVICE_POWER),
	.udi_usbk_data              = UDI_USBK_DATA_DESC,
};

//! Associate a UDI for each USB interface into a global UDC structure.
UDC_DESC_STORAGE udi_api_t *udi_apis[1] =
{
	&udi_api_usbk_data,
};

//! Add UDI with USB descriptors FS & HS into a global UDC structure.
UDC_DESC_STORAGE udc_config_speed_t udc_config_fshs[1] =
{
	{
		.desc = (usb_conf_desc_t UDC_DESC_STORAGE*)&udc_desc,
		.udi_apis = udi_apis,
	}
};

//! Add all information about USB device into a global UDC structure.
UDC_DESC_STORAGE udc_config_t udc_config =
{
	.confdev_lsfs = &udc_device_desc,
	.conf_lsfs = udc_config_fshs,
#ifdef USB_DEVICE_HS_SUPPORT
	.confdev_hs = &udc_device_desc,
	.qualifier = &udc_device_qual,
	.conf_hs = udc_config_fshs,
#endif
};

//! Add the user specific interface API functions into a global UDC structure.
UDC_DESC_STORAGE udi_api_t udi_api_usbk_data =
{
	.enable = udi_usbk_data_enable,
	.disable = udi_usbk_data_disable,
	.setup = udi_usbk_data_setup,
	.getsetting = udi_usbk_getsetting,
	.sof_notify = udi_usbk_data_sof_notify,
};

//! Called by UDC when this interface is enabled.
bool udi_usbk_data_enable(void)
{
	udi_usbk_running=true;
	return udi_usbk_running;
}

//! Called by UDC when this interface is disabled.
void udi_usbk_data_disable(void)
{
	udi_usbk_running=false;
}

//! Called by UDC when this interface is configured.
bool udi_usbk_data_setup(void)
{
	// request not supported
	return false;
}

//! Called by UDC when the alt setting number for this interface is requested.
uint8_t udi_usbk_getsetting(void)
{
	// only supporting alt-setting #0
	return 0;
}

//! [INACTIVATED BY TRAVIS]
//!
void udi_usbk_data_sof_notify(void)
{
}
