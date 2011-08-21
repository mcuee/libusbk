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

#ifndef UNIT_TEST_LOOP_COUNT
#define UNIT_TEST_LOOP_COUNT 1
static int gLoop = 0;
#else
extern int gLoop;
#endif

VOID KUSB_API OnHotPlug(
    __in KHOT_HANDLE Handle,
    __in KHOT_PARAMS* Params,
    __in KLST_DEVINFO_HANDLE DeviceInfo,
    __in KLST_SYNC_FLAG NotificationType)
{
	UNREFERENCED_PARAMETER(Handle);

	printf(
	    "\n"
	    "[%s] %s (%s) [%s]\n"
	    "  InstanceID          : %s\n"
	    "  DeviceInterfaceGUID : %s\n"
	    "  DevicePath          : %s\n"
	    "  \n",
	    NotificationType == KLST_SYNC_FLAG_ADDED ? "ARRIVAL" : "REMOVAL",
	    DeviceInfo->DeviceDesc,
	    DeviceInfo->Mfg,
	    DeviceInfo->Service,
	    DeviceInfo->InstanceID,
	    DeviceInfo->DeviceInterfaceGUID,
	    DeviceInfo->DevicePath);
}

DWORD __cdecl main(int argc, char* argv[])
{
	DWORD errorCode = ERROR_SUCCESS;
	KHOT_HANDLE hotHandle = NULL;
	KHOT_PARAMS hotParams;
	CHAR chKey;

	for (gLoop = 0; gLoop < UNIT_TEST_LOOP_COUNT; gLoop++)
	{
		memset(&hotParams, 0, sizeof(hotParams));
		hotParams.OnHotPlug = OnHotPlug;
		hotParams.Flags |= KHOT_FLAG_PLUG_ALL_ON_INIT;
		strcpy(hotParams.PatternMatch.InstanceID, "*");

		printf("Initialize a HotK device notification event monitor..\n");
		printf("Looking for devices with instances IDs matching the pattern '%s'..\n", hotParams.PatternMatch.InstanceID);
		printf("Press 'q' to exit..\n\n");
		if (!HotK_Init(&hotHandle, &hotParams))
		{
			errorCode = GetLastError();
			printf("HotK_Init failed. ErrorCode: %08Xh\n",  errorCode);
			goto Done;
		}

		printf("HotK monitor initialized. ErrorCode: %08Xh\n",  errorCode);

		for(;;)
		{
			if (_kbhit())
			{
				chKey = _getch();
				if (chKey == 'q' || chKey == 'Q')
					break;

				chKey = '\0';
				continue;
			}

			Sleep(100);
		}

		if (!HotK_Free(hotHandle))
		{
			errorCode = GetLastError();
			printf("HotK_Free failed. ErrorCode: %08Xh\n",  errorCode);
			goto Done;
		}

		printf("HotK monitor closed. ErrorCode: %08Xh\n",  errorCode);

	}

Done:
	return errorCode;
}

/*
Initialize a HotK device notification event monitor..
Looking for devices with instances IDs matching the pattern '*'..
Press 'q' to exit..

HotK monitor initialized. ErrorCode: 00000000h

[ARRIVAL] Benchmark Device (Microchip Technology, Inc.) [libusbK]
  InstanceID          : USB\VID_04D8&PID_FA2E\LUSBW1
  DeviceInterfaceGUID : {716cdf1f-418b-4b80-a07d-1311dffdc8b8}
  DevicePath          : \\?\USB#VID_04D8&PID_FA2E#LUSBW1#{716cdf1f-418b-4b80-a07d-1311dffdc8b8}


[REMOVAL] Benchmark Device (Microchip Technology, Inc.) [libusbK]
  InstanceID          : USB\VID_04D8&PID_FA2E\LUSBW1
  DeviceInterfaceGUID : {716cdf1f-418b-4b80-a07d-1311dffdc8b8}
  DevicePath          : \\?\USB#VID_04D8&PID_FA2E#LUSBW1#{716cdf1f-418b-4b80-a07d-1311dffdc8b8}

HotK monitor closed. ErrorCode: 00000000h
*/