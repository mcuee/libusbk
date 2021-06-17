/* "libusbK.lib" version header. ++ auto-generated
*/
#ifndef VERSION


#ifndef DEFINE_TO_STR
#define _DEFINE_TO_STR(x) #x
#define  DEFINE_TO_STR(x) _DEFINE_TO_STR(x)
#endif

#ifndef DEFINE_TO_STRW
#define _DEFINE_TO_STRW(x) L#x
#define  DEFINE_TO_STRW(x) _DEFINE_TO_STRW(x)
#endif

#define VERSION_MAJOR 3
#define VERSION_MINOR 0
#define VERSION_MICRO 8
#define VERSION_NANO 0
#define VERSION_DATE 06/17/2021
#define RC_FILENAME_STR "libusbK.lib"

#define RC_VERSION VERSION_MAJOR,VERSION_MINOR,VERSION_MICRO,VERSION_NANO
#define VERSION VERSION_MAJOR.VERSION_MINOR.VERSION_MICRO.VERSION_NANO
#define RC_VERSION_STR DEFINE_TO_STR(VERSION)
#define VERSION_DATE_STR DEFINE_TO_STR(VERSION_DATE)

#endif
