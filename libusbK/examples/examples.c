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
BOOL Examples_GetTestDevice(KLST_HANDLE* DeviceList,
                            KLST_DEVINFO_HANDLE* DeviceInfo,
                            int argc,
                            char* argv[])
{
	return Examples_GetTestDeviceEx(DeviceList,
	                                DeviceInfo,
	                                argc,
	                                argv,
	                                0);

}

BOOL Examples_GetArgVal(int argc, char* argv[], LPCSTR argName, PUINT argValue, BOOL isHex)
{
	int argPos;
	BOOL useNextArg = FALSE;
	for (argPos = 1; argPos < argc; argPos++)
	{
		CHAR buf[128];
		UINT i;

		const UINT len = strlen(argv[argPos]);
		for (i = 0; i < len && i < _countof(buf) - 1; i++)
			buf[i] = (char) tolower(argv[argPos][i]);
		buf[i] = 0;
		
		if (useNextArg)
		{
			PCHAR pBuf = buf;
			if (pBuf[0] == '\"') pBuf++;
			if (pBuf[0] == '0' && (pBuf[1] == 'x'))
			{
				pBuf += 2;
				isHex = TRUE;
			}

			*argValue = strtoul(pBuf, NULL, (isHex ? 16 : 10));
			return TRUE;
		}

		char* pFound = strstr(buf, argName);
		if (pFound)
		{
			pFound += strlen(argName);
			if (pFound[0] == '=') pFound++;
			if (pFound[0]=='\0')
			{
				useNextArg = TRUE;
				continue;
			}
			if (pFound[0] == '0' && (pFound[1] == 'x'))
			{
				pFound += 2;
				isHex = TRUE;
			}
			*argValue = strtoul(pFound, NULL, (isHex?16:10));
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Examples_GetArgStr(int argc, char* argv[], LPCSTR argName, LPSTR argValue, PUINT argValLength)
{
	int argPos;
	BOOL useNextArg = FALSE;
	for (argPos = 1; argPos < argc; argPos++)
	{
		CHAR buf[128];
		UINT i;

		const UINT len = strlen(argv[argPos]);
		for (i = 0; i < len && i < _countof(buf) - 1; i++)
			buf[i] = (char)tolower(argv[argPos][i]);
		buf[i] = 0;

		if (useNextArg)
		{
			PCHAR pBuf = buf;
			*argValLength = min(strlen(buf), *argValLength);
			if (buf[0]=='\"' && buf[strlen(buf)-1]=='\"')
			{
				pBuf++;
				*argValLength = *argValLength-2;
				
			}
			strncpy(argValue, pBuf, *argValLength);
			argValue[*argValLength] = '\0';
			return TRUE;
		}
		char* pFound = strstr(buf, argName);
		if (pFound)
		{
			pFound += strlen(argName);
			
			if (pFound[0] == '=') pFound++;
			
			if (pFound[0] == '\0')
			{
				useNextArg = TRUE;
				continue;
			}
			
			*argValLength = min(strlen(pFound), *argValLength);
			if (pFound[0] == '\"' && pFound[strlen(pFound)-1]=='\"')
			{
				pFound++;
				*argValLength = *argValLength - 2;
			}
			strncpy(argValue, pFound, *argValLength);
			argValue[*argValLength] = '\0';
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Examples_GetTestDeviceEx(KLST_HANDLE* DeviceList,
                              KLST_DEVINFO_HANDLE* DeviceInfo,
                              int argc,
                              char* argv[],
                              KLST_FLAG Flags)
{
	UINT vidArg = EXAMPLE_VID;
	UINT pidArg = EXAMPLE_PID;
	ULONG deviceCount = 0;
	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;

	// init
	*DeviceList = NULL;
	*DeviceInfo = NULL;

	// Get the test device vid/pid from the command line (if specified)
	Examples_GetArgVal(argc, argv, "vid=", &vidArg, TRUE);
	Examples_GetArgVal(argc, argv, "pid=", &pidArg, TRUE);

	// Get the device list
	if (!LstK_Init(&deviceList, Flags))
	{
		printf("Error initializing device list.\n");
		return FALSE;
	}

	LstK_Count(deviceList, &deviceCount);
	if (!deviceCount)
	{
		printf("Device list empty.\n");
		SetLastError(ERROR_DEVICE_NOT_CONNECTED);

		// If LstK_Init returns TRUE, the list must be freed.
		LstK_Free(deviceList);

		return FALSE;
	}

	printf("Looking for device vid/pid %04X/%04X..\n", vidArg, pidArg);

	LstK_FindByVidPid(deviceList, vidArg, pidArg, &deviceInfo);

	if (deviceInfo)
	{
		// This function returns the device list and the device info
		// element which matched.  The caller is responsible for freeing
		// this list when it is no longer needed.
		*DeviceList = deviceList;
		*DeviceInfo = deviceInfo;

		// Report the connection state of the example device
		printf("Using %04X:%04X (%s): %s - %s\n",
		       deviceInfo->Common.Vid,
		       deviceInfo->Common.Pid,
		       deviceInfo->Common.InstanceID,
		       deviceInfo->DeviceDesc,
		       deviceInfo->Mfg);

		return TRUE;
	}
	else
	{
		// Display some simple usage information for the example applications.
		CHAR programPath[MAX_PATH] = {0};
		PCHAR programExe = programPath;
		GetModuleFileNameA(GetModuleHandleA(NULL), programPath, sizeof(programPath));
		while(strpbrk(programExe, "\\/")) programExe = strpbrk(programExe, "\\/") + 1;
		printf("Device vid/pid %04X/%04X not found.\n\n", vidArg, pidArg);
		printf("USAGE: %s vid=%04X pid=%04X\n\n", programExe, vidArg, pidArg);

		// If LstK_Init returns TRUE, the list must be freed.
		LstK_Free(deviceList);

		return FALSE;
	}
}

BOOL Bench_Configure(KUSB_HANDLE UsbHandle,
                     BM_COMMAND Command,
                     UCHAR InterfaceNumber,
                     PKUSB_DRIVER_API DriverAPI,
                     PBM_TEST_TYPE TestType)
{
#if (EXAMPLES_USE_BENCHMARK_CONFIGURE==1)
	DWORD transferred = 0;
	BOOL success;
	WINUSB_SETUP_PACKET Pkt;
	KUSB_SETUP_PACKET* defPkt = (KUSB_SETUP_PACKET*)&Pkt;

	memset(&Pkt, 0, sizeof(Pkt));
	defPkt->BmRequest.Dir	= BMREQUEST_DIR_DEVICE_TO_HOST;
	defPkt->BmRequest.Type	= BMREQUEST_TYPE_VENDOR;
	defPkt->Request			= (UCHAR) Command;
	defPkt->Value			= (UCHAR) * TestType;
	defPkt->Index			= InterfaceNumber;
	defPkt->Length			= 1;

	if (DriverAPI)
		success = DriverAPI->ControlTransfer(UsbHandle, Pkt, (PUCHAR)TestType, 1, &transferred, NULL);
	else
		success = UsbK_ControlTransfer(UsbHandle, Pkt, (PUCHAR)TestType, 1, &transferred, NULL);

	return success;
#else
	UNREFERENCED_PARAMETER(UsbHandle);
	UNREFERENCED_PARAMETER(Command);
	UNREFERENCED_PARAMETER(InterfaceNumber);
	UNREFERENCED_PARAMETER(DriverAPI);
	UNREFERENCED_PARAMETER(TestType);

	return TRUE;
#endif
}
