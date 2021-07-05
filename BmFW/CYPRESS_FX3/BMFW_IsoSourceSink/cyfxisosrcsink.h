/*
 ## Cypress USB 3.0 Platform header file (cyfxisosrcsink.h)
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

/* This file contains the constants used by the ISO source sink application example */

#ifndef _INCLUDED_CYFXISOSRCSINK_H_
#define _INCLUDED_CYFXISOSRCSINK_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3externcstart.h"

#define BWFW_PID (0x00FD)

// when defined, transfer statistics are reported every 1 second.
#define BMFW_REPORT_STATISTICS

#define CY_FX_ISOSRCSINK_DMA_BUF_COUNT      (3)                       /* Bulk loop channel buffer count */
#define CY_FX_ISOSRCSINK_DMA_TX_SIZE        (0)                       /* DMA transfer size is set to infinite */
#define CY_FX_ISOSRCSINK_THREAD_STACK       (0x1000)                  /* Bulk loop application thread stack size */
#define CY_FX_ISOSRCSINK_THREAD_PRIORITY    (1)                       /* Bulk loop application thread priority */
#define CY_FX_ISOSRCSINK_PATTERN            (0xAA)                    /* 8-bit pattern to be loaded to the source buffers. */

/* Endpoint and socket definitions for the bulkloop application */

/* To change the producer and consumer EP enter the appropriate EP numbers for the #defines.
 * In the case of IN endpoints enter EP number along with the direction bit.
 * For eg. EP 6 IN endpoint is 0x86
 *     and EP 6 OUT endpoint is 0x06.
 * To change sockets mention the appropriate socket number in the #defines. */

/* Note: For USB 2.0 the endpoints and corresponding sockets are one-to-one mapped
         i.e. EP 1 is mapped to UIB socket 1 and EP 2 to socket 2 so on */

#define CY_FX_EP_PRODUCER               0x03    /* EP 1 OUT */
#define CY_FX_EP_CONSUMER               0x83    /* EP 1 IN */

#define CY_FX_EP_PRODUCER_SOCKET        CY_U3P_UIB_SOCKET_PROD_3    /* Socket 3 is producer */
#define CY_FX_EP_CONSUMER_SOCKET        CY_U3P_UIB_SOCKET_CONS_3    /* Socket 3 is consumer */

/* Maximum packet size for the endpoint. Can take any value from 1 to 1024. */
#define CY_FX_ISO_MAXPKT                (1024)

/* ISO Mult settings for SS and HS operation. */
#define CY_FX_ISO_PKTS                  (3)

/* Burst length in packets supported by the ISO endpoints, when operating in USB 3.0 mode. */
#define CY_FX_ISO_BURST                 (8)

/* Dma buffer size multiplier. As ISO endpoints will only transfer ISO_PKTS * ISO_BURST packets of data in
 * each interval, this can be set to 1 without any performance limitation. */
#define CY_FX_DMA_MULTIPLIER            (1)

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

#include "cyu3externcend.h"

#endif /* _INCLUDED_CYFXISOSRCSINK_H_ */

/*[]*/
