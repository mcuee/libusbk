#include "examples.h"

BOOL Examples_GetTestDevice( __deref_out PKUSB_DEV_LIST* DeviceList,
                             __deref_out PKUSB_DEV_INFO* DeviceInfo,
                             __in int argc,
                             __in char* argv[])
{
	ULONG vidArg = EXAMPLE_VID;
	ULONG pidArg = EXAMPLE_PID;
	int argPos;
	PKUSB_DEV_LIST deviceList = NULL;
	PKUSB_DEV_INFO deviceInfo = NULL;

	// init
	*DeviceList = NULL;
	*DeviceInfo = NULL;

	// Get the test device vid/pid from the command line (if specified)
	for (argPos = 1; argPos < argc; argPos++)
	{
		sscanf(argv[argPos], "vid=%04x", &vidArg);
		sscanf(argv[argPos], "pid=%04x", &pidArg);
	}

	// Get the device list
	if (!LstK_Init(&deviceList, NULL))
	{
		printf("Error initializing device list.\n");
		return FALSE;
	}
	if (!deviceList->DeviceCount)
	{
		printf("No device not connected.\n");
		SetLastError(ERROR_DEVICE_NOT_CONNECTED);

		// If LstK_Init returns TRUE, the list must be freed.
		LstK_Free(&deviceList);

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
		LstK_Free(&deviceList);

		return FALSE;
	}
}
