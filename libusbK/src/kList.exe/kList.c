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

#pragma warning(disable:4201)
typedef struct _UNI_DESCRIPTOR
{
	union
	{
		USB_COMMON_DESCRIPTOR Common;
		USB_DEVICE_DESCRIPTOR Device;
		USB_CONFIGURATION_DESCRIPTOR Config;
		USB_STRING_DESCRIPTOR String;
		USB_INTERFACE_DESCRIPTOR Interface;
		USB_ENDPOINT_DESCRIPTOR Endpoint;
		USB_INTERFACEASSOCIATION_DESCRIPTOR InterfaceAssociation;
		HID_DESCRIPTOR Hid;
	};
} UNI_DESCRIPTOR, *PUNI_DESCRIPTOR, ** PPUNI_DESCRIPTOR;
#pragma warning(default:4201)

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

LPCSTR GetBmAttributes(UCHAR bmAttributes);
CONST PCHAR GetDescriptorString(USHORT stringIndex);

VOID DumpDescriptorDevice(PUSB_DEVICE_DESCRIPTOR desc);

BOOL BeginDescriptor(USB_DESCRIPTOR_TYPE type, PPUNI_DESCRIPTOR desc);
BOOL DumpDescriptorConfig(PPUNI_DESCRIPTOR desc, PLONG remainingLength);
BOOL DumpDescriptorInterface(PPUNI_DESCRIPTOR desc, PLONG remainingLength);
BOOL DumpDescriptorInterfaceAssociation(PPUNI_DESCRIPTOR desc, PLONG remainingLength);
BOOL DumpDescriptorHid(PPUNI_DESCRIPTOR desc, PLONG remainingLength);
BOOL DumpDescriptorEndpoint(PPUNI_DESCRIPTOR desc, PLONG remainingLength);
BOOL DumpDescriptorCommon(PPUNI_DESCRIPTOR desc, PLONG remainingLength);

static LPCSTR gTabIndents[] = {" ", "  ", "    ", "      ", "        ", "          ", "            ",};
static UCHAR gTab = 0;

LPCSTR UsbIdsText = NULL;


static LPCSTR DescriptorTypeString[33] =
{
	"-DEVICE:",
	"-CONFIGURATION:",
	"-STRING:",
	"-INTERFACE:",
	"-ENDPOINT:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-INTERFACE ASSOCIATION:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-UNKNOWN:",
	"-HID:",
};
#define GetDescriptorTypeString(DescriptorId) (DescriptorTypeString[(DescriptorId)>((sizeof(DescriptorTypeString)/sizeof(LPCSTR)))?11:((DescriptorId)-1)])

static LPCSTR DrvIdNames[8] = {"libusbK", "libusb0", "libusb0 filter", "WinUSB", "Unknown", "Unknown", "Unknown", "Unknown"};
#define GetDrvIdString(DrvId)	(DrvIdNames[((((LONG)(DrvId))<0) || ((LONG)(DrvId)) >= KUSB_DRVID_COUNT)?KUSB_DRVID_COUNT:(DrvId)])

CONST PCHAR PipeTypeStrings[4] = {"Control", "Isochronous", "Bulk", "Interrupt"};
#define GetPipeTypeString(PipeType) PipeTypeStrings[(PipeType) & 0x3]

CONST PCHAR IsoSynchronizationStrings[4] = {"No Synchonization", "Asynchronous", "Adaptive", "Synchronous"};
#define GetIsoSyncronizationString(BmAttributes) IsoSynchronizationStrings[((BmAttributes)>>2) & 0x3]

CONST PCHAR IsoUsageTypeStrings[4] = {"Data Endpoint", "Feedback Endpoint", "Explicit Feedback Data Endpoint", "Reserved"};
#define GetIsoUsageTypeString(BmAttributes) IsoUsageTypeStrings[((BmAttributes)>>4) & 0x3]

#define PrintfDeviceElement(DeviceListFieldName) printf("    %-21s: %s\n",DEFINE_TO_STR(DeviceListFieldName),deviceElement->DeviceListFieldName)

#define DESC_LINE(format,...) printf("%s"format"\n",gTabIndents[gTab],__VA_ARGS__)
#define DESC_SUB_BEGIN(DescId) printf("%s%s\n",gTabIndents[gTab++], GetDescriptorTypeString(DescId))
#define DESC_BEGIN(DescId) printf("\n%s%s\n",gTabIndents[gTab++], GetDescriptorTypeString(DescId))
#define DESC_END()(--gTab)

#define DESC_UNI_VALUE(Descriptor,Section,FieldName,format,...) printf("%s%-18s:"format,gTabIndents[gTab],DEFINE_TO_STR(FieldName),(*(Descriptor))->Section.FieldName,__VA_ARGS__)
#define DESC_UNI_STRID(Descriptor,Section,FieldName,format,...) DESC_UNI_VALUE(Descriptor,Section,FieldName,"%u%s\n"format,GetDescriptorString((*(Descriptor))->Section.FieldName),__VA_ARGS__)

#define DESC_VALUE(Descriptor,FieldName,format,...) printf("%s%-18s:"format, gTabIndents[gTab], DEFINE_TO_STR(FieldName),Descriptor->FieldName,__VA_ARGS__)
#define DESC_iVALUE(Descriptor, StringIndex) DESC_VALUE(Descriptor, StringIndex, "%u%s\n", GetDescriptorString(Descriptor->StringIndex))

#define IsDescValid(DescriptorPtr, RemainingTotalSize)\
	(((RemainingTotalSize) > sizeof(USB_COMMON_DESCRIPTOR)) && (RemainingTotalSize) >= (DescriptorPtr)->bLength)



#define AdvanceDescriptor(DescriptorPtr, RemainingLength)										\
{																								\
	RemainingLength -= DescriptorPtr->bLength;													\
	DescriptorPtr = (PUSB_COMMON_DESCRIPTOR)(((PUCHAR)DescriptorPtr) + DescriptorPtr->bLength);	\
}

#define IsUniDescriptorValid(DescriptorPtr, RemainingTotalSize)\
	(((RemainingTotalSize) > sizeof(USB_COMMON_DESCRIPTOR)) && (RemainingTotalSize) >= (DescriptorPtr)->Common.bLength)

#define AdvanceUniDescriptor(DescriptorPtr, RemainingLength)										\
{																								\
	(RemainingLength) -= (DescriptorPtr)->Common.bLength;													\
	(DescriptorPtr) = (PUNI_DESCRIPTOR)(((PUCHAR)(DescriptorPtr)) + (DescriptorPtr)->Common.bLength);	\
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
			goto Error;
		}

		DESC_BEGIN(USB_DEVICE_DESCRIPTOR_TYPE);
		DumpDescriptorDevice(&deviceDescriptor);
		DESC_END();

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
			PUNI_DESCRIPTOR hdr = (PUNI_DESCRIPTOR)configDescriptor;

			remainingLength = (LONG)length;

			success = DumpDescriptorConfig(&hdr, &remainingLength);
			if (!success && !remainingLength)
				success = TRUE;
		}
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
BOOL BeginDescriptor(USB_DESCRIPTOR_TYPE type, PPUNI_DESCRIPTOR desc)
{
	UNREFERENCED_PARAMETER(desc);
	DESC_SUB_BEGIN(type);
	return TRUE;
}

BOOL DumpDescriptorConfig(PPUNI_DESCRIPTOR desc, PLONG remainingLength)
{
	INT numInterfaces;
	BOOL success = TRUE;

	if (!IsUniDescriptorValid(*desc, *remainingLength))
		return FALSE;

	if (!BeginDescriptor(USB_CONFIGURATION_DESCRIPTOR_TYPE, desc))
		return FALSE;

	numInterfaces = (INT)((*desc)->Config.bNumInterfaces);

	DESC_UNI_VALUE(desc, Config, bLength, "%u\n");
	DESC_UNI_VALUE(desc, Config, bDescriptorType, "%02Xh\n");
	DESC_UNI_VALUE(desc, Config, wTotalLength, "%u\n");
	DESC_UNI_VALUE(desc, Config, bNumInterfaces, "%u\n");
	DESC_UNI_VALUE(desc, Config, bConfigurationValue, "%02Xh\n");
	DESC_UNI_STRID(desc, Config, iConfiguration, "");
	DESC_UNI_VALUE(desc, Config, bmAttributes, "%02Xh\n");
	DESC_UNI_VALUE(desc, Config, MaxPower, "%u (%u ma)\n", (*desc)->Config.MaxPower * 2);

	AdvanceUniDescriptor(*desc, *remainingLength);

	while ((success = IsUniDescriptorValid(*desc, *remainingLength)) == TRUE)
	{
		switch ((*desc)->Common.bDescriptorType)
		{
		case USB_INTERFACE_DESCRIPTOR_TYPE:
			if ((--numInterfaces) < 0)
			{
				printf("Config descriptor is mis-reporting bNumInterfaces.\n");
				return FALSE;
			}
			success = DumpDescriptorInterface(desc, remainingLength);
			break;
		case USB_INTERFACEASSOCIATION_DESCRIPTOR_TYPE:
			success = DumpDescriptorInterfaceAssociation(desc, remainingLength);
			break;
		default:
			success = DumpDescriptorCommon(desc, remainingLength);
			break;
		}
		if (!success)
			break;
	}

	DESC_END();

	return success;
}

BOOL DumpDescriptorInterfaceAssociation(PPUNI_DESCRIPTOR desc, PLONG remainingLength)
{
	BOOL success = TRUE;

	if (!IsUniDescriptorValid(*desc, *remainingLength))
		return FALSE;

	if (!BeginDescriptor(USB_INTERFACEASSOCIATION_DESCRIPTOR_TYPE, desc))
		return FALSE;

	DESC_UNI_VALUE(desc, InterfaceAssociation, bLength, "%u\n");
	DESC_UNI_VALUE(desc, InterfaceAssociation, bDescriptorType, "%02Xh\n");
	DESC_UNI_VALUE(desc, InterfaceAssociation, bFirstInterface, "%02Xh\n");
	DESC_UNI_VALUE(desc, InterfaceAssociation, bInterfaceCount, "%u\n");
	DESC_UNI_VALUE(desc, InterfaceAssociation, bFunctionClass, "%02Xh\n");
	DESC_UNI_VALUE(desc, InterfaceAssociation, bFunctionSubClass, "%02Xh\n");
	DESC_UNI_VALUE(desc, InterfaceAssociation, bFunctionProtocol, "%02Xh\n");
	DESC_UNI_STRID(desc, InterfaceAssociation, iFunction, "");

	AdvanceUniDescriptor(*desc, *remainingLength);

	DESC_END();

	return success;
}

BOOL DumpDescriptorInterface(PPUNI_DESCRIPTOR desc, PLONG remainingLength)
{
	BOOL success = TRUE;
	CHAR className[MAX_PATH];
	CHAR subClassName[MAX_PATH];
	CHAR protocolName[MAX_PATH];
	INT numEndpoints = (INT)((*desc)->Interface.bNumEndpoints);

	if (!IsUniDescriptorValid(*desc, *remainingLength))
		return FALSE;

	if (!BeginDescriptor(USB_INTERFACE_DESCRIPTOR_TYPE, desc))
		return FALSE;

	GetClassDisplayText(
	    (*desc)->Interface.bInterfaceClass,
	    (*desc)->Interface.bInterfaceSubClass,
	    (*desc)->Interface.bInterfaceProtocol,
	    className,
	    subClassName,
	    protocolName);

	DESC_UNI_VALUE(desc, Interface, bLength, "%u\n");
	DESC_UNI_VALUE(desc, Interface, bDescriptorType, "%02Xh\n");
	DESC_UNI_VALUE(desc, Interface, bInterfaceNumber, "%02u\n");
	DESC_UNI_VALUE(desc, Interface, bAlternateSetting, "%02u\n");
	DESC_UNI_VALUE(desc, Interface, bNumEndpoints, "%u\n");
	DESC_UNI_VALUE(desc, Interface, bInterfaceClass, "%02Xh %s\n", className);
	DESC_UNI_VALUE(desc, Interface, bInterfaceSubClass, "%02Xh %s\n", subClassName);
	DESC_UNI_VALUE(desc, Interface, bInterfaceProtocol, "%02Xh %s\n", protocolName);
	DESC_UNI_STRID(desc, Interface, iInterface, "");

	AdvanceUniDescriptor(*desc, *remainingLength);

	while ((success = IsUniDescriptorValid(*desc, *remainingLength)) == TRUE)
	{
		switch ((*desc)->Common.bDescriptorType)
		{

		case USB_ENDPOINT_DESCRIPTOR_TYPE:
			if ((--numEndpoints) < 0)
			{
				printf("Interface descriptor is mis-reporting bNumEndpoints.\n");
				return FALSE;
			}
			success = DumpDescriptorEndpoint(desc, remainingLength);
			break;

		case USB_HID_DESCRIPTOR_TYPE:
			success = DumpDescriptorHid(desc, remainingLength);
			break;

		default:
			DESC_END();
			return TRUE;
		}
		if (!success)
			break;
	}

	DESC_END();

	return success;
}
BOOL DumpDescriptorHid(PPUNI_DESCRIPTOR desc, PLONG remainingLength)
{
	BOOL success = TRUE;

	if (!IsUniDescriptorValid(*desc, *remainingLength))
		return FALSE;

	if (!BeginDescriptor(USB_HID_DESCRIPTOR_TYPE, desc))
		return FALSE;

	DESC_UNI_VALUE(desc, Hid, bLength, "%u\n");
	DESC_UNI_VALUE(desc, Hid, bDescriptorType, "%02Xh\n");
	DESC_UNI_VALUE(desc, Hid, bcdHID, "%04X\n");
	DESC_UNI_VALUE(desc, Hid, bCountry, "%02Xh\n");
	DESC_UNI_VALUE(desc, Hid, bNumDescriptors, "%u\n");

	AdvanceUniDescriptor(*desc, *remainingLength);

	DESC_END();

	return success;
}
LPCSTR GetBmAttributes(UCHAR bmAttributes)
{

	static CHAR rtn[128];

	memset(rtn, 0, sizeof(rtn));
	strcat_s(rtn, sizeof(rtn) - 1, GetPipeTypeString(bmAttributes));

	if ((bmAttributes & 0x03) == 0x01)
	{
		// isochronous
		strcat_s(rtn, sizeof(rtn) - 1, ", ");
		strcat_s(rtn, sizeof(rtn) - 1, GetIsoSyncronizationString(bmAttributes));
		strcat_s(rtn, sizeof(rtn) - 1, ", ");
		strcat_s(rtn, sizeof(rtn) - 1, GetIsoUsageTypeString(bmAttributes));
	}

	return rtn;
}

BOOL DumpDescriptorEndpoint(PPUNI_DESCRIPTOR desc, PLONG remainingLength)
{
	BOOL success = TRUE;

	if (!IsUniDescriptorValid(*desc, *remainingLength))
		return FALSE;

	if (!BeginDescriptor(USB_ENDPOINT_DESCRIPTOR_TYPE, desc))
		return FALSE;

	DESC_UNI_VALUE(desc, Endpoint, bLength, "%u\n");
	DESC_UNI_VALUE(desc, Endpoint, bDescriptorType, "%02Xh\n");
	DESC_UNI_VALUE(desc, Endpoint, bEndpointAddress, "%02Xh\n");
	DESC_UNI_VALUE(desc, Endpoint, bmAttributes, "%02Xh (%s)\n", GetBmAttributes((*desc)->Endpoint.bmAttributes));
	DESC_UNI_VALUE(desc, Endpoint, wMaxPacketSize, "%u\n");
	DESC_UNI_VALUE(desc, Endpoint, bInterval, "%02Xh\n");

	AdvanceUniDescriptor(*desc, *remainingLength);

	DESC_END();
	return success;
}
BOOL DumpDescriptorCommon(PPUNI_DESCRIPTOR desc, PLONG remainingLength)
{
	BOOL success = TRUE;

	if (!IsUniDescriptorValid(*desc, *remainingLength))
		return FALSE;

	DESC_SUB_BEGIN(0xff);

	DESC_UNI_VALUE(desc, Common, bLength, "%u\n");
	DESC_UNI_VALUE(desc, Common, bDescriptorType, "%02Xh\n");

	AdvanceUniDescriptor(*desc, *remainingLength);

	DESC_END();

	return success;
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
