/*! \file conf_benchmark.h
*  Created: 6/8/2011 10:31:05 PM
*  Author : Travis
* 
* \brief Benchmark USB device configuration file.
*
* - Copyright (c) 2011, Travis Lee Robinson
* - All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     - Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     - Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     - Neither the name of Travis Lee Robinson nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
* IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS ROBINSON BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _CONF_BENCHMARK_H_
#define _CONF_BENCHMARK_H_

typedef void (*Bm_RunTestDelegate) (void);

#define MAKE_INTERVAL_SIZE(Size, SizeMultiplier, Interval) ((((Size*SizeMultiplier)*Interval)+(Interval-1))/Interval)

/*!
 * USB descriptor data endpoint types.
 */
#define EP_TYPE_ISO		0x01
#define EP_TYPE_BULK	0x02
#define EP_TYPE_INT		0x03

/*!
* USB descriptor Isochronous endpoint synchronization types.
*/
//! No Synchronization
#define EP_ISO_SYNC_NS		(0x00<<2)
//! Asynchronous
#define EP_ISO_SYNC_AS		(0x01<<2)
//! Adaptive
#define EP_ISO_SYNC_AD		(0x02<<2)
//! Synchronous
#define EP_ISO_SYNC_SY		(0x03<<2)

/*!
* USB descriptor isochronous endpoint usage types.
*/
//! Data endpoint
#define EP_ISO_USAGE_DE		(0x00<<4)
//! Feedback endpoint
#define EP_ISO_USAGE_FE		(0x01<<4)
//! Implicit feedback Data endpoint
#define EP_ISO_USAGE_IE		(0x02<<4)


//! Benchmark TX (IN, DeviceToHost) endpoint address.
#define  BM_EP_TX					(1 | USB_EP_DIR_IN)

//! Benchmark RX (OUT, HostToDevice) endpoint address.
#define  BM_EP_RX					(2 | USB_EP_DIR_OUT)

//! Benchmark RX/TX endpoint type.							(User Assignable)
#define BM_EP_TYPE					EP_TYPE_ISO
//#define BM_EP_TYPE					EP_TYPE_BULK
//#define BM_EP_TYPE					EP_TYPE_INT

//! Benchmark interface number.
#define  BM_INTF_NUMBER				0

//! Benchmark TX/RX endpoint packet size.					(User Assignable)
#define BM_EP_MAX_PACKET_SIZE       64

/*!
* Endpoint type-specific configuration options.				(User Assignable)
*/
#if (BM_EP_TYPE==EP_TYPE_BULK)
	#define BM_EP_ATTRIBUTES	BM_EP_TYPE
	#define BM_EP_INTERVAL		0
#elif (BM_EP_TYPE==EP_TYPE_INT)
	#define BM_EP_ATTRIBUTES	BM_EP_TYPE
	#define BM_EP_INTERVAL		4
#elif (BM_EP_TYPE==EP_TYPE_ISO)
	#define BM_EP_ATTRIBUTES	BM_EP_TYPE|EP_ISO_SYNC_NS|EP_ISO_USAGE_DE
	#define BM_EP_INTERVAL		4
#endif

//! Number of benchmark endpoints.
#define BM_EP_COUNT		2

//! Number of buffers to use per endpoint.
#define BM_BANK_COUNT	2

//! Configured endpoint bank size.
#define BM_BANK_SIZE	512

//! Benchmark VBus event handler.
extern void Bm_VBus_Handler(bool bIsAttached);

//! Benchmark vendor request event handler.
extern bool Bm_Vendor_Handler(void);

extern volatile Bm_RunTestDelegate Bm_SofEvent;
#define USB_DEVICE_SPECIFIC_REQUEST()	Bm_Vendor_Handler()
#define  UDC_VBUS_EVENT(bIsAttached)	Bm_VBus_Handler(bIsAttached)

#if (BM_EP_TYPE==EP_TYPE_ISO)
	#define  UDC_SOF_EVENT()			if (Bm_SofEvent)(Bm_SofEvent())
#endif

#include "conf_bm_iso.h"

/////////////////////////////////////////////////////////////
/* Benchmark configuration validation. (should not modify) */

#ifdef USB_DEVICE_HS_SUPPORT
#if (BM_EP_TYPE==EP_TYPE_BULK && BM_EP_MAX_PACKET_SIZE!=512)
#warning "High speed bulk endpoints require a 512b packet size to meet USB 2.0 specifications"  
#endif
#endif

#endif // _CONF_BENCHMARK_H_
