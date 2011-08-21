/*!
#
# Copyright (c) 2011 Travis Robinson <libusbdotnet@gmail.com>
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

// This function is called by the LstK_Enumerate function for each
// device until it returns FALSE.
static BOOL KUSB_API ShowDevicesCB(KLST_HANDLE DeviceList,
                                   KLST_DEVINFO_HANDLE deviceInfo,
                                   PVOID MyContext)
{
	// print some information about the device.
	printf("%04X:%04X (%s): %s - %s\n",
	       deviceInfo->Common.Vid,
	       deviceInfo->Common.Pid,
	       deviceInfo->Common.InstanceID,
	       deviceInfo->DeviceDesc,
	       deviceInfo->Mfg);

	// If this function returns FALSE then enumeration ceases.
	return TRUE;
}

DWORD __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	DWORD errorCode = ERROR_SUCCESS;
	ULONG count = 0;

	// Initialize a new device list.  This populates the list with the
	// usb devices libusbK can access.
	if (!LstK_Init(&deviceList, 0))
	{
		errorCode = GetLastError();
		printf("An error occured getting the device list. errorCode=%08Xh\n", errorCode);
		return errorCode;
	}

	LstK_Count(deviceList, &count);
	if (!count)
	{
		printf("No devices connected.\n");

		// Always free the device list if LstK_Init returns TRUE
		LstK_Free(deviceList);

		return ERROR_DEVICE_NOT_CONNECTED;
	}

	/*
	There are three (3) ways to search the device list:
	- #1 LstK_FindByVidPid
	- #2 LstK_MoveReset, LstK_MoveNext, and LstK_Current
	- #3 LstK_Enumerate
	*/

	// #1
	// Simple means of locating the fist device matching a vid/pid.
	//
	if (LstK_FindByVidPid(deviceList, EXAMPLE_VID, EXAMPLE_PID, &deviceInfo))
		printf("LstK_FindByVidPid: Example device connected!\n");
	else
		printf("Example device not found.\n");

	// #2
	// Enumerates the device list using it's internal "current" position.
	//
	// Reset the device list position.
	LstK_MoveReset(deviceList);
	//
	errorCode = ERROR_NO_MATCH;
	//
	// Call LstK_MoveNext after a LstK_MoveReset to advance to the first
	// element.
	while(LstK_MoveNext(deviceList, &deviceInfo))
	{
		if (deviceInfo->Common.Vid == EXAMPLE_VID &&
		        deviceInfo->Common.Pid == EXAMPLE_PID)
		{
			errorCode = ERROR_SUCCESS;
			break;
		}
	}
	//
	// Report the connection state
	if (deviceInfo)
		printf("LstK_MoveNext: Example device connected!\n");
	else
		printf("Example device not found.\n");

	// #3
	// Enumerates the device list using the user supplied callback
	// function, ShowDevicesCB(). LstK_Enumerate calls this function for
	// each device info element until ShowDevicesCB(0 returns FALSE.
	//
	// Show all devices using the enumerate function.
	LstK_Enumerate(deviceList, ShowDevicesCB, NULL);

	// Free the device list
	LstK_Free(deviceList);

	// return the win32 error code.
	return errorCode;
}
