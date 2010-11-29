/* libusb-win32 WDF, Generic KMDF Windows USB Driver
 * Copyright (c) 2010-2011 Travis Robinson <libusbdotnet@gmail.com>
 * Copyright (c) 2002-2005 Stephan Meyer <ste_meyer@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "private.h"

//
// Globals
//

ULONG   DebugLevel = 4;


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
)
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
	WDF_DRIVER_CONFIG       config;
	NTSTATUS                status;

	USBMSG0("libusb-win32 - Driver Framework Edition.\n");
	USBMSG("Built %s %s\n", __DATE__, __TIME__);

	//
	// Initiialize driver config to control the attributes that
	// are global to the driver. Note that framework by default
	// provides a driver unload routine. If you create any resources
	// in the DriverEntry and want to be cleaned in driver unload,
	// you can override that by manually setting the EvtDriverUnload in the
	// config structure. In general xxx_CONFIG_INIT macros are provided to
	// initialize most commonly used members.
	//

	WDF_DRIVER_CONFIG_INIT(
	    &config,
	    LUsbW_EvtDeviceAdd
	);

	//
	// Create a framework driver object to represent our driver.
	//
	status = WdfDriverCreate(
	             DriverObject,
	             RegistryPath,
	             WDF_NO_OBJECT_ATTRIBUTES, // Driver Attributes
	             &config,          // Driver Config Info
	             WDF_NO_HANDLE // hDriver
	         );

	if (!NT_SUCCESS(status))
	{
		USBERR("WdfDriverCreate failed. status=%Xh\n", status);
	}

	return status;
}



