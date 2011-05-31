#include "examples.h"

int __cdecl main(int argc, char* argv[])
{
	PKUSB_DEV_LIST headEL = NULL;
	PKUSB_DEV_LIST itemEL = NULL;
	LONG ret;

	// Get the device list
	ret = LstK_GetDeviceList(&headEL, NULL);

	// If ret == 0 then no supported devices were found.
	if (!ret) return -ERROR_DEVICE_NOT_CONNECTED;

	// If LstK_GetDeviceList returns a negative value,
	// it represents a negative win32 error code.
	if (ret < 0) return ret;

	// Print some basic information for each device element
	DL_FOREACH(headEL, itemEL)
	{
		printf("%s: %s (%s)\n",
		       itemEL->DeviceInstance,
		       itemEL->DeviceDesc,
		       itemEL->Mfg);
	}

	// Search for the example device hardware id using
	// the custom search macro, Match_DeviceID(), defined in example.h.
	DL_SEARCH(headEL, itemEL, EXAMPLE_HWID, Match_DeviceID);

	// Report the connection state of the example device
	if (itemEL)
	{
		printf("Example device connected!\n");
	}
	else
	{
		printf("Example device not found.\n");
		ret = -ERROR_NO_MATCH;
	}

	// Free the device list
	LstK_FreeDeviceList(&headEL);

	// return 0 if the example device is connected.
	return itemEL ? 0 : ret;
}
