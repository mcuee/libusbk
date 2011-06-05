#include "examples.h"

// Device configuration:
#define EP_WRITE			0x01
#define EP_READ				0x81
#define EP_MAX_PACKET_SIZE	32

DWORD __cdecl main(int argc, char* argv[])
{
	PKUSB_DEV_LIST deviceList = NULL;
	PKUSB_DEV_INFO deviceInfo = NULL;
	LIBUSBK_INTERFACE_HANDLE handle = NULL;

	POVERLAPPED_K ovOut, ovIn;
	ULONG lengthOut, lengthIn;
	DWORD ec, ecIn, ecOut;
	UCHAR bufferOut[EP_MAX_PACKET_SIZE];
	UCHAR bufferIn[EP_MAX_PACKET_SIZE];

	// init vars
	ec = ecIn = ecOut		= ERROR_SUCCESS;
	lengthOut = lengthIn	= 0;
	ovIn = ovOut			= NULL;

	// Find the test device using arguments supplied on the command line.
	// (default is: vid=04D8 pid=FA2E)
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	// Open the device
	if (!UsbK_Open(deviceInfo, &handle))
	{
		ec = GetLastError();
		printf("Open device failed. Win32Error=%u (0x%08X)\n", ec, ec);
		goto Done;
	}
	printf("Device opened successfully!\n");

	ovOut = OvlK_Acquire(0);
	ovIn  = OvlK_Acquire(0);

	UsbK_WritePipe(handle, EP_WRITE, bufferOut, sizeof(bufferOut), NULL, ovOut);
	UsbK_ReadPipe (handle, EP_READ, bufferIn, sizeof(bufferIn), NULL, ovIn);

	if (!OvlK_Wait(ovIn, 100, WAIT_FLAGS_CANCEL_ON_TIMEOUT,  &lengthIn))
		ecIn = GetLastError();

	if (!OvlK_Wait(ovOut, 0, WAIT_FLAGS_CANCEL_ON_TIMEOUT, &lengthOut))
		ecOut = GetLastError();

	if (ecOut == ERROR_SUCCESS)
		printf("%d bytes sent successfully!\n", lengthOut);
	else
		printf("Write failed. Win32Error=%u (0x%08X)\n", ecOut, ecOut);

	if (ecIn == ERROR_SUCCESS)
		printf("%d bytes received successfully!\n", lengthIn);
	else
		printf("Read failed. Win32Error=%u (0x%08X)\n", ecIn, ecIn);

Done:
	OvlK_Release(ovOut);
	OvlK_Release(ovIn);

	// Close the device handle
	// If handle is invalid (NULL), has no effect
	UsbK_Close(handle);

	// Free the device list
	// If deviceList is invalid (NULL), has no effect
	LstK_Free(&deviceList);

	return ec | ecIn | ecOut;
}
