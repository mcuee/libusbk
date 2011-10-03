========================================================================
    MAKEFILE PROJECT : libusb-win32 KMDF Driver Project Overview
========================================================================

REQUIREMENTS:
	* Windows Driver Kit (WDK) 6001.18002 or greater

PAGED_CODE() MACRO:
http://msdn.microsoft.com/en-us/library/ff558773%28VS.85%29.aspx

The PAGED_CODE macro ensures that the calling thread is running at an 
IRQL that is low enough to permit paging. 

If the IRQL > APC_LEVEL, the PAGED_CODE macro causes the system to 
ASSERT. 

A call to this macro should be made at the beginning of every driver 
routine that either contains pageable code or accesses pageable code. 

The PAGED_CODE macro only checks IRQL at the point the code executes the 
macro. If the code subsequently raises IRQL, it will not be detected. 
Driver writers should use the driver verifier to detect when the IRQL is 
raised improperly. 

PAGED_CODE only works in checked builds.

To detect code that runs at IRQL >= DISPATCH_LEVEL, use the PAGED_CODE 
macro. In debug mode, this macro generates a message if the code runs at 
IRQL >= DISPATCH_LEVEL. Add the macro as the first statement in a 
routine to mark the whole routine as paged code. 

To make sure that you are doing this correctly, run the Driver Verifier 
against your finished driver with the Force IRQL Checking option 
enabled. This option causes the system to automatically page out all 
pageable code every time that the driver raises IRQL to DISPATCH_LEVEL 
or above. Using the Driver Verifier, you can quickly find any driver 
bugs in this area. Otherwise, these bugs will typically be found only by 
customers and they can frequently be very hard for you to reproduce. 

Regex vstudio match/replace transforms:

[exported typedef transform]
Match:
{typedef}:b+{[A-Za-z_0-9]+}:b+{KUSB_API}:b+{KUSB_}{[A-Za-z_0-9]+}.*{\(}{([^\)]|\n)+}{\);}
\1 =typedef
\2 = [TYPE]
\3 = KUSB_API
\4 = KUSB_
\5 = [FUNCTION_SHORTNAME]
\6 = (
\7 = [FUNCTION_PARAMETERS]
\8 = );

Generate .h from .c
M:
\n\{[^\n]*\n((\t[^\n]*\n)|([ \t]*\n))+\}
R:
;
