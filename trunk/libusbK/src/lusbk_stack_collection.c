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

#include "lusbk_private.h"
#include "lusbk_handles.h"
#include "lusbk_stack_collection.h"

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#ifndef DESCRIPTOR_PARSE_BUILD_STACK___________________________________

typedef struct _DESCRIPTOR_ITERATOR
{
	LONG	Remaining;

	UCHAR NextInterfaceIndex;
	UCHAR NextAltInterfaceIndex;
	UCHAR NextPipeIndex;

	union
	{
		PUCHAR						Offset;

		PUSB_COMMON_DESCRIPTOR		Comn;
		PUSB_INTERFACE_DESCRIPTOR	Intf;
		PUSB_ENDPOINT_DESCRIPTOR	Pipe;
	} Ptr;
} DESCRIPTOR_ITERATOR, *PDESCRIPTOR_ITERATOR;

static BOOL u_Desc_Next(PDESCRIPTOR_ITERATOR desc)
{
	if (desc->Remaining < sizeof(USB_COMMON_DESCRIPTOR)) return FALSE;

	desc->Remaining -= desc->Ptr.Comn->bLength;

	if (desc->Remaining >= sizeof(USB_COMMON_DESCRIPTOR))
	{
		desc->Ptr.Offset += desc->Ptr.Comn->bLength;
		return TRUE;
	}
	return FALSE;
}

static void u_Add_Pipe(PKUSB_ALT_INTERFACE_EL altInterfaceEL, PDESCRIPTOR_ITERATOR desc)
{
	PKUSB_PIPE_EL pipeEL;
	DESCRIPTOR_ITERATOR descPrev;

	memcpy(&descPrev, desc, sizeof(descPrev));

	while (u_Desc_Next(desc))
	{
		if (desc->Ptr.Comn->bDescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
		{
			memcpy(desc, &descPrev, sizeof(*desc));
			break;
		}
		else if (desc->Ptr.Comn->bDescriptorType == USB_DESCRIPTOR_TYPE_ENDPOINT)
		{
			FindPipeEL(altInterfaceEL, pipeEL, FALSE, desc->Ptr.Pipe->bEndpointAddress);
			if (!pipeEL)
			{
				pipeEL = Mem_Alloc(sizeof(*pipeEL));
				if (!pipeEL) return;

				pipeEL->Descriptor	= desc->Ptr.Pipe;
				pipeEL->ID			= pipeEL->Descriptor->bEndpointAddress;
				pipeEL->Index		= desc->NextPipeIndex++;

				DL_APPEND(altInterfaceEL->PipeList, pipeEL);
			}
		}
		memcpy(&descPrev, desc, sizeof(descPrev));
	}
}

static void u_Add_Interface(PKUSB_HANDLE_INTERNAL Handle, PDESCRIPTOR_ITERATOR desc)
{
	PKUSB_INTERFACE_EL interfaceEL;
	PKUSB_ALT_INTERFACE_EL altInterfaceEL;

	FindInterfaceEL(Handle->Device->UsbStack, interfaceEL, FALSE, desc->Ptr.Intf->bInterfaceNumber);
	if (!interfaceEL)
	{
		interfaceEL = Mem_Alloc(sizeof(*interfaceEL));
		if (!interfaceEL) return;

		interfaceEL->ID		= desc->Ptr.Intf->bInterfaceNumber;
		interfaceEL->Index	= desc->NextInterfaceIndex++;

		interfaceEL->SharedInterface		= &Get_SharedInterface(Handle, interfaceEL->Index);
		interfaceEL->SharedInterface->ID	= interfaceEL->ID;
		interfaceEL->SharedInterface->Index	= interfaceEL->Index;

		desc->NextAltInterfaceIndex = 0;

		DL_APPEND(Handle->Device->UsbStack->InterfaceList,	interfaceEL);
	}

	FindAltInterfaceEL(interfaceEL, altInterfaceEL, FALSE, desc->Ptr.Intf->bAlternateSetting);
	if (!altInterfaceEL)
	{
		altInterfaceEL = Mem_Alloc(sizeof(*altInterfaceEL));
		if (!altInterfaceEL) return;

		altInterfaceEL->Descriptor	= desc->Ptr.Intf;
		altInterfaceEL->ID			= desc->Ptr.Intf->bAlternateSetting;
		altInterfaceEL->Index		= desc->NextAltInterfaceIndex++;

		desc->NextPipeIndex			= 0;

		DL_APPEND(Handle->Device->UsbStack->InterfaceList->AltInterfaceList, altInterfaceEL);
	}

	u_Add_Pipe(altInterfaceEL, desc);
}

static BOOL u_Init_Config(__in PKUSB_HANDLE_INTERNAL Handle)
{
	DWORD errorCode = ERROR_SUCCESS;
	DESCRIPTOR_ITERATOR desc;
	PUSB_CONFIGURATION_DESCRIPTOR cfg = Handle->Device->ConfigDescriptor;

	Mem_Zero(&desc, sizeof(desc));
	desc.Ptr.Offset = (PUCHAR)cfg;
	desc.Remaining = (LONG)cfg->wTotalLength;

	while(u_Desc_Next(&desc))
	{
		if (desc.Ptr.Comn->bDescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
			u_Add_Interface(Handle, &desc);
	}

	return LusbwError(errorCode);
}

#endif

static BOOL u_Init_Handle(__out PKUSB_HANDLE_INTERNAL* Handle,
                          __in KUSB_DRVID DriverID,
                          __in_opt PKDEV_HANDLE_INTERNAL SharedDevice,
                          __in_opt PKUSB_STACK_CB Init_BackendCB,
                          __in PKOBJ_CB Cleanup_UsbK,
                          __in PKOBJ_CB Cleanup_DevK)
{
	PKUSB_HANDLE_INTERNAL handle = NULL;

	UNREFERENCED_PARAMETER(DriverID);

	ErrorHandle(!IsHandleValid(Handle), Error, "Handle");

	handle = PoolHandle_Acquire_UsbK(Cleanup_UsbK);
	ErrorNoSet(!IsHandleValid(handle), Error, "->PoolHandle_Acquire_UsbK");

	if (SharedDevice)
	{
		// this is a cloned/associated handle.
		ErrorSet(!PoolHandle_Inc_DevK(SharedDevice), Error, ERROR_RESOURCE_NOT_AVAILABLE, "->PoolHandle_Inc_DevK");
		handle->Device = SharedDevice;
		handle->IsClone = TRUE;
	}
	else
	{
		handle->Device = PoolHandle_Acquire_DevK(Cleanup_DevK);
		ErrorNoSet(!IsHandleValid(handle->Device), Error, "->PoolHandle_Acquire_DevK");

		if (Init_BackendCB)
		{
			ErrorNoSet(!Init_BackendCB(handle), Error, "->Init_BackendCB");
		}
	}

	*Handle = handle;
	return TRUE;

Error:
	if (handle) PoolHandle_Dec_UsbK(handle);
	*Handle = NULL;
	return FALSE;
}

BOOL UsbStack_Init(
    __out		KUSB_HANDLE* Handle,
    __in_opt	KUSB_DRVID DriverID,
    __in		BOOL UsePipeCache,
    __in_opt	HANDLE DeviceHandle,
    __in_opt	KLST_DEVINFO_HANDLE DevInfo,
    __in_opt	PKDEV_HANDLE_INTERNAL SharedDevice,
    __in		PKUSB_STACK_CB Init_ConfigCB,
    __in_opt	PKUSB_STACK_CB Init_BackendCB,
    __in		PKOBJ_CB Cleanup_UsbK,
    __in		PKOBJ_CB Cleanup_DevK)
{
	BOOL success;
	PKUSB_HANDLE_INTERNAL handle = NULL;
	const size_t extraMemAlloc = sizeof(KUSB_INTERFACE_STACK) + sizeof(KUSB_DRIVER_API) + ((sizeof(KDEV_SHARED_INTERFACE) * KDEV_SHARED_INTERFACE_COUNT));
	PUCHAR extraMem;

	success = u_Init_Handle(&handle, DriverID, SharedDevice, Init_BackendCB, Cleanup_UsbK, Cleanup_DevK);
	ErrorNoSetAction(!success, return FALSE, "->u_Init_Handle");

	// if SharedDevice is non-null this handle is an associated handle
	if (!SharedDevice)
	{
		if (!IsHandleValid(DeviceHandle))
		{
			DeviceHandle = CreateDeviceFile(DevInfo->DevicePath);
			ErrorNoSet(!IsHandleValid(DeviceHandle), Error, "CreateDeviceFile failed.");
			DriverID = (KUSB_DRVID)DevInfo->DriverID;
			handle->Device->DevicePath = Str_Dupe(DevInfo->DevicePath);
		}

		handle->Device->MasterDeviceHandle = DeviceHandle;

		extraMem = Mem_Alloc(extraMemAlloc);
		ErrorMemoryAction(!IsHandleValid(extraMem), return FALSE);

		handle->Device->UsbStack = (KUSB_INTERFACE_STACK*)extraMem;
		extraMem += sizeof(KUSB_INTERFACE_STACK);

		handle->Device->DriverAPI = (KUSB_DRIVER_API*)extraMem;
		extraMem += sizeof(KUSB_DRIVER_API);

		handle->Device->SharedInterfaces = (KDEV_SHARED_INTERFACE*)extraMem;
		extraMem += ((sizeof(KDEV_SHARED_INTERFACE) * KDEV_SHARED_INTERFACE_COUNT));

		handle->Device->UsbStack->UsePipeCache = UsePipeCache;

		success = Init_ConfigCB(handle);
		ErrorNoSet(!success, Error, "->Init_ConfigCB");

		handle->Selected_SharedInterface_Index = 0;
		success = u_Init_Config(handle);
		ErrorNoSet(!success, Error, "->u_Init_Config");

		// load a driver API
		LibK_LoadDriverAPI(handle->Device->DriverAPI, DriverID);

		if (handle->Device->UsbStack->UsePipeCache)
			UsbStack_RefreshPipeCache(handle);
	}

	*Handle = (KUSB_HANDLE)handle;
	if (!SharedDevice)
	{
		PoolHandle_Live_DevK(handle->Device);
		PoolHandle_Live_UsbK(handle);
	}
	return TRUE;

Error:
	if (IsHandleValid(handle)) PoolHandle_Dec_UsbK(handle);
	*Handle = NULL;
	return FALSE;
}

BOOL UsbStack_CloneHandle (
    __in KUSB_HANDLE Handle,
    __out KUSB_HANDLE* ClonedHandle)
{
	PKUSB_HANDLE_INTERNAL handle;
	PKUSB_HANDLE_INTERNAL clonedHandle = NULL;
	BOOL success;

	Pub_To_Priv_UsbK(Handle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	success = UsbStack_Init(
	              (KUSB_HANDLE*)&clonedHandle,
	              handle->Device->DriverAPI->Info.DriverID,
	              handle->Device->UsbStack->UsePipeCache,
	              NULL,
	              NULL,
	              handle->Device,
	              NULL,
	              NULL,
	              handle->Base.Evt.Cleanup,
	              handle->Device->Base.Evt.Cleanup);

	ErrorNoSet(!success, Error, "->UsbStack_Init");

	clonedHandle->Selected_SharedInterface_Index = handle->Selected_SharedInterface_Index;

	*ClonedHandle = (KUSB_HANDLE)clonedHandle;
	PoolHandle_Dec_UsbK(handle);
	PoolHandle_Live_UsbK(clonedHandle);
	return TRUE;

Error:
	if (IsHandleValid(clonedHandle)) PoolHandle_Dec_UsbK(clonedHandle);
	PoolHandle_Dec_UsbK(handle);
	*ClonedHandle = NULL;
	return FALSE;
}

BOOL UsbStack_GetAssociatedInterface (
    __in KUSB_HANDLE Handle,
    __in UCHAR AssociatedInterfaceIndex,
    __out KUSB_HANDLE* AssociatedHandle)
{
	PKUSB_HANDLE_INTERNAL handle;
	PKUSB_HANDLE_INTERNAL associatedHandle = NULL;
	UCHAR nextIndex;
	PKUSB_INTERFACE_EL interfaceEL;
	BOOL success;

	Pub_To_Priv_UsbK(Handle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	nextIndex = ((UCHAR)handle->Selected_SharedInterface_Index) + 1 + AssociatedInterfaceIndex;

	FindInterfaceEL(handle->Device->UsbStack, interfaceEL, TRUE, nextIndex);
	ErrorSet(!interfaceEL, Error, ERROR_NO_MORE_ITEMS, "Interface index %u not found.", nextIndex);

	success = UsbStack_CloneHandle((KUSB_HANDLE)handle, (KUSB_HANDLE*)&associatedHandle);
	ErrorNoSet(!success, Error, "->UsbStack_CloneHandle");

	associatedHandle->Selected_SharedInterface_Index = nextIndex;
	*AssociatedHandle = (KUSB_HANDLE)associatedHandle;
	PoolHandle_Dec_UsbK(handle);

	PoolHandle_Live_UsbK(associatedHandle);
	return TRUE;

Error:
	if (IsHandleValid(associatedHandle)) PoolHandle_Dec_UsbK(associatedHandle);
	PoolHandle_Dec_UsbK(handle);
	*AssociatedHandle = NULL;
	return FALSE;
}

VOID UsbStack_Clear(PKUSB_INTERFACE_STACK UsbStack)
{
	PKUSB_INTERFACE_EL nextInterfaceEL, t0;
	PKUSB_ALT_INTERFACE_EL nextAltInterfaceEL, t1;
	PKUSB_PIPE_EL nextPipeEL, t2;

	if (!IsHandleValid(UsbStack)) return;

	// free the interface list
	DL_FOREACH_SAFE(UsbStack->InterfaceList, nextInterfaceEL, t0)
	{
		// free the stack collection lists
		DL_FOREACH_SAFE(nextInterfaceEL->AltInterfaceList, nextAltInterfaceEL, t1)
		{
			DL_FOREACH_SAFE(nextAltInterfaceEL->PipeList, nextPipeEL, t2)
			{
				DL_DELETE(nextAltInterfaceEL->PipeList, nextPipeEL);
				Mem_Free(&nextPipeEL);
			}

			DL_DELETE(nextInterfaceEL->AltInterfaceList, nextAltInterfaceEL);
			Mem_Free(&nextAltInterfaceEL);
		}

		DL_DELETE(UsbStack->InterfaceList, nextInterfaceEL);
		Mem_Free(&nextInterfaceEL);
	}
}

BOOL UsbStack_Rebuild(
    __in PKUSB_HANDLE_INTERNAL Handle,
    __in PKUSB_STACK_CB Init_ConfigCB)
{
	BOOL success;

	if (DecLock(ALLK_GETREF_HANDLE(Handle->Device)) != 1)
	{
		IncLock(ALLK_GETREF_HANDLE(Handle->Device));
		return LusbwError(ERROR_RESOURCE_NOT_AVAILABLE);
	}

	if (DecLock(ALLK_GETREF_HANDLE(Handle->Device)) != 0)
	{
		IncLock(ALLK_GETREF_HANDLE(Handle->Device));
		IncLock(ALLK_GETREF_HANDLE(Handle->Device));
		return LusbwError(ERROR_RESOURCE_NOT_AVAILABLE);
	}

	UsbStack_Clear(Handle->Device->UsbStack);
	Mem_Free(&Handle->Device->ConfigDescriptor);
	Handle->Selected_SharedInterface_Index = 0;
	memset(Handle->Device->SharedInterfaces, 0, sizeof(Handle->Device->SharedInterfaces));
	memset(&Handle->Move, 0, sizeof(Handle->Move));

	success = Init_ConfigCB(Handle);
	ErrorNoSet(!success, Error, "->Init_ConfigCB");

	success = u_Init_Config(Handle);
	ErrorNoSet(!success, Error, "->u_Init_Config");

	IncLock(ALLK_GETREF_HANDLE(Handle->Device));
	IncLock(ALLK_GETREF_HANDLE(Handle->Device));
	return TRUE;

Error:
	IncLock(ALLK_GETREF_HANDLE(Handle->Device));
	IncLock(ALLK_GETREF_HANDLE(Handle->Device));
	return FALSE;
}

BOOL UsbStack_QueryInterfaceSettings (
    __in KUSB_HANDLE Handle,
    __in UCHAR AltSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	PKUSB_HANDLE_INTERNAL handle;
	PKUSB_INTERFACE_EL intfEL;
	PKUSB_ALT_INTERFACE_EL altfEL;

	ErrorParamAction(!IsHandleValid(UsbAltInterfaceDescriptor), "UsbAltInterfaceDescriptor", return FALSE);
	ErrorParamAction(AltSettingNumber > 0x7F, "AltSettingNumber", return FALSE);

	Pub_To_Priv_UsbK(Handle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	FindInterfaceEL(handle->Device->UsbStack, intfEL, TRUE, handle->Selected_SharedInterface_Index);
	ErrorSet(!intfEL, Error, ERROR_NO_MORE_ITEMS, "Failed locating interface number %u.", handle->Selected_SharedInterface_Index);

	FindAltInterfaceEL(intfEL, altfEL, FALSE, AltSettingNumber);
	ErrorSet(!altfEL, Error, ERROR_NO_MORE_ITEMS, "AltSettingNumber %u does not exists.", AltSettingNumber);

	memcpy(UsbAltInterfaceDescriptor, altfEL->Descriptor, sizeof(*UsbAltInterfaceDescriptor));
	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

BOOL UsbStack_SelectInterface (
    __in KUSB_HANDLE Handle,
    __in UCHAR IndexOrNumber,
    __in BOOL IsIndex)
{
	PKUSB_HANDLE_INTERNAL handle;
	PKUSB_INTERFACE_EL intfEL;

	ErrorParamAction(IndexOrNumber > 0x7F, "IndexOrNumber", return FALSE);

	Pub_To_Priv_UsbK(Handle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	FindInterfaceEL(handle->Device->UsbStack, intfEL, IsIndex, IndexOrNumber);
	ErrorSet(!intfEL, Error, ERROR_NO_MORE_ITEMS, "Failed locating interface %s %u.", IsIndex ? "index" : "number", IndexOrNumber);

	InterlockedExchange(&handle->Selected_SharedInterface_Index, intfEL->Index);

	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

BOOL UsbStack_QuerySelectedEndpoint(
    __in KUSB_HANDLE Handle,
    __in UCHAR EndpointAddressOrIndex,
    __in BOOL IsIndex,
    __out PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor)
{
	PKUSB_HANDLE_INTERNAL handle;
	PKUSB_INTERFACE_EL intfEL;
	PKUSB_ALT_INTERFACE_EL altfEL;
	PKUSB_PIPE_EL pipeEL;
	UCHAR altSetting;

	ErrorParamAction(!IsHandleValid(EndpointDescriptor), "EndpointDescriptor", return FALSE);

	Pub_To_Priv_UsbK(Handle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	FindInterfaceEL(handle->Device->UsbStack, intfEL, TRUE, handle->Selected_SharedInterface_Index);
	ErrorSet(!intfEL, Error, ERROR_NO_MORE_ITEMS, "Failed locating interface number %u.", handle->Selected_SharedInterface_Index);

	altSetting = (UCHAR)handle->Device->SharedInterfaces[handle->Selected_SharedInterface_Index].CurrentAltSetting;

	FindAltInterfaceEL(intfEL, altfEL, FALSE, altSetting);
	ErrorSet(!altfEL, Error, ERROR_NO_MORE_ITEMS, "AltSettingNumber %u does not exists.", altSetting);

	FindPipeEL(altfEL, pipeEL, IsIndex, EndpointAddressOrIndex);
	ErrorSet(!pipeEL, Error, ERROR_NO_MORE_ITEMS, "Endpoint %s %u does not exists.", IsIndex ? "index" : "address", EndpointAddressOrIndex);

	memcpy(EndpointDescriptor, pipeEL->Descriptor, sizeof(*EndpointDescriptor));

	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

BOOL UsbStack_QueryPipe(
    __in KUSB_HANDLE Handle,
    __in UCHAR AltSettingNumber,
    __in UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	PKUSB_HANDLE_INTERNAL handle;
	PKUSB_INTERFACE_EL intfEL;
	PKUSB_ALT_INTERFACE_EL altfEL;
	PKUSB_PIPE_EL pipeEL;

	ErrorParamAction(AltSettingNumber > 0x7F, "AltSettingNumber", return FALSE);
	ErrorParamAction(PipeIndex > 0x1F, "PipeIndex", return FALSE);
	ErrorParamAction(!IsHandleValid(PipeInformation), "PipeInformation", return FALSE);

	Pub_To_Priv_UsbK(Handle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_UsbK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_UsbK");

	FindInterfaceEL(handle->Device->UsbStack, intfEL, TRUE, handle->Selected_SharedInterface_Index);
	ErrorSet(!intfEL, Error, ERROR_NO_MORE_ITEMS, "Failed locating interface number %u.", handle->Selected_SharedInterface_Index);

	FindAltInterfaceEL(intfEL, altfEL, FALSE, AltSettingNumber);
	ErrorSet(!altfEL, Error, ERROR_NO_MORE_ITEMS, "AltSettingNumber %u does not exists.", AltSettingNumber);

	FindPipeEL(altfEL, pipeEL, TRUE, PipeIndex);
	ErrorSet(!pipeEL, Error, ERROR_NO_MORE_ITEMS, "PipeIndex %u does not exists.", PipeIndex);

	PipeInformation->PipeType			= pipeEL->Descriptor->bmAttributes & 0x03;
	PipeInformation->MaximumPacketSize	= pipeEL->Descriptor->wMaxPacketSize;
	PipeInformation->PipeId				= pipeEL->Descriptor->bEndpointAddress;
	PipeInformation->Interval			= pipeEL->Descriptor->bInterval;

	PoolHandle_Dec_UsbK(handle);
	return TRUE;

Error:
	PoolHandle_Dec_UsbK(handle);
	return FALSE;
}

BOOL UsbStack_RefreshPipeCache(PKUSB_HANDLE_INTERNAL Handle)
{
	PKUSB_INTERFACE_EL		intfEL;
	PKUSB_ALT_INTERFACE_EL	altfEL;
	PKUSB_PIPE_EL			pipeEL;
	PKDEV_SHARED_INTERFACE	sharedInterface;
	KUSB_PIPE_CACHE*		pipeCacheItem;

	DL_FOREACH(Handle->Device->UsbStack->InterfaceList, intfEL)
	{
		sharedInterface = &Get_SharedInterface(Handle, intfEL->Index);
		DL_FOREACH(intfEL->AltInterfaceList, altfEL)
		{
			DL_FOREACH(altfEL->PipeList, pipeEL)
			{
				pipeCacheItem = &Handle->Device->UsbStack->PipeCache[PIPEID_TO_IDX(pipeEL->ID)];
				if (altfEL->ID == sharedInterface->CurrentAltSetting)
				{
					InterlockedExchangePointer(&pipeCacheItem->InterfaceHandle, sharedInterface->InterfaceHandle);
				}
				else if (pipeCacheItem->InterfaceHandle == NULL)
				{
					if (sharedInterface->InterfaceHandle)
						InterlockedExchangePointer(&pipeCacheItem->InterfaceHandle, sharedInterface->InterfaceHandle);
					else
						InterlockedExchangePointer(&pipeCacheItem->InterfaceHandle, Handle->Device->MasterInterfaceHandle);
				}
			}
		}
	}

	return TRUE;
}
