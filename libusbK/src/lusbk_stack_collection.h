/*!********************************************************************
libusbK - Multi-driver USB library.
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

#ifndef __LUSBK_STACK_COLLECTION_
#define __LUSBK_STACK_COLLECTION_

#include "lusbk_handles.h"
#include "lusbk_private.h"
#include "lusbk_linked_list.h"

typedef BOOL KUSB_STACK_CB(HANDLE MasterHandle);
typedef KUSB_STACK_CB* PKUSB_STACK_CB;

BOOL UsbStack_Init(
    __out		KUSB_HANDLE* Handle,
    __in		KUSB_DRVID DriverID,
    __in		BOOL UsePipeCache,
    __in_opt	HANDLE DeviceHandle,
    __in_opt	KLST_DEVINFO_HANDLE DevInfo,
    __in_opt	PKDEV_HANDLE_INTERNAL SharedDevice,
    __in		PKUSB_STACK_CB Init_ConfigCB,
    __in_opt	PKUSB_STACK_CB Init_BackendCB,
    __in		PKOBJ_CB Cleanup_UsbK,
    __in		PKOBJ_CB Cleanup_DevK);

BOOL UsbStack_GetAssociatedInterface (
    __in KUSB_HANDLE Handle,
    __in UCHAR AssociatedInterfaceIndex,
    __out KUSB_HANDLE* AssociatedHandle);

BOOL UsbStack_CloneHandle (
    __in KUSB_HANDLE Handle,
    __out KUSB_HANDLE* ClonedHandle);

VOID UsbStack_Clear(PKUSB_INTERFACE_STACK UsbStack);

BOOL UsbStack_Rebuild(
    __in PKUSB_HANDLE_INTERNAL Handle,
    __in PKUSB_STACK_CB Init_ConfigCB);

BOOL UsbStack_QueryInterfaceSettings (
    __in KUSB_HANDLE Handle,
    __in UCHAR AltSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);

BOOL UsbStack_QueryPipe(
    __in KUSB_HANDLE Handle,
    __in UCHAR AltSettingNumber,
    __in UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation);

BOOL UsbStack_SelectInterface (
    __in KUSB_HANDLE Handle,
    __in UCHAR IndexOrNumber,
    __in BOOL IsIndex);

BOOL UsbStack_RefreshPipeCache(PKUSB_HANDLE_INTERNAL Handle);

BOOL UsbStack_QuerySelectedEndpoint(
    __in KUSB_HANDLE Handle,
    __in UCHAR EndpointAddressOrIndex,
    __in BOOL IsIndex,
    __out PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor);

#endif
