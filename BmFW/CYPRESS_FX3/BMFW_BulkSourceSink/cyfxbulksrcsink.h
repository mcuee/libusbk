/*
 ## Cypress USB 3.0 Platform header file (cyfxbulksrcsink.h)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2018,
 ##  All Rights Reserved
 ##  UNPUBLISHED, LICENSED SOFTWARE.
 ##
 ##  CONFIDENTIAL AND PROPRIETARY INFORMATION
 ##  WHICH IS THE PROPERTY OF CYPRESS.
 ##
 ##  Use of this file is governed
 ##  by the license agreement included in the file
 ##
 ##     <install>/license/license.txt
 ##
 ##  where <install> is the Cypress software
 ##  installation root directory path.
 ##
 ## ===========================
*/

/* This file contains the constants used by the bulk source sink application example */

#ifndef _INCLUDED_CYFXBULKSRCSINK_H_
#define _INCLUDED_CYFXBULKSRCSINK_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

#define BWFW_PID (0x00FB)

/* Endpoint and socket definitions for the bulk source sink application */

/* To change the producer and consumer EP enter the appropriate EP numbers for the #defines.
 * In the case of IN endpoints enter EP number along with the direction bit.
 * For eg. EP 6 IN endpoint is 0x86
 *     and EP 6 OUT endpoint is 0x06.
 * To change sockets mention the appropriate socket number in the #defines. */

/* Note: For USB 2.0 the endpoints and corresponding sockets are one-to-one mapped
         i.e. EP 1 is mapped to UIB socket 1 and EP 2 to socket 2 so on */

#define CY_FX_EP_PRODUCER               0x01    /* EP 1 OUT */
#define CY_FX_EP_CONSUMER               0x81    /* EP 1 IN */

#define CY_FX_EP_PRODUCER_SOCKET        CY_U3P_UIB_SOCKET_PROD_1    /* Socket 1 is producer */
#define CY_FX_EP_CONSUMER_SOCKET        CY_U3P_UIB_SOCKET_CONS_1    /* Socket 1 is consumer */

#define CY_FX_BULKSRCSINK_DMA_TX_SIZE        (0)        /* DMA transfer size is set to infinite */
#define CY_FX_BULKSRCSINK_THREAD_STACK       (0x1000)   /* Bulk loop application thread stack size */
#define CY_FX_BULKSRCSINK_THREAD_PRIORITY    (8)        /* Bulk loop application thread priority */

/* Burst mode definitions: Only for super speed operation. The maximum burst mode 
 * supported is limited by the USB hosts available. The maximum value for this is 16
 * and the minimum (no-burst) is 1. */

#ifdef CYMEM_256K

/*
   As we have only 32 KB total DMA buffers available on the CYUSB3011/CYUSB3012 parts, the buffering
   needs to be reduced.
 */

/* Burst length in 1 KB packets. Only applicable to USB 3.0. */
#define CY_FX_EP_BURST_LENGTH                   (4)

/* Multiplication factor used when allocating DMA buffers to reduce DMA callback frequency. */
#define CY_FX_DMA_SIZE_MULTIPLIER               (1)

/* Number of DMA buffers to be used. More buffers can give better throughput. */
#define CY_FX_BULKSRCSINK_DMA_BUF_COUNT         (2)

#else

/* Burst length in 1 KB packets. Only applicable to USB 3.0. */
#define CY_FX_EP_BURST_LENGTH                   (16)

/* Multiplication factor used when allocating DMA buffers to reduce DMA callback frequency. */
#define CY_FX_DMA_SIZE_MULTIPLIER               (2)

/* Number of DMA buffers to be used. More buffers can give better throughput. */
#define CY_FX_BULKSRCSINK_DMA_BUF_COUNT         (3)

#endif

/* Byte value that is filled into the source buffers that FX3 sends out. */
#define CY_FX_BULKSRCSINK_PATTERN            (0xAA)

/* Extern definitions for the USB Descriptors */
extern const uint8_t CyFxUSB20DeviceDscr[];
extern const uint8_t CyFxUSB30DeviceDscr[];
extern const uint8_t CyFxUSBDeviceQualDscr[];
extern const uint8_t CyFxUSBFSConfigDscr[];
extern const uint8_t CyFxUSBHSConfigDscr[];
extern const uint8_t CyFxUSBBOSDscr[];
extern const uint8_t CyFxUSBSSConfigDscr[];
extern const uint8_t CyFxUSBStringLangIDDscr[];
extern const uint8_t CyFxUSBManufactureDscr[];
extern const uint8_t CyFxUSBProductDscr[];
extern const uint8_t CyFxUsbOSDscr[];

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYFXBULKSRCSINK_H_ */

/*[]*/
