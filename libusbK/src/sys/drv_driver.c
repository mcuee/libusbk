/*!********************************************************************
libusbK - WDF USB driver.
Copyright (C) 2012 Travis Lee Robinson. All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Lee Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen         (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/

#include "drv_common.h"

//////////////////////////////////////////////////////////////////////////////
// Driver Globals
//
ULONG DebugLevel = 4;
volatile LONG DeviceInstanceNumber = 0;
WDFDRIVER gWdfDriver = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
//#pragma alloc_text(PAGE, DriverExit)
#endif

NTSTATUS
DriverEntry(
    __in PDRIVER_OBJECT  DriverObject,
    __in PUNICODE_STRING RegistryPath)
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

	DbgPrint("[%s](%s) v%s built-on: %s %s\n", __FUNCTION__, RC_FILENAME_STR, RC_VERSION_STR, __DATE__, __TIME__);

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
	    Device_OnAdd
	);

	config.EvtDriverUnload = DriverExit;
	//
	// Create a framework driver object to represent our driver.
	//
	status = WdfDriverCreate(
	             DriverObject,
	             RegistryPath,
	             WDF_NO_OBJECT_ATTRIBUTES, // Driver Attributes
	             &config,          // Driver Config Info
	             &gWdfDriver);	// hDriver
	         );

	if (!NT_SUCCESS(status))
	{
		DbgPrint("Test\n");
		USBERR("WdfDriverCreate failed. status=%Xh\n", status);
	}

	return status;
}


VOID DriverExit(__in WDFDRIVER Driver)
{
	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE();

	DbgPrint("[%s](%s) v%s built-on: %s %s\n", __FUNCTION__, RC_FILENAME_STR, RC_VERSION_STR, __DATE__, __TIME__);
}
