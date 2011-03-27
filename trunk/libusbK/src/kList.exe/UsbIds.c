#include "UsbIds.h"

LPCSTR WINAPI FindUsbClassLineCb(LPCSTR line,
                                 PVOID matchValue,
                                 PULONG lineLength);

LPCSTR WINAPI FindUsbVendorLineCb(LPCSTR line,
                                  PVOID matchValue,
                                  PULONG lineLength);

LPCSTR WINAPI FindUsbProductLineCb(LPCSTR line,
                                   PVOID matchValue,
                                   PULONG lineLength);

LPCSTR WINAPI FindUsbSubClassLineCb(LPCSTR line,
                                    PVOID matchValue,
                                    PULONG lineLength);

LPCSTR WINAPI FindUsbProtocolLineCb(LPCSTR line,
                                    PVOID matchValue,
                                    PULONG lineLength);

BOOL WINAPI EnumLines(PFIND_USBIDS_CONTEXT context)
{

	ULONG matchLineLength;
	LPCSTR chMatchStart;

	if (context->SourceText)
	{
		context->NextLine = context->SourceText;
		context->SourceText = NULL;
	}
	chMatchStart = context->NextLine;
	if (!chMatchStart) return FALSE;

	context->NextLine = strstr(chMatchStart, "\r\n");
	if (!context->NextLine)
	{
		matchLineLength = (ULONG)strlen(chMatchStart);
	}
	else
	{
		matchLineLength = (ULONG)(context->NextLine - chMatchStart);
		context->NextLine += 2;
	}

	if (matchLineLength < 6 ||
	        chMatchStart[0] == '#')
		return TRUE;

	chMatchStart = context->MatchLinePredicate(chMatchStart, context->MatchValue, &matchLineLength);
	if (chMatchStart)
	{
		context->Found.MatchStart = chMatchStart;
		context->Found.MatchLength = matchLineLength;
		return FALSE;
	}
	if (matchLineLength == MATCH_END)
		return FALSE;

	return TRUE;
}

LPCSTR WINAPI FindUsbClassLineCb(LPCSTR line, PVOID matchValue, PULONG lineLength)
{
	UCHAR matchOn = *((PUCHAR)matchValue);
	ULONG length = *lineLength;
	ULONG startPos = 0;
	if (line[startPos++] == 'C' && line[startPos++] == ' ')
	{
		int classID;
		if (sscanf(&line[startPos], "%02x", &classID) == 1)
		{
			startPos += 2;
			if (classID == matchOn)
			{
				if (line[startPos++] == ' ' && line[startPos++] == ' ')
				{
					*lineLength = length - startPos;
					return &line[startPos];
				}
			}
		}
	}

	return NULL;
}

LPCSTR WINAPI FindUsbVendorLineCb(LPCSTR line, PVOID matchValue, PULONG lineLength)
{
	USHORT matchOn = *((PUSHORT)matchValue);
	ULONG length = *lineLength;
	ULONG startPos = 0;
	int ID;
	if (sscanf(&line[startPos], "%04x", &ID) == 1)
	{
		startPos += 4;
		if (ID == matchOn)
		{
			if (line[startPos++] == ' ' && line[startPos++] == ' ')
			{
				*lineLength = length - startPos;
				return &line[startPos];
			}
		}
	}

	return NULL;
}

LPCSTR WINAPI FindUsbProductLineCb(LPCSTR line, PVOID matchValue, PULONG lineLength)
{
	USHORT matchOn = *((PUSHORT)matchValue);
	ULONG length = *lineLength;
	ULONG startPos = 0;
	if (line[startPos++] == '\t')
	{
		int ID;
		if (sscanf(&line[startPos], "%04x", &ID) == 1)
		{
			startPos += 4;
			if (ID == matchOn)
			{
				if (line[startPos++] == ' ' && line[startPos++] == ' ')
				{
					*lineLength = length - startPos;
					return &line[startPos];
				}
			}
		}
	}
	else
	{
		*lineLength = MATCH_END;
	}

	return NULL;
}

LPCSTR WINAPI FindUsbSubClassLineCb(LPCSTR line, PVOID matchValue, PULONG lineLength)
{
	UCHAR matchOn = *((PUCHAR)matchValue);
	ULONG length = *lineLength;
	ULONG startPos = 0;
	if (line[startPos++] == '\t')
	{
		int ID;
		if (sscanf(&line[startPos], "%02x", &ID) == 1)
		{
			startPos += 2;
			if (ID == matchOn)
			{
				if (line[startPos++] == ' ' && line[startPos++] == ' ')
				{
					*lineLength = length - startPos;
					return &line[startPos];
				}
			}
		}
	}
	else
	{
		*lineLength = MATCH_END;
	}

	return NULL;
}

LPCSTR WINAPI FindUsbProtocolLineCb(LPCSTR line, PVOID matchValue, PULONG lineLength)
{
	UCHAR matchOn = *((PUCHAR)matchValue);
	ULONG length = *lineLength;
	ULONG startPos = 0;
	if (line[startPos++] == '\t' && line[startPos++] == '\t')
	{
		int ID;
		if (sscanf(&line[startPos], "%02x", &ID) == 1)
		{
			startPos += 2;
			if (ID == matchOn)
			{
				if (line[startPos++] == ' ' && line[startPos++] == ' ')
				{
					*lineLength = length - startPos;
					return &line[startPos];
				}
			}
		}
	}
	else
	{
		*lineLength = MATCH_END;
	}

	return NULL;
}

VOID WINAPI GetHwIdDisplayText(__in USHORT VendorID,
                               __in USHORT ProductID,
                               __out PCHAR VendorName,
                               __out PCHAR ProductName)
{
	FIND_USBIDS_CONTEXT FindUsbIdsContext;
	LPCSTR usbIds = UsbIdsText;

	VendorName[0] = '\0';
	ProductName[0] = '\0';

	if (!usbIds)
		return;

	memset(&FindUsbIdsContext, 0, sizeof(FindUsbIdsContext));
	InitUsbIdsContext(FindUsbIdsContext, usbIds, FindUsbVendorLineCb, &VendorID);

	while(EnumLines(&FindUsbIdsContext));
	if (FindUsbIdsContext.Found.MatchStart)
	{
		CopyUsbIdText(FindUsbIdsContext, VendorName);

		memset(&FindUsbIdsContext.Found, 0, sizeof(FindUsbIdsContext.Found));
		InitUsbIdsContext(FindUsbIdsContext, FindUsbIdsContext.NextLine, FindUsbProductLineCb, &ProductID);

		while(EnumLines(&FindUsbIdsContext));
		if (FindUsbIdsContext.Found.MatchStart)
		{
			CopyUsbIdText(FindUsbIdsContext, ProductName);
		}

	}
	return;
}

VOID WINAPI GetClassDisplayText(__in UCHAR ClassID,
                                __in UCHAR SubClassID,
                                __in UCHAR ProtocolID,
                                __out PCHAR ClassName,
                                __out PCHAR SubClassName,
                                __out PCHAR ProtocolName)
{
	FIND_USBIDS_CONTEXT FindUsbIdsContext;
	LPCSTR usbIds = UsbIdsText;

	ClassName[0] = '\0';
	SubClassName[0] = '\0';
	ProtocolName[0] = '\0';

	if (!usbIds)
		return;

	memset(&FindUsbIdsContext, 0, sizeof(FindUsbIdsContext));
	InitUsbIdsContext(FindUsbIdsContext, usbIds, FindUsbClassLineCb, &ClassID);

	while(EnumLines(&FindUsbIdsContext));
	if (FindUsbIdsContext.Found.MatchStart)
	{
		CopyUsbIdText(FindUsbIdsContext, ClassName);

		memset(&FindUsbIdsContext.Found, 0, sizeof(FindUsbIdsContext.Found));
		InitUsbIdsContext(FindUsbIdsContext, FindUsbIdsContext.NextLine, FindUsbSubClassLineCb, &SubClassID);

		while(EnumLines(&FindUsbIdsContext));
		if (FindUsbIdsContext.Found.MatchStart)
		{
			CopyUsbIdText(FindUsbIdsContext, SubClassName);

			memset(&FindUsbIdsContext.Found, 0, sizeof(FindUsbIdsContext.Found));
			InitUsbIdsContext(FindUsbIdsContext, FindUsbIdsContext.NextLine, FindUsbProtocolLineCb, &ProtocolID);

			while(EnumLines(&FindUsbIdsContext));
			if (FindUsbIdsContext.Found.MatchStart)
			{
				CopyUsbIdText(FindUsbIdsContext, ProtocolName);
			}
		}

	}
	return;
}
