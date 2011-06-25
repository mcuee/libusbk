/**
 * \file
 *
 * \brief USB Device Communication Device Class (USBK) interface definitions.
 *
 * Copyright (C) 2009 Atmel Corporation. All rights reserved.
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 * Atmel AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef _UDI_USBK_H_
#define _UDI_USBK_H_

#include "conf_usb.h"
#include "usb_protocol.h"
#include "udd.h"
#include "udc_desc.h"
#include "udi.h"

//! Status of USBK interface
extern volatile bool udi_usbk_running;

bool udi_usbk_data_enable(void);
void udi_usbk_data_disable(void);
bool udi_usbk_data_setup(void);
uint8_t udi_usbk_getsetting(void);
void udi_usbk_data_sof_notify(void);

//! Interface descriptor with associated endpoint descriptors.
typedef struct
{
	//! Standard interface descriptor
	usb_iface_desc_t iface;

	//! Data IN/OUT endpoint descriptors
	usb_ep_desc_t ep_in;
	usb_ep_desc_t ep_out;
} udi_usbk_data_desc_t;

//! Content of interface descriptor for all speeds
#define UDI_USBK_DATA_DESC     {\
   .iface.bInterfaceNumber       = BM_INTF_NUMBER,\
   .iface.bLength                = sizeof(usb_iface_desc_t),\
   .iface.bDescriptorType        = USB_DT_INTERFACE,\
   .iface.bAlternateSetting      = 0,\
   .iface.bNumEndpoints          = 2,\
   .iface.bInterfaceClass        = 0,\
   .iface.bInterfaceSubClass     = 0,\
   .iface.bInterfaceProtocol     = 0,\
   .iface.iInterface             = 0,\
   .ep_in.bLength                = sizeof(usb_ep_desc_t),\
   .ep_in.bDescriptorType        = USB_DT_ENDPOINT,\
   .ep_in.bEndpointAddress       = BM_EP_TX,\
   .ep_in.wMaxPacketSize         = LE16(BM_EP_MAX_PACKET_SIZE),\
   .ep_in.bmAttributes           = BM_EP_ATTRIBUTES,\
   .ep_in.bInterval              = BM_EP_INTERVAL,\
   .ep_out.bLength               = sizeof(usb_ep_desc_t),\
   .ep_out.bDescriptorType       = USB_DT_ENDPOINT,\
   .ep_out.bEndpointAddress      = BM_EP_RX,\
   .ep_out.wMaxPacketSize        = LE16(BM_EP_MAX_PACKET_SIZE),\
   .ep_out.bmAttributes          = BM_EP_ATTRIBUTES,\
   .ep_out.bInterval             = BM_EP_INTERVAL,\
   }
//@}

//! Global structure containing standard UDI API for UDC
extern UDC_DESC_STORAGE udi_api_t udi_api_usbk_data;


#ifdef __cplusplus
}
#endif
#endif // _UDI_USBK_H_
