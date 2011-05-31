#include "examples.h"

DWORD Examples_GetTestDevice(__deref_out PKUSB_DEV_LIST* headDeviceList,
                             __deref_out PKUSB_DEV_LIST* foundDevice,
                             __in int argc,
                             __in char* argv[])
{
	PKUSB_DEV_LIST headEL = NULL;
	PKUSB_DEV_LIST itemEL = NULL;
	LONG ret;
	ULONG vidArg = EXAMPLE_VID;
	ULONG pidArg = EXAMPLE_PID;
	int argPos;
	char exampleDeviceID[24];

	// init
	*foundDevice = NULL;

	// Get the test device vid/pid from the command line (if specified)
	for (argPos = 1; argPos < argc; argPos++)
	{
		sscanf(argv[argPos], "vid=%04x", &vidArg);
		sscanf(argv[argPos], "pid=%04x", &pidArg);
	}

	// Get the device list
	ret = LstK_GetDeviceList(headDeviceList, NULL);

	// If ret == 0 then no supported devices were found.
	if (ret < 1)
	{
		printf("No device not connected.\n");
		// If ret < 0 is is a negative win32 error code.
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	headEL = *headDeviceList;

	memset(exampleDeviceID, 0, sizeof(exampleDeviceID));
	sprintf(exampleDeviceID, EXAMPLES_DEVICE_HWID_FORMAT, vidArg, pidArg);
	printf("Looking for device %s..\n", exampleDeviceID);

	// Search for the example device hardware id using the DL_SEARCH macro.
	// This will call Match_DeviceID() for each device element until it returns 0.
	DL_SEARCH(headEL, itemEL, exampleDeviceID, Match_DeviceID);

	// Report the connection state of the example device
	if (itemEL)
	{
		printf("Using device %s: %s (%s)\n",
		       itemEL->DeviceInstance,
		       itemEL->DeviceDesc,
		       itemEL->Mfg);

		*foundDevice = itemEL;
		return ERROR_SUCCESS;
	}
	else
	{
		printf("Device not found.\n\n");
		printf("USAGE: program.exe vid=hhhh pid=hhhh\n");
		printf("       e.g. vid=04D8 pid=FA2E\n\n");

		// Free the device list
		LstK_FreeDeviceList(headDeviceList);

		return ERROR_NO_MATCH;
	}
}
