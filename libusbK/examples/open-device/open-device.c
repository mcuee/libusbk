#include "examples.h"

DWORD __cdecl main(int argc, char* argv[])
{
	PKUSB_DEV_LIST deviceList = NULL;
	PKUSB_DEV_INFO deviceInfo = NULL;
	LIBUSBK_INTERFACE_HANDLE handle = NULL;
	DWORD ec = ERROR_SUCCESS;
	UCHAR pipeIndex = 0;
	WINUSB_PIPE_INFORMATION pipeInfo;

	// Find the test device.  Uses "vid/pid=hhhh" arguments supplied
	// on the command line. (default is: vid=04D8 pid=FA2E)
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

	// while the device is opened, query information on the endpoints
	// of the first alternate setting of the current interface.
	printf("Pipe Information:\n");
	while (UsbK_QueryPipe(handle, 0, pipeIndex++, &pipeInfo))
	{
		printf("\tPipeId=0x%02X PipeType=0x%02X Interval=%u MaximumPacketSize=%u\n",
		       pipeInfo.PipeId, pipeInfo.PipeType, pipeInfo.Interval, pipeInfo.MaximumPacketSize);
	}

Done:
	// Close the device handle
	// if handle is invalid (NULL), has no effect
	UsbK_Close(handle);

	// Free the device list
	// if deviceList is invalid (NULL), has no effect
	LstK_Free(&deviceList);

	return ec;
}
