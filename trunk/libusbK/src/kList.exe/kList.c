/* USB Benchmark for libusb-win32

 Copyright © 2010 Travis Robinson. <libusbdotnet@gmail.com>
 website: http://sourceforge.net/projects/libusb-win32

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, please visit www.gnu.org.
*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <setupapi.h>

#include "lusbk_version.h"
#include "lusbk_usb.h"
#include "UsbIds.h"

static KUSB_DRIVER_API K;
WINUSB_INTERFACE_HANDLE InterfaceHandle;

LONG WinError(__in_opt DWORD errorCode);

BOOL OpenDeviceFileHandle(__in LPCSTR deviceFileName,
                          __out HANDLE* fileHandle);

BOOL GetDescriptorReport(__in PKUSB_DEV_LIST deviceElement,
                         __in BOOL detailed);

BOOL GetRealConfigDescriptor(__in UCHAR Index,
                             __out_opt PUCHAR Buffer,
                             __in ULONG BufferLength,
                             __out PULONG LengthTransferred);

LPCSTR LoadResourceUsbIds(void);
VOID DumpDescriptorDevice(PUSB_DEVICE_DESCRIPTOR desc);
VOID DumpDescriptorInterface(PUSB_INTERFACE_DESCRIPTOR desc);
VOID DumpDescriptorEndpoint(PUSB_ENDPOINT_DESCRIPTOR desc);
VOID DumpDescriptorConfig(PUSB_CONFIGURATION_DESCRIPTOR desc);
VOID DumpDescriptorCommon(PUSB_COMMON_DESCRIPTOR desc);

static LPCSTR gTabIndents3[] = {" ", "| ", "| | ", "| | | ", "| | | | "};
static LPCSTR gTabIndents2[] = {"-", "| -", "| | -", "| | | -", "| | | | -"};
static UCHAR gTab = 0;

LPCSTR UsbIdsText = NULL;

static LPCSTR DescriptorTypeString[] =
{
	"DEVICE:",
	"CONFIGURATION:",
	"STRING:",
	"INTERFACE:",
	"ENDPOINT:",
	"UNKNOWN:",
	"UNKNOWN:",
	"UNKNOWN:"
};
#define GetDescriptorTypeString(DescriptorId) (DescriptorTypeString[(DescriptorId)>USB_ENDPOINT_DESCRIPTOR_TYPE?7:((DescriptorId)-1)&0x07])

static LPCSTR DrvIdNames[8] = {"libusbK", "libusb0", "libusb0 filter", "WinUSB", "Unknown", "Unknown", "Unknown", "Unknown"};
#define GetDrvIdString(DrvId)	(DrvIdNames[((((LONG)(DrvId))<0) || ((LONG)(DrvId)) >= KUSB_DRVID_COUNT)?KUSB_DRVID_COUNT:(DrvId)])

#define GetPipeTypeString(PipeType) PipeTypeStrings[(PipeType) & 0x7]
CONST PCHAR PipeTypeStrings[8] = {"Control", "Isochronous", "Bulk", "Interrupt", "na", "na", "na", "na"};

#define PrintfDeviceElement(DeviceListFieldName) printf("    %-21s: %s\n",DEFINE_TO_STR(DeviceListFieldName),deviceElement->DeviceListFieldName)
#define DESC_LINE(format,...) printf("%s"format"\n",gTabIndents3[gTab],__VA_ARGS__)
#define DESC_BEGIN(DescId) printf("%s%s\n",gTabIndents2[gTab++], GetDescriptorTypeString(DescId))
#define DESC_VALUE(Descriptor,FieldName,format,...) printf("%s%-18s:"format, gTabIndents3[gTab], DEFINE_TO_STR(FieldName),Descriptor->FieldName,__VA_ARGS__)
#define DESC_iVALUE(Descriptor, StringIndex) DESC_VALUE(Descriptor, StringIndex, "%u%s\n", GetDescriptorString(Descriptor->StringIndex))
#define DESC_END(DescriptorType) printf("%s\n",gTabIndents3[--gTab])

#define IsDescValid(DescriptorPtr, RemainingTotalSize)\
	(((RemainingTotalSize) > sizeof(USB_COMMON_DESCRIPTOR)) && (RemainingTotalSize) >= (DescriptorPtr)->bLength)



#define AdvanceDescriptor(DescriptorPtr, RemainingLength)										\
{																								\
	RemainingLength -= DescriptorPtr->bLength;													\
	DescriptorPtr = (PUSB_COMMON_DESCRIPTOR)(((PUCHAR)DescriptorPtr) + DescriptorPtr->bLength);	\
}

#define AdvanceDescriptorWithErrorBreak(DescriptorPtr, RemainingLength)							\
	AdvanceDescriptor(DescriptorPtr, RemainingLength);											\
	if (!IsDescValid(DescriptorPtr, RemainingLength))											\
	break

void ShowHelp(void);
void ShowCopyright(void);

int __cdecl main(int argc, char** argv)
{
	LONG ec;
	PKUSB_DEV_LIST deviceList = NULL;
	PKUSB_DEV_LIST deviceElement;
	LONG devicePos = 0;
	LONG_PTR selection;

	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	UsbIdsText = LoadResourceUsbIds();

	ec = LUsbK_GetDeviceList(&deviceList, NULL);
	if (ec < 0)
	{
		printf("failed getting device list.\n");
		return WinError(0);
	}
	if (ec == 0)
	{
		printf("No devices found.\n");
		ec = -1;
		goto Done;
	}
	deviceElement = deviceList;
	while(deviceElement)
	{
		printf("%2d. %-21s: %s\n", devicePos + 1, "Driver", GetDrvIdString(deviceElement->DrvId));
		PrintfDeviceElement(DeviceDesc);
		PrintfDeviceElement(Mfg);
		PrintfDeviceElement(ClassGUID);
		PrintfDeviceElement(DeviceInstance);
		PrintfDeviceElement(DeviceInterfaceGUID);
		PrintfDeviceElement(SymbolicLink);
		PrintfDeviceElement(DevicePath);
		printf("\n");

		devicePos++;
		deviceElement = deviceElement->next;

	}

	if (!devicePos)
	{
		printf("device list error\n");
		ec = -1;
		goto Done;
	}

	printf("Select device (1-%u) :", devicePos);
	while(_kbhit()) _getch();
	if (
	    ((((LONG)scanf_s("%u", &selection)) != 1) ||
	     (selection) > devicePos)
	)
	{
		printf("\ninvalid selection\n");
		ec = -1;
		goto Done;
	}
	printf("\n");

	devicePos = 1;
	deviceElement = deviceList;
	while(deviceElement)
	{
		if (devicePos == selection)
			break;
		devicePos++;
		deviceElement = deviceElement->next;
	}
	if (!deviceElement)
	{
		printf("device list error\n");
		goto Done;
	}

	printf("Loading driver api..\n");
	if (!LUsbK_LoadDriverApi(&K, deviceElement->DrvId))
	{
		ec = WinError(0);
		goto Done;
	}
	printf("Getting desciptor report..\n");
	if (!GetDescriptorReport(deviceElement, TRUE))
	{
		ec = WinError(0);
		goto Done;
	}

Done:
	LUsbK_FreeDeviceList(&deviceList);
	return ec;
}

BOOL GetRealConfigDescriptor(__in UCHAR Index,
                             __out_opt PUCHAR Buffer,
                             __in ULONG BufferLength,
                             __out PULONG LengthTransferred)
{
	WINUSB_SETUP_PACKET Pkt;
	PUSB_DEFAULT_PIPE_SETUP_PACKET defPkt = (PUSB_DEFAULT_PIPE_SETUP_PACKET)&Pkt;

	memset(&Pkt, 0, sizeof(Pkt));
	defPkt->bmRequestType.Dir = BMREQUEST_DEVICE_TO_HOST;
	defPkt->bRequest = USB_REQUEST_GET_DESCRIPTOR;
	defPkt->wValue.HiByte = USB_CONFIGURATION_DESCRIPTOR_TYPE;
	defPkt->wValue.LowByte = Index;
	defPkt->wLength = (USHORT)BufferLength;

	return K.ControlTransfer(InterfaceHandle, Pkt, Buffer, BufferLength, LengthTransferred, NULL);
}

BOOL OpenDeviceFileHandle(__in LPCSTR deviceFileName, __out HANDLE* fileHandle)
{
	HANDLE handle = CreateFileA(deviceFileName,
	                            GENERIC_READ | GENERIC_WRITE,
	                            FILE_SHARE_READ | FILE_SHARE_WRITE,
	                            NULL,
	                            OPEN_EXISTING,
	                            FILE_FLAG_OVERLAPPED,
	                            NULL);

	if (!handle || handle == INVALID_HANDLE_VALUE)
	{
		printf("failed creating device file handle:\n:%s\n", deviceFileName);
		return FALSE;
	}

	*fileHandle = handle;
	return TRUE;
}

BOOL GetDescriptorReport(__in PKUSB_DEV_LIST deviceElement, __in BOOL detailed)
{
	HANDLE fileHandle = NULL;
	ULONG length;
	USB_DEVICE_DESCRIPTOR deviceDescriptor;
	BOOL success = FALSE;
	PUSB_CONFIGURATION_DESCRIPTOR configDescriptor = NULL;

	if ((success = OpenDeviceFileHandle(deviceElement->DevicePath, &fileHandle)) == FALSE)
		goto Error;

	if (deviceElement)
	{

		if (!K.Initialize(fileHandle, &InterfaceHandle))
		{
			WinError(0);
			goto Error;
		}

		if (!K.GetDescriptor(InterfaceHandle,
		                     USB_DEVICE_DESCRIPTOR_TYPE,
		                     0, 0,
		                     (PUCHAR)&deviceDescriptor,
		                     sizeof(deviceDescriptor),
		                     &length))
		{
			WinError(0);
			return FALSE;
		}

		DESC_BEGIN(USB_DEVICE_DESCRIPTOR_TYPE);
		DumpDescriptorDevice(&deviceDescriptor);
		DESC_END(USB_DEVICE_DESCRIPTOR_TYPE);

		configDescriptor = malloc(4096);
		memset(configDescriptor, 0, 4096);

		if (detailed)
			success = GetRealConfigDescriptor(0, (PUCHAR)configDescriptor, 4096, &length);
		else
			success = K.GetDescriptor(InterfaceHandle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, (PUCHAR)configDescriptor, 4096, &length);

		if (!success)
		{
			printf("failed getting config descriptor.\n");
			goto Error;
		}
		else
		{
			LONG remainingLength = 0;
			PUSB_COMMON_DESCRIPTOR hdr = (PUSB_COMMON_DESCRIPTOR)configDescriptor;

			DESC_BEGIN(USB_CONFIGURATION_DESCRIPTOR_TYPE);
			remainingLength = (LONG)length;

			DumpDescriptorConfig((PUSB_CONFIGURATION_DESCRIPTOR)hdr);
			AdvanceDescriptor(hdr, remainingLength);

			while(IsDescValid(hdr, remainingLength))
			{
				if (hdr->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
				{
					DESC_LINE("");

					DESC_BEGIN(USB_INTERFACE_DESCRIPTOR_TYPE);

					DumpDescriptorInterface((PUSB_INTERFACE_DESCRIPTOR)hdr);
					AdvanceDescriptor(hdr, remainingLength);

					if (hdr->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE)
					{
						DESC_LINE("");
						while (hdr->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE)
						{
							DESC_BEGIN(USB_ENDPOINT_DESCRIPTOR_TYPE);

							DumpDescriptorEndpoint((PUSB_ENDPOINT_DESCRIPTOR)hdr);

							DESC_END(USB_ENDPOINT_DESCRIPTOR_TYPE);

							AdvanceDescriptorWithErrorBreak(hdr, remainingLength);
						}
					}
					else
					{
						DESC_LINE("");
						DESC_BEGIN(0xFF);
						DumpDescriptorCommon(hdr);
						DESC_END(0xFF);
						AdvanceDescriptorWithErrorBreak(hdr, remainingLength);
					}

					DESC_END(USB_INTERFACE_DESCRIPTOR_TYPE);
				}
				else
				{
					DESC_LINE("");
					DESC_BEGIN(0xFF);
					DumpDescriptorCommon(hdr);
					DESC_END(0xFF);
					AdvanceDescriptorWithErrorBreak(hdr, remainingLength);
				}
			}
		}

		success = TRUE;
	}

Error:
	if (InterfaceHandle)
		K.Free(InterfaceHandle);

	if (fileHandle)
		CloseHandle(fileHandle);
	return success;
}


LONG WinError(__in_opt DWORD errorCode)
{
	LPSTR buffer = NULL;

	errorCode = errorCode ? labs(errorCode) : GetLastError();
	if (!errorCode) return errorCode;

	if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
	                   NULL, errorCode, 0, (LPSTR)&buffer, 0, NULL) > 0)
	{
		printf("%s\n", buffer);
	}
	else
	{
		printf("FormatMessage error!\n");
	}

	if (buffer)
		LocalFree(buffer);

	return -labs(errorCode);
}

CONST PCHAR GetDescriptorString(USHORT stringIndex)
{
	static CHAR rtn[128];

	BOOL success;
	DWORD length;
	DWORD size = sizeof(USB_STRING_DESCRIPTOR) + sizeof(rtn) * 2;
	PUSB_STRING_DESCRIPTOR stringDesc = NULL;

	memset(rtn, 0, sizeof(rtn));
	if (!stringIndex) return rtn;

	stringDesc = malloc(size);
	memset(stringDesc, 0, size);

	success = K.GetDescriptor(InterfaceHandle, USB_STRING_DESCRIPTOR_TYPE, (UCHAR)stringIndex, 0, (PUCHAR)stringDesc, size, &length);
	if (success && length > sizeof(USB_COMMON_DESCRIPTOR))
	{
		PCHAR  dst = rtn;
		PWCHAR src = stringDesc->bString;
		length -= sizeof(USB_COMMON_DESCRIPTOR);
		length /= 2;
		dst[0] = ' ';
		dst[1] = '(';
		dst += 2;

		while(length-- && *src)
		{
			*dst = (CHAR) * src;
			dst++;
			src++;
		}
		dst[0] = ')';
		dst[1] = '\0';
	}

	if (stringDesc)
		free(stringDesc);

	return rtn;
}

VOID DumpDescriptorDevice(PUSB_DEVICE_DESCRIPTOR desc)
{
	CHAR vendorName[MAX_PATH];
	CHAR productName[MAX_PATH];
	CHAR className[MAX_PATH];
	CHAR subClassName[MAX_PATH];
	CHAR protocolName[MAX_PATH];

	GetHwIdDisplayText(desc->idVendor, desc->idProduct, vendorName, productName);

	GetClassDisplayText(desc->bDeviceClass, desc->bDeviceSubClass, desc->bDeviceProtocol,
	                    className, subClassName, protocolName);

	DESC_VALUE(desc, bLength, "%u\n");
	DESC_VALUE(desc, bDescriptorType, "%u\n");
	DESC_VALUE(desc, bcdUSB, "%04Xh\n");
	DESC_VALUE(desc, bDeviceClass, "%02Xh %s\n", className);
	DESC_VALUE(desc, bDeviceSubClass, "%02Xh %s\n", subClassName);
	DESC_VALUE(desc, bDeviceProtocol, "%02Xh %s\n", protocolName);
	DESC_VALUE(desc, bMaxPacketSize0, "%u\n");
	DESC_VALUE(desc, idVendor, "%04Xh %s\n", vendorName);
	DESC_VALUE(desc, idProduct, "%04Xh %s\n", productName);
	DESC_VALUE(desc, bcdDevice, "%04X\n");
	DESC_iVALUE(desc, iManufacturer);
	DESC_iVALUE(desc, iProduct);
	DESC_iVALUE(desc, iSerialNumber);
	DESC_VALUE(desc, bNumConfigurations, "%u\n");
}

VOID DumpDescriptorInterface(PUSB_INTERFACE_DESCRIPTOR desc)
{
	CHAR className[MAX_PATH];
	CHAR subClassName[MAX_PATH];
	CHAR protocolName[MAX_PATH];

	GetClassDisplayText(desc->bInterfaceClass, desc->bInterfaceSubClass, desc->bInterfaceProtocol,
	                    className, subClassName, protocolName);

	DESC_VALUE(desc, bLength, "%u\n");
	DESC_VALUE(desc, bDescriptorType, "%02Xh\n");
	DESC_VALUE(desc, bInterfaceNumber, "%02u\n");
	DESC_VALUE(desc, bAlternateSetting, "%02u\n");
	DESC_VALUE(desc, bNumEndpoints, "%u\n");
	DESC_VALUE(desc, bInterfaceClass, "%02Xh %s\n", className);
	DESC_VALUE(desc, bInterfaceSubClass, "%02Xh %s\n", subClassName);
	DESC_VALUE(desc, bInterfaceProtocol, "%02Xh %s\n", protocolName);
	DESC_iVALUE(desc, iInterface);
}

VOID DumpDescriptorEndpoint(PUSB_ENDPOINT_DESCRIPTOR desc)
{
	DESC_VALUE(desc, bLength, "%u\n");
	DESC_VALUE(desc, bDescriptorType, "%02Xh\n");
	DESC_VALUE(desc, bEndpointAddress, "%02Xh\n");
	DESC_VALUE(desc, bmAttributes, "%02Xh (%s)\n", GetPipeTypeString(desc->bmAttributes));
	DESC_VALUE(desc, wMaxPacketSize, "%u\n");
	DESC_VALUE(desc, bInterval, "%02Xh\n");
}

VOID DumpDescriptorConfig(PUSB_CONFIGURATION_DESCRIPTOR desc)
{
	DESC_VALUE(desc, bLength, "%u\n");
	DESC_VALUE(desc, bDescriptorType, "%02Xh\n");
	DESC_VALUE(desc, wTotalLength, "%u\n");
	DESC_VALUE(desc, bNumInterfaces, "%u\n");
	DESC_VALUE(desc, bConfigurationValue, "%02Xh\n");
	DESC_iVALUE(desc, iConfiguration);
	DESC_VALUE(desc, bmAttributes, "%02Xh\n");
	DESC_VALUE(desc, MaxPower, "%u (%u ma)\n", desc->MaxPower * 2);
}

VOID DumpDescriptorCommon(PUSB_COMMON_DESCRIPTOR desc)
{
	DESC_VALUE(desc, bLength, "%u\n");
	DESC_VALUE(desc, bDescriptorType, "%02Xh\n");
}

LPCSTR LoadResourceUsbIds(void)
{
#define ID_USBIDS_TEXT  10021
#define ID_DOS_TEXT   300

	LPCSTR src;
	DWORD src_count;
	HGLOBAL res_data;
	HRSRC hSrc;
	LPCSTR usbIds = NULL;

	hSrc = FindResourceA(NULL, MAKEINTRESOURCEA(ID_USBIDS_TEXT), MAKEINTRESOURCEA(ID_DOS_TEXT));
	if (!hSrc)	return NULL;

	src_count = SizeofResource(NULL, hSrc);

	res_data = LoadResource(NULL, hSrc);
	if (!res_data)	return NULL;

	src = (char*) LockResource(res_data);
	if (!src) return NULL;

	usbIds = (CONST PCHAR)LocalAlloc(LPTR, src_count + 1);
	memcpy((PVOID)usbIds, src, src_count);
	return usbIds;
}

//////////////////////////////////////////////////////////////////////////////
/* END OF PROGRAM                                                           */
//////////////////////////////////////////////////////////////////////////////
void ShowHelp(void)
{
#define ID_HELP_TEXT  10020
#define ID_DOS_TEXT   300

	CONST CHAR* src;
	DWORD src_count, charsWritten;
	HGLOBAL res_data;
	HANDLE handle;
	HRSRC hSrc;

	ShowCopyright();

	hSrc = FindResourceA(NULL, MAKEINTRESOURCEA(ID_HELP_TEXT), MAKEINTRESOURCEA(ID_DOS_TEXT));
	if (!hSrc)	return;

	src_count = SizeofResource(NULL, hSrc);

	res_data = LoadResource(NULL, hSrc);
	if (!res_data)	return;

	src = (char*) LockResource(res_data);
	if (!src) return;

	if ((handle = GetStdHandle(STD_ERROR_HANDLE)) != INVALID_HANDLE_VALUE)
		WriteConsoleA(handle, src, src_count, &charsWritten, NULL);
}

void ShowCopyright(void)
{
	printf("%s v%s (%s)\n",
	       RC_FILENAME_STR,
	       RC_VERSION_STR,
	       DEFINE_TO_STR(VERSION_DATE));

	printf("%s", "Copyright (c) 2011 Travis Robinson. <libusbdotnet@gmail.com>\n\n");
}
