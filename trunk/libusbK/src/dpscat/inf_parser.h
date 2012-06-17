#ifndef __INF_PARSER_H__
#define __INF_PARSER_H__

#include "lusbk_linked_list.h"

#if !defined(KINF_API)
#define KINF_API
#endif

#define MAX_PATH_LENGTH 512
#define MAX_STR_LENGTH 1024

typedef struct _KINF_DEVICE_EL
{
	WCHAR HardwareID[MAX_PATH];
	WCHAR CatKey[8];

	struct _KINF_DEVICE_EL* next;
	struct _KINF_DEVICE_EL* prev;
} KINF_DEVICE_EL, *PKINF_DEVICE_EL;

typedef struct _KINF_FILE_EL
{
	WCHAR Filename[MAX_PATH];
	WCHAR SubDir[MAX_PATH];

	struct _KINF_FILE_EL* next;
	struct _KINF_FILE_EL* prev;
} KINF_FILE_EL, *PKINF_FILE_EL;

typedef struct _KINF_EL
{
	WCHAR BaseDir[MAX_PATH_LENGTH];

	WCHAR InfFullPath[MAX_PATH_LENGTH];
	WCHAR InfTitle[MAX_PATH];

	WCHAR CatFullPath[MAX_PATH_LENGTH];
	WCHAR CatTitle[MAX_PATH];

	WCHAR Provider[MAX_PATH_LENGTH];

	PKINF_DEVICE_EL Devices;

	PKINF_FILE_EL	Files;

	struct _KINF_EL* next;
	struct _KINF_EL* prev;
} KINF_EL, *PKINF_EL;

typedef struct _KINF_LIST
{
	HANDLE HeapHandle;

	PKINF_EL Files;

} KINF_LIST, *PKINF_LIST;

BOOL KINF_API InfK_Init(PKINF_LIST* List);
BOOL KINF_API InfK_Free(PKINF_LIST List);
BOOL KINF_API InfK_AddInfFile(PKINF_LIST List, LPCWSTR InfFilename);

LPCSTR WcsToTempMbs(LPCWSTR str);
LPCWSTR MbsToTempWcs(LPCSTR str);

BOOL SelfSignFile(LPCWSTR szFileName, LPCWSTR szCertSubject);
BOOL CreateCatEx(PKINF_EL infEL);

#endif
