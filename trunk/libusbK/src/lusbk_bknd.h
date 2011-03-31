/*! \file lusbk_bknd.h
*/

#ifndef __LUSBK_BKND_
#define __LUSBK_BKND_

#include "lusbk_private.h"

BOOL GetProcAddress_libusbk(__out KPROC* ProcAddress, __in ULONG FunctionID);
BOOL GetProcAddress_libusb0(__out KPROC* ProcAddress, __in ULONG FunctionID);
BOOL GetProcAddress_winusb(__out KPROC* ProcAddress, __in ULONG FunctionID);

// libusb0.sys backend function prototypes
//

#endif
