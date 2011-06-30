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

VOID KUSB_API OnHotPlug(
    __in KHOT_HANDLE Handle,
    __in PKHOT_PARAMS Params,
    __in PCKLST_DEV_INFO DeviceInfo,
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
	    NotificationType == SYNC_FLAG_ADDED ? "ARRIVAL" : "REMOVAL",
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

	memset(&hotParams, 0, sizeof(hotParams));
	hotParams.OnHotPlug = OnHotPlug;
	hotParams.Flags.PlugAllOnInit = 1;
	hotParams.Flags.AllowDupeInstanceIDs = 1;
	strcpy(hotParams.PatternMatch.InstanceID, "*");

	printf("Initialize a HotK device notification event monitor..\n");
	printf("Looking for devices with instances IDs matching the pattern '%s'..\n", hotParams.PatternMatch.InstanceID);
	printf("Press 'q' to exit..\n\n");

	if (!HotK_Init(&hotHandle, &hotParams))
	{
		errorCode = GetLastError();
		printf("HotK_Init failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}

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
		SwitchToThread();
	}

	if (!HotK_Free(&hotHandle))
	{
		errorCode = GetLastError();
		printf("HotK_Free failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}

Done:
	return errorCode;
}
