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

#define BM_MANAGE_SOF_PERIOD

#if (BM_EP_TYPE==EP_TYPE_ISO)
	#ifndef USB_DEVICE_HS_SUPPORT
		// FS ISO Setup
		#if (BM_EP_INTERVAL == 1)
			#define BM_EP_POLLCOUNT			1
			#define BM_BUFFER_MULTIPLIER	1
		#elif (BM_EP_INTERVAL > 1 && BM_EP_INTERVAL < 4)
			#define BM_EP_POLLCOUNT			4
			#define BM_BUFFER_MULTIPLIER	2
		#elif (BM_EP_INTERVAL > 3 && BM_EP_INTERVAL < 8)
			#define BM_EP_POLLCOUNT			4
			#define BM_BUFFER_MULTIPLIER	4
		#elif (BM_EP_INTERVAL > 7 && BM_EP_INTERVAL < 16)
			#define BM_EP_POLLCOUNT			8
			#define BM_BUFFER_MULTIPLIER	8
		#else
			#error "BM_EP_INTERVAL is out of range."
		#endif
	#else
		// HS ISO Setup
		#if (BM_EP_INTERVAL == 1)
			#define BM_EP_POLLCOUNT			4
			#define BM_BUFFER_MULTIPLIER	32
		#elif (BM_EP_INTERVAL == 2)
			#define BM_EP_POLLCOUNT			4
			#define BM_BUFFER_MULTIPLIER	16
		#elif (BM_EP_INTERVAL == 3)
			#define BM_EP_POLLCOUNT			4
			#define BM_BUFFER_MULTIPLIER	8
		#elif (BM_EP_INTERVAL == 4)
			#define BM_EP_POLLCOUNT			4
			#define BM_BUFFER_MULTIPLIER	4
		#else
			#error "BM_EP_INTERVAL is out of range."
		#endif
	#endif

#elif !(BM_EP_TYPE==EP_TYPE_ISO)
	// Bulk/Interrupt Setup
	#define BM_BUFFER_MULTIPLIER	(4096/BM_EP_MAX_PACKET_SIZE)
#endif // EndOf (BM_EP_TYPE==EP_TYPE_ISO)

//! Size of each buffer. Must be a multiple of the configured BANK size. (should not modify)
#define BM_BUFFER_SIZE			MAKE_INTERVAL_SIZE(BM_EP_MAX_PACKET_SIZE,BM_BUFFER_MULTIPLIER,BM_BANK_SIZE)

#ifdef BM_MANAGE_SOF_PERIOD
#define BM_MAX_TRANSFER_SIZE	MAKE_INTERVAL_SIZE(BM_EP_MAX_PACKET_SIZE,BM_BUFFER_MULTIPLIER,BM_EP_MAX_PACKET_SIZE)
#else
#define BM_MAX_TRANSFER_SIZE	BM_BUFFER_SIZE
#endif

#endif /* CONF_BM_ISO_H_ */
