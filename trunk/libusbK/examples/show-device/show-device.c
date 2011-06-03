#include "examples.h"

// This function is called by the LstK_Enumerate function for each
// device until it returns FALSE.
static BOOL KUSB_API ShowDevicesCB(PKUSB_DEV_LIST DeviceList,
                                   PKUSB_DEV_INFO deviceInfo,
                                   PVOID MyContext)
{
	// print some information about the device.
	printf("%s: %s (%s)\n",
	       deviceInfo->DeviceInstance,
	       deviceInfo->DeviceDesc,
	       deviceInfo->Mfg);

	// If this function returns FALSE then enumeration ceases.
	return TRUE;
}

DWORD __cdecl main(int argc, char* argv[])
{
	PKUSB_DEV_LIST deviceList = NULL;
	PKUSB_DEV_INFO deviceInfo = NULL;
	DWORD errorCode = ERROR_SUCCESS;

	// Initialize a new device list.  This populates the list
	// with the usb devices libusbK can use.
	if (!LstK_Init(&deviceList, NULL))
	{
		errorCode = GetLastError();
		printf("An error occured getting the device list. errorCode=%08Xh\n", errorCode);
		return errorCode;
	}

	// If DeviceCount = 0, no supported devices were found.
	if (!deviceList->DeviceCount)
	{
		printf("No devices connected.\n");

		// Always free the device list if LstK_Init returns TRUE
		LstK_Free(&deviceList);

		return ERROR_DEVICE_NOT_CONNECTED;
	}

	// Show all devices using the enumerate function.
	LstK_Enumerate(deviceList, ShowDevicesCB, NULL);

	// You can also use the the reset, next and current functions for the same tasks.

	// Reset the device list position.
	LstK_Reset(deviceList);

	// Always start with a call LstK_Next after LstK_Reset to advance to the first element.
	while(LstK_Next(deviceList, &deviceInfo))
	{
		// NOTE: LstK_Next will only return FALSE once;  If it is called after a FALSE
		//       return, it behaves as if it were reset and starts from the beginning.

		if (Match_DeviceID(deviceInfo, EXAMPLE_HWID) == 0)
			break;
	}

	// Report the connection state of the example device
	if (deviceInfo)
	{
		printf("Example device connected!\n");
	}
	else
	{
		printf("Example device not found.\n");
		errorCode = ERROR_NO_MATCH;
	}

	// Always free the device list if LstK_Init returns TRUE
	LstK_Free(&deviceList);

	// return the win32 error code.
	return errorCode;
}
