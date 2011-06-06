#include "examples.h"

DWORD __cdecl main(int argc, char* argv[])
{
	PKUSB_DEV_LIST deviceList = NULL;
	PKUSB_DEV_INFO deviceInfo = NULL;
	LIBUSBK_INTERFACE_HANDLE handle = NULL;
	DWORD errorCode = ERROR_SUCCESS;
	KUSB_DRIVER_API K;

	// Find the test device.  Uses "vid/pid=hhhh" arguments supplied
	// on the command line. (default is: vid=04D8 pid=FA2E)
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	// load a dynamic driver api for this device.  The dynamic driver api
	// is more versatile because it adds support for winusb.sys devices.
	if (!DrvK_LoadDriverApi(&K, deviceInfo->DrvId, sizeof(K)))
	{
		errorCode = GetLastError();
		printf("Loading driver api failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}

	// Display some information on the driver api type.
	switch(deviceInfo->DrvId)
	{
	case KUSB_DRVID_LIBUSBK:
		printf("libusbK driver api loaded!\n");
		break;
	case KUSB_DRVID_LIBUSB0:
		printf("libusb0 driver api loaded!\n");
		break;
	case KUSB_DRVID_WINUSB:
		printf("WinUSB driver api loaded!\n");
		break;
	case KUSB_DRVID_LIBUSB0_FILTER:
		printf("libusb0/filter driver api loaded!\n");
		break;
	}

	/*
	From this point forth, do not use the exported "UsbK_" functions. Instead,
	use the functions in the driver api initialized above.
	*/

	// Open the device with the "dynamic" Open function
	if (!K.Open(deviceInfo, &handle))
	{
		errorCode = GetLastError();
		printf("Open device failed. Win32Error=%u (0x%08X)\n", errorCode, errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

Done:
	// Close the device handle
	// if handle is invalid (NULL), has no effect
	UsbK_Close(handle);

	// Free the device list
	// if deviceList is invalid (NULL), has no effect
	LstK_Free(&deviceList);

	return errorCode;
}

/*
Console Output:

Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
libusbK driver api loaded!
Device opened successfully!
*/