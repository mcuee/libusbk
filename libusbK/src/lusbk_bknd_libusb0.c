#include "lusbk_bknd.h"

BOOL GetProcAddress_libusb0(__out KPROC* ProcAddress, __in ULONG FunctionID)
{
	return GetProcAddress_libusbk(ProcAddress, FunctionID);
}
