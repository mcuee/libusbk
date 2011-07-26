/*!
#
# Copyright (c) 2011 Travis Robinson <libusbdotnet@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS LEE ROBINSON
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
*/

#include "unit_tests.h"

static DWORD gErrorCode = ERROR_SUCCESS;
static CHAR gErrorString[512];

static KUSB_DRIVER_API Usb;

// Globals
UNIT_TEST_CONFIG Exhaust_Handles = {"AllK", "Exhaust Handles", 0};
#define TEST_LOOP_COUNT 2

static DWORD show_error(__in_opt LPCSTR message)
{
	gErrorCode = UnitK_GetError(gErrorString, sizeof(gErrorString));
	if (gErrorCode != ERROR_SUCCESS)
	{
		if (message)
			UnitK_Ln("%s:%08Xh", message, gErrorCode);
		else
			UnitK_Ln("Result:%08Xh", gErrorCode);

		UnitK_Ln("%s", gErrorString);
	}
	return gErrorCode;
}

static int gContextCounters[KLIB_HANDLE_TYPE_COUNT] = {0, 0, 0, 0, 0, 0};
static LPCSTR gContextNames[KLIB_HANDLE_TYPE_COUNT] = {"HotK", "UsbK", "UsbSharedK", "LstK", "LstInfoK", "OvlK", "OvlPoolK"};

static LONG KUSB_API Init_Handle(
    __in KLIB_HANDLE Handle,
    __in KLIB_HANDLE_TYPE HandleType,
    __in PKLIB_USER_CONTEXT UserContext)
{
	LPCSTR cat = Exhaust_Handles.Category;
	Exhaust_Handles.Category = gContextNames[HandleType];

	UnitK_Ln("%s:%d", __FUNCTION__, gContextCounters[HandleType]);
	UserContext->Value = gContextCounters[HandleType]++;

	Exhaust_Handles.Category = cat;

	return ERROR_SUCCESS;
}

static LONG KUSB_API Free_Handle(
    __in KLIB_HANDLE Handle,
    __in KLIB_HANDLE_TYPE HandleType,
    __in PKLIB_USER_CONTEXT UserContext)
{
	LPCSTR cat = Exhaust_Handles.Category;
	Exhaust_Handles.Category = gContextNames[HandleType];

	UnitK_Ln("%s:%d", __FUNCTION__, UserContext->Value);
	Exhaust_Handles.Category = gContextNames[HandleType];

	Exhaust_Handles.Category = cat;

	return ERROR_SUCCESS;
}

DWORD exhaust_handles_usbk(int argc, char* argv[])
{
	KLST_HANDLE devList;
	KLST_DEVINFO_HANDLE devInfo;
	KUSB_HANDLE masterDevice;
	KUSB_HANDLE devices[2048 + 1];
	DWORD index, freeCount, loop;

	memset(devices, 0, sizeof(devices));
	Exhaust_Handles.Name = "Exhaust UsbK Handles";

	UnitK_Init(&Exhaust_Handles);

	if (!UnitK_GetTestDevice(&devList, &devInfo, argc, argv))
		return GetLastError();

	LibK_LoadDriverApi(&Usb, devInfo->DrvId, sizeof(KUSB_DRIVER_API));
	if (!Usb.Open(devInfo, &masterDevice))
	{
		return show_error("Failed opening device");
	}

	Exhaust_Handles.Tab++;

	for (loop = 0; loop < TEST_LOOP_COUNT; loop++)
	{
		index = (DWORD) - 1;
		while (Usb.Clone(masterDevice, &devices[++index]));

		if (show_error("Exhausted all UsbK handles") != ERROR_OUT_OF_STRUCTURES)
		{
			UnitK_Ln("");
			UnitK_Ln("[FAILED!]");
			UnitK_Ln("");
		}

		freeCount = 0;
		for (index = 0; devices[index] != NULL; index++)
		{
			if (!Usb.Free(devices[index])) show_error("Usb.Free");
			devices[index] = NULL;
			freeCount++;
		}
	}

	Exhaust_Handles.Tab--;

	Usb.Free(masterDevice);
	LstK_Free(devList);

	return gErrorCode;
}

DWORD exhaust_handles_ovlk(int argc, char* argv[])
{
	KOVL_POOL_HANDLE pool = NULL;
	KOVL_HANDLE ovlHandles[2048 + 1];
	BOOL success;
	DWORD index, freeCount, loop;

	memset(ovlHandles, 0, sizeof(ovlHandles));
	Exhaust_Handles.Name = "Exhaust OvlK Handles";
	UnitK_Init(&Exhaust_Handles);

	success = OvlK_InitPool(&pool, 0);
	if (!success) return show_error("OvlK_InitPool");

	Exhaust_Handles.Tab++;

	for (loop = 0; loop < TEST_LOOP_COUNT; loop++)
	{
		index = (DWORD) - 1;
		while (OvlK_Acquire(&ovlHandles[++index], pool));

		if (show_error("Exhausted all OvlK handles") != ERROR_OUT_OF_STRUCTURES)
		{
			UnitK_Ln("");
			UnitK_Ln("[FAILED!]");
			UnitK_Ln("");
		}

		freeCount = 0;
		for (index = 0; ovlHandles[index] != NULL; index++)
		{
			if (!OvlK_Release(ovlHandles[index])) show_error("OvlK_Release");
			ovlHandles[index] = NULL;
			freeCount++;
		}
	}

	Exhaust_Handles.Tab--;

	OvlK_FreePool(pool);
	return gErrorCode;
}

DWORD exhaust_handles_lstk(int argc, char* argv[])
{
	KLST_DEVINFO_HANDLE devInfo[2048 + 1];
	KLST_HANDLE devList[2048 + 1];
	DWORD index, freeCount, loop, infoIndex;

	memset(devInfo, 0, sizeof(devInfo));
	memset(devList, 0, sizeof(devList));

	Exhaust_Handles.Name = "Exhaust LstK Handles";
	UnitK_Init(&Exhaust_Handles);

	Exhaust_Handles.Tab++;

	for (loop = 0; loop < TEST_LOOP_COUNT; loop++)
	{
		index = (DWORD) - 1;
		while (LstK_Init(&devList[++index], NULL))
		{
			KLST_DEVINFO_HANDLE checkInfo;
			if (LstK_MoveNext(devList[index], &checkInfo))
			{
				infoIndex = (DWORD) - 1;
				while(LstK_CloneInfo(checkInfo, &devInfo[++infoIndex]));
				if (show_error("Exhausted all LstInfoK handles") != ERROR_OUT_OF_STRUCTURES)
				{
					UnitK_Ln("");
					UnitK_Ln("[FAILED!]");
					UnitK_Ln("");
				}

				freeCount = 0;
				for (infoIndex = 0; devInfo[infoIndex] != NULL; infoIndex++)
				{
					if (!LstK_FreeInfo(devInfo[infoIndex])) show_error("LstK_FreeInfo");
					devInfo[infoIndex] = NULL;
					freeCount++;
				}
			}
		}

		if (show_error("Exhausted all LstK handles") != ERROR_OUT_OF_STRUCTURES)
		{
			UnitK_Ln("");
			UnitK_Ln("[FAILED!]");
			UnitK_Ln("");
		}

		freeCount = 0;
		for (index = 0; devList[index] != NULL; index++)
		{
			if (!LstK_Free(devList[index])) show_error("LstK_Free");
			devList[index] = NULL;
			freeCount++;
		}
	}

	Exhaust_Handles.Tab--;

	return gErrorCode;
}

DWORD exhaust_handles_hotk(int argc, char* argv[])
{
	KHOT_HANDLE hotHandles[2048 + 1];
	DWORD index, freeCount, loop;
	KHOT_PARAMS hotParams;

	memset(hotHandles, 0, sizeof(hotHandles));
	memset(&hotParams, 0, sizeof(hotParams));

	strcpy(hotParams.PatternMatch.InstanceID, "*");

	Exhaust_Handles.Name = "Exhaust HotK Handles";
	UnitK_Init(&Exhaust_Handles);

	Exhaust_Handles.Tab++;

	for (loop = 0; loop < TEST_LOOP_COUNT; loop++)
	{
		index = (DWORD) - 1;
		while (HotK_Init(&hotHandles[++index], &hotParams));

		if (show_error("Exhausted all HotK handles") != ERROR_OUT_OF_STRUCTURES)
		{
			UnitK_Ln("");
			UnitK_Ln("[FAILED!]");
			UnitK_Ln("");
		}

		freeCount = 0;
		for (index = 0; hotHandles[index] != NULL; index++)
		{
			if (!HotK_Free(hotHandles[index])) show_error("HotK_Free");
			hotHandles[index] = NULL;
			freeCount++;
		}
	}

	Exhaust_Handles.Tab--;

	return gErrorCode;
}

DWORD __cdecl main(int argc, char* argv[])
{
	LibK_SetHandleCallbacks(KLIB_HANDLE_TYPE_HOTK, Init_Handle, Free_Handle);
	LibK_SetHandleCallbacks(KLIB_HANDLE_TYPE_USBK, Init_Handle, Free_Handle);
	LibK_SetHandleCallbacks(KLIB_HANDLE_TYPE_USBSHAREDK, Init_Handle, Free_Handle);
	LibK_SetHandleCallbacks(KLIB_HANDLE_TYPE_LSTK, Init_Handle, Free_Handle);
	LibK_SetHandleCallbacks(KLIB_HANDLE_TYPE_LSTINFOK, Init_Handle, Free_Handle);
	LibK_SetHandleCallbacks(KLIB_HANDLE_TYPE_OVLK, Init_Handle, Free_Handle);
	LibK_SetHandleCallbacks(KLIB_HANDLE_TYPE_OVLPOOLK, Init_Handle, Free_Handle);

	exhaust_handles_lstk(argc, argv);
	exhaust_handles_usbk(argc, argv);
	exhaust_handles_ovlk(argc, argv);
	exhaust_handles_hotk(argc, argv);
}
