/// Benchmark.c 
/// Single interface ping-pong buffering enabled.

/* USB Benchmark for libusb-win32

    Copyright � 2010 Travis Robinson. <libusbdotnet@gmail.com>
    website: http://sourceforge.net/projects/libusb-win32
 
    Software License Agreement:
    
    The software supplied herewith is intended for use solely and
    exclusively on Microchip PIC Microcontroller products. This 
    software is owned by Travis Robinson, and is protected under
	applicable copyright laws. All rights are reserved. Any use in 
	violation of the foregoing restrictions may subject the user to 
	criminal sanctions under applicable laws, as well as to civil 
	liability for the breach of the terms and conditions of this
    license.

	You may redistribute and/or modify this file under the terms
	described above.
    
    THIS SOFTWARE IS PROVIDED IN AN �AS IS� CONDITION. NO WARRANTIES,
    WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
    TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE OWNER SHALL NOT,
    IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
    CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.	
*/

/** INCLUDES *******************************************************/
#include "USB/usb.h"
#include "USB/usb_function_generic.h"
#include "HardwareProfile.h"
#include "Benchmark.h"
#include "PicFWCommands.h"
#include "BDT_transfer.h"
#if defined(WITH_SERIAL_DEBUG)
#include "usart.h"
#endif

/** VARIABLES ******************************************************/
//NOTE:  The below endpoint buffers need to be located in a section of
//system SRAM that is accessible by the USB module.  The USB module on all
//currently existing Microchip USB microcontrollers use a dedicated DMA
//interface for reading/writing USB data into/out of main system SRAM.

//On some USB PIC microcontrollers, all of the microcontroller SRAM is dual
//access, and therefore all of it can be accessed by either the USB 
//module or the microcontroller core.  On other devices, only a certain 
//portion of the SRAM is accessible by the USB module. Therefore, on some 
//devices, it is important to place USB data buffers in certain sections of
//SRAM, while on other devices, the buffers can be placed anywhere.



//For all PIC18F87J50 family and PIC18F46J50 family devices: all SRAM is USB accessible.
//For PIC18F1xK50 devices: 0x200-0x2FF is USB accessible.
//For PIC18F4450/2450 devices: 0x400-0x4FF is USB accessible.
//For PIC18F4550/4553/4455/4458/2550/2553/2455/2458: 0x400-0x7FF is USB accessible.
#if defined(__18F14K50) || defined(__18F13K50) || defined(__18LF14K50) || defined(__18LF13K50) 
	#pragma udata USB_VARIABLES=0x240
#elif defined(__18F2455) || defined(__18F2550) || defined(__18F4455) || defined(__18F4550) || defined(__18F2458) || defined(__18F2453) || defined(__18F4558) || defined(__18F4553)
	#pragma udata USB_VARIABLES=0x440
#elif defined(__18F4450) || defined(__18F2450)
	#pragma udata USB_VARIABLES=0x440
#else
	#pragma udata
#endif

// Data buffers
BYTE BenchmarkBuffers_INTF0[2][PP_COUNT][USBGEN_EP_SIZE_INTF0];
#ifdef DUAL_INTERFACE
BYTE BenchmarkBuffers_INTF1[2][PP_COUNT][USBGEN_EP_SIZE_INTF1];
#endif

#if defined(VENDOR_BUFFER_ENABLED)
BYTE VendorBuffer[8];
#endif

// The below variables are only accessed by the CPU and can be placed anywhere in RAM.
#pragma udata

// Temporary variables 
WORD counter;

// Internal test variables
__attribute__ ((aligned (4))) volatile BYTE TestType_INTF0;
volatile BYTE PrevTestType_INTF0;

volatile BYTE FillCount_INTF0;
volatile BYTE NextPacketKey_INTF0;

#ifdef DUAL_INTERFACE
	__attribute__ ((aligned (4))) volatile BYTE TestType_INTF1;
	volatile BYTE PrevTestType_INTF1;

	volatile BYTE FillCount_INTF1;
	volatile BYTE NextPacketKey_INTF1;
#endif

/** EXTERNS ********************************************************/
extern void BlinkUSBStatus(void);
extern USB_VOLATILE BYTE USBAlternateInterface[USB_MAX_NUM_INT];
extern volatile WORD led_count;

#if defined(DESCRIPTOR_COUNTING_ENABLED)
unsigned char gDeviceDescriptorCount = 0;
unsigned char gConfigDescriptorCount = 0;
#endif

/** BMARK FUNCTIONS ************************************************/
void doBenchmarkLoop_INTF0(void);
void doBenchmarkWrite_INTF0(void);
void doBenchmarkRead_INTF0(void);

#ifdef DUAL_INTERFACE
	void doBenchmarkLoop_INTF1(void);
	void doBenchmarkWrite_INTF1(void);
	void doBenchmarkRead_INTF1(void);
#endif

void fillBuffer(BYTE* pBuffer, WORD size);

/** BMARK MACROS ****************************************************/
#define	mBenchMarkInit(IntfSuffix)				\
{												\
	TestType_##IntfSuffix=TEST_LOOP;			\
	PrevTestType_##IntfSuffix=TEST_LOOP;		\
	FillCount_##IntfSuffix=0;					\
	NextPacketKey_##IntfSuffix=0;				\
}


#define mSetWritePacketID(BufferPtr, BufferSize, FillCount, NextPacketKey)	\
{																			\
	if ((BufferSize)>0)														\
	{																		\
		if (FillCount < 3)													\
		{																	\
			FillCount++;													\
			fillBuffer((BYTE*)BufferPtr,BufferSize);						\
		}																	\
		BufferPtr[1]=NextPacketKey++;										\
	}																		\
}

// If interface #0 is iso, use an iso specific submit macro
#if (INTF0==EP_ISO)
	#define mSubmitTransfer_INTF0(BdtPtr, BufferLength) mBDT_SubmitIsoTransfer(BdtPtr, BufferLength)
#else
	#define mSubmitTransfer_INTF0(BdtPtr, BufferLength) mBDT_SubmitTransfer(BdtPtr)
#endif

// If interface #1 is iso, use an iso specific submit macro
#if (INTF1==EP_ISO)
	#define mSubmitTransfer_INTF1(BdtPtr, BufferLength) mBDT_SubmitIsoTransfer(BdtPtr, BufferLength)
#else
	#define mSubmitTransfer_INTF1(BdtPtr, BufferLength) mBDT_SubmitTransfer(BdtPtr)
#endif

#define GetBenchmarkBuffer(IntfSuffix, Direction, IsOdd) BenchmarkBuffers_##IntfSuffix[Direction][IsOdd]

// Swaps byte pointers
#define Swap(r1,r2) { pSwapBufferTemp = r1; r1 = r2; r2 = pSwapBufferTemp; }

/** BMARK DECLARATIONS *********************************************/
#pragma code

#if defined(VENDOR_BUFFER_ENABLED)

void OnSetVendorBufferComplete(void)
{
	// set the blink rate to double (for one blink interval).
	led_count=20000U;

	// clear the callback function pointer.
	outPipes[0].pFunc=NULL;

} // EndOf OnSetVendorBufferComplete

void PicFWSetVendorBufferHandler(void)
{
	BYTE bLength = SetupPkt.wLength & 0xFF;

	// save host data into VendorBuffer
	outPipes[0].pDst.bRam = VendorBuffer;
	outPipes[0].wCount.v[0] = (bLength < sizeof(VendorBuffer)) ? bLength : sizeof(VendorBuffer);

	// Tell the MCP framework to call OnSetVendorBufferComplete() when we have
	// all of the host data.
	outPipes[0].pFunc = OnSetVendorBufferComplete;

	outPipes[0].info.bits.busy = 1;	// claim HostToDevice session ownership
	inPipes[0].info.bits.busy = 0;	// release DeviceToHost session ownership (Necessary?)
}
#endif

// The Benchmark firmware "overrides" the USBCBInitEP to initialize
// the OUT (MCU Rx) endpoint with the first BenchmarkBuffer.
void USBCBInitEP(void)
{
    USBEnableEndpoint(USBGEN_EP_NUM_INTF0,USB_OUT_ENABLED|USB_IN_ENABLED|USBGEN_EP_HANDSHAKE_INTF0|USB_DISALLOW_SETUP);

	//Prepare the OUT endpoints to receive the first packets from the host.
	pBdtRxEp1->ADR = ConvertToPhysicalAddress(GetBenchmarkBuffer(INTF0, OUT_FROM_HOST, mBDT_IsOdd(pBdtRxEp1)));
	#if (PP_COUNT==(2))
		mBDT_TogglePP(pBdtRxEp1);
		pBdtRxEp1->ADR = ConvertToPhysicalAddress(GetBenchmarkBuffer(INTF0, OUT_FROM_HOST, mBDT_IsOdd(pBdtRxEp1)));
		mBDT_TogglePP(pBdtRxEp1);
	#endif

	doBenchmarkRead_INTF0();
	doBenchmarkRead_INTF0();
	
	pBdtTxEp1->ADR = ConvertToPhysicalAddress(GetBenchmarkBuffer(INTF0, IN_TO_HOST, mBDT_IsOdd(pBdtTxEp1)));
	#if (PP_COUNT==(2))
		mBDT_TogglePP(pBdtTxEp1);
		pBdtTxEp1->ADR = ConvertToPhysicalAddress(GetBenchmarkBuffer(INTF0, IN_TO_HOST, mBDT_IsOdd(pBdtTxEp1)));
		mBDT_TogglePP(pBdtTxEp1);
	#endif

	#ifdef DUAL_INTERFACE
		USBEnableEndpoint(USBGEN_EP_NUM_INTF1,USB_OUT_ENABLED|USB_IN_ENABLED|USBGEN_EP_HANDSHAKE_INTF1|USB_DISALLOW_SETUP);

		//Prepare the OUT endpoints to receive the first packets from the host.
		pBdtRxEp2->ADR = ConvertToPhysicalAddress(GetBenchmarkBuffer(INTF1, OUT_FROM_HOST, mBDT_IsOdd(pBdtRxEp2)));
		#if (PP_COUNT==(2))
			mBDT_TogglePP(pBdtRxEp2);
			pBdtRxEp2->ADR = ConvertToPhysicalAddress(GetBenchmarkBuffer(INTF1, OUT_FROM_HOST, mBDT_IsOdd(pBdtRxEp2)));
			mBDT_TogglePP(pBdtRxEp2);
		#endif

		doBenchmarkRead_INTF1();
		doBenchmarkRead_INTF1();
		
		pBdtTxEp2->ADR = ConvertToPhysicalAddress(GetBenchmarkBuffer(INTF1, IN_TO_HOST, mBDT_IsOdd(pBdtTxEp2)));
		#if (PP_COUNT==(2))
			mBDT_TogglePP(pBdtTxEp2);
			pBdtTxEp2->ADR = ConvertToPhysicalAddress(GetBenchmarkBuffer(INTF1, IN_TO_HOST, mBDT_IsOdd(pBdtTxEp2)));
			mBDT_TogglePP(pBdtTxEp2);
		#endif
	#endif

}
void USBCBCheckOtherReq(void)
{
	if (SetupPkt.RequestType != USB_SETUP_TYPE_VENDOR_BITFIELD) return;

	switch (SetupPkt.bRequest)
	{
	case PICFW_SET_TEST:
#ifdef DUAL_INTERFACE
		if ((SetupPkt.wIndex & 0xff) == INTF1_NUMBER)
		{
			TestType_INTF1=SetupPkt.wValue & 0xff;
			USBEP0SendRAMPtr((BYTE*)&TestType_INTF1, 1, USB_EP0_RAM | USB_EP0_INCLUDE_ZERO);
//			inPipes[0].pSrc.bRam = (BYTE*)&TestType_INTF1;  // Set Source
//			inPipes[0].info.bits.ctrl_trf_mem = USB_EP0_RAM;		// Set memory type
//			inPipes[0].wCount.v[0] = 1;						// Set data count
//			inPipes[0].info.bits.busy = 1;
		}
		else
#endif
		{
			TestType_INTF0=SetupPkt.wValue & 0xff;
			USBEP0SendRAMPtr((BYTE*)&TestType_INTF0, 1, USB_EP0_RAM | USB_EP0_INCLUDE_ZERO);
//			inPipes[0].pSrc.bRam = (BYTE*)&TestType_INTF0;  // Set Source
//			inPipes[0].info.bits.ctrl_trf_mem = USB_EP0_RAM;		// Set memory type
//			inPipes[0].wCount.v[0] = 1;						// Set data count
//			inPipes[0].info.bits.busy = 1;
		}
		break;
	case PICFW_GET_TEST:
#ifdef DUAL_INTERFACE
		if ((SetupPkt.wIndex & 0xff) == INTF1_NUMBER)
		{
			USBEP0SendRAMPtr((BYTE*)&TestType_INTF1, 1, USB_EP0_RAM | USB_EP0_INCLUDE_ZERO);
//			inPipes[0].pSrc.bRam = (BYTE*)&TestType_INTF1;		// Set Source
//			inPipes[0].info.bits.ctrl_trf_mem = USB_EP0_RAM;	// Set memory type
//			inPipes[0].wCount.v[0] = 1;							// Set data count
//			inPipes[0].info.bits.busy = 1;
		}
		else
#endif
		{
			USBEP0SendRAMPtr((BYTE*)&TestType_INTF0, 1, USB_EP0_RAM | USB_EP0_INCLUDE_ZERO);
//			inPipes[0].pSrc.bRam = (BYTE*)&TestType_INTF0;		// Set Source
//			inPipes[0].info.bits.ctrl_trf_mem = USB_EP0_RAM;	// Set memory type
//			inPipes[0].wCount.v[0] = 1;							// Set data count
//			inPipes[0].info.bits.busy = 1;
		}
		break;
#if defined(VENDOR_BUFFER_ENABLED)

	case PICFW_SET_VENDOR_BUFFER:
		PicFWSetVendorBufferHandler();
		break;

	case PICFW_GET_VENDOR_BUFFER:
#if defined(DESCRIPTOR_COUNTING_ENABLED)
		VendorBuffer[0]=gDeviceDescriptorCount;
		VendorBuffer[1]=gConfigDescriptorCount;
#endif
		inPipes[0].pSrc.bRam = VendorBuffer;				// Set Source
		inPipes[0].info.bits.ctrl_trf_mem = USB_EP0_RAM;	// Set memory type
		
		// Set data count
		inPipes[0].wCount.v[0] = SetupPkt.wLength & 0xFF;
		if (inPipes[0].wCount.v[0] > sizeof(VendorBuffer))
			inPipes[0].wCount.v[0]=sizeof(VendorBuffer);

		inPipes[0].info.bits.busy = 1;
		break;

#endif
	default:
		break;
	}//end switch

} // EndOf USBCBCheckOtherReq


void USBCBStdSetDscHandler(void)
{
#if defined(VENDOR_BUFFER_ENABLED)
	if(SetupPkt.bDescriptorType == USB_DESCRIPTOR_STRING && SetupPkt.bDscIndex == 0x55)
	{
		PicFWSetVendorBufferHandler();
	}//end if
#endif
} // EndOf USBCBStdSetDscHandler

#if defined(WITH_SERIAL_DEBUG)
void SerialDbg_Init(void)
{
	OpenUSART(USART_TX_INT_OFF &
	          USART_RX_INT_OFF &
	          USART_ASYNCH_MODE &
	          USART_EIGHT_BIT &
	          USART_CONT_RX &
	          USART_BRGH_HIGH,
	          CLOCK_FREQ*10/BAUDRATE/64 - 2);
	printf("Benchmark Firmware initializing...\r\n");
}
#endif

void Benchmark_Init(void)
{
		mBenchMarkInit(INTF0);
#ifdef DUAL_INTERFACE
		mBenchMarkInit(INTF1);
#endif

}//end UserInit

void fillBuffer(BYTE* pBuffer, WORD size)
{
	BYTE dataByte = 0;
	for (counter=0; counter < size; counter++)
	{
		pBuffer[counter] = dataByte++;
		if (dataByte == 0) dataByte++;
	}
}

void Benchmark_ProcessIO(void)
{
	//Blink the LEDs according to the USB device status, but only do so if the PC application isn't connected and controlling the LEDs.
	BlinkUSBStatus();

	//Don't attempt to read/write over the USB until after the device has been fully enumerated.
	if((USBDeviceState < CONFIGURED_STATE)||(USBSuspendControl==1))
	{
		return;
	}

	if (TestType_INTF0!=PrevTestType_INTF0)
	{
		FillCount_INTF0=0;
		NextPacketKey_INTF0=0;
		PrevTestType_INTF0=TestType_INTF0;
	}

	switch(TestType_INTF0)
	{
	case TEST_PCREAD:
		doBenchmarkWrite_INTF0();
		break;
	case TEST_PCWRITE:
		doBenchmarkRead_INTF0();
		break;
	case TEST_LOOP:
		doBenchmarkLoop_INTF0();
		break;
	default:
		doBenchmarkRead_INTF0();
		break;
	}

#ifdef DUAL_INTERFACE
	if (TestType_INTF1!=PrevTestType_INTF1)
	{
		FillCount_INTF1=0;
		NextPacketKey_INTF1=0;
		PrevTestType_INTF1=TestType_INTF1;
	}

	switch(TestType_INTF1)
	{
	case TEST_PCREAD:
		doBenchmarkWrite_INTF1();
		break;
	case TEST_PCWRITE:
		doBenchmarkRead_INTF1();
		break;
	case TEST_LOOP:
		doBenchmarkLoop_INTF1();
		break;
	default:
		doBenchmarkRead_INTF1();
		break;
	}
#endif

}//end Benchmark_ProcessIO

void doBenchmarkWrite_INTF0(void)
{
	WORD Length;
	BYTE* pBufferTx;

	if (!USBHandleBusy(pBdtTxEp1))
	{
		#if defined(SINGLE_INTERFACE_WITH_ALTSETTINGS)
			Length=(USBAlternateInterface[INTF0_NUMBER]==1) ? USBGEN_EP_SIZE_INTF0_ALT1 : USBGEN_EP_SIZE_INTF0_ALT0;
		#else
			Length=USBGEN_EP_SIZE_INTF0;
		#endif

		pBufferTx = USBHandleGetAddr(pBdtTxEp1);
		mSetWritePacketID(pBufferTx, Length, FillCount_INTF0, NextPacketKey_INTF0);
		mBDT_FillTransfer(pBdtTxEp1, pBufferTx, Length);
		mSubmitTransfer_INTF0(pBdtTxEp1, Length);

		mBDT_TogglePP(pBdtTxEp1);
	}
}

void doBenchmarkLoop_INTF0(void)
{
	WORD Length;
	BYTE* pBufferRx;
	BYTE* pBufferTx;

	if (!USBHandleBusy(pBdtRxEp1) && !USBHandleBusy(pBdtTxEp1))
	{
		pBufferTx = USBHandleGetAddr(pBdtRxEp1);
		pBufferRx = USBHandleGetAddr(pBdtTxEp1);
		#if INTF0==EP_ISO
			#if defined(SINGLE_INTERFACE_WITH_ALTSETTINGS)
				Length=(USBAlternateInterface[INTF0_NUMBER]==1) ? USBGEN_EP_SIZE_INTF0_ALT1 : USBGEN_EP_SIZE_INTF0_ALT0;
			#else
				Length=USBGEN_EP_SIZE_INTF0;
			#endif
		#else
			Length = mBDT_GetLength(pBdtRxEp1);
		#endif
		mBDT_FillTransfer(pBdtTxEp1, pBufferTx, Length);
		mSubmitTransfer_INTF0(pBdtTxEp1, Length);
		mBDT_TogglePP(pBdtTxEp1);

		#if defined(SINGLE_INTERFACE_WITH_ALTSETTINGS)
			Length=(USBAlternateInterface[INTF0_NUMBER]==1) ? USBGEN_EP_SIZE_INTF0_ALT1 : USBGEN_EP_SIZE_INTF0_ALT0;
		#else
			Length=USBGEN_EP_SIZE_INTF0;
		#endif

		mBDT_FillTransfer(pBdtRxEp1, pBufferRx, Length);
		mSubmitTransfer_INTF0(pBdtRxEp1, Length);
		mBDT_TogglePP(pBdtRxEp1);
	
	}
}

void doBenchmarkRead_INTF0(void)
{
	WORD Length;
	BYTE* pBufferRx;

	if (!USBHandleBusy(pBdtRxEp1))
	{
		#if defined(SINGLE_INTERFACE_WITH_ALTSETTINGS)
			Length=(USBAlternateInterface[INTF0_NUMBER]==1) ? USBGEN_EP_SIZE_INTF0_ALT1 : USBGEN_EP_SIZE_INTF0_ALT0;
		#else
			Length=USBGEN_EP_SIZE_INTF0;
		#endif

		pBufferRx = USBHandleGetAddr(pBdtRxEp1);
		mBDT_FillTransfer(pBdtRxEp1, pBufferRx, Length);
		mSubmitTransfer_INTF0(pBdtRxEp1, Length);
		mBDT_TogglePP(pBdtRxEp1);
	}
}

#ifdef DUAL_INTERFACE
void doBenchmarkWrite_INTF1(void)
{
	BYTE* pBufferTx;
	if (!USBHandleBusy(pBdtTxEp2))
	{
		pBufferTx = USBHandleGetAddr(pBdtTxEp2);
		mSetWritePacketID(pBufferTx, USBGEN_EP_SIZE_INTF1, FillCount_INTF1, NextPacketKey_INTF1);
		mBDT_FillTransfer(pBdtTxEp2, pBufferTx, USBGEN_EP_SIZE_INTF1);
		mSubmitTransfer_INTF1(pBdtTxEp2, USBGEN_EP_SIZE_INTF1);

		mBDT_TogglePP(pBdtTxEp2);
	}
}

void doBenchmarkLoop_INTF1(void)
{
	WORD Length;
	BYTE* pBufferRx;
	BYTE* pBufferTx;

	if (!USBHandleBusy(pBdtRxEp2) && !USBHandleBusy(pBdtTxEp2))
	{
		pBufferTx = USBHandleGetAddr(pBdtRxEp2);
		pBufferRx = USBHandleGetAddr(pBdtTxEp2);
#if INTF1==EP_ISO
		Length = USBGEN_EP_SIZE_INTF1;
#else
		Length = mBDT_GetLength(pBdtRxEp2);
#endif
		mBDT_FillTransfer(pBdtTxEp2, pBufferTx, Length);
		mSubmitTransfer_INTF1(pBdtTxEp2, Length);
		mBDT_TogglePP(pBdtTxEp2);

		mBDT_FillTransfer(pBdtRxEp2, pBufferRx, USBGEN_EP_SIZE_INTF1);
		mSubmitTransfer_INTF1(pBdtRxEp2, USBGEN_EP_SIZE_INTF1);
		mBDT_TogglePP(pBdtRxEp2);
	
	}
}

void doBenchmarkRead_INTF1(void)
{
	BYTE* pBufferRx;

	if (!USBHandleBusy(pBdtRxEp2))
	{
		pBufferRx = USBHandleGetAddr(pBdtRxEp2);
		mBDT_FillTransfer(pBdtRxEp2, pBufferRx, USBGEN_EP_SIZE_INTF1);
		mSubmitTransfer_INTF1(pBdtRxEp2, USBGEN_EP_SIZE_INTF1);
		mBDT_TogglePP(pBdtRxEp2);
	}
}

#endif
