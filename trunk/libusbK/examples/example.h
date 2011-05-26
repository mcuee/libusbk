/*! \file example.h
* \brief Common include file used only in examples.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include "lusbk_usb.h"

#ifndef __EXAMPLE_H_
#define __EXAMPLE_H_

//! Device hardware id match string used for all examples.
//! The \b default is the official Microchip PIC Benchmark Device hardware id.
#define EXAMPLE_HWID "USB\\VID_04D8&PID_FA2E"

//! Compares the \ref KUSB_DEV_LIST::DeviceInstance member of \c deviceElement with \c hwid.
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

#endif
