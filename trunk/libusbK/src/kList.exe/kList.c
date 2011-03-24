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

#include "lusbk_usb.h"
#include "lusbk_version.h"

KUSB_DRIVER_API K;
LONG WinError(__in_opt DWORD errorCode);

BOOL GetDescriptorReport(__in PKUSB_DEV_LIST deviceElement,
                         __in PKUSB_DRIVER_API driverApi,
                         __in BOOL detailed);

BOOL GetRealConfigDescriptor(__in  PKUSB_DRIVER_API driverApi, 
							 __in WINUSB_INTERFACE_HANDLE InterfaceHandle, 
							 __in UCHAR Index,
							 __out_opt PUCHAR Buffer, 
							 __in ULONG BufferLength, 
							 __out PULONG LengthTransferred);


static LPCSTR gTabIndents[] = {"", "  ", "    ", "      ", "        ", "          ", "            "};
static UCHAR gTab = 0;

static LPCSTR DrvIdNames[8] = {"libusbK", "libusb0", "libusb0 filter", "WinUSB", "Unknown", "Unknown", "Unknown", "Unknown"};
#define GetDrvIdString(DrvId)	(DrvIdNames[((((LONG)(DrvId))<0) || ((LONG)(DrvId)) >= KUSB_DRVID_COUNT)?KUSB_DRVID_COUNT:(DrvId)])

#define GetPipeTypeString(PipeType) PipeTypeStrings[(PipeType) & 0x7]
CONST PCHAR PipeTypeStrings[8] = {"Control", "Isochronous", "Bulk", "Interrupt", "na", "na", "na", "na"};

#define PrintfDeviceElement(DeviceListFieldName) printf("    %-21s: %s\n",DEFINE_TO_STR(DeviceListFieldName),deviceElement->DeviceListFieldName)
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

	ShowCopyright();

	ec = LUsbK_GetDeviceList(NULL, &deviceList);
	if (ec < 0)
	{
		printf("failed getting device list.\n");
		return WinError(0);
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
	if (!GetDescriptorReport(deviceElement, &K, TRUE))
	{
		ec = WinError(0);
		goto Done;
	}

Done:
	LUsbK_FreeDeviceList(&deviceList);
	return ec;
}

BOOL GetRealConfigDescriptor(__in  PKUSB_DRIVER_API driverApi, 
							 __in WINUSB_INTERFACE_HANDLE InterfaceHandle, 
							 __in UCHAR Index,
							 __out_opt PUCHAR Buffer, 
							 __in ULONG BufferLength, 
							 __out PULONG LengthTransferred)
{
	USB_DEFAULT_PIPE_SETUP_PACKET Pkt;
	WINUSB_SETUP_PACKET Pkt2;
	
	memset(&Pkt,0,sizeof(Pkt));
	Pkt.bmRequestType.Dir=BMREQUEST_DEVICE_TO_HOST;
	Pkt.bRequest=USB_REQUEST_GET_DESCRIPTOR;
	Pkt.wValue.HiByte=USB_CONFIGURATION_DESCRIPTOR_TYPE;
	Pkt.wValue.LowByte=Index;
	Pkt.wLength=(USHORT)BufferLength;


	memcpy(&Pkt2,&Pkt,sizeof(Pkt2));
	return driverApi->ControlTransfer(InterfaceHandle,Pkt2,Buffer,BufferLength,LengthTransferred,NULL);
}

BOOL GetDescriptorReport(__in PKUSB_DEV_LIST deviceElement, __in  PKUSB_DRIVER_API driverApi, __in BOOL detailed)
{
#define START_DESC(DisplayName) printf("\n%s[%s]\n",gTabIndents[gTab++], DisplayName)
#define DESC_VALUE(Descriptor,FieldName,format,...) printf("%s%-18s:"format, gTabIndents[gTab], DEFINE_TO_STR(FieldName),Descriptor.FieldName,__VA_ARGS__)
#define DESC_PVALUE(Descriptor,FieldName,format,...) printf("%s%-18s:"format, gTabIndents[gTab], DEFINE_TO_STR(FieldName),Descriptor->FieldName,__VA_ARGS__)
#define END_DESC() (--gTab)

	UCHAR altSetting;
	UCHAR interfaceIndex = UCHAR_MAX;
	HANDLE fileHandle = NULL;
	WINUSB_INTERFACE_HANDLE handle = NULL;
	WINUSB_INTERFACE_HANDLE associatedHandle = NULL;
	ULONG length;
	USB_DEVICE_DESCRIPTOR deviceDescriptor;
	USB_INTERFACE_DESCRIPTOR interfaceDescriptor;
	WINUSB_PIPE_INFORMATION pipeInfo;
	BOOL success = TRUE;
	PUSB_CONFIGURATION_DESCRIPTOR configDescriptor = NULL;
	if (deviceElement)
	{
		fileHandle = CreateFileA(deviceElement->DevicePath,
		                         GENERIC_READ | GENERIC_WRITE,
		                         FILE_SHARE_READ | FILE_SHARE_WRITE,
		                         NULL,
		                         OPEN_EXISTING,
		                         FILE_FLAG_OVERLAPPED,
		                         NULL);

		if (!fileHandle || fileHandle == INVALID_HANDLE_VALUE)
		{
			fileHandle = NULL;
			printf("failed creating device file handle:\n:%s\n", deviceElement->SymbolicLink);
			success = FALSE;
			goto Error;
		}
		if (!driverApi->Initialize(fileHandle, &handle))
		{
			WinError(0);
			goto Error;
		}

		if (!driverApi->GetDescriptor(handle,
		                              USB_DEVICE_DESCRIPTOR_TYPE,
		                              0, 0,
		                              (PUCHAR)&deviceDescriptor,
		                              sizeof(deviceDescriptor),
		                              &length))
		{
			WinError(0);
			return FALSE;
		}

		// TODO: show device descriptor
		START_DESC("Device Descriptor");
		DESC_VALUE(deviceDescriptor, bLength, "%u\n");
		DESC_VALUE(deviceDescriptor, bDescriptorType, "%u\n");
		DESC_VALUE(deviceDescriptor, bcdUSB, "%04Xh\n");
		DESC_VALUE(deviceDescriptor, bDeviceClass, "%02Xh\n");
		DESC_VALUE(deviceDescriptor, bDeviceProtocol, "%02Xh\n");
		DESC_VALUE(deviceDescriptor, bMaxPacketSize0, "%u\n");
		DESC_VALUE(deviceDescriptor, idVendor, "%04Xh\n");
		DESC_VALUE(deviceDescriptor, idProduct, "%04Xh\n");
		DESC_VALUE(deviceDescriptor, bcdDevice, "%04X\n");
		DESC_VALUE(deviceDescriptor, iManufacturer, "%u\n");
		DESC_VALUE(deviceDescriptor, iSerialNumber, "%u\n");
		DESC_VALUE(deviceDescriptor, bNumConfigurations, "%u\n");

		if (detailed)
		{
			LONG size = 0;

			configDescriptor = malloc(4096);
			memset(configDescriptor, 0, 4096);

			if (GetRealConfigDescriptor(driverApi,handle,0,(PUCHAR)configDescriptor,4096,&length))
			/*
			if (driverApi->GetDescriptor(handle,
			                             USB_CONFIGURATION_DESCRIPTOR_TYPE,
			                             ++configIndex, 0,
			                             (PUCHAR)configDescriptor,
			                             4096,
			                             &length))
			 */
			{
				PUSB_COMMON_DESCRIPTOR hdr = (PUSB_COMMON_DESCRIPTOR)configDescriptor;
				PUCHAR uc = (PUCHAR)hdr;
				size = (LONG)length;
				START_DESC("Config Descriptor");
				DESC_PVALUE(configDescriptor, bLength, "%u\n");
				DESC_PVALUE(configDescriptor, bDescriptorType, "%02Xh\n");
				DESC_PVALUE(configDescriptor, wTotalLength, "%u\n");
				DESC_PVALUE(configDescriptor, bNumInterfaces, "%u\n");
				DESC_PVALUE(configDescriptor, bConfigurationValue, "%02Xh\n");
				DESC_PVALUE(configDescriptor, iConfiguration, "%u\n");
				DESC_PVALUE(configDescriptor, bmAttributes, "%02Xh\n");
				DESC_PVALUE(configDescriptor, MaxPower, "%u\n");


				length = sizeof(USB_CONFIGURATION_DESCRIPTOR);
				uc += length;
				size -= length;
				hdr = (PUSB_COMMON_DESCRIPTOR)uc;
				while((size > sizeof(USB_COMMON_DESCRIPTOR)) && size >= hdr->bLength)
				{
					switch(hdr->bDescriptorType)
					{
					case USB_INTERFACE_DESCRIPTOR_TYPE:
						START_DESC("Interface Descriptor");
						DESC_PVALUE(((PUSB_INTERFACE_DESCRIPTOR)hdr), bLength, "%u\n");
						DESC_PVALUE(((PUSB_INTERFACE_DESCRIPTOR)hdr), bDescriptorType, "%02Xh\n");
						DESC_PVALUE(((PUSB_INTERFACE_DESCRIPTOR)hdr), bInterfaceNumber, "%02u\n");
						DESC_PVALUE(((PUSB_INTERFACE_DESCRIPTOR)hdr), bAlternateSetting, "%02u\n");
						DESC_PVALUE(((PUSB_INTERFACE_DESCRIPTOR)hdr), bNumEndpoints, "%u\n");
						DESC_PVALUE(((PUSB_INTERFACE_DESCRIPTOR)hdr), bInterfaceClass, "%02Xh\n");
						DESC_PVALUE(((PUSB_INTERFACE_DESCRIPTOR)hdr), bInterfaceSubClass, "%02Xh\n");
						DESC_PVALUE(((PUSB_INTERFACE_DESCRIPTOR)hdr), bInterfaceProtocol, "%02Xh\n");
						DESC_PVALUE(((PUSB_INTERFACE_DESCRIPTOR)hdr), iInterface, "%u\n");
						END_DESC();
						break;
					case USB_ENDPOINT_DESCRIPTOR_TYPE:
						START_DESC("Endpoint Descriptor");
						DESC_PVALUE(((PUSB_ENDPOINT_DESCRIPTOR)hdr), bLength, "%u\n");
						DESC_PVALUE(((PUSB_ENDPOINT_DESCRIPTOR)hdr), bDescriptorType, "%02Xh\n");
						DESC_PVALUE(((PUSB_ENDPOINT_DESCRIPTOR)hdr), bEndpointAddress, "%02Xh\n");
						DESC_PVALUE(((PUSB_ENDPOINT_DESCRIPTOR)hdr), bmAttributes, "%02Xh (%s)\n", GetPipeTypeString(((PUSB_ENDPOINT_DESCRIPTOR)hdr)->bmAttributes));
						DESC_PVALUE(((PUSB_ENDPOINT_DESCRIPTOR)hdr), wMaxPacketSize, "%u\n");
						DESC_PVALUE(((PUSB_ENDPOINT_DESCRIPTOR)hdr), bInterval, "%02Xh\n");
						END_DESC();
						break;

					}
					uc += hdr->bLength;
					size -= hdr->bLength;
					hdr = (PUSB_COMMON_DESCRIPTOR)uc;
				}

				END_DESC();
			}
			success = TRUE;
			goto Done;
		}

NextInterface:
		memset(&interfaceDescriptor, 0, sizeof(interfaceDescriptor));
		altSetting = 0;
		while(driverApi->QueryInterfaceSettings(handle, altSetting, &interfaceDescriptor))
		{
			UCHAR pipeIndex = 0;

			// show interface descriptor
			START_DESC("Interface Descriptor");
			DESC_VALUE(interfaceDescriptor, bLength, "%u\n");
			DESC_VALUE(interfaceDescriptor, bDescriptorType, "%02Xh\n");
			DESC_VALUE(interfaceDescriptor, bInterfaceNumber, "%02u\n");
			DESC_VALUE(interfaceDescriptor, bAlternateSetting, "%02u\n");
			DESC_VALUE(interfaceDescriptor, bNumEndpoints, "%u\n");
			DESC_VALUE(interfaceDescriptor, bInterfaceClass, "%02Xh\n");
			DESC_VALUE(interfaceDescriptor, bInterfaceSubClass, "%02Xh\n");
			DESC_VALUE(interfaceDescriptor, bInterfaceProtocol, "%02Xh\n");
			DESC_VALUE(interfaceDescriptor, iInterface, "%u\n");

			memset(&pipeInfo, 0, sizeof(pipeInfo));
			while(driverApi->QueryPipe(handle, altSetting, pipeIndex, &pipeInfo))
			{
				// TODO: show pipe information
				START_DESC("Pipe Info");
				DESC_VALUE(pipeInfo, PipeType, "%02Xh (%s)\n", GetPipeTypeString(pipeInfo.PipeType));
				DESC_VALUE(pipeInfo, PipeId, "%02Xh\n");
				DESC_VALUE(pipeInfo, MaximumPacketSize, "%u\n");
				DESC_VALUE(pipeInfo, Interval, "%u\n");
				END_DESC();

				pipeIndex++;
			}
			altSetting++;
			memset(&interfaceDescriptor, 0, sizeof(interfaceDescriptor));
		}
		END_DESC();

		associatedHandle = NULL;
		if (driverApi->GetAssociatedInterface(handle, ++interfaceIndex, &associatedHandle))
		{
			// this device has more interfaces to look at.
			//
			driverApi->Free(handle);
			handle = associatedHandle;
			goto NextInterface;
		}

	}

	END_DESC(); // Device
Done:
	success = TRUE;
Error:
	if (driverApi)
	{
		if (handle)
			driverApi->Free(handle);

		if (fileHandle)
			CloseHandle(fileHandle);
	}
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
