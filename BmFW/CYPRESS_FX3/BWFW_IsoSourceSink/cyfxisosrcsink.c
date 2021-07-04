/*
 ## Cypress USB 3.0 Platform source file (cyfxisosrcsink.c)
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

/* This file illustrates the ISO source sink Application example using the DMA MANUAL_IN
 * and DMA MANUAL_OUT mode */

/*
   This example illustrates USB endpoint data source and data sink mechanism. The example
   comprises of vendor class USB enumeration descriptors with two Iso endpoints. An ISO OUT
   endpoint acts as the producer of data and acts as the sink to the host. An ISO IN endpoint
   acts as the consumer of data and acts as the source to the host.

   The data source and sink is achieved with the help of a DMA MANUAL_IN channel and a DMA
   MANUAL_OUT channel. A DMA MANUAL_IN channel is created between the producer USB ISO endpoint
   and the CPU. A DMA MANUAL_OUT channel is created between the CPU and the consumer USB ISO
   endpoint. Data is received in the IN channel DMA buffer from the host through the producer 
   endpoint. CPU is signalled of the data reception using DMA callbacks. The CPU discards this
   buffer. This leads to the sink mechanism.

   A constant patern data is loaded onto the OUT channel DMA buffer whenever the buffer is available.
   CPU issues commit of the DMA data transfer to the consumer endpoint which then gets transferred 
   to the host. This leads to a constant source mechanism.

   The DMA buffer size is defined based on the USB speed. 64 for full speed, 1024 for high speed
   and 1024 for super speed. CY_FX_ISOSRCSINK_DMA_BUF_COUNT in the header file defines the number
   of DMA buffers.

   For performance optimizations refer the readme.txt.
 */

#include "cyu3types.h"
#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyfxisosrcsink.h"
#include "cyu3usb.h"
#include "cyu3uart.h"
#include "cyu3socket.h"
#include "sock_regs.h"
#include "cyu3utils.h"
#include "cyu3gpio.h"
#include "stdint.h"
#include "stddef.h"

CyU3PThread     isoSrcSinkAppThread;	 /* ISO loop application thread structure */
CyU3PDmaChannel glChHandleIsoSink;       /* DMA MANUAL_IN channel handle */
CyU3PDmaChannel glChHandleIsoSrc;        /* DMA MANUAL_OUT channel handle */

static volatile uint8_t glBenchmarkMode = 0;

static volatile CyBool_t glIsApplnActive = CyFalse;     /* Whether the loopback application is active or not. */
static volatile CyBool_t glIsDevConfigured = CyFalse;   /* Whether the device configuration has been completed. */
static volatile uint32_t glDmaPktsReceived  = 0;        /* Number of OUT packets received. */
static volatile uint32_t glDmaPktsDiscarded = 0;        /* Number of OUT packets discarded. */
static volatile uint32_t glDmaPktsCommitted = 0;        /* Number of IN packets committed. */
static volatile uint32_t glDmaPktsSent      = 0;        /* Actual number of packets sent. */

static volatile CyBool_t glResetDevice      = CyFalse;  /* Reset request received. */
static volatile uint16_t LastDmaDescriptor;             /* Active descriptor on the ISO OUT endpoint. */
static volatile uint16_t glPacketCounter = 0;
uint8_t glEp0Buffer[32] __attribute__ ((aligned (32))); /* Local buffer used for vendor command handling. */

/* Register and macros used to force a USB OUT DMA socket to wrap-up (commit) a partially filled buffer.
 * See the SCK_STATUS in the FX3 TRM.
 */
#define CY_FX_USB_EPO_SOCK_STAT(ep)     (*(uvint32_t *)(0xE004800C + ((ep) * 0x80)))
#define CY_FX_USB_EPO_SOCK_WRAPUP       (0x10000000)

/* Macro to check whether a specific DMA descriptor is occupied. */
#define CY_FX_DMA_DSCR_IS_OCCUPIED(idx) ((*(uvint32_t *)(0x4000000C + ((idx) << 4))) & 0x08)

/* Register and macros used to check the current status of a USB 3.0 OUT endpoint.
 * See the PROT_EPO_CS1 register in the FX3 TRM.
 */
#define CY_FX_USB3_EPO_CTRL_STAT(ep)    (*(uvint32_t *)(0xE0033600 + ((ep) << 2)))
#define CY_FX_USB3_EPO_CTRL_MASK        (0x3FF8003F)
#define CY_FX_USB3_EPO_COMMIT_STAT      (0x00000100)

/* Register and macros used to block/unblock all data coming in on the USB 3.0 port.
 * See the PROT_CS register in the FX3 TRM.
 */
#define CY_FX_USB3_EPM_CTRL_REG         (*(uvint32_t *)(0xE0033400))
#define CyFxUsb3EpmDataBlock()                                                          \
{                                                                                       \
    CY_FX_USB3_EPM_CTRL_REG = (CY_FX_USB3_EPM_CTRL_REG & 0xFFFEFFFF) | 0x00020000;      \
}
#define CyFxUsb3EpmDataUnblock()                                                        \
{                                                                                       \
    CY_FX_USB3_EPM_CTRL_REG = (CY_FX_USB3_EPM_CTRL_REG & 0xFFFCFFFF);                   \
}

/* Application Error Handler */
void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus    /* API return status */
        )
{
    /* Application failed with the error code apiRetStatus */

    /* Add custom debug or recovery actions here */

    /* Loop Indefinitely */
    for (;;)
    {
        /* Thread sleep : 100 ms */
        CyU3PThreadSleep (100);
    }
}

/* This function initializes the debug module. The debug prints
 * are routed to the UART and can be seen using a UART console
 * running at 115200 baud rate. */
void
CyFxIsoSrcSinkApplnDebugInit (void)
{
    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the UART for printing debug messages */
    apiRetStatus = CyU3PUartInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set UART configuration */
    CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma = CyTrue;

    apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set the UART transfer to a really large value. */
    apiRetStatus = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Initialize the debug module. */
    apiRetStatus = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(apiRetStatus);
    }

    CyU3PDebugPreamble (CyFalse);
    CyU3PDebugPrint (2, "Ready\r\n");
}

/* Initialize the GPIO block and configure GPIO[17] as a simple OUTPUT GPIO. */
static void
CyFxIsoSrcSinkGpioInit (
        void)
{
    CyU3PGpioClock_t        gpioClock;
    CyU3PGpioSimpleConfig_t gpioConf = {CyTrue, CyTrue, CyTrue, CyFalse, CY_U3P_GPIO_NO_INTR};

    /* Initialize the GPIO block. */
    gpioClock.fastClkDiv = 2;
    gpioClock.slowClkDiv = 32;
    gpioClock.simpleDiv  = CY_U3P_GPIO_SIMPLE_DIV_BY_16;
    gpioClock.clkSrc     = CY_U3P_SYS_CLK_BY_2;
    gpioClock.halfDiv    = 0;

    CyU3PGpioInit (&gpioClock, NULL);

    /* Override PIN 17 as a simple GPIO. */
    CyU3PDeviceGpioOverride (17, CyTrue);

    /* Start PIN 17 off by driving it high. */
    CyU3PGpioSetSimpleConfig (17, &gpioConf);
}

/* Callback funtion for the DMA event notification. */
void
CyFxIsoSrcSinkDmaCallback (
        CyU3PDmaChannel   *chHandle, /* Handle to the DMA channel. */
        CyU3PDmaCbType_t  type,      /* Callback type.             */
        CyU3PDmaCBInput_t *input)    /* Callback status.           */
{
    if (type == CY_U3P_DMA_CB_PROD_EVENT)
    {
        /* If all buffers are full, enable entry into LOW POWER mode. */
        glDmaPktsReceived++;
        if ((glDmaPktsReceived - glDmaPktsDiscarded) >= CY_FX_ISOSRCSINK_DMA_BUF_COUNT)
            CyU3PUsbLPMEnable ();

        /* Data is discarded in the thread. Callback only tracks the number of buffers. */
    }

    if (type == CY_U3P_DMA_CB_CONS_EVENT)
    {
        /* If all buffers have become empty, enable entry into LOW POWER mode. */
        glDmaPktsSent++;
        if (glDmaPktsCommitted == glDmaPktsSent)
            CyU3PUsbLPMEnable ();
    }

    if (type == CY_U3P_DMA_CB_ERROR)
        CyU3PDebugPrint (2, "DMA Error\r\n");
}

/* This function starts the ISO loop application. This is called
 * when a SET_CONF event is received from the USB host. The endpoints
 * are configured and the DMA pipe is setup in this function. */
void
CyFxIsoSrcSinkApplnStart (
        void)
{
    uint16_t size = 0, index = 0;
    uint16_t j;
    CyU3PEpConfig_t epCfg;
    CyU3PDmaBuffer_t buf_p;
    CyU3PDmaChannelConfig_t dmaCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();
    CyU3PDmaSocketConfig_t sockConf;

    /* Configure the endpoint. For Full speed, limit the MULT and MAXPKT values to spec defined values. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable   = CyTrue;
    epCfg.epType   = CY_U3P_USB_EP_ISO;
    epCfg.burstLen = (usbSpeed == CY_U3P_SUPER_SPEED) ? (CY_FX_ISO_BURST) : 1;
    epCfg.streams  = 0;
    epCfg.pcktSize = (usbSpeed == CY_U3P_FULL_SPEED) ? (CY_U3P_MIN (1023, CY_FX_ISO_MAXPKT)) : CY_FX_ISO_MAXPKT;
    epCfg.isoPkts  = (usbSpeed == CY_U3P_FULL_SPEED) ? 1 : CY_FX_ISO_PKTS;

    /* Producer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

    /* Clear transfer counts. */
    glDmaPktsReceived  = 0;
    glDmaPktsDiscarded = 0;
    glDmaPktsCommitted = 0;
    glDmaPktsSent      = 0;

    /* The DMA buffer size is CY_FX_DMA_MULTIPLIER times that maximum amount of data that can be received per
     * service interval. The size has to be rounded up to a multiple of 16 bytes.
     */
    size = CY_FX_ISO_MAXPKT * CY_FX_ISO_BURST * CY_FX_ISO_PKTS * CY_FX_DMA_MULTIPLIER;
    dmaCfg.size           = ((size + 0x0F) & ~0x0F);
    dmaCfg.count          = CY_FX_ISOSRCSINK_DMA_BUF_COUNT;
    dmaCfg.prodSckId      = CY_FX_EP_PRODUCER_SOCKET;
    dmaCfg.consSckId      = CY_U3P_CPU_SOCKET_CONS;
    dmaCfg.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.cb             = CyFxIsoSrcSinkDmaCallback;
    dmaCfg.prodHeader     = 0;
    dmaCfg.prodFooter     = 0;
    dmaCfg.consHeader     = 0;
    dmaCfg.prodAvailCount = 0;
    dmaCfg.notification   = CY_U3P_DMA_CB_PROD_EVENT | CY_U3P_DMA_CB_ERROR;

    apiRetStatus = CyU3PDmaChannelCreate (&glChHandleIsoSink, CY_U3P_DMA_TYPE_MANUAL_IN, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelCreate failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Create a DMA MANUAL_OUT channel for the consumer socket. */
    dmaCfg.prodSckId    = CY_U3P_CPU_SOCKET_PROD;
    dmaCfg.consSckId    = CY_FX_EP_CONSUMER_SOCKET;
    dmaCfg.notification = CY_U3P_DMA_CB_CONS_EVENT | CY_U3P_DMA_CB_ERROR;

    apiRetStatus = CyU3PDmaChannelCreate (&glChHandleIsoSrc, CY_U3P_DMA_TYPE_MANUAL_OUT, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelCreate failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set DMA Channel transfer size */
    apiRetStatus = CyU3PDmaChannelSetXfer (&glChHandleIsoSink, CY_FX_ISOSRCSINK_DMA_TX_SIZE);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set the number of packets that can be stored in one OUT endpoint buffer. */
    CyU3PUsbEpSetPacketsPerBuffer (CY_FX_EP_PRODUCER, CY_FX_ISO_BURST * CY_FX_ISO_PKTS * CY_FX_DMA_MULTIPLIER);

    /* Find the active descriptor for the producer socket. */
    CyU3PDmaSocketGetConfig (CY_FX_EP_PRODUCER_SOCKET, &sockConf);
    LastDmaDescriptor = sockConf.dscrChain & CY_U3P_DSCR_NUMBER_MASK;

    /* Clear the commit interrupt status bit for the OUT endpoint. */
    CY_FX_USB3_EPO_CTRL_STAT(CY_FX_EP_PRODUCER) =
        (CY_FX_USB3_EPO_CTRL_STAT(CY_FX_EP_PRODUCER) & CY_FX_USB3_EPO_CTRL_MASK) | CY_FX_USB3_EPO_COMMIT_STAT;

    apiRetStatus = CyU3PDmaChannelSetXfer (&glChHandleIsoSrc, CY_FX_ISOSRCSINK_DMA_TX_SIZE);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Now preload all buffers in the MANUAL_OUT pipe with the required data. */
    for (index = 0; index < CY_FX_ISOSRCSINK_DMA_BUF_COUNT; index++)
    {
        apiRetStatus = CyU3PDmaChannelGetBuffer (&glChHandleIsoSrc, &buf_p, CYU3P_NO_WAIT);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelGetBuffer failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }

        for (j = 0; j < CY_FX_ISO_PKTS * CY_FX_ISO_BURST * CY_FX_DMA_MULTIPLIER; j++)
		{
        	uint8_t fillByte = 0;
        	uint32_t k;
        	for (k=0; k < CY_FX_ISO_MAXPKT; k++)
        	{
        		buf_p.buffer[j * CY_FX_ISO_MAXPKT + k]=fillByte++;
        	}
		}

        buf_p.buffer[0] = 0;
        buf_p.buffer[1] = glPacketCounter;
        glPacketCounter++;

        apiRetStatus = CyU3PDmaChannelCommitBuffer (&glChHandleIsoSrc, buf_p.size, 0);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "CyU3PDmaChannelCommitBuffer failed, Error code = %d\n", apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }
        else
        {
            glDmaPktsCommitted++;
        }
    }

    /* Enable SOF/ITP event callbacks. */
    CyU3PUsbEnableITPEvent (CyTrue);

    /* Update the flag so that the application thread is notified of this. */
    glIsApplnActive = CyTrue;
    CyU3PDebugPrint (8, "App Started\r\n");
}

/* This function stops the ISO loop application. This shall be called whenever
 * a RESET or DISCONNECT event is received from the USB host. The endpoints are
 * disabled and the DMA pipe is destroyed by this function. */
void
CyFxIsoSrcSinkApplnStop (
        void)
{
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Disable ITP event callbacks. */
    CyU3PUsbEnableITPEvent (CyFalse);

    /* Update the flag so that the application thread is notified of this. */
    glIsApplnActive = CyFalse;

    /* Destroy the channels */
    CyU3PDmaChannelDestroy (&glChHandleIsoSink);
    CyU3PDmaChannelDestroy (&glChHandleIsoSrc);

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

    /* Disable endpoints. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;

    /* Producer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }
    glPacketCounter = 0;
    CyU3PDebugPrint (8, "App Stopped\r\n");
}

/* Callback to handle the USB setup requests. */
CyBool_t
CyFxIsoSrcSinkApplnUSBSetupCB (
        uint32_t setupdat0, /* SETUP Data 0 */
        uint32_t setupdat1  /* SETUP Data 1 */
    )
{
    /* Fast enumeration is used. Only requests addressed to the interface, class,
     * vendor and interface/endpoint control requests are received by this function.
     */

    uint8_t  bRequest, bReqType;
    uint8_t  bType, bTarget;
    uint16_t wValue;
    CyBool_t isHandled = CyFalse;

    /* Decode the fields from the setup request. */
    bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType    = (bReqType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);

    if (bType == CY_U3P_USB_STANDARD_RQT)
    {
        /* Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
         * requests here. It should be allowed to pass if the device is in configured
         * state and failed otherwise. */
        if ((bTarget == CY_U3P_USB_TARGET_INTF) && ((bRequest == CY_U3P_USB_SC_SET_FEATURE)
                    || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0))
        {
            if (glIsDevConfigured)
                CyU3PUsbAckSetup ();
            else
                CyU3PUsbStall (0, CyTrue, CyFalse);

            isHandled = CyTrue;
        }
    }

    if (bType == CY_U3P_USB_VENDOR_RQT)
    {
    	// Benchmark set test
		if (bRequest == 0x0E)
		{
			glBenchmarkMode = wValue & 0xFF;
			glEp0Buffer[0] = glBenchmarkMode;
			CyU3PUsbSendEP0Data(1, glEp0Buffer);
			isHandled     = CyTrue;
		}
    	// Benchmark get test
		if (bRequest == 0x0F)
		{
			glEp0Buffer[0] = glBenchmarkMode;
			CyU3PUsbSendEP0Data(1, glEp0Buffer);
			isHandled     = CyTrue;
		}

        if (bRequest == 0xE0)
        {
            CyU3PUsbAckSetup ();
            glResetDevice = CyTrue;
            isHandled     = CyTrue;
        }

        if (bRequest == 0xE1)
        {
            /* Request to inject a delay so that we can test SOF/ITP event frequency. */
            CyU3PDebugPrint (2, "Delay command\r\n");
            CyU3PThreadSleep (200);
            CyU3PDebugPrint (2, "Done\r\n");
            CyU3PUsbAckSetup ();
            isHandled = CyTrue;
        }
    }

    if (!isHandled)
    {
    	CyU3PDebugPrint(
    			2,
    			"EPO Unhandled: bReqType:%x bType:%x bTarget:%x bRequest:%x wValue:%x\r\n",
    			(uint32_t)bReqType, (uint32_t)bType, (uint32_t)bTarget, (uint32_t)bRequest, (uint32_t)wValue);
    }
    return isHandled;
}

/*
 * This function is executed whenever an ITP packet is received by FX3, and is called from an interrupt context.
 * It implements a firmware work-around to ensure that partially filled DMA buffers (without a short packet) are
 * wrapped up and forwarded to the consumer in a timely manner.
 *
 * The function performs the following actions:
 * 1. Check whether the CY_FX_EP_PRODUCER endpoint has received any new data since the last function call.
 * 2. If data has been received:
 *    a. Wait until no new data is received for 5 us. This is to avoid the possibility of modifying the DMA
 *       channel while FX3 is still receiving data.
 *    b. Block the USB 3.0 endpoint memory to stop receiving data and apply a 1 us delay. The total delay for
 *       which this block is applied should be small to avoid data loss.
 *    c. Check whether the DMA socket corresponding CY_FX_EP_PRODUCER has moved to a new descriptor.
 *    d. If the descriptor has not changed:
 *       i. Force the buffer to wrap-up (commit) so that the data can go out.
 *    e. Unblock the USB 3.0 endpoint memory.
 */
static void
CyFxIsoOutCommitPartialBuffer (
        void)
{
    CyU3PDmaSocketConfig_t sockConf;
    uint16_t               activeDscr;
    uint8_t                maxWait = CY_FX_ISO_BURST;

    CyU3PGpioSimpleSetValue (17, CyFalse);

    /* If a packet has been committed and the active descriptor has not moved; force the buffer to wrap up. */
    if ((CY_FX_USB3_EPO_CTRL_STAT(CY_FX_EP_PRODUCER) & CY_FX_USB3_EPO_COMMIT_STAT) != 0)
    {
        /* Wait until data is no longer being committed. This is to ensure that we don't
           wrap up a buffer in the middle of a burst. */
        do {
            /* Clear the commit interrupt status bit for the OUT endpoint. */
            CY_FX_USB3_EPO_CTRL_STAT(CY_FX_EP_PRODUCER) =
                (CY_FX_USB3_EPO_CTRL_STAT(CY_FX_EP_PRODUCER) & CY_FX_USB3_EPO_CTRL_MASK) | CY_FX_USB3_EPO_COMMIT_STAT;
            CyU3PBusyWait (5);
        } while (((CY_FX_USB3_EPO_CTRL_STAT(CY_FX_EP_PRODUCER) & CY_FX_USB3_EPO_COMMIT_STAT) != 0) && (maxWait--));

        /* Don't allow data to be received while the wrapup is being done. */
        CyFxUsb3EpmDataBlock ();
        CyU3PBusyWait (1);

        /* Find the active descriptor for the producer socket. */
        CyU3PDmaSocketGetConfig (CY_FX_EP_PRODUCER_SOCKET, &sockConf);
        activeDscr = sockConf.dscrChain & CY_U3P_DSCR_NUMBER_MASK;

        /* If the descriptor has not changed, the data would be a multiple of the max packet size. Forcibly wrap
           up the buffer. */
        if ((activeDscr == LastDmaDescriptor) && (activeDscr < 512)
                && (CY_FX_DMA_DSCR_IS_OCCUPIED (activeDscr) == 0))
        {
            CY_FX_USB_EPO_SOCK_STAT(CY_FX_EP_PRODUCER) = sockConf.status | CY_FX_USB_EPO_SOCK_WRAPUP;
        }

        CyFxUsb3EpmDataUnblock ();
    }

    /* Clear the commit interrupt status bit for the OUT endpoint. */
    CY_FX_USB3_EPO_CTRL_STAT(CY_FX_EP_PRODUCER) =
        (CY_FX_USB3_EPO_CTRL_STAT(CY_FX_EP_PRODUCER) & CY_FX_USB3_EPO_CTRL_MASK) | CY_FX_USB3_EPO_COMMIT_STAT;

    /* Get the next DMA descriptor. The status needs to be queried again, as the buffer may have been wrapped up. */
    CyU3PDmaSocketGetConfig (CY_FX_EP_PRODUCER_SOCKET, &sockConf);
    LastDmaDescriptor = sockConf.dscrChain & CY_U3P_DSCR_NUMBER_MASK;

    CyU3PGpioSimpleSetValue (17, CyTrue);
}

/* This is the callback function to handle the USB events. */
void
CyFxIsoSrcSinkApplnUSBEventCB (
    CyU3PUsbEventType_t evtype, /* Event type */
    uint16_t            evdata  /* Event data */
    )
{
    switch (evtype)
    {
        case CY_U3P_USB_EVENT_SETCONF:
            if (evdata != 0)
                glIsDevConfigured = CyTrue;
            break;

        case CY_U3P_USB_EVENT_SETINTF:
            /* Stop the application before restarting. */
            if (glIsApplnActive)
            {
                CyFxIsoSrcSinkApplnStop ();
            }

            /* Start the loop back function if alternate setting 1 has been selected. */
            if (evdata == 0x0001)
                CyFxIsoSrcSinkApplnStart ();
            break;

        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_DISCONNECT:
            /* Stop the loop back function. */
            if (glIsApplnActive)
            {
                CyFxIsoSrcSinkApplnStop ();
            }
            glIsDevConfigured = CyFalse;
            break;

        case CY_U3P_USB_EVENT_SOF_ITP:
            if (CyU3PUsbGetSpeed() == CY_U3P_SUPER_SPEED)
                CyFxIsoOutCommitPartialBuffer ();
            break;

        case CY_U3P_USB_EVENT_EP_UNDERRUN:
            CyU3PDebugPrint (2, "EP Underrun\r\n");
            break;

        default:
            break;
    }
}

/* Callback function to handle LPM requests from the USB 3.0 host. This function is invoked by the API
   whenever a state change from U0 -> U1 or U0 -> U2 happens. If we return CyTrue from this function, the
   FX3 device is retained in the low power state. If we return CyFalse, the FX3 device immediately tries
   to trigger an exit back to U0.

   This application does not have any state in which we should not allow U1/U2 transitions; and therefore
   the function always return CyTrue.
 */
CyBool_t
CyFxIsoSrcSinkApplnLPMRqtCB (
        CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

/* This function initializes the USB Module, sets the enumeration descriptors.
 * This function does not start the ISO streaming and this is done only when
 * SET_CONF event is received. */
void
CyFxIsoSrcSinkApplnInit (void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Start the USB functionality. */
    apiRetStatus = CyU3PUsbStart();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PUsbStart failed to Start, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* The fast enumeration is the easiest way to setup a USB connection,
     * where all enumeration phase is handled by the library. Only the
     * class / vendor requests need to be handled by the application. */
    CyU3PUsbRegisterSetupCallback(CyFxIsoSrcSinkApplnUSBSetupCB, CyTrue);

    /* Setup the callback to handle the USB events. */
    CyU3PUsbRegisterEventCallback(CyFxIsoSrcSinkApplnUSBEventCB);

    /* Register a callback to handle LPM requests from the USB 3.0 host. */
    CyU3PUsbRegisterLPMRequestCallback(CyFxIsoSrcSinkApplnLPMRqtCB);
    
    /* Set the USB Enumeration descriptors */

    /* Super speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, 0, (uint8_t *)CyFxUSB30DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* High speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, 0, (uint8_t *)CyFxUSB20DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* BOS descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, 0, (uint8_t *)CyFxUSBBOSDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set configuration descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Device qualifier descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, 0, (uint8_t *)CyFxUSBDeviceQualDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device qualifier descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Super speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBSSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set configuration descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* High speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBHSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Other Speed Descriptor failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Full speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBFSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Configuration Descriptor failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 0 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 1 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 2 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Connect the USB Pins with super speed operation enabled. */
    apiRetStatus = CyU3PConnectState(CyTrue, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Connect failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    CyU3PDebugPrint(8,"App Initialized\r\n");
}

/* Entry function for the IsoSrcSinkAppThread. */
void
IsoSrcSinkAppThread_Entry (
        uint32_t input)
{
    CyU3PDmaBuffer_t    dmaInfo;
    CyU3PReturnStatus_t status;

    /* Initialize the debug module */
    CyFxIsoSrcSinkApplnDebugInit();

    /* Start the GPIO module and configure the status pin. */
    CyFxIsoSrcSinkGpioInit ();

    /* Initialize the ISO loop application */
    CyFxIsoSrcSinkApplnInit();

    for (;;)
    {
        if (glDmaPktsReceived > glDmaPktsDiscarded)
        {
            /* Check if there is any data on the OUT endpoint. Wait up to 2 ms for data. */
            status = CyU3PDmaChannelGetBuffer (&glChHandleIsoSink, &dmaInfo, 2);
            if (status == CY_U3P_SUCCESS)
            {
                //CyU3PDebugPrint (2, "ISOOUT: size=%d\r\n", dmaInfo.count);

                status = CyU3PDmaChannelDiscardBuffer (&glChHandleIsoSink);
                if (status == CY_U3P_SUCCESS)
                {
                    /* If we have freed up at least one buffer, keep LOW POWER mode disabled. */
                    glDmaPktsDiscarded++;
                    CyU3PUsbLPMDisable ();
                }
            }
        }

        /* Check if there is a free buffer on the IN endpoint. Wait up to 2 ms for data. */
        status = CyU3PDmaChannelGetBuffer (&glChHandleIsoSrc, &dmaInfo, 2);
        if (status == CY_U3P_SUCCESS)
        {
        	dmaInfo.buffer[0] = 0;
        	dmaInfo.buffer[1] = glPacketCounter;
            glPacketCounter++;

        	//uint32_t j;
            //for (j = 0; j < CY_FX_ISO_PKTS * CY_FX_ISO_BURST * CY_FX_DMA_MULTIPLIER; j++)
    		//{
            //	dmaInfo.buffer[j*CY_FX_ISO_MAXPKT] = (glPacketCounter >> 8) & 0xFF;
            //	dmaInfo.buffer[j*CY_FX_ISO_MAXPKT+1] = glPacketCounter & 0xFF;
            //	glPacketCounter++;
    		//}

            status = CyU3PDmaChannelCommitBuffer (&glChHandleIsoSrc, dmaInfo.size, 0);
            if (status == CY_U3P_SUCCESS)
            {
                /* If we have committed at least one buffer, keep LOW POWER mode disabled. */
                glDmaPktsCommitted++;
                CyU3PUsbLPMDisable ();
            }
        }

        if (glResetDevice)
        {
            CyU3PConnectState (CyFalse, CyTrue);
            CyU3PThreadSleep (1000);
            CyU3PDeviceReset (CyFalse);
            CyU3PThreadSleep (1000);
        }
    }
}

/* Application define function which creates the threads. */
void
CyFxApplicationDefine (
        void)
{
    void *ptr = NULL;
    uint32_t retThrdCreate = CY_U3P_SUCCESS;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_ISOSRCSINK_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (
            &isoSrcSinkAppThread,               /* ISO loop App Thread structure */
            "21:IsoSourceSink",                 /* Thread ID and Thread name */
            IsoSrcSinkAppThread_Entry,          /* ISO loop App Thread Entry function */
            0,                                  /* No input parameter to thread */
            ptr,                                /* Pointer to the allocated thread stack */
            CY_FX_ISOSRCSINK_THREAD_STACK,      /* ISO loop App Thread stack size */
            CY_FX_ISOSRCSINK_THREAD_PRIORITY,   /* ISO loop App Thread priority */
            CY_FX_ISOSRCSINK_THREAD_PRIORITY,   /* ISO loop App Thread priority */
            CYU3P_NO_TIME_SLICE,                /* No time slice for the application thread */
            CYU3P_AUTO_START                    /* Start the Thread immediately */
            );

    /* Check the return code */
    if (retThrdCreate != 0)
    {
        /* Thread Creation failed with the error code retThrdCreate */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }
}

/*
 * Main function
 */
int
main (void)
{
    CyU3PIoMatrixConfig_t io_cfg;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Initialize the device */
    status = CyU3PDeviceInit (NULL);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Initialize the caches. The D-Cache is kept disabled. Enabling it will cause performance to drop, as
       the driver will start performing un-necessary cache clean/flush operations.
     */
    status = CyU3PDeviceCacheControl (CyTrue, CyFalse, CyFalse);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix for the device. On the FX3 DVK board, the COM port 
     * is connected to the IO(53:56). This means that either DQ32 mode should be
     * selected or lppMode should be set to UART_ONLY. Here we are choosing
     * UART_ONLY configuration. */
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.s0Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.s1Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyFalse;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY;
    /* No GPIOs are enabled. */
    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0;
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;
    status = CyU3PDeviceConfigureIOMatrix (&io_cfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* This is a non returnable call for initializing the RTOS kernel */
    CyU3PKernelEntry ();

    /* Dummy return to make the compiler happy */
    return 0;

handle_fatal_error:

    /* Cannot recover from this error. */
    while (1);
}

/* [ ] */

