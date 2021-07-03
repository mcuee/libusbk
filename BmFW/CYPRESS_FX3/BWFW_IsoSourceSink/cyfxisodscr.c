/*
 ## Cypress USB 3.0 Platform header file (cyfxisodscr.c)
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

/* This file contains the USB enumeration descriptors for the ISO source sink application example.
 * The descriptor arrays must be 32 byte aligned and multiple of 32 bytes if the D-cache is
 * turned on. If the linker used is not capable of supporting the aligned feature for this,
 * either the descriptors must be placed in a different section and the section should be 
 * 32 byte aligned and 32 byte multiple; or dynamically allocated buffer allocated using
 * CyU3PDmaBufferAlloc must be used, and the descriptor must be loaded into it. The example
 * assumes that the aligned attribute for 32 bytes is supported by the linker. Do not add
 * any other variables to this file other than USB descriptors. This is not the only
 * pre-requisite to enabling the D-cache. Refer to the documentation for
 * CyU3PDeviceCacheControl for more information.
 */

#include "cyfxisosrcsink.h"
#include "cyu3utils.h"
#include "stdint.h"
#include "stddef.h"

/* Standard device descriptor for USB 3.0 */
const uint8_t CyFxUSB30DeviceDscr[] __attribute__ ((aligned (32))) =
{
    0x12,                           /* Descriptor size */
    CY_U3P_USB_DEVICE_DESCR,        /* Device descriptor type */
    0x10,0x03,                      /* USB 3.1 */
    0x00,                           /* Device class */
    0x00,                           /* Device sub-class */
    0x00,                           /* Device protocol */
    0x09,                           /* Maxpacket size for EP0 : 2^9 */
    0xB5,0x04,                      /* Vendor ID */

    /* Product ID */
    CY_U3P_GET_LSB(BWFW_PID),CY_U3P_GET_MSB(BWFW_PID),

    0x00,0x00,                      /* Device release number */
    0x01,                           /* Manufacture string index */
    0x02,                           /* Product string index */
    0x00,                           /* Serial number string index */
    0x01                            /* Number of configurations */
};

/* Standard device descriptor for USB 2.0 */
const uint8_t CyFxUSB20DeviceDscr[] __attribute__ ((aligned (32))) =
{
    0x12,                           /* Descriptor size */
    CY_U3P_USB_DEVICE_DESCR,        /* Device descriptor type */
    0x10,0x02,                      /* USB 2.10 */
    0x00,                           /* Device class */
    0x00,                           /* Device sub-class */
    0x00,                           /* Device protocol */
    0x40,                           /* Maxpacket size for EP0 : 64 bytes */
    0xB5,0x04,                      /* Vendor ID */

    /* Product ID */
    CY_U3P_GET_LSB(BWFW_PID),CY_U3P_GET_MSB(BWFW_PID),

    0x00,0x00,                      /* Device release number */
    0x01,                           /* Manufacture string index */
    0x02,                           /* Product string index */
    0x00,                           /* Serial number string index */
    0x01                            /* Number of configurations */
};

/* Binary device object store descriptor */
const uint8_t CyFxUSBBOSDscr[] __attribute__ ((aligned (32))) =
{
    0x05,                           /* Descriptor size */
    CY_U3P_BOS_DESCR,               /* Device descriptor type */
    0x16,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x02,                           /* Number of device capability descriptors */

    /* USB 2.0 extension */
    0x07,                           /* Descriptor size */
    CY_U3P_DEVICE_CAPB_DESCR,       /* Device capability type descriptor */
    CY_U3P_USB2_EXTN_CAPB_TYPE,     /* USB 2.0 extension capability type */
    0x1E,0x64,0x00,0x00,            /* Supported device level features: LPM support, BESL supported,
                                       Baseline BESL=400 us, Deep BESL=1000 us. */

    /* SuperSpeed device capability */
    0x0A,                           /* Descriptor size */
    CY_U3P_DEVICE_CAPB_DESCR,       /* Device capability type descriptor */
    CY_U3P_SS_USB_CAPB_TYPE,        /* SuperSpeed device capability type */
    0x00,                           /* Supported device level features  */
    0x0E,0x00,                      /* Speeds supported by the device : SS, HS and FS */
    0x03,                           /* Functionality support */
    0x00,                           /* U1 Device Exit latency */
    0x00,0x00                       /* U2 Device Exit latency */
};

/* Standard device qualifier descriptor */
const uint8_t CyFxUSBDeviceQualDscr[] __attribute__ ((aligned (32))) =
{
    0x0A,                           /* Descriptor size */
    CY_U3P_USB_DEVQUAL_DESCR,       /* Device qualifier descriptor type */
    0x00,0x02,                      /* USB 2.0 */
    0x00,                           /* Device class */
    0x00,                           /* Device sub-class */
    0x00,                           /* Device protocol */
    0x40,                           /* Maxpacket size for EP0 : 64 bytes */
    0x01,                           /* Number of configurations */
    0x00                            /* Reserved */
};

/* Standard super speed configuration descriptor */
const uint8_t CyFxUSBSSConfigDscr[] __attribute__ ((aligned (32))) =
{
    /* Configuration descriptor */
    0x09,                               /* Descriptor size */
    CY_U3P_USB_CONFIG_DESCR,            /* Configuration descriptor type */
    0x35,0x00,                          /* Length of this descriptor and all sub descriptors */
    0x01,                               /* Number of interfaces */
    0x01,                               /* Configuration number */
    0x00,                               /* COnfiguration string index */
    0x80,                               /* Config characteristics - Bus powered */
    0x32,                               /* Max power consumption of device (in 8mA unit) : 400mA */

    /* Interface descriptor: Interface 0, Alt setting 0 - No endpoints */
    0x09,                               /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,            /* Interface Descriptor type */
    0x00,                               /* Interface number */
    0x00,                               /* Alternate setting number */
    0x00,                               /* Number of end points */
    0xFF,                               /* Interface class */
    0x00,                               /* Interface sub class */
    0x00,                               /* Interface protocol code */
    0x00,                               /* Interface descriptor string index */

    /* Interface descriptor: Interface 0, Alt setting 1 - Two endpoints */
    0x09,                               /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,            /* Interface Descriptor type */
    0x00,                               /* Interface number */
    0x01,                               /* Alternate setting number */
    0x02,                               /* Number of end points */
    0xFF,                               /* Interface class */
    0x00,                               /* Interface sub class */
    0x00,                               /* Interface protocol code */
    0x00,                               /* Interface descriptor string index */

    /* Endpoint descriptor for producer EP */
    0x07,                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,            /* Endpoint descriptor type */
    CY_FX_EP_PRODUCER,                  /* Endpoint address and description */
    CY_U3P_USB_EP_ISO,                  /* ISO endpoint type */
    (CY_U3P_GET_LSB(CY_FX_ISO_MAXPKT)), /* LS Byte of maximum packet size. */
    (CY_U3P_GET_MSB(CY_FX_ISO_MAXPKT)), /* MS Byte of maximum packet size. */
    0x01,                               /* Servicing interval for data transfers: 1 uFrame */

    /* Super speed endpoint companion descriptor for producer EP */
    0x06,                                                                       /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,                                                   /* SS EP companion descriptor type */
    (CY_FX_ISO_BURST - 1),                                                      /* Number of packets per burst. */
    (CY_FX_ISO_PKTS - 1),                                                       /* Number of bursts per interval. */
    (CY_U3P_GET_LSB(CY_FX_ISO_MAXPKT * CY_FX_ISO_PKTS * CY_FX_ISO_BURST)),      /* LS Byte of data per interval. */
    (CY_U3P_GET_MSB(CY_FX_ISO_MAXPKT * CY_FX_ISO_PKTS * CY_FX_ISO_BURST)),      /* MS Byte of data per interval. */

    /* Endpoint descriptor for consumer EP */
    0x07,                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,            /* Endpoint descriptor type */
    CY_FX_EP_CONSUMER,                  /* Endpoint address and description */
    CY_U3P_USB_EP_ISO,                  /* ISO endpoint type */
    (CY_U3P_GET_LSB(CY_FX_ISO_MAXPKT)), /* LS Byte of maximum packet size. */
    (CY_U3P_GET_MSB(CY_FX_ISO_MAXPKT)), /* MS Byte of maximum packet size. */
    0x01,                               /* Servicing interval for data transfers: 1 uFrame */

    /* Super speed endpoint companion descriptor for consumer EP */
    0x06,                                                                       /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,                                                   /* SS EP companion descriptor type */
    (CY_FX_ISO_BURST - 1),                                                      /* Number of packets per burst. */
    (CY_FX_ISO_PKTS - 1),                                                       /* Number of bursts per interval. */
    (CY_U3P_GET_LSB(CY_FX_ISO_MAXPKT * CY_FX_ISO_PKTS * CY_FX_ISO_BURST)),      /* LS Byte of data per interval. */
    (CY_U3P_GET_MSB(CY_FX_ISO_MAXPKT * CY_FX_ISO_PKTS * CY_FX_ISO_BURST))       /* MS Byte of data per interval. */
};

/* Standard high speed configuration descriptor */
const uint8_t CyFxUSBHSConfigDscr[] __attribute__ ((aligned (32))) =
{
    /* Configuration descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_CONFIG_DESCR,        /* Configuration descriptor type */
    0x29,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x01,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* COnfiguration string index */
    0x80,                           /* Config characteristics - bus powered */
    0x32,                           /* Max power consumption of device (in 2mA unit) : 100mA */

    /* Interface descriptor: Interface 0, Alt setting 0 - No endpoints */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x00,                           /* Number of end points */
    0xFF,                           /* Interface class */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Interface descriptor: Interface 0, Alt setting 1 - Two endpoints */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x01,                           /* Alternate setting number */
    0x02,                           /* Number of endpoints */
    0xFF,                           /* Interface class */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Endpoint descriptor for producer EP */
    0x07,                                                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,                                            /* Endpoint descriptor type */
    CY_FX_EP_PRODUCER,                                                  /* Endpoint address and description */
    CY_U3P_USB_EP_ISO,                                                  /* Isochronous endpoint type */
    (CY_U3P_GET_LSB(CY_FX_ISO_MAXPKT)),                                 /* LS Byte of maximum packet size. */
    (CY_U3P_GET_MSB(CY_FX_ISO_MAXPKT) | ((CY_FX_ISO_PKTS - 1) << 3)),   /* MS Byte of max. packet size and MULT. */
    0x01,                                                               /* Servicing interval for data transfers */

    /* Endpoint descriptor for consumer EP */
    0x07,                                                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,                                            /* Endpoint descriptor type */
    CY_FX_EP_CONSUMER,                                                  /* Endpoint address and description */
    CY_U3P_USB_EP_ISO,                                                  /* Isochronous endpoint type */
    (CY_U3P_GET_LSB(CY_FX_ISO_MAXPKT)),                                 /* LS Byte of maximum packet size. */
    (CY_U3P_GET_MSB(CY_FX_ISO_MAXPKT) | ((CY_FX_ISO_PKTS - 1) << 3)),   /* MS Byte of max. packet size and MULT. */
    0x01                                                                /* Servicing interval for data transfers */
};

/* Standard full speed configuration descriptor */
const uint8_t CyFxUSBFSConfigDscr[] __attribute__ ((aligned (32))) =
{
    /* Configuration descriptor */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_CONFIG_DESCR,        /* Configuration descriptor type */
    0x29,0x00,                      /* Length of this descriptor and all sub descriptors */
    0x01,                           /* Number of interfaces */
    0x01,                           /* Configuration number */
    0x00,                           /* COnfiguration string index */
    0x80,                           /* Config characteristics - bus powered */
    0x32,                           /* Max power consumption of device (in 2mA unit) : 100mA */

    /* Interface descriptor: Interface 0, Alt setting 0 - No endpoints */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
    0x00,                           /* Interface number */
    0x00,                           /* Alternate setting number */
    0x00,                           /* Number of end points */
    0xFF,                           /* Interface class */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Interface descriptor: Interface 0, Alt setting 1 - Two endpoints */
    0x09,                           /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,        /* Interface descriptor type */
    0x00,                           /* Interface number */
    0x01,                           /* Alternate setting number */
    0x02,                           /* Number of endpoints */
    0xFF,                           /* Interface class */
    0x00,                           /* Interface sub class */
    0x00,                           /* Interface protocol code */
    0x00,                           /* Interface descriptor string index */

    /* Endpoint descriptor for producer EP */
    0x07,                                                       /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,                                    /* Endpoint descriptor type */
    CY_FX_EP_PRODUCER,                                          /* Endpoint address and description */
    CY_U3P_USB_EP_ISO,                                          /* ISO endpoint type */
    (CY_U3P_GET_LSB(CY_U3P_MIN(1023,CY_FX_ISO_MAXPKT))),        /* LS Byte of Max Packet size. Limit to 1023. */
    (CY_U3P_GET_MSB(CY_U3P_MIN(1023,CY_FX_ISO_MAXPKT))),        /* LS Byte of Max Packet size. Limit to 1023. */
    0x01,                                                       /* Servicing interval for data transfers */

    /* Endpoint descriptor for consumer EP */
    0x07,                                                       /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,                                    /* Endpoint descriptor type */
    CY_FX_EP_CONSUMER,                                          /* Endpoint address and description */
    CY_U3P_USB_EP_ISO,                                          /* ISO endpoint type */
    (CY_U3P_GET_LSB(CY_U3P_MIN(1023,CY_FX_ISO_MAXPKT))),        /* LS Byte of Max Packet size. Limit to 1023. */
    (CY_U3P_GET_MSB(CY_U3P_MIN(1023,CY_FX_ISO_MAXPKT))),        /* LS Byte of Max Packet size. Limit to 1023. */
    0x01                                                        /* Servicing interval for data transfers */
};

/* Standard language ID string descriptor */
const uint8_t CyFxUSBStringLangIDDscr[] __attribute__ ((aligned (32))) =
{
    0x04,                           /* Descriptor size */
    CY_U3P_USB_STRING_DESCR,        /* Device descriptor type */
    0x09,0x04                       /* Language ID supported */
};

/* Standard manufacturer string descriptor */
const uint8_t CyFxUSBManufactureDscr[] __attribute__ ((aligned (32))) =
{
    0x10,                           /* Descriptor size */
    CY_U3P_USB_STRING_DESCR,        /* Device descriptor type */
    'C',0x00,
    'y',0x00,
    'p',0x00,
    'r',0x00,
    'e',0x00,
    's',0x00,
    's',0x00
};

/* Standard product string descriptor */
const uint8_t CyFxUSBProductDscr[] __attribute__ ((aligned (32))) =
{
    0x1C,                           /* Descriptor size */
    CY_U3P_USB_STRING_DESCR,        /* Device descriptor type */
    'F',0x00,
    'X',0x00,
    '3',0x00,

    ' ',0x00,
    'I',0x00,
    'S',0x00,
    'O',0x00,
    'S',0x00,
    'R',0x00,
    'C',0x00,
    'S',0x00,
    'N',0x00,
    'K',0x00,

};

/* Place this buffer as the last buffer so that no other variable / code shares
 * the same cache line. Do not add any other variables / arrays in this file.
 * This will lead to variables sharing the same cache line. */
const uint8_t CyFxUsbDscrAlignBuffer[32] __attribute__ ((aligned (32)));

/* [ ] */

