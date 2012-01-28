/*! \file wdk_app_config.h
*/

#include <windows.h>
#include "wdk_msvc_config.h"

#if defined(_MSC_VER)

#if defined(_WIN32) && !defined(_DEBUG)
///////////////////////////////////////////////////////////////////////////////
// Important: LNK1104 ERROR
// If your getting a LNK1104 error it is because WDK is not installed or
// the paths are incorrect (see wdk_msvc_config.h).  Removing this block of
// code will resolve the problem but the module will then link through the
// Microsoft visual c runtime (msvcr) which is not available on all systems
// without pre-installation. IE:
// http://www.microsoft.com/download/en/details.aspx?id=5555
//
// 1) DOWNLOAD AND INSTALL WDK TO SOLVE THIS PROBLEM
// How to get WDK? (http://msdn.microsoft.com/en-us/windows/hardware/gg487463)
// 2) Update wdk_msvc_config.h accordingly.
///////////////////////////////////////////////////////////////////////////////
#	if !defined(_WIN64) && defined(_M_IX86)
#		pragma comment( lib, WDK_LIB_FOLDER"\\crt\\i386\\msvcrt.lib" )
#	elif defined(_WIN64) && defined(_M_X64)
#		pragma comment( lib, WDK_LIB_FOLDER"\\crt\\amd64\\msvcrt.lib" )
#	endif
#endif

#endif
