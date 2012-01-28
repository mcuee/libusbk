/*!********************************************************************
libusbK - kList descriptor diagnostic tool.
Copyright (C) 2011 All Rights Reserved.
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

#ifndef __USBIDS_H_
#define __USBIDS_H_

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define MATCH_END 0xffffffffUL  /* maximum unsigned long value */

extern LPCSTR UsbIdsText;

#ifndef InitUsbIdsContext
#define InitUsbIdsContext(UsbIdsContext,StartTextPtr,PredicateCallback,MatchValuePtr)	\
	{																						\
		UsbIdsContext.MatchLinePredicate = &PredicateCallback;								\
		UsbIdsContext.MatchValue = MatchValuePtr;											\
		UsbIdsContext.SourceText = StartTextPtr;											\
	}
#endif

#ifndef CopyUsbIdText
#define CopyUsbIdText(UsbIdsContext,Buffer)												\
	{																						\
		strncpy(Buffer, UsbIdsContext.Found.MatchStart, UsbIdsContext.Found.MatchLength);	\
		Buffer[UsbIdsContext.Found.MatchLength]='\0';									\
	}
#endif

typedef struct _FIND_USBIDS_CONTEXT
{

	LPCSTR SourceText;
	LPCSTR (WINAPI* MatchLinePredicate)(LPCSTR, PVOID, PULONG);
	PVOID MatchValue;
	LPCSTR NextLine;
	struct
	{
		ULONG MatchLength;
		LPCSTR MatchStart;

	} Found;
} FIND_USBIDS_CONTEXT, *PFIND_USBIDS_CONTEXT;

BOOL WINAPI EnumLines(PFIND_USBIDS_CONTEXT context);

VOID WINAPI GetHwIdDisplayText(__in USHORT VendorID,
                               __in USHORT ProductID,
                               __out PCHAR VendorName,
                               __out PCHAR ProductName);

VOID WINAPI GetClassDisplayText(__in UCHAR ClassID,
                                __in UCHAR SubClassID,
                                __in UCHAR ProtocolID,
                                __out PCHAR ClassName,
                                __out PCHAR SubClassName,
                                __out PCHAR ProtocolName);

VOID WINAPI GetHidDescriptorItemDisplayText(__in UCHAR ItemType,
        __out PCHAR ItemName);

VOID WINAPI GetHidUsagePageText(__in UCHAR UsagePage,
                                __out PCHAR UsagePageName,
                                __out PFIND_USBIDS_CONTEXT context);

VOID WINAPI GetHidUsageText(__in USHORT Usage,
                            __out PCHAR UsageName,
                            __in PFIND_USBIDS_CONTEXT startContext);

VOID WINAPI GetHidCountryCodeText(__in UCHAR CountryCode,
                                  __out PCHAR CountryName);
#endif
