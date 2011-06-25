/**
 * \file
 *
 * \brief USB configuration file for USBK application
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

#ifndef _CONF_USB_H_
#define _CONF_USB_H_

//! UDC patched by Travis to disable interface  level SOF handlers.
#define UDC_INTERFACE_SOF_HANDLERS_DISABLED

#include "compiler.h"

/**
 * USB Device Configuration
 * @{
 */

//! Device definition (mandatory)
#define  USB_DEVICE_VENDOR_ID             0x03EB
#define  USB_DEVICE_PRODUCT_ID            0x2315
#define  USB_DEVICE_MAJOR_VERSION         1
#define  USB_DEVICE_MINOR_VERSION         0
#define  USB_DEVICE_POWER                 50 // Consumption on Vbus line (mA)

#define  USB_DEVICE_ATTR                  (USB_CONFIG_ATTR_BUS_POWERED)
//#define  USB_DEVICE_ATTR (USB_CONFIG_ATTR_BUS_POWERED)
//#define  USB_DEVICE_ATTR (USB_CONFIG_ATTR_REMOTE_WAKEUP|USB_CONFIG_ATTR_SELF_POWERED)
//#define  USB_DEVICE_ATTR (USB_CONFIG_ATTR_REMOTE_WAKEUP|USB_CONFIG_ATTR_BUS_POWERED)

//! USB Device string definitions (Optional)
#define  USB_DEVICE_MANUFACTURE_NAME      "Travis Robinson"
#define  USB_DEVICE_PRODUCT_NAME          "Benchmark Device"
#define  USB_DEVICE_SERIAL_NAME           "KUC3A0001"

//! To authorize the High speed
#if (UC3A3||UC3A4)
// #define  USB_DEVICE_HS_SUPPORT
#endif

//! Control endpoint size. (Endpoint 0)
#define  USB_DEVICE_EP_CTRL_SIZE	64

//! Number of endpoints used by all interfaces.
#define  USB_DEVICE_MAX_EP			2

//! USB Device Callbacks definitions (Optional)
extern void user_callback_vbus_event(bool b_high);
#define  UDC_VBUS_EVENT(b_vbus_high) user_callback_vbus_event(b_vbus_high)

// #define  UDC_SOF_EVENT()                  user_callback_sof_action()
// #define  UDC_SUSPEND_EVENT()              user_callback_suspend_action()
// #define  UDC_RESUME_EVENT()               user_callback_resume_action()

//! Mandatory when USB_DEVICE_ATTR authorizes remote wakeup feature
// #define  UDC_REMOTEWAKEUP_ENABLE()        user_callback_remotewakeup_enable()
// #define  UDC_REMOTEWAKEUP_DISABLE()       user_callback_remotewakeup_disable()

//! When a extra string descriptor must be supported other than manufacturer, product and serial string
// #define  UDC_GET_EXTRA_STRING()

#include "conf_benchmark.h"

#endif // _CONF_USB_H_
