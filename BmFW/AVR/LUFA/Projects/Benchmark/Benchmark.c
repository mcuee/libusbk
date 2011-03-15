/*
			 LUFA Library
	 Copyright (C) Dean Camera, 2010.

  dean [at] fourwalledcubicle [dot] com
		   www.lufa-lib.org
*/

/*
  Copyright 2011  Pete Batard (pbatard [at] gmail [dot] com)
  Copyright 2010-2011 Travis Robinson (libusb.win32.support [at] gmail [dot] com)
  Copyright 2010  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortuous action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the RelayBoard program. This file contains the main tasks of
 *  the project and is responsible for the initial application hardware configuration.
 */

#include "Benchmark.h"
#include "USB_Config.h"

/* Debugging */
#define SERIAL_DEBUG
#if defined(SERIAL_DEBUG)
#include <LUFA/Drivers/Peripheral/Serial.h>
#define bm_dbg printf
#else
#define bm_dbg(...) do {} while (0)
#endif

/* Data buffers */
volatile uint8_t BenchmarkBuffers_INTF0[2][PP_COUNT][USBGEN_EP_SIZE_INTF0];
#ifdef DUAL_INTERFACE
volatile uint8_t BenchmarkBuffers_INTF1[2][PP_COUNT][USBGEN_EP_SIZE_INTF1];
#endif

#define GetBenchmarkBuffer(IntfSuffix, Direction, IsOdd) BenchmarkBuffers_##IntfSuffix[Direction][IsOdd]

#ifdef ENABLE_VENDOR_BUFFER_AND_SET_DESCRIPTOR
uint8_t VendorBuffer[8];
#endif

/* Temporary variables */
uint16_t counter;

/* Internal test variables */
volatile uint8_t TestType_INTF0;
volatile uint8_t PrevTestType_INTF0;

volatile uint8_t FillCount_INTF0;
volatile uint8_t NextPacketKey_INTF0;

#ifdef DUAL_INTERFACE
	volatile uint8_t TestType_INTF1;
	volatile uint8_t PrevTestType_INTF1;

	volatile uint8_t FillCount_INTF1;
	volatile uint8_t NextPacketKey_INTF1;
#endif

/* Externs */
extern void BlinkUSBStatus(void);
extern volatile uint8_t USBAlternateInterface[USB_MAX_NUM_INT];
extern volatile uint16_t led_count;

/* Benchmark functions */
void doBenchmarkLoop_INTF0(void);
void doBenchmarkWrite_INTF0(void);
void doBenchmarkRead_INTF0(void);

#ifdef DUAL_INTERFACE
	void doBenchmarkLoop_INTF1(void);
	void doBenchmarkWrite_INTF1(void);
	void doBenchmarkRead_INTF1(void);
#endif

void fillBuffer(uint8_t* pBuffer, uint16_t size);

/* Benchmark macros */
#define mBenchMarkInit(IntfSuffix)				\
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
			fillBuffer((uint8_t*)BufferPtr,BufferSize);						\
		}																	\
		BufferPtr[1]=NextPacketKey++;										\
	}																		\
}

// If interface #0 is iso, use an iso specific submit macro
#if (EP_INTF0 == EP_TYPE_ISOCHRONOUS)
	#define mSubmitTransfer_INTF0(BdtPtr, BufferLength) mBDT_SubmitIsoTransfer(BdtPtr, BufferLength)
#else
	#define mSubmitTransfer_INTF0(BdtPtr, BufferLength) mBDT_SubmitTransfer(BdtPtr)
#endif

// If interface #1 is iso, use an iso specific submit macro
#if (EP_INTF1 == EP_TYPE_ISOCHRONOUS)
	#define mSubmitTransfer_INTF1(BdtPtr, BufferLength) mBDT_SubmitIsoTransfer(BdtPtr, BufferLength)
#else
	#define mSubmitTransfer_INTF1(BdtPtr, BufferLength) mBDT_SubmitTransfer(BdtPtr)
#endif

#define GetBenchmarkBuffer(IntfSuffix, Direction, IsOdd) BenchmarkBuffers_##IntfSuffix[Direction][IsOdd]

// Swaps byte pointers
#define Swap(r1,r2) { pSwapBufferTemp = r1; r1 = r2; r2 = pSwapBufferTemp; }


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();
	
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	sei();

	for (;;) {
		Benchmark_ProcessIO();
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the project's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
#if defined(SERIAL_DEBUG)
	Serial_Init(115200, true);
	Serial_CreateStream(NULL);
#endif
	LEDs_Init();	
	USB_Init();
//	bm_dbg("SetupHardware\r\n");
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
//	bm_dbg("Connect\r\n");
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
//	bm_dbg("Disconnect\r\n");
}

/** Event handler for the library USB Configuration Changed event. */
/* Do not add debug statements or any delay to this call! */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;
	int i;

	LEDs_SetAllLEDs(LEDMASK_BUSY);
	// TODO: good place to call USBCBInitEP?

	/* Setup Data Endpoint(s) */
	ConfigSuccess &= Endpoint_ConfigureEndpoint(USBGEN_EP_NUM_INTF0, EP_TYPE_BULK, ENDPOINT_DIR_OUT,
												USBGEN_EP_SIZE_INTF0_ALT0, ENDPOINT_BANK_SINGLE);

	ConfigSuccess &= Endpoint_ConfigureEndpoint(USBGEN_EP_NUM_INTF0, EP_TYPE_BULK, ENDPOINT_DIR_IN,
												USBGEN_EP_SIZE_INTF0_ALT0, ENDPOINT_BANK_SINGLE);

	while (!ConfigSuccess) {
		LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		_delay_ms(500);
		LEDs_SetAllLEDs(LEDMASK_BUSY);
		_delay_ms(500);
	}

	/* If you don't add a delay here, Windows will stall the device! */
	for (i = 0; i < 5; i ++) {
		LEDs_SetAllLEDs(LEDMASK_BUSY);
		_delay_ms(50);
		LEDs_SetAllLEDs(LEDMASK_USB_READY);
		_delay_ms(50);
	}

	// You do NOT want to add a delay with a debug message here - it may stall the device
//	bm_dbg("ConfigurationChanged: %s\r\n", ConfigSuccess ? "SUCCESS" : "FAILED!");	
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	volatile uint8_t* dataToSend = GetBenchmarkBuffer(INTF0, 1, 0);
	
	bm_dbg("Ctrl=%x\r\n", USB_ControlRequest.bRequest);

	/* Process General control requests */
	switch (USB_ControlRequest.bRequest) {
		case REQ_SetInterface:
			/* Set Interface is not handled by the library, as its function is application-specific */
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_INTERFACE)) {
				Endpoint_ClearSETUP();
				Endpoint_ClearStatusStage();
				bm_dbg("SetInt=%x\r\n", USB_ControlRequest.wIndex);
			}
			break;
		case FW_SET_TEST:
			// TODO: is REQDIR_DEVICETOHOST expected here?
			if (USB_ControlRequest.bmRequestType & REQTYPE_VENDOR) {
				TestType_INTF0 = USB_ControlRequest.wValue & 0xff;
			
				/* Write a one byte packet to acknowledge the test */
				dataToSend[0] = TestType_INTF0;
				Endpoint_ClearSETUP();
				Endpoint_Write_Control_Stream_LE(&dataToSend, 1);
				Endpoint_ClearOUT();
				bm_dbg("SetTest=%d\r\n", TestType_INTF0);
			}
			break;
	}
}

void fillBuffer(uint8_t* pBuffer, uint16_t size)
{
	uint8_t dataByte = 0;
	for (counter=0; counter<size; counter++) {
		pBuffer[counter] = dataByte++;
		if (dataByte == 0) dataByte++;
	}
}

void doBenchmarkRead_INTF0(void)
{
	uint8_t ErrorCode;
	volatile uint8_t* pBufferRx = GetBenchmarkBuffer(INTF0, 0, 0);
	
	LEDs_SetAllLEDs(LEDMASK_BUSY);
	
	Endpoint_SelectEndpoint(USBGEN_EP_NUM_INTF0);

	if (!Endpoint_IsConfigured()) {
		return;
	}

	Endpoint_SetEndpointDirection(ENDPOINT_DIR_OUT);
	if (Endpoint_IsOUTReceived() && Endpoint_IsReadWriteAllowed()) {
		ErrorCode = Endpoint_Read_Stream_LE((void*)pBufferRx, USBGEN_EP_SIZE_INTF0_ALT0, NULL);
		if (ErrorCode != ENDPOINT_RWSTREAM_NoError) {
			bm_dbg("RRerr %d\r\n", ErrorCode);
		} else {
//			bm_dbg("RRead %d\r\n", USBGEN_EP_SIZE_INTF0_ALT0);
		}
		Endpoint_ClearOUT();
	}

	LEDs_SetAllLEDs(LEDMASK_USB_READY);
}

void doBenchmarkWrite_INTF0(void)
{
	uint8_t ErrorCode;
	volatile uint8_t* pBufferTx = GetBenchmarkBuffer(INTF0, 0, 0);
	
	LEDs_SetAllLEDs(LEDMASK_BUSY);
	
	Endpoint_SelectEndpoint(USBGEN_EP_NUM_INTF0);

	if (!Endpoint_IsConfigured()) {
		return;
	}	

	Endpoint_SetEndpointDirection(ENDPOINT_DIR_IN);
	if (Endpoint_IsINReady() && Endpoint_IsReadWriteAllowed()) {
		ErrorCode = Endpoint_Write_Stream_LE((void*)pBufferTx, USBGEN_EP_SIZE_INTF0_ALT0, NULL);
		if (ErrorCode != ENDPOINT_RWSTREAM_NoError) {
			bm_dbg("WWerr %d\r\n", ErrorCode);
		} else {
//			bm_dbg("WWrote %d\r\n", USBGEN_EP_SIZE_INTF0_ALT0);
		}
		Endpoint_ClearIN();
	}

	LEDs_SetAllLEDs(LEDMASK_USB_READY);
}


void doBenchmarkLoop_INTF0(void)
{
	uint8_t ErrorCode;
	volatile uint8_t* pBufferTx = GetBenchmarkBuffer(INTF0, 1, 0);
	volatile uint8_t* pBufferRx = GetBenchmarkBuffer(INTF0, 0, 0);
	
	Endpoint_SelectEndpoint(USBGEN_EP_NUM_INTF0);

	if (!Endpoint_IsConfigured()) {
//		bm_dbg("NOCF\r\n");
		return;
	}
	
}

void Benchmark_Init(void)
{
		mBenchMarkInit(INTF0);
#ifdef DUAL_INTERFACE
		mBenchMarkInit(INTF1);
#endif
}

void Benchmark_ProcessIO(void)
{
	/* Device must be connected and configured for the task to run */
	if (USB_DeviceState != DEVICE_STATE_Configured) {
		Benchmark_Init();
		return;
	}

	if (TestType_INTF0 != PrevTestType_INTF0)
	{
		FillCount_INTF0 = 0;
		NextPacketKey_INTF0 = 0;
		PrevTestType_INTF0 = TestType_INTF0;
	}

	switch(TestType_INTF0) {
	case TEST_PCREAD:
		doBenchmarkWrite_INTF0();
		break;
	case TEST_PCWRITE:
		doBenchmarkRead_INTF0();
		break;
	case TEST_LOOP:
//		doBenchmarkLoop_INTF0();
		break;
	default:
		doBenchmarkRead_INTF0();
		break;
	}
}
