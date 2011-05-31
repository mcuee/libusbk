#include "examples.h"

int __cdecl main(int argc, char* argv[])
{
	PKUSB_DEV_LIST headEL = NULL;
	PKUSB_DEV_LIST itemEL = NULL;
	LIBUSBK_INTERFACE_HANDLE handle = NULL;
	DWORD ec = ERROR_SUCCESS;

	// Find the test device.  Uses "vid/pid=hhhh" arguments supplied
	// on the command line. (default is: vid=04D8 pid=FA2E)
	ec = Examples_GetTestDevice(&headEL, &itemEL, argc, argv);
	if (ec != ERROR_SUCCESS) return ec;

	// Open the device
	if (!UsbK_Open(itemEL, &handle))
	{
		ec = GetLastError();
		printf("Open device failed. Win32Error=%u (0x%X)\n", ec, ec);
		goto Done;
	}
	printf("Device opened successfully!\n");

Done:
	// Close the device handle
	// (if handle is invalid, has no effect)
	UsbK_Close(handle);

	// Free the device list
	// (if headEL is invalid, has no effect)
	LstK_FreeDeviceList(&headEL);

	return (int)ec;
}
