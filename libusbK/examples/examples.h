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

//! Default example vendor id
#define EXAMPLE_VID 0x04D8

//! Default example product id
#define EXAMPLE_PID 0xFA2E

//! Helper function for examples; searches a command line argument list for devices matching a specific vid/pid.
/*!
* Arguments are interpreted as follows:
* - vid=<4 digit hex>
*   - hex number is a vendor id.
* - pid=<4 digit hex>
*   - hex number is a product id.
*
* \param DeviceList
* On success, contains a pointer to the device list.
*
* \param DeviceInfo
* On success, contains a pointer to the first device info structure which matched.
*
* \param argc
* The \c argc parameter of the \b main() application function.
*
* \param argv
* The \c argv parameter of the \b main() application function.
*
* \returns
* TRUE if a devices was found, otherwise FALSE.
*/
BOOL Examples_GetTestDevice( __deref_out PKUSB_DEV_LIST* DeviceList,
                             __deref_out PKUSB_DEV_INFO* DeviceInfo,
                             __in int argc,
                             __in char* argv[]);
#endif
