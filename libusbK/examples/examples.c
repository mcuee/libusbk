#include "examples.h"

BOOL Examples_GetTestDevice( __deref_out PKUSB_DEV_LIST* DeviceList,
                             __deref_out PKUSB_DEV_INFO* DeviceInfo,
                             __in int argc,
                             __in char* argv[])
{
	ULONG vidArg = EXAMPLE_VID;
	ULONG pidArg = EXAMPLE_PID;
	int argPos;
	char exampleDeviceID[24];
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
		LstK_Free(&deviceList);
		return FALSE;
	}

	memset(exampleDeviceID, 0, sizeof(exampleDeviceID));
	sprintf(exampleDeviceID, EXAMPLES_DEVICE_HWID_FORMAT, vidArg, pidArg);
	printf("Looking for device %s..\n", exampleDeviceID);

	LstK_Reset(deviceList);
	while(LstK_Next(deviceList, &deviceInfo))
	{
		if (Match_DeviceID(deviceInfo, exampleDeviceID) == 0)
			break;
	}

	// Report the connection state of the example device
	if (deviceInfo)
	{
		printf("Using device %s: %s (%s)\n",
		       deviceInfo->DeviceInstance,
		       deviceInfo->DeviceDesc,
		       deviceInfo->Mfg);

		*DeviceInfo = deviceInfo;
		*DeviceList = deviceList;
		return TRUE;
	}
	else
	{
		printf("Device not found.\n\n");
		printf("USAGE: program.exe vid=hhhh pid=hhhh\n");
		printf("       e.g. vid=04D8 pid=FA2E\n\n");

		// Free the device list
		LstK_Free(&deviceList);

		return FALSE;
	}
}
