/*! \file examples.h
* \brief Common include file used only in examples.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include "lusbk_usb.h"

#ifndef __EXAMPLE_H_
#define __EXAMPLE_H_

//! Handy format string for matching against the \ref KUSB_DEV_LIST::DeviceInstance prefix.
#define EXAMPLES_DEVICE_HWID_FORMAT "USB\\VID_%04X&PID_%04X"

//! Default example vendor id
#define EXAMPLE_VID 0x04D8

//! Default example product id
#define EXAMPLE_PID 0xFA2E

//! Device hardware id match string used for all examples.
//! The \b default is the official Microchip PIC Benchmark Device hardware id.
#define EXAMPLE_HWID "USB\\VID_04D8&PID_FA2E"

//! Compares the \ref KUSB_DEV_LIST::DeviceInstance of \c deviceElement with \c hwid.
/*!
* \param deviceElement
* The device list element to test
*
* \param hwid
* The string compared to the \c DeviceInstance member.
* By default, all examples pass \ref EXAMPLE_HWID for this parameter.
*
* \returns
* 0 if hwid is a match, otherwise a non-zero value.
*/
#define Match_DeviceID(deviceElement, hwid)	\
	_strnicmp(deviceElement->DeviceInstance, hwid, strlen(hwid))

#define Parse_DeviceID(deviceElement, vidRtn, pidRtn)	\
	(sscanf(deviceElement->DeviceInstance, "USB\\VID_%04X&PID_%04X", vidRtn, pidRtn)==2?TRUE:FALSE)

//! Searches a command line argument list for devices matching a specific vid/pid.
/*!
* Arguments are Interpreted as follows:
* - vid=<4 digit hex>
*   - hex number is a vendor id.
* - pid=<4 digit hex>
*   - hex number is a product id.
*
* \param headDeviceList
* On success, the head device list element.
*
* \param foundDevice
* On success, the device list element that matches.
*
* \param argc
* The \c argc parameter of the \b main() application function.
*
* \param argv
* The \c argv parameter of the \b main() application function.
*
* \returns
* On success 0, otherwise a win32 system error code value.
*/
DWORD Examples_GetTestDevice(__deref_out PKUSB_DEV_LIST* headDeviceList,
                             __deref_out PKUSB_DEV_LIST* foundDevice,
                             __in int argc,
                             __in char* argv[]);
#endif
