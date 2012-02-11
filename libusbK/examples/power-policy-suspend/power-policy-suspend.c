/*!
#
# Copyright (c) 2012 Travis Robinson <libusbdotnet@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS LEE ROBINSON
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
*/
#include "examples.h"

// Example configuration:

// Globals:
KUSB_DRIVER_API Usb;

DWORD __cdecl main(int argc, char* argv[])
{
	DWORD errorCode = ERROR_SUCCESS;
	BOOL success;

	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE usbHandle = NULL;
	ULONG polLength;
	BYTE polAutoSuspend;
	ULONG polSuspendDelay;

	/*
	Find the test device. Uses "vid=hhhh pid=hhhh" arguments supplied on the
	command line. (default is: vid=04D8 pid=FA2E)
	*/
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();


	if (deviceInfo->DriverID != KUSB_DRVID_LIBUSBK && deviceInfo->DriverID != KUSB_DRVID_WINUSB)
	{
		printf(
			"[Warning] libusb-win32 driver (libusb0.sys) does not support power\n"
			"          management.\n");
	}
	else
	{
		printf(
		    "[Note] If the DeviceIdleEnabled policy is not set in the .inf file when\n"
		    "       the device is installed, the AUTO_SUSPEND policy will be ignored.\n");

		if (deviceInfo->DriverID == KUSB_DRVID_WINUSB)
		{
			printf(
			    "[Note] WinUSB behaves slightly different then libusbK because it reset\n"
			    "       power policies back to their default values when the usb handle\n"
			    "       is closed.\n");
		}
	}

	/*
	This example will use the dynamic driver api so that it can be used
	with all supported drivers.
	*/
	LibK_LoadDriverAPI(&Usb, deviceInfo->DriverID);

	/*
	Initialize the device. This creates the physical usb handle.
	*/
	if (!Usb.Init(&usbHandle, deviceInfo))
	{
		errorCode = GetLastError();
		printf("Init device failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");


	/*!
	Get the auto suspend power policy.
	*/
	polLength = 1;
	success = Usb.GetPowerPolicy(usbHandle, AUTO_SUSPEND, &polLength, &polAutoSuspend);
	if (!success)
	{
		errorCode = GetLastError();
		printf("GetPowerPolicy AUTO_SUSPEND failed. ErrorCode: %08Xh\n", errorCode);
		goto Done;
	}
	printf("AUTO_SUSPEND  is currently %s.\n", (polAutoSuspend ? "enabled" : "disabled"));

	/*!
	Get the suspend delay power policy.
	*/
	polLength = 4;
	success = Usb.GetPowerPolicy(usbHandle, SUSPEND_DELAY, &polLength, &polSuspendDelay);
	if (!success)
	{
		errorCode = GetLastError();
		printf("GetPowerPolicy SUSPEND_DELAY failed. ErrorCode: %08Xh\n", errorCode);
		goto Done;
	}
	if (polAutoSuspend)
	{
		printf("SUSPEND_DELAY is set to %d ms\n", polSuspendDelay);
	}

	/*!
	Toggle the auto suspend power policy.
	*/
	polAutoSuspend = polAutoSuspend ? 0 : 1;
	polLength = 1;
	success = Usb.SetPowerPolicy(usbHandle, AUTO_SUSPEND, polLength, &polAutoSuspend);
	if (!success)
	{
		errorCode = GetLastError();
		printf("SetPowerPolicy AUTO_SUSPEND=%u failed. ErrorCode: %08Xh\n", polAutoSuspend, errorCode);
		goto Done;
	}
	printf("Set AUTO_SUSPEND = %s.\n", (polAutoSuspend ? "On" : "Off"));

	/*!
	If the auto suspend policy is now enabled, change the suspend delay power policy value.
	*/
	if (polAutoSuspend)
	{
		polSuspendDelay *= 2;
		if (!polSuspendDelay || polSuspendDelay > 12800)
			polSuspendDelay = 100;

		polLength = 4;
		success = Usb.SetPowerPolicy(usbHandle, SUSPEND_DELAY, polLength, &polSuspendDelay);
		if (!success)
		{
			errorCode = GetLastError();
			printf("SetPowerPolicy SUSPEND_DELAY=%u failed. ErrorCode: %08Xh\n", polSuspendDelay, errorCode);
			goto Done;
		}
		printf("Set SUSPEND_DELAY = %d ms\n", polSuspendDelay);
	}

Done:

	/*
	Close the usb handle.
	*/
	if (usbHandle) Usb.Free(usbHandle);
	/*
	Free the device list.
	*/
	LstK_Free(deviceList);

	return errorCode;
}
/*!
RUN #1
Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
[Note] If the DeviceIdleEnabled policy is not set in the .inf file when
       the device is installed, the AUTO_SUSPEND policy will be ignored.
Device opened successfully!
AUTO_SUSPEND  is currently enabled.
SUSPEND_DELAY is set to 5000 ms
Set AUTO_SUSPEND = Off.

RUN #2
Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
[Note] If the DeviceIdleEnabled policy is not set in the .inf file when
       the device is installed, the AUTO_SUSPEND policy will be ignored.
Device opened successfully!
AUTO_SUSPEND  is currently disabled.
Set AUTO_SUSPEND = On.
Set SUSPEND_DELAY = 10000 ms

RUN #3
Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
[Note] If the DeviceIdleEnabled policy is not set in the .inf file when
       the device is installed, the AUTO_SUSPEND policy will be ignored.
Device opened successfully!
AUTO_SUSPEND  is currently enabled.
SUSPEND_DELAY is set to 10000 ms
Set AUTO_SUSPEND = Off.

RUN #4
Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
[Note] If the DeviceIdleEnabled policy is not set in the .inf file when
       the device is installed, the AUTO_SUSPEND policy will be ignored.
Device opened successfully!
AUTO_SUSPEND  is currently disabled.
Set AUTO_SUSPEND = On.
Set SUSPEND_DELAY = 100 ms
*/
