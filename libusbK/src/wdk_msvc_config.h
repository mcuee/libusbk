/*! \file wdk_mscv_config.h
*/
#ifndef __WDK_MSVC_CONFG_H_
#define __WDK_MSVC_CONFG_H_

//// WDK MSVC CONFIGURATION.  Set these according to your installation. ////
#define WDK_INSTALLATIONS_FOLDER	"Z:\\WinDDK"
#define WDK_MSVC_USE_VERSION		"7600.16385.1"
/////////////////////////////// END OF SETUP ///////////////////////////////

// WDK base folder. eg: Z:\WinDDK\7600.16385.1
#define WDK_BASE_FOLDER WDK_INSTALLATIONS_FOLDER "\\" WDK_MSVC_USE_VERSION

// WDK library folder. eg: Z:\WinDDK\7600.16385.1\lib
#define WDK_LIB_FOLDER WDK_BASE_FOLDER "\\lib"

#define _MSC_VER_MIN	1400
#define _MSC_VER_MAX	1600

#if defined(_MSC_VER) && (_MSC_VER < _MSC_VER_MIN || _MSC_VER > _MSC_VER_MAX)
	#pragma message( "Warning: This MS compiler is not tested" )
	#undef _MSC_VER_OK	
#else
	#define _MSC_VER_OK	
#endif // _MSC_VER

#endif
