/*
 ## Cypress USB 3.0 Platform source file (cyfxbulklpauto.cpp)
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

/* This file illustrates the bulkloop application example using the DMA AUTO mode */

/*
   This example illustrates a loopback mechanism between two USB bulk endpoints. The example comprises of
   vendor class USB enumeration descriptors with 2 bulk endpoints. A bulk OUT endpoint acts as the producer
   of data from the host. A bulk IN endpint acts as the consumer of data to the host.

   The loopback is achieved with the help of a DMA AUTO channel. DMA AUTO channel is created between the
   producer USB bulk endpoint and the consumer USB bulk endpoint. Data is transferred from the host into
   the producer endpoint which is then directly transferred to the consumer endpoint by the DMA engine.
   CPU is not involved in the data transfer.

   The DMA buffer size is defined based on the USB speed. 64 for full speed, 512 for high speed and 1024
   for super speed. CY_FX_BULKLP_DMA_BUF_COUNT in the header file defines the number of DMA buffers.
 */
#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyfxbulklpauto.h"
#include "cyu3usb.h"
#include "cyu3uart.h"
#include "cyu3utils.h"
#include <cstddef>

uint8_t glEp0Buffer[32] __attribute__ ((aligned (32))); /* Local buffer used for vendor command handling. */
uint8_t glBenchmarkMode = 3;

/* Class definition */
class CyFxBulkLoopApplication {
public:
    CyFxBulkLoopApplication ();         /* Constructor */
    CyU3PDmaChannel chHandleBulkLp;   /* DMA Channel handle */
    static CyBool_t isApplnActive;    /* Whether the application is active or not. */
    void CyFxAppErrorHandler (CyU3PReturnStatus_t apiRetStatus);
    static CyBool_t CyFxBulkLpApplnUSBSetupCB (uint32_t setupdat0, uint32_t setupdat1);
    static void CyFxBulkLpApplnUSBEventCB (CyU3PUsbEventType_t evtype, uint16_t evdata);
    static CyBool_t CyFxBulkLpApplnLPMRqtCB (CyU3PUsbLinkPowerMode link_mode);
};

/* Application Error Handler */
void CyFxBulkLoopApplication::CyFxAppErrorHandler (
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

CyBool_t CyFxBulkLoopApplication::isApplnActive = CyFalse;
CyFxBulkLoopApplication *glBulkLoop_p;
CyU3PThread     BulkLpAppThread;	 /* Bulk loop application thread structure */

/* This function initializes the debug module. The debug prints
 * are routed to the UART and can be seen using a UART console
 * running at 115200 baud rate. */
static void
CyFxBulkLpApplnDebugInit (void)
{
    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize the UART for printing debug messages */
    apiRetStatus = CyU3PUartInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set UART configuration */
    CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));

    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit  = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity   = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma    = CyTrue;

    apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* Set the UART transfer to a really large value. */
    apiRetStatus = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* Initialize the debug module. */
    apiRetStatus = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* Disable the header associated with debug logs. */
    CyU3PDebugPreamble (CyFalse);
}

/* This function starts the bulk loop application. This is called
 * when a SET_CONF event is received from the USB host. The endpoints
 * are configured and the DMA pipe is setup in this function. */
static void
CyFxBulkLpApplnStart (
        void)
{
    uint16_t size = 0;
    CyU3PEpConfig_t epCfg;
    CyU3PDmaChannelConfig_t dmaCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();

    /* First identify the usb speed. Once that is identified,
     * create a DMA channel and start the transfer on this. */

    /* Based on the Bus Speed configure the endpoint packet size */
    switch (usbSpeed)
    {
        case CY_U3P_FULL_SPEED:
            size = 64;
            break;

        case CY_U3P_HIGH_SPEED:
            size = 512;
            break;

        case  CY_U3P_SUPER_SPEED:
            size = 1024;
            break;

        default:
            CyU3PDebugPrint (4, "Error! Invalid USB speed.\n");
            glBulkLoop_p->CyFxAppErrorHandler (CY_U3P_ERROR_FAILURE);
            break;
    }

    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable   = CyTrue;
    epCfg.epType   = CY_U3P_USB_EP_BULK;
    epCfg.burstLen = 1;
    epCfg.streams  = 0;
    epCfg.pcktSize = size;

    /* Producer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler (apiRetStatus);
    }

    /* Create a DMA Auto Channel between two sockets of the U port.
     * DMA buffer size is set based on the USB speed. */
    dmaCfg.size           = size;
    dmaCfg.count          = CY_FX_BULKLP_DMA_BUF_COUNT;
    dmaCfg.prodSckId      = CY_FX_EP_PRODUCER_SOCKET;
    dmaCfg.consSckId      = CY_FX_EP_CONSUMER_SOCKET;
    dmaCfg.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    dmaCfg.notification   = 0;
    dmaCfg.cb             = NULL;
    dmaCfg.prodHeader     = 0;
    dmaCfg.prodFooter     = 0;
    dmaCfg.consHeader     = 0;
    dmaCfg.prodAvailCount = 0;

    apiRetStatus = CyU3PDmaChannelCreate (&glBulkLoop_p->chHandleBulkLp,
            CY_U3P_DMA_TYPE_AUTO, &dmaCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelCreate failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* Flush the Endpoint memory */
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

    /* Set DMA Channel transfer size */
    apiRetStatus = CyU3PDmaChannelSetXfer (&glBulkLoop_p->chHandleBulkLp, CY_FX_BULKLP_DMA_TX_SIZE);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PDmaChannelSetXfer Failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* Update the status flag. */
    glBulkLoop_p->isApplnActive = CyTrue;

    CyU3PDebugPrint(8,"App Started\r\n");
}

/* This function stops the bulk loop application. This shall be called whenever
 * a RESET or DISCONNECT event is received from the USB host. The endpoints are
 * disabled and the DMA pipe is destroyed by this function. */
static void
CyFxBulkLpApplnStop (
        void)
{
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Update the flag. */
    glBulkLoop_p->isApplnActive = CyFalse;

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);

    /* Destroy the channel */
    CyU3PDmaChannelDestroy (&glBulkLoop_p->chHandleBulkLp);

    /* Disable endpoints. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;

    /* Producer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler (apiRetStatus);
    }

    CyU3PDebugPrint(8,"App Stopped\r\n");
}

/* Callback to handle the USB setup requests. */
CyBool_t CyFxBulkLoopApplication::CyFxBulkLpApplnUSBSetupCB (
        uint32_t setupdat0, /* SETUP Data 0 */
        uint32_t setupdat1  /* SETUP Data 1 */
    )
{
    /* Fast enumeration is used. Only requests addressed to the interface, class,
     * vendor and unknown control requests are received by this function.
     * This application does not support any class or vendor requests. */

    uint8_t  bRequest, bReqType;
    uint8_t  bType, bTarget;
    uint16_t wValue, wIndex;
    CyBool_t isHandled = CyFalse;

    /* Decode the fields from the setup request. */
    bReqType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
    bType    = (bReqType & CY_U3P_USB_TYPE_MASK);
    bTarget  = (bReqType & CY_U3P_USB_TARGET_MASK);
    bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
    wValue   = ((setupdat0 & CY_U3P_USB_VALUE_MASK)   >> CY_U3P_USB_VALUE_POS);
    wIndex   = ((setupdat1 & CY_U3P_USB_INDEX_MASK)   >> CY_U3P_USB_INDEX_POS);

    if (bType == CY_U3P_USB_STANDARD_RQT)
    {
        /* Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
         * requests here. It should be allowed to pass if the device is in configured
         * state and failed otherwise. */
        if ((bTarget == CY_U3P_USB_TARGET_INTF) && ((bRequest == CY_U3P_USB_SC_SET_FEATURE)
                    || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0))
        {
            if (glBulkLoop_p->isApplnActive)
                CyU3PUsbAckSetup ();
            else
                CyU3PUsbStall (0, CyTrue, CyFalse);

            isHandled = CyTrue;
        }

        /* CLEAR_FEATURE request for endpoint is always passed to the setup callback
         * regardless of the enumeration model used. When a clear feature is received,
         * the previous transfer has to be flushed and cleaned up. This is done at the
         * protocol level. Since this is just a loopback operation, there is no higher
         * level protocol. So flush the EP memory and reset the DMA channel associated
         * with it. If there are more than one EP associated with the channel reset both
         * the EPs. The endpoint stall and toggle / sequence number is also expected to be
         * reset. Return CyFalse to make the library clear the stall and reset the endpoint
         * toggle. Or invoke the CyU3PUsbStall (ep, CyFalse, CyTrue) and return CyTrue.
         * Here we are clearing the stall. */
        if ((bTarget == CY_U3P_USB_TARGET_ENDPT) && (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
                && (wValue == CY_U3P_USBX_FS_EP_HALT))
        {
            if ((wIndex == CY_FX_EP_PRODUCER) || (wIndex == CY_FX_EP_CONSUMER))
            {
                if (glBulkLoop_p->isApplnActive)
                {
                    /* We NAK both the endpoints and allow some delay to let the NAK take effect. This has to
                     * be done before resetting the DMA channel.
                     */

                    CyU3PUsbSetEpNak (CY_FX_EP_PRODUCER, CyTrue);
                    CyU3PUsbSetEpNak (CY_FX_EP_CONSUMER, CyTrue);
                    CyU3PBusyWait (125);

                    /* Reset the DMA channel and reset/flush the endpoints. Both endpoints are reset because
                     * they are connected to the same DMA channel.
                     */
                    CyU3PDmaChannelReset (&glBulkLoop_p->chHandleBulkLp);
                    CyU3PUsbFlushEp (CY_FX_EP_PRODUCER);
                    CyU3PUsbFlushEp (CY_FX_EP_CONSUMER);
                    CyU3PUsbResetEp (CY_FX_EP_PRODUCER);
                    CyU3PUsbResetEp (CY_FX_EP_CONSUMER);
                    CyU3PDmaChannelSetXfer (&glBulkLoop_p->chHandleBulkLp, CY_FX_BULKLP_DMA_TX_SIZE);
                    CyU3PUsbStall (wIndex, CyFalse, CyTrue);

                    /* Un-NAK the endpoints. */
                    CyU3PUsbSetEpNak (CY_FX_EP_PRODUCER, CyFalse);
                    CyU3PUsbSetEpNak (CY_FX_EP_CONSUMER, CyFalse);

                    CyU3PUsbAckSetup ();
                    isHandled = CyTrue;
                }
            }
        }
    }

    if (bType == CY_U3P_USB_VENDOR_RQT)
    {
		// Benchmark set/get test
		if (bRequest == 0x0E || bRequest == 0x0F)
		{
			// only support the loop mode
			if (bRequest == 0x0E && ((wValue & 0xFF)!=3))
			{
				CyU3PUsbStall (0, CyTrue, CyFalse);
				isHandled = CyTrue;
			}
			else
			{
				glEp0Buffer[0] = 3;
				CyU3PUsbSendEP0Data(1, glEp0Buffer);
				isHandled     = CyTrue;
			}
		}
        if (bRequest == 0xE0)
        {
            isHandled = CyTrue;
            CyU3PUsbAckSetup ();
            CyU3PThreadSleep (100);
            CyU3PDeviceReset (CyFalse);
        }
    }

    return isHandled;
}

/* This is the callback function to handle the USB events. */
void CyFxBulkLoopApplication::CyFxBulkLpApplnUSBEventCB (
    CyU3PUsbEventType_t evtype, /* Event type */
    uint16_t            evdata  /* Event data */
    )
{
    switch (evtype)
    {
		case CY_U3P_USB_EVENT_SETINTF:
			/* Stop the application before re-enabling. */
			if (glBulkLoop_p->isApplnActive)
			{
				CyFxBulkLpApplnStop ();
			}

			/* If alt. setting 1 is selected, start the loop back function. */
			if (evdata == 0x0001)
			{
				/* Disable the low power entry to optimize USB throughput */
				CyU3PUsbLPMDisable();
				CyFxBulkLpApplnStart ();
			}
			break;

		case CY_U3P_USB_EVENT_RESET:
		case CY_U3P_USB_EVENT_DISCONNECT:
			/* Stop the loop back function. */
			if (glBulkLoop_p->isApplnActive)
			{
				CyFxBulkLpApplnStop ();
			}
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
CyFxBulkLoopApplication::CyFxBulkLpApplnLPMRqtCB (
        CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

/* This function initializes the USB Module, sets the enumeration descriptors.
 * This function does not start the bulk streaming and this is done only when
 * SET_CONF event is received. */
static void
CyFxBulkLpApplnInit (void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Start the USB functionality. */
    apiRetStatus = CyU3PUsbStart();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "CyU3PUsbStart failed to Start, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* The fast enumeration is the easiest way to setup a USB connection,
     * where all enumeration phase is handled by the library. Only the
     * class / vendor requests need to be handled by the application. */
    CyU3PUsbRegisterSetupCallback((CyU3PUSBSetupCb_t)glBulkLoop_p->CyFxBulkLpApplnUSBSetupCB, CyTrue);

    /* Setup the callback to handle the USB events. */
    CyU3PUsbRegisterEventCallback((CyU3PUSBEventCb_t)glBulkLoop_p->CyFxBulkLpApplnUSBEventCB);

    /* Register a callback to handle LPM requests from the USB 3.0 host. */
    CyU3PUsbRegisterLPMRequestCallback((CyU3PUsbLPMReqCb_t)glBulkLoop_p->CyFxBulkLpApplnLPMRqtCB);    

    /* Set the USB Enumeration descriptors */

    /* Super speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, 0, (uint8_t *)CyFxUSB30DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device descriptor failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* High speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, 0, (uint8_t *)CyFxUSB20DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device descriptor failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* BOS descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, 0, (uint8_t *)CyFxUSBBOSDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set configuration descriptor failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* Device qualifier descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, 0, (uint8_t *)CyFxUSBDeviceQualDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set device qualifier descriptor failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* Super speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBSSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set configuration descriptor failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* High speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBHSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Other Speed Descriptor failed, Error Code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* Full speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBFSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Set Configuration Descriptor failed, Error Code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 0 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 1 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* String descriptor 2 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB set string descriptor failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    /* Connect the USB Pins with super speed operation enabled. */
    apiRetStatus = CyU3PConnectState(CyTrue, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Connect failed, Error code = %d\n", apiRetStatus);
        glBulkLoop_p->CyFxAppErrorHandler(apiRetStatus);
    }

    CyU3PDebugPrint(8,"App Initialized\r\n");
}


CyFxBulkLoopApplication::CyFxBulkLoopApplication (void)
{
    CyFxBulkLpApplnDebugInit ();
    CyFxBulkLpApplnInit ();
}

/* Entry function for the BulkLpAppThread. */
void
BulkLpAppThread_Entry (
        uint32_t input)
{
    glBulkLoop_p = new CyFxBulkLoopApplication;

    for (;;)
    {
        CyU3PThreadSleep (1000);
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
    ptr = (void *)new uint8_t[CY_FX_BULKLP_THREAD_STACK];

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&BulkLpAppThread,           /* Bulk loop App Thread structure */
                          "21:Bulk_loop_AUTO",                     /* Thread ID and Thread name */
                          BulkLpAppThread_Entry,                   /* Bulk loop App Thread Entry function */
                          0,                                       /* No input parameter to thread */
                          ptr,                                     /* Pointer to the allocated thread stack */
                          CY_FX_BULKLP_THREAD_STACK,               /* Bulk loop App Thread stack size */
                          CY_FX_BULKLP_THREAD_PRIORITY,            /* Bulk loop App Thread priority */
                          CY_FX_BULKLP_THREAD_PRIORITY,            /* Bulk loop App Thread priority */
                          CYU3P_NO_TIME_SLICE,                     /* No time slice for the application thread */
                          CYU3P_AUTO_START                         /* Start the Thread immediately */
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

    /* Initialize the caches. Enable both instruction and data caches. */
    status = CyU3PDeviceCacheControl (CyTrue, CyTrue, CyTrue);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix for the device. On the FX3 DVK board, the COM port 
     * is connected to the IO(53:56). This means that either DQ32 mode should be
     * selected or lppMode should be set to UART_ONLY. Here we are choosing
     * UART_ONLY configuration. */
    io_cfg.isDQ32Bit        = CyFalse;
    io_cfg.s0Mode           = CY_U3P_SPORT_INACTIVE;
    io_cfg.s1Mode           = CY_U3P_SPORT_INACTIVE;
    io_cfg.useUart          = CyTrue;
    io_cfg.useI2C           = CyFalse;
    io_cfg.useI2S           = CyFalse;
    io_cfg.useSpi           = CyFalse;
    io_cfg.lppMode          = CY_U3P_IO_MATRIX_LPP_UART_ONLY;
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

