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

DWORD __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE handle = NULL;
	DWORD ec = ERROR_SUCCESS;
	UCHAR pipeIndex = 0;
	WINUSB_PIPE_INFORMATION pipeInfo;

	// Find the test device.  Uses "vid/pid=hhhh" arguments supplied
	// on the command line. (default is: vid=04D8 pid=FA2E)
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	// Open the device
	if (!UsbK_Open(deviceInfo, &handle))
	{
		ec = GetLastError();
		printf("Open device failed. Win32Error=%u (0x%08X)\n", ec, ec);
		goto Done;
	}
	printf("Device opened successfully!\n");

	// while the device is opened, query information on the endpoints
	// of the first alternate setting of the current interface.
	printf("Pipe Information:\n");
	while (UsbK_QueryPipe(handle, 0, pipeIndex++, &pipeInfo))
	{
		printf("  PipeId=0x%02X PipeType=0x%02X Interval=%u MaximumPacketSize=%u\n",
		       pipeInfo.PipeId, pipeInfo.PipeType, pipeInfo.Interval, pipeInfo.MaximumPacketSize);
	}

Done:
	// Close the device handle
	// if handle is invalid (NULL), has no effect
	UsbK_Close(handle);

	// Free the device list
	// if deviceList is invalid (NULL), has no effect
	LstK_Free(deviceList);

	return ec;
}

/*
Console Output:
  Looking for device vid/pid 04D8/FA2E..
  Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
  Device opened successfully!
  Pipe Information:
    PipeId=0x01 PipeType=0x02 Interval=0 MaximumPacketSize=32
    PipeId=0x81 PipeType=0x02 Interval=0 MaximumPacketSize=32
*/