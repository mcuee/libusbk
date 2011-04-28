/*!********************************************************************
libusbK - Multi-driver USB library.
Copyright (C) 2011 All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen     (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/

#ifndef __LUSBK_STACK_COLLECTION_
#define __LUSBK_STACK_COLLECTION_

#include "lusbk_private.h"
#include "lusbk_common.h"
#include "lusbk_linked_list.h"

/*!********************************************************************
Interface stack structures for libusbK, libusb0, and WinUSB
********************************************************************!*/

#define GetPipeDeviceHandle(UsbStack, PipeNumber)	\
	(&(UsbStack))->PipeNumberCache[((((PipeNumber) & 0xF)|(((PipeNumber)>>3) & 0x10)) & 0x1F)].DeviceHandle

#define GetPipeInterfaceHandle(UsbStack, PipeNumber)	\
	(&(UsbStack))->PipeNumberCache[((((PipeNumber) & 0xF)|(((PipeNumber)>>3) & 0x10)) & 0x1F)].InterfaceHandle

typedef enum _USB_STACK_HANDLER_RESULT
{
	HANDLER_HANDLED_WITH_FALSE,
	HANDLER_HANDLED_WITH_TRUE,
	HANDLER_NOT_HANDLED,

} USB_STACK_HANDLER_RESULT;

typedef struct _KUSB_PIPE_CACHE
{
	INT Index;
	INT Number;

	HANDLE DeviceHandle;
	HANDLE InterfaceHandle;
} KUSB_PIPE_CACHE, *PKUSB_PIPE_CACHE;

typedef struct _KUSB_PIPE_EL
{
	INT Index;
	INT Number;
	USB_ENDPOINT_DESCRIPTOR Descriptor;

	struct _KUSB_PIPE_EL* next;
	struct _KUSB_PIPE_EL* prev;
} KUSB_PIPE_EL, *PKUSB_PIPE_EL;


typedef struct _KUSB_ALT_INTERFACE_EL
{
	INT Index;
	INT Number;
	USB_INTERFACE_DESCRIPTOR Descriptor;
	PKUSB_PIPE_EL PipeList;

	struct _KUSB_ALT_INTERFACE_EL* next;
	struct _KUSB_ALT_INTERFACE_EL* prev;

} KUSB_ALT_INTERFACE_EL, *PKUSB_ALT_INTERFACE_EL;

typedef struct _KUSB_DEVICE_EL
{
	PKUSB_SHARED_DEVICE Device;

	PVOID Context;

	struct _KUSB_DEVICE_EL* next;
	struct _KUSB_DEVICE_EL* prev;

} KUSB_DEVICE_EL, *PKUSB_DEVICE_EL;

typedef struct _KUSB_INTERFACE_EL
{
	INT CurrentAltSetting;

	INT Index;
	INT Number;

	INT VirtualIndex;
	PKUSB_ALT_INTERFACE_EL AltInterfaceList;

	PKUSB_DEVICE_EL ParentDevice;

	struct _KUSB_INTERFACE_EL* next;
	struct _KUSB_INTERFACE_EL* prev;

} KUSB_INTERFACE_EL, *PKUSB_INTERFACE_EL;

typedef struct _KUSB_ASSOCIATED_INTERFACE_EL
{
	HANDLE DeviceHandle;
	HANDLE ParentHandle;
	HANDLE Handle;
	INT Index;

	struct _KUSB_ASSOCIATED_INTERFACE_EL* next;
	struct _KUSB_ASSOCIATED_INTERFACE_EL* prev;

} KUSB_ASSOCIATED_INTERFACE_EL, *PKUSB_ASSOCIATED_INTERFACE_EL;

typedef struct _KUSB_INTERFACE_STACK
{
	KUSB_PIPE_CACHE PipeNumberCache[32];

	struct
	{
		LONG (*GetConfigDescriptor)		(struct _KUSB_INTERFACE_STACK*, LPCSTR, HANDLE, HANDLE, PUSB_CONFIGURATION_DESCRIPTOR*, PVOID);
		LONG (*ClaimReleaseInterface)	(struct _KUSB_INTERFACE_STACK*, PKUSB_INTERFACE_EL, BOOL, INT, BOOL, PVOID);
		LONG (*OpenInterface)			(struct _KUSB_INTERFACE_STACK*, HANDLE, INT, PHANDLE, PVOID);
		LONG (*CloseInterface)			(struct _KUSB_INTERFACE_STACK*, PKUSB_INTERFACE_EL, PVOID);
		LONG (*CloseDevice)				(struct _KUSB_INTERFACE_STACK*, PKUSB_DEVICE_EL, PVOID);
	} Cb;

	PUSB_CONFIGURATION_DESCRIPTOR DynamicConfigDescriptor;
	ULONG DynamicConfigDescriptorSize;

	PKUSB_INTERFACE_EL InterfaceList;
	PKUSB_DEVICE_EL DeviceList;
	PKUSB_ASSOCIATED_INTERFACE_EL AssociatedList;

	ULONG DeviceCount;

} KUSB_INTERFACE_STACK, *PKUSB_INTERFACE_STACK;

typedef LONG KUSB_INTERFACE_CB (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in PKUSB_INTERFACE_EL InterfaceElement,
    __in BOOL IsClaim,
    __in INT InterfaceIndexOrNumber,
    __in BOOL IsIndex,
    __in PVOID Context);

typedef LONG KUSB_DEVICE_ACTION_CB (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in PKUSB_DEVICE_EL DeviceElement,
    __in PVOID Context);

typedef LONG KUSB_INTERFACE_ACTION_CB (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in PKUSB_INTERFACE_EL InterfaceElement,
    __in PVOID Context);

typedef LONG KUSB_SET_ALT_INTERFACE_CB (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in PKUSB_INTERFACE_EL InterfaceElement,
    __in PKUSB_ALT_INTERFACE_EL AltInterfaceElement,
    __in INT InterfaceIndexOrNumber,
    __in BOOL IsInterfaceIndex,
    __in INT AltSettingIndexOrNumber,
    __in BOOL IsAltSettingIndex,
    __in_opt PVOID Context);

typedef LONG KUSB_GET_ALT_INTERFACE_CB (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in PKUSB_INTERFACE_EL InterfaceElement,
    __in INT InterfaceIndexOrNumber,
    __in BOOL IsInterfaceIndex,
    __in_opt PVOID Context);

typedef LONG KUSB_GET_CFG_DESCRIPTOR_CB (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in_opt LPCSTR DevicePath,
    __in_opt HANDLE DeviceHandle,
    __in_opt HANDLE InterfaceHandle,
    __out PUSB_CONFIGURATION_DESCRIPTOR* ConfigDescriptorRef,
    __in_opt PVOID Context);

typedef LONG KUSB_OPEN_ASSOCIATED_INTERFACE_CB (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in HANDLE DeviceOrInterfaceHandle,
    __in INT InterfaceIndex,
    __out PHANDLE AssociatedHandleRef,
    __in_opt PVOID Context);

typedef LONG KUSB_CLOSE_ASSOCIATED_INTERFACE_CB (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in PKUSB_INTERFACE_EL InterfaceElement,
    __in_opt PVOID Context);

typedef KUSB_INTERFACE_CB* PKUSB_INTERFACE_CB;
typedef KUSB_SET_ALT_INTERFACE_CB* PKUSB_SET_ALT_INTERFACE_CB;
typedef KUSB_GET_ALT_INTERFACE_CB* PKUSB_GET_ALT_INTERFACE_CB;
typedef KUSB_DEVICE_ACTION_CB* PKUSB_DEVICE_ACTION_CB;
typedef KUSB_INTERFACE_ACTION_CB* PKUSB_INTERFACE_ACTION_CB;
typedef KUSB_GET_CFG_DESCRIPTOR_CB* PKUSB_GET_CFG_DESCRIPTOR_CB;
typedef KUSB_OPEN_ASSOCIATED_INTERFACE_CB* PKUSB_OPEN_ASSOCIATED_INTERFACE_CB;

BOOL UsbStack_Init(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in PKUSB_GET_CFG_DESCRIPTOR_CB GetConfigDescriptorCB,
    __in PKUSB_INTERFACE_CB ClaimReleaseInterfaceCB,
    __in_opt PKUSB_OPEN_ASSOCIATED_INTERFACE_CB OpenInterfaceCB,
    __in_opt PKUSB_INTERFACE_ACTION_CB CloseInterfaceCB,
    __in_opt PKUSB_DEVICE_ACTION_CB CloseDeviceCB);

BOOL UsbStack_Free(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in_opt PVOID Context);

BOOL UsbStack_Add(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in PKUSB_SHARED_DEVICE SharedDevice,
    __in_opt PVOID Context);

BOOL UsbStack_AddDevice(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in_opt LPCSTR DevicePath,
    __in_opt HANDLE DeviceHandle,
    __in_opt PVOID Context);

LONG UsbStack_ClaimOrRelease(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in BOOL IsClaim,
    __in INT InterfaceIndexOrNumber,
    __in BOOL IsIndex,
    __in_opt PVOID Context);

BOOL UsbStack_BuildCache(
    __in PKUSB_INTERFACE_STACK UsbStack);


BOOL UsbStack_GetCache(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __inout_opt PKUSB_DEVICE_EL Device,
    __inout_opt PKUSB_INTERFACE_EL Interface);

VOID UsbStack_CloneList(
    __in PKUSB_INTERFACE_EL InterfaceListSrcPtr,
    __inout PKUSB_INTERFACE_EL* InterfaceLisDstPtr);

VOID UsbStack_Clone(
    __in PKUSB_INTERFACE_STACK SrcUsbStack,
    __in PKUSB_INTERFACE_STACK DstUsbStack);

USB_STACK_HANDLER_RESULT UsbStackHandler_GetDescriptor (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in UCHAR DescriptorType,
    __in UCHAR Index,
    __in USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out PULONG LengthTransferred);

USB_STACK_HANDLER_RESULT UsbStackHandler_SetAltInterface(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in INT InterfaceIndexOrNumber,
    __in BOOL IsInterfaceIndex,
    __in INT AltSettingIndexOrNumber,
    __in BOOL IsAltSettingIndex);

USB_STACK_HANDLER_RESULT UsbStackHandler_GetAltInterface(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in INT InterfaceIndexOrNumber,
    __in BOOL IsInterfaceIndex,
    __out PINT AltSettingNumber);

USB_STACK_HANDLER_RESULT UsbStackHandler_QueryInterfaceSettings (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in  UCHAR AlternateSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);

USB_STACK_HANDLER_RESULT UsbStackHandler_QueryPipe (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in  UCHAR AlternateSettingNumber,
    __in  UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation);

USB_STACK_HANDLER_RESULT UsbStackHandler_GetConfiguration (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __out PUCHAR ConfigurationNumber);

USB_STACK_HANDLER_RESULT UsbStackHandler_SetConfiguration (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in UCHAR ConfigurationNumber);

#endif
