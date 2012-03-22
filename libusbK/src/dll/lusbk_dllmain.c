/*!********************************************************************
libusbK - Multi-driver USB library.
Copyright (C) 2012 Travis Lee Robinson. All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Lee Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen         (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/

#include "lusbk_private.h"
#include "lusbk_handles.h"

#if defined(DYNAMIC_DLL)

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

/* DLL main entry point */
BOOL WINAPI DllMain(HANDLE module, DWORD reason, LPVOID reserved)
{
	UNREFERENCED_PARAMETER(module);
	UNREFERENCED_PARAMETER(reserved);

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		if (!CheckLibInit())
		{
			return FALSE;
		}
		break;
	case DLL_PROCESS_DETACH:
		if (AllK)
		{
#ifdef DEBUG_LOGGING_ENABLED
			POOLHANDLE_LIB_EXIT_CHECK(HotK);
			POOLHANDLE_LIB_EXIT_CHECK(LstK);
			POOLHANDLE_LIB_EXIT_CHECK(LstInfoK);
			POOLHANDLE_LIB_EXIT_CHECK(UsbK);
			POOLHANDLE_LIB_EXIT_CHECK(DevK);
			POOLHANDLE_LIB_EXIT_CHECK(OvlK);
			POOLHANDLE_LIB_EXIT_CHECK(OvlPoolK);
			POOLHANDLE_LIB_EXIT_CHECK(StmK);
#endif
			if (AllK->ProcessHeap == AllK->Heap)
				HeapFree(AllK->Heap, 0, AllK);
			else
				HeapDestroy(AllK->Heap);

			AllK = NULL;
		}
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}
#endif
