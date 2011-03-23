#include "lusbk_private.h"

#if defined(DYNAMIC_DLL)

/* DLL main entry point */
BOOL WINAPI DllMain(HANDLE module, DWORD reason, LPVOID reserved)
{
	UNREFERENCED_PARAMETER(module);
	UNREFERENCED_PARAMETER(reserved);

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
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
