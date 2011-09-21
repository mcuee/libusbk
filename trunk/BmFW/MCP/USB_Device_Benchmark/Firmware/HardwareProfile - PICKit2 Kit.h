/********************************************************************
 FileName:     	HardwareProfile - PICKit2 Kit
 Dependencies:  See INCLUDES section
 Processor:     PIC18F2250 USB Microcontroller
 Hardware:      PICKit2 Kit
 Compiler:      Microchip C18
 Company:       Microchip Technology, Inc. modified by Pete Batard
                pbatard@gmail.com

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the “Company”) for its PIC® Microcontroller is intended and
 supplied to you, the Company’s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Date         Description
  1.0   2010.03.05   Initial release
********************************************************************/

#ifndef HARDWARE_PROFILE_PIKKIT2_KIT_H
#define HARDWARE_PROFILE_PIKKIT2_KIT_H

    /*******************************************************************/
    /******** USB stack hardware selection options *********************/
    /*******************************************************************/
    //This section is the set of definitions required by the MCHPFSUSB
    //  framework.  These definitions tell the firmware what mode it is
    //  running in, and where it can find the results to some information
    //  that the stack needs.
    //These definitions are required by every application developed with
    //  this revision of the MCHPFSUSB framework.  Please review each
    //  option carefully and determine which options are desired/required
    //  for your application.

    //The PICDEM FS USB Demo Board platform supports the USE_SELF_POWER_SENSE_IO
    //and USE_USB_BUS_SENSE_IO features.  Uncomment the below line(s) if
    //it is desireable to use one or both of the features.
    //#define USE_SELF_POWER_SENSE_IO
    #define tris_self_power     TRISBbits.TRISB5    // Input
    #if defined(USE_SELF_POWER_SENSE_IO)
    #define self_power          PORTBbits.RB5
    #else
    #define self_power          1
    #endif

    //#define USE_USB_BUS_SENSE_IO
    #define tris_usb_bus_sense  TRISBbits.TRISB5    // Input
    #if defined(USE_USB_BUS_SENSE_IO)
    #define USB_BUS_SENSE       PORTBbits.RB5
    #else
    #define USB_BUS_SENSE       1
    #endif


    //Uncomment the following line to make the output HEX of this  
    //  project work with the MCHPUSB Bootloader    
    //#define PROGRAMMABLE_WITH_USB_MCHPUSB_BOOTLOADER
	
    //Uncomment the following line to make the output HEX of this 
    //  project work with the HID Bootloader
    #define PROGRAMMABLE_WITH_USB_HID_BOOTLOADER		

    /*******************************************************************/
    /*******************************************************************/
    /*******************************************************************/
    /******** Application specific definitions *************************/
    /*******************************************************************/
    /*******************************************************************/
    /*******************************************************************/

    /** Board definition ***********************************************/
    //These defintions will tell the main() function which board is
    //  currently selected.  This will allow the application to add
    //  the correct configuration bits as wells use the correct
    //  initialization functions for the board.  These defitions are only
    //  required in the stack provided demos.  They are not required in
    //  final application design.
    #define DEMO_BOARD PICKIT2_KIT_USB
    #define PICKIT2_KIT_USB
    #define CLOCK_FREQ 20000000

    /** LED ************************************************************/
    #define mInitAllLEDs()      LATC &= 0xFE; TRISC &= 0xFE; LATB &= 0xEF; TRISB &= 0xEF;
    
    #define mLED_1              LATCbits.LATC0
    #define mLED_2              LATBbits.LATB4
    
    #define mGetLED_1()         mLED_1
    #define mGetLED_2()         mLED_2

    #define mLED_1_On()         mLED_1 = 1;
    #define mLED_2_On()         mLED_2 = 1;
    
    #define mLED_1_Off()        mLED_1 = 0;
    #define mLED_2_Off()        mLED_2 = 0;
    
    #define mLED_1_Toggle()     mLED_1 = !mLED_1;
    #define mLED_2_Toggle()     mLED_2 = !mLED_2;
    
    /** SWITCH *********************************************************/
    #define mInitAllSwitches()  TRISBbits.TRISB5=1;
    #define mInitSwitch1()      TRISBbits.TRISB5=1;
    #define sw1                 PORTBbits.RB5

    /** Serial Debug ***************************************************/
    // Uncomment the following if you want serial debug
    #define WITH_SERIAL_DEBUG
    #define BAUDRATE 115200

    /** USART fix ******************************************************/
    // If you use one of the optional 24LC512 PCB location as connector for 
    // (TTL) Serial output, then, if you didn't cut the traces, you will have
    // SCL (RB1) connected to WP (RC6) and SDA (RB0) grounded.
    // To make sure these pins don't interfere with our serial operations, we
    // set them as input.
    #define mDisableSCLSDA()    TRISC |= 0x03;	// Set as inputs
    
    /** I/O pin definitions ********************************************/
    #define INPUT_PIN 1
    #define OUTPUT_PIN 0
#endif  //HARDWARE_PROFILE_PIKKIT2_KIT_H
