/*! conf_bm_iso.h
 Created: 6/27/2011 5:57:39 PM
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

#ifndef CONF_BM_ISO_H_
#define CONF_BM_ISO_H_

#if (BM_EP_TYPE==EP_TYPE_ISO)

	#define BM_MANAGE_SOF_PERIOD_RX
	#define BM_MANAGE_SOF_PERIOD_TX

	#ifdef USB_DEVICE_HS_SUPPORT
		// HS ISO Setup
		#if (BM_EP_INTERVAL == 1)
			#define BM_EP_POLLCOUNT			8
			#define BM_EP_PACKET_MULTIPLIER	64
		#elif (BM_EP_INTERVAL == 2)
			#define BM_EP_POLLCOUNT			8
			#define BM_EP_PACKET_MULTIPLIER	32
		#elif (BM_EP_INTERVAL == 3)
			#define BM_EP_POLLCOUNT			8
			#define BM_EP_PACKET_MULTIPLIER	16
		#elif (BM_EP_INTERVAL == 4)
			#define BM_EP_POLLCOUNT			8
			#define BM_EP_PACKET_MULTIPLIER	8
		#else
			#error "BM_EP_INTERVAL is out of range."
		#endif
	#else
		// FS ISO Setup
		#if (BM_EP_INTERVAL == 1)
			#define BM_EP_POLLCOUNT			8
			#define BM_EP_PACKET_MULTIPLIER	8
		#else
			#error "BM_EP_INTERVAL is out of range."
		#endif
	#endif
	#define BM_MAX_TRANSFER_SIZE	(BM_EP_MAX_PACKET_SIZE*BM_EP_PACKET_MULTIPLIER)
#else
	// Bulk/Interrupt Setup
	#define BM_EP_PACKET_MULTIPLIER	(8192/BM_EP_MAX_PACKET_SIZE)
	#define BM_MAX_TRANSFER_SIZE	(BM_EP_MAX_PACKET_SIZE*BM_EP_PACKET_MULTIPLIER)
#endif // EndOf (BM_EP_TYPE==EP_TYPE_ISO)

//! Size of each buffer. Must be a multiple of the configured BANK size. (should not modify)
#define BM_BUFFER_SIZE			MAKE_INTERVAL_SIZE(BM_EP_MAX_PACKET_SIZE,BM_EP_PACKET_MULTIPLIER,BM_BANK_SIZE)

#ifndef BM_MAX_TRANSFER_SIZE
#error "BM_MAX_TRANSFER_SIZE not defined"
#endif

#endif /* CONF_BM_ISO_H_ */
