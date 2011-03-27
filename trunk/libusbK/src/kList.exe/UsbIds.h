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
	Buffer[FindUsbIdsContext.Found.MatchLength]='\0';									\
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

#endif
