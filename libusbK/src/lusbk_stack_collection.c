#include "lusbk_bknd.h"
#include "lusbk_private.h"
#include "lusbk_stack_collection.h"


// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)


#define AdvanceDescriptor(DescriptorPtr, SizeLeft)																\
{																												\
	if (!DescriptorPtr || SizeLeft < (INT)(DescriptorPtr->bLength+sizeof(USB_COMMON_DESCRIPTOR)))				\
	{																											\
		SizeLeft=0;																								\
		DescriptorPtr = NULL;																					\
	}																											\
	else																										\
	{																											\
		SizeLeft-=(INT)DescriptorPtr->bLength;																	\
		DescriptorPtr = (PUSB_COMMON_DESCRIPTOR)&((PUCHAR)DescriptorPtr)[DescriptorPtr->bLength];				\
	}																											\
}

#define GetPipeNumberCache(PipeElement, PipeNumber)	\
	(PipeElement->PipeNumberCache[((((PipeNumber) & 0xF)|(((PipeNumber)>>3) & 0x10)) & 0x1F)])


#define L_Clone(dst,src) if (src) { (dst) = Mem_Alloc(sizeof(*(dst))); if ((dst)) { memcpy((dst), (src), sizeof(*(dst))); (dst)->next=NULL; (dst)->prev=NULL; } else break; } else break

BOOL UsbStack_Init(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in PKUSB_GET_CFG_DESCRIPTOR_CB GetConfigDescriptorCB,
    __in PKUSB_INTERFACE_CB ClaimReleaseInterfaceCB,
    __in_opt PKUSB_OPEN_ASSOCIATED_INTERFACE_CB OpenInterfaceCB,
    __in_opt PKUSB_INTERFACE_ACTION_CB CloseInterfaceCB,
    __in_opt PKUSB_DEVICE_ACTION_CB CloseDeviceCB)
{
	if (!Mem_Zero(UsbStack, sizeof(*UsbStack)))
	{
		return LusbwError(ERROR_INVALID_HANDLE);
	}
	UsbStack->Cb.GetConfigDescriptor = GetConfigDescriptorCB;
	UsbStack->Cb.ClaimReleaseInterface = ClaimReleaseInterfaceCB;
	UsbStack->Cb.OpenInterface = OpenInterfaceCB;
	UsbStack->Cb.CloseInterface = CloseInterfaceCB;
	UsbStack->Cb.CloseDevice = CloseDeviceCB;

	return TRUE;
}

VOID UsbStack_Clone(
    __in PKUSB_INTERFACE_STACK SrcUsbStack,
    __in PKUSB_INTERFACE_STACK DstUsbStack)
{
	PKUSB_INTERFACE_EL nextInterfaceEL, newInterfaceEL;
	PKUSB_ALT_INTERFACE_EL nextAltInterfaceEL, newAltInterfaceEL;
	PKUSB_PIPE_EL nextPipeEL, newPipeEL;
	PKUSB_DEVICE_EL nextDeviceEL, newDeviceEL;

	newDeviceEL = NULL;

	DL_FOREACH(SrcUsbStack->DeviceList, nextDeviceEL)
	{
		if (!nextDeviceEL->Device->ConfigDescriptor)
		{
			USBERR("null config descriptor.\n");
			SetLastError(ERROR_SOURCE_ELEMENT_EMPTY);
			return;
		}

		// clone the device
		L_Clone(newDeviceEL, nextDeviceEL);

		// increment the reference count on this device.
		InterlockedIncrement(&newDeviceEL->Device->OpenedRefCount);
		IncDeviceUsageCount(newDeviceEL->Device);

		// add cloned device
		DL_APPEND(DstUsbStack->DeviceList, newDeviceEL);
	}

	DL_FOREACH(SrcUsbStack->InterfaceList, nextInterfaceEL)
	{

		DL_SEARCH_SCALAR(DstUsbStack->DeviceList, nextDeviceEL, Device, nextInterfaceEL->ParentDevice->Device);

		// clone interface
		L_Clone(newInterfaceEL, nextInterfaceEL);
		newInterfaceEL->AltInterfaceList = NULL;
		newInterfaceEL->ParentDevice = nextDeviceEL;

		DL_FOREACH(nextInterfaceEL->AltInterfaceList, nextAltInterfaceEL)
		{
			// clone alt-interface
			L_Clone(newAltInterfaceEL, nextAltInterfaceEL);
			newAltInterfaceEL->PipeList = NULL;

			DL_FOREACH(nextAltInterfaceEL->PipeList, nextPipeEL)
			{
				// clone pipe
				L_Clone(newPipeEL, nextPipeEL);

				// add cloned pipe
				DL_APPEND(newAltInterfaceEL->PipeList, newPipeEL);
			}

			// add cloned alt-interface
			DL_APPEND(newInterfaceEL->AltInterfaceList, newAltInterfaceEL);
		}

		// add clone interface
		DL_APPEND(DstUsbStack->InterfaceList, newInterfaceEL);
	}
}

BOOL UsbStack_Free(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in_opt PVOID Context)
{
	PKUSB_INTERFACE_EL nextInterfaceEL, t0;
	PKUSB_ALT_INTERFACE_EL nextAltInterfaceEL, t1;
	PKUSB_PIPE_EL nextPipeEL, t2;
	PKUSB_DEVICE_EL nextDeviceEL, t3;

	if (!IsHandleValid(UsbStack))
		return FALSE;

	// safely free the device list.
	// the caller may supply a call back function
	// to free additional resources for the device.
	DL_FOREACH_SAFE(UsbStack->DeviceList, nextDeviceEL, t3)
	{
		long lockCount;
		AquireDeviceActionPendingLock(nextDeviceEL->Device);

		if ((lockCount = InterlockedDecrement(&nextDeviceEL->Device->OpenedRefCount)) == 0)
		{
			// When Device->OpenedRefCount falls to zero, there are no more internal K handles
			// using this device.
			if (UsbStack->Cb.CloseInterface)
			{
				// * Call CloseInterfaceCB for all of the elements that belong to this device.
				// * Update the shared device interface array.
				DL_FOREACH_SAFE(UsbStack->InterfaceList, nextInterfaceEL, t0)
				{
					if (nextInterfaceEL->ParentDevice == nextDeviceEL)
					{
						UsbStack->Cb.CloseInterface(UsbStack, nextInterfaceEL, Context);

						InterlockedExchangePointer(
						    &nextDeviceEL->Device->SharedInterfaces[nextInterfaceEL->Number].InterfaceHandle,
						    NULL);
					}
				}
			}

			if (UsbStack->Cb.CloseDevice)
			{
				// The caller is responsible for his own cleanup.
				UsbStack->Cb.CloseDevice(UsbStack, nextDeviceEL, Context);
			}
			else
			{

				// the default action is to close devices if the device path is valid.
				if (!Str_IsNullOrEmpty(nextDeviceEL->Device->DevicePath))
				{
					CloseHandle(nextDeviceEL->Device->MasterDeviceHandle);
				}
			}
		}
		else
		{
			USBDBG("skipped close; device is still referenced. OpenedRefCount=%d\n", lockCount);
		}

		// allow another thread to (again) open/close this handle.
		ReleaseDeviceActionPendingLock(nextDeviceEL->Device);

		if (DecDeviceUsageCount(nextDeviceEL->Device) == 1)
		{
			// return this one to the pool.  this was the last stack using it.
			Mem_Free(&nextDeviceEL->Device->ConfigDescriptor);
			DecDeviceUsageCount(nextDeviceEL->Device);
		}

		DL_DELETE(UsbStack->DeviceList, nextDeviceEL);
		Mem_Free(&nextDeviceEL);
	}

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

	Mem_Free(&UsbStack->DynamicConfigDescriptor);

	return TRUE;
}


static int Predicate_FindDeviceEL_ByPath(PKUSB_DEVICE_EL deviceEL, LPCSTR DevicePath)
{
	return Str_Cmp(DevicePath, deviceEL->Device->DevicePath);
}

#define AllocEL(el)						\
{										\
	el = Mem_Alloc(sizeof(*el));		\
	if (!IsHandleValid(el))	goto Error;	\
}

BOOL UsbStack_AddDevice(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in_opt LPCSTR DevicePath,
    __in_opt HANDLE DeviceHandle,
    __in_opt PVOID Context)
{
	BOOL newDeviceOpen = FALSE;
	BOOL fetchedConfigDescriptor = FALSE;
	BOOL success = FALSE;
	PKUSB_SHARED_DEVICE sharedDevice;
	LONG errorCode = ERROR_SUCCESS;

	if (!IsHandleValid(DeviceHandle) &&
	        !IsHandleValid(DevicePath))
	{
		return LusbwError(ERROR_INVALID_PARAMETER);
	}

	sharedDevice = GetSharedDevicePoolHandle(DevicePath);
	if (!sharedDevice)
		return FALSE;

	if (IsHandleValid(DeviceHandle))
	{
		sharedDevice->MasterDeviceHandle = DeviceHandle;
	}

	if (!IsHandleValid(sharedDevice->MasterDeviceHandle) &&
	        !Str_IsNullOrEmpty(sharedDevice->DevicePath))
	{
		sharedDevice->MasterDeviceHandle = CreateDeviceFile(sharedDevice->DevicePath);
		newDeviceOpen = IsHandleValid(sharedDevice->MasterDeviceHandle);
	}

	if (!IsHandleValid(sharedDevice->MasterDeviceHandle))
	{
		errorCode = GetLastError();
		if (errorCode == ERROR_SUCCESS)
			errorCode = ERROR_DEVICE_IN_USE;

		DecDeviceUsageCount(sharedDevice);
		ReleaseDeviceActionPendingLock(sharedDevice);

		USBERR("failed creating device file handle. ErrorCode=%08Xh\n\tDevicePath=%s\n",
		       errorCode, sharedDevice->DevicePath);

		return LusbwError(errorCode);
	}

	if (!sharedDevice->ConfigDescriptor)
	{
		HANDLE newInterefaceHandle = sharedDevice->MasterDeviceHandle;

		if (UsbStack->Cb.OpenInterface &&
		        !IsHandleValid(sharedDevice->MasterInterfaceHandle))
		{
			// Call OpenInterface once here.
			// We will need it to get the config descriptor.
			errorCode = UsbStack->Cb.OpenInterface(
			                UsbStack,
			                newInterefaceHandle,
			                0,
			                &newInterefaceHandle,
			                Context);

			if (errorCode == ERROR_SUCCESS)
				sharedDevice->MasterInterfaceHandle = newInterefaceHandle;
		}

		if (errorCode == ERROR_SUCCESS)
		{
			errorCode = UsbStack->Cb.GetConfigDescriptor(
			                UsbStack,
			                DevicePath,
			                sharedDevice->MasterDeviceHandle,
			                sharedDevice->MasterInterfaceHandle,
			                &sharedDevice->ConfigDescriptor,
			                Context);

			if (errorCode == ERROR_SUCCESS)
				fetchedConfigDescriptor = TRUE;
		}
	}

	if (errorCode != ERROR_SUCCESS)
	{
		success = LusbwError(errorCode);
	}
	else
	{
		success = UsbStack_Add(
		              UsbStack,
		              sharedDevice,
		              Context);
	}

	if (success)
	{
		// inc the device open count
		InterlockedIncrement(&sharedDevice->OpenedRefCount);

		// UsageCount=2 (for a new device)
		IncDeviceUsageCount(sharedDevice);

		// release the action lock
		ReleaseDeviceActionPendingLock(sharedDevice);
		return TRUE;
	}

	if (newDeviceOpen)
		CloseHandle(sharedDevice->MasterDeviceHandle);

	if (fetchedConfigDescriptor)
		Mem_Free(&sharedDevice->ConfigDescriptor);

	// return the ShardDevice back to the pool. (GetSharedDevicePoolHandle incs the count)
	DecDeviceUsageCount(sharedDevice);
	ReleaseDeviceActionPendingLock(sharedDevice);

	return FALSE;
}

BOOL mergeConfigDescriptor(__in PKUSB_INTERFACE_STACK UsbStack, PUSB_CONFIGURATION_DESCRIPTOR cfgToAdd)
{
	PUSB_CONFIGURATION_DESCRIPTOR merged = NULL;

	if (!UsbStack->DynamicConfigDescriptor)
	{
		UsbStack->DynamicConfigDescriptor = Mem_Alloc(cfgToAdd->wTotalLength);
		if (!IsHandleValid(UsbStack->DynamicConfigDescriptor))
			goto Error;

		memcpy(
		    UsbStack->DynamicConfigDescriptor,
		    cfgToAdd,
		    cfgToAdd->wTotalLength);

		UsbStack->DynamicConfigDescriptorSize = cfgToAdd->wTotalLength;
	}
	else
	{
		PUCHAR mergeDst, configSrc;
		ULONG addLength = cfgToAdd->wTotalLength - sizeof(USB_CONFIGURATION_DESCRIPTOR);
		ULONG newConfigDescriptorSize = (ULONG) UsbStack->DynamicConfigDescriptorSize + addLength;

		merged = Mem_Alloc(newConfigDescriptorSize);
		if (!IsHandleValid(merged))
			goto Error;

		memcpy(merged, UsbStack->DynamicConfigDescriptor, UsbStack->DynamicConfigDescriptorSize);

		mergeDst = &(((PUCHAR)merged)[UsbStack->DynamicConfigDescriptorSize]);
		configSrc = &(((PUCHAR)cfgToAdd)[sizeof(USB_CONFIGURATION_DESCRIPTOR)]);

		memcpy(mergeDst, configSrc, addLength);

		merged->wTotalLength += (USHORT)addLength;
		merged->bNumInterfaces += cfgToAdd->bNumInterfaces;

		Mem_Free(&UsbStack->DynamicConfigDescriptor);
		UsbStack->DynamicConfigDescriptor = merged;
		UsbStack->DynamicConfigDescriptorSize = newConfigDescriptorSize;
	}

	return TRUE;
Error:
	Mem_Free(&merged);
	return FALSE;
}

BOOL UsbStack_Add(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in PKUSB_SHARED_DEVICE SharedDevice,
    __in_opt PVOID Context)
{
	PKUSB_INTERFACE_EL nextInterfaceEL = NULL;
	PKUSB_ALT_INTERFACE_EL nextAltInterfaceEL = NULL;
	PKUSB_PIPE_EL nextPipeEL = NULL;
	PKUSB_DEVICE_EL nextDeviceEL = NULL;

	PUSB_COMMON_DESCRIPTOR nextNewDescriptor;
	PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
	PUSB_ENDPOINT_DESCRIPTOR endpointDescriptor;
	INT interfaceIndex, altIndex, pipeIndex;
	INT ConfigSizeLeft;
	BOOL success = FALSE;

	// don't add duplicates
	DL_SEARCH_SCALAR(UsbStack->DeviceList, nextDeviceEL, Device, SharedDevice);
	if (nextDeviceEL)
	{
		USBWRN("attempted to add a duplicate device.\n\tDevicePath = %s\n", SharedDevice->DevicePath);
		return LusbwError(ERROR_DUP_NAME);
	}

	AllocEL(nextDeviceEL);
	nextDeviceEL->Device = SharedDevice;
	nextDeviceEL->Context = Context;

	DL_APPEND(UsbStack->DeviceList, nextDeviceEL);
	success = mergeConfigDescriptor(UsbStack, SharedDevice->ConfigDescriptor);
	ErrorNoSet(!success, Error, "->mergeConfigDescriptor");

	ConfigSizeLeft = SharedDevice->ConfigDescriptor->wTotalLength;
	nextNewDescriptor = (PUSB_COMMON_DESCRIPTOR)SharedDevice->ConfigDescriptor;
	AdvanceDescriptor(nextNewDescriptor, ConfigSizeLeft);

	interfaceIndex = 0;
	altIndex = 0;
	while (nextNewDescriptor)
	{
		while (nextNewDescriptor && nextNewDescriptor->bDescriptorType != USB_INTERFACE_DESCRIPTOR_TYPE)
			AdvanceDescriptor(nextNewDescriptor, ConfigSizeLeft);

		if (!nextNewDescriptor) break;

AddInterfaceDescriptorJump:

		interfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)nextNewDescriptor;

		DL_SEARCH_SCALAR(UsbStack->InterfaceList, nextInterfaceEL, Number, interfaceDescriptor->bInterfaceNumber);
		if (!nextInterfaceEL)
		{
			HANDLE newInterfaceHandle = NULL;
			// New interface found


			// if there is an interface callback function, an interface handle is required.
			if (UsbStack->Cb.OpenInterface)
			{
				if (!(IsHandleValid(SharedDevice->SharedInterfaces[interfaceDescriptor->bInterfaceNumber].InterfaceHandle)))
				{
					if (interfaceIndex > 0)
					{
						success = UsbStack->Cb.OpenInterface(UsbStack, SharedDevice->MasterDeviceHandle, interfaceIndex, &newInterfaceHandle, Context);
						ErrorNoSet(!success, Error, "->UsbStack->Cb.OpenInterface");
					}
					else
					{
						newInterfaceHandle = SharedDevice->MasterInterfaceHandle;
					}
					SharedDevice->SharedInterfaces[interfaceDescriptor->bInterfaceNumber].InterfaceHandle = newInterfaceHandle;
				}
			}
			altIndex = 0;
			AllocEL(nextInterfaceEL);

			nextInterfaceEL->Index				= interfaceIndex++;
			nextInterfaceEL->ParentDevice		= nextDeviceEL;
			nextInterfaceEL->Number				= interfaceDescriptor->bInterfaceNumber;

			DL_APPEND(UsbStack->InterfaceList, nextInterfaceEL);
		}
		else
		{
			altIndex++;
		}

		DL_SEARCH_SCALAR(nextInterfaceEL->AltInterfaceList, nextAltInterfaceEL, Number, interfaceDescriptor->bAlternateSetting);

		if (nextAltInterfaceEL)
		{
			USBWRN("duplicate alternate interface #%02Xh on interface #%02Xh\n",
			       interfaceDescriptor->bAlternateSetting,
			       interfaceDescriptor->bInterfaceNumber);
		}
		else
		{
			AllocEL(nextAltInterfaceEL);

			memcpy(&nextAltInterfaceEL->Descriptor, interfaceDescriptor, sizeof(nextAltInterfaceEL->Descriptor));
			nextAltInterfaceEL->Number = interfaceDescriptor->bAlternateSetting;
			nextAltInterfaceEL->Index = altIndex++;

			DL_APPEND(nextInterfaceEL->AltInterfaceList, nextAltInterfaceEL);

		}

		AdvanceDescriptor(nextNewDescriptor, ConfigSizeLeft);

		// find endpoints in the new config descriptor
		pipeIndex = 0;
		while (nextNewDescriptor)
		{
			// continue searching/advancing; looking for endpoint descriptors until we reach
			// the end or hit another interface descriptor.
			if (nextNewDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
				goto AddInterfaceDescriptorJump;

			if (nextNewDescriptor->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE)
			{
				// new endpoint found
				endpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)nextNewDescriptor;

				// make sure we don't already have this one.
				DL_SEARCH_SCALAR(nextAltInterfaceEL->PipeList, nextPipeEL, Number, endpointDescriptor->bEndpointAddress);
				if (nextPipeEL)
				{
					USBWRN("duplicate pipe id %02Xh on alternate interface #%02Xh on interface #%02Xh\n",
					       endpointDescriptor->bEndpointAddress,
					       interfaceDescriptor->bAlternateSetting,
					       interfaceDescriptor->bInterfaceNumber);
				}
				else
				{
					// new pipe address
					AllocEL(nextPipeEL);

					nextPipeEL->Number = endpointDescriptor->bEndpointAddress;
					nextPipeEL->Index = pipeIndex++;
					memcpy(&nextPipeEL->Descriptor, endpointDescriptor, sizeof(nextPipeEL->Descriptor));

					DL_APPEND(nextAltInterfaceEL->PipeList, nextPipeEL);
				}
			}
			AdvanceDescriptor(nextNewDescriptor, ConfigSizeLeft);
		}
	}

	// build virtual interface indexes
	interfaceIndex = 0;
	DL_FOREACH(UsbStack->InterfaceList, nextInterfaceEL)
	{
		nextInterfaceEL->VirtualIndex = interfaceIndex++;
	}

	success = UsbStack_BuildCache(UsbStack);
	ErrorNoSet(!success, Error, "->UsbStack_BuildCache");

	return TRUE;

Error:
	return FALSE;
}



LONG UsbStack_ClaimOrRelease(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in BOOL IsClaim,
    __in INT InterfaceIndexOrNumber,
    __in BOOL IsIndex,
    __in_opt PVOID Context)
{
	PKUSB_INTERFACE_EL interfaceEL;
	LONG action = ERROR_SUCCESS;

	if (IsIndex)
	{
		DL_SEARCH_SCALAR(UsbStack->InterfaceList, interfaceEL, VirtualIndex, InterfaceIndexOrNumber);
	}
	else
	{
		DL_SEARCH_SCALAR(UsbStack->InterfaceList, interfaceEL, Number, InterfaceIndexOrNumber);
	}

	if (UsbStack->Cb.ClaimReleaseInterface)
	{
		action = UsbStack->Cb.ClaimReleaseInterface(UsbStack, interfaceEL, IsClaim, InterfaceIndexOrNumber, IsIndex, Context);
	}
	return action;
}

USB_STACK_HANDLER_RESULT UsbStackHandler_SetAltInterface(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in INT InterfaceIndexOrNumber,
    __in BOOL IsInterfaceIndex,
    __in INT AltSettingIndexOrNumber,
    __in BOOL IsAltSettingIndex)
{
	PKUSB_INTERFACE_EL interfaceEL = NULL;
	PKUSB_ALT_INTERFACE_EL altInterfaceEL = NULL;

	if (IsInterfaceIndex)
	{
		DL_SEARCH_SCALAR(UsbStack->InterfaceList, interfaceEL, Index, InterfaceIndexOrNumber);
	}
	else
	{
		DL_SEARCH_SCALAR(UsbStack->InterfaceList, interfaceEL, Number, InterfaceIndexOrNumber);
	}
	if (interfaceEL)
	{
		if (IsAltSettingIndex)
		{
			DL_SEARCH_SCALAR(interfaceEL->AltInterfaceList, altInterfaceEL, Index, AltSettingIndexOrNumber);
		}
		else
		{
			DL_SEARCH_SCALAR(interfaceEL->AltInterfaceList, altInterfaceEL, Number, AltSettingIndexOrNumber);
		}
	}

	if (interfaceEL && altInterfaceEL)
	{
		interfaceEL->CurrentAltSetting = altInterfaceEL->Number;

		DL_DELETE(interfaceEL->AltInterfaceList, altInterfaceEL);
		DL_PREPEND(interfaceEL->AltInterfaceList, altInterfaceEL);

		UsbStack_BuildCache(UsbStack);

		return HANDLER_HANDLED_WITH_TRUE;
	}

	LusbwError(ERROR_NOT_FOUND);
	return HANDLER_HANDLED_WITH_FALSE;
}

USB_STACK_HANDLER_RESULT UsbStackHandler_GetAltInterface(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in INT InterfaceIndexOrNumber,
    __in BOOL IsInterfaceIndex,
    __out PINT AltSettingNumber)
{
	PKUSB_INTERFACE_EL interfaceEL = NULL;
	PKUSB_ALT_INTERFACE_EL altInterfaceEL = NULL;

	if (IsInterfaceIndex)
	{
		DL_SEARCH_SCALAR(UsbStack->InterfaceList, interfaceEL, Index, InterfaceIndexOrNumber);
	}
	else
	{
		DL_SEARCH_SCALAR(UsbStack->InterfaceList, interfaceEL, Number, InterfaceIndexOrNumber);
	}
	if (interfaceEL)
	{
		*AltSettingNumber = interfaceEL->CurrentAltSetting;
		DL_SEARCH_SCALAR(interfaceEL->AltInterfaceList, altInterfaceEL, Number, interfaceEL->CurrentAltSetting);
	}

	if (interfaceEL && altInterfaceEL)
	{
		return HANDLER_HANDLED_WITH_TRUE;
	}

	LusbwError(ERROR_NOT_FOUND);
	return HANDLER_HANDLED_WITH_FALSE;
}

BOOL UsbStack_BuildCache(
    __in PKUSB_INTERFACE_STACK UsbStack)
{
	PKUSB_INTERFACE_EL nextInterfaceEL = NULL;
	PKUSB_ALT_INTERFACE_EL nextAltInterfaceEL = NULL;
	PKUSB_PIPE_EL nextPipeEL = NULL;
	PKUSB_DEVICE_EL nextDeviceEL = NULL;
	int pos;
	HANDLE firstDeviceHandle = NULL;
	HANDLE firstInterfaceHandle = NULL;
	ULONG deviceCount = 0;

	DL_FOREACH(UsbStack->DeviceList, nextDeviceEL)
	{
		deviceCount++;
	}
	UsbStack->DeviceCount = deviceCount;

	Mem_Zero(&UsbStack->PipeNumberCache, sizeof(UsbStack->PipeNumberCache));

	DL_FOREACH(UsbStack->InterfaceList, nextInterfaceEL)
	{
		DL_FOREACH(nextInterfaceEL->AltInterfaceList, nextAltInterfaceEL)
		{
			DL_FOREACH(nextAltInterfaceEL->PipeList, nextPipeEL)
			{
				PKUSB_PIPE_CACHE pipeCache;
				pipeCache = &GetPipeNumberCache(UsbStack, nextPipeEL->Number);

				if (!firstDeviceHandle && IsHandleValid(nextInterfaceEL->ParentDevice->Device->MasterDeviceHandle))
					firstDeviceHandle = nextInterfaceEL->ParentDevice->Device->MasterDeviceHandle;

				if (!firstInterfaceHandle && IsHandleValid(nextInterfaceEL->ParentDevice->Device->MasterInterfaceHandle))
					firstInterfaceHandle = nextInterfaceEL->ParentDevice->Device->MasterInterfaceHandle;

				if (pipeCache->Number && nextAltInterfaceEL->Number != nextInterfaceEL->CurrentAltSetting)
					continue;

				pipeCache->DeviceHandle		= nextInterfaceEL->ParentDevice->Device->MasterDeviceHandle;
				pipeCache->InterfaceHandle	= nextInterfaceEL->ParentDevice->Device->SharedInterfaces[nextInterfaceEL->Number].InterfaceHandle;

				pipeCache->Index			= nextPipeEL->Index;
				pipeCache->Number			= nextPipeEL->Number;
			}
		}
	}

	for (pos = 0; pos < sizeof(UsbStack->PipeNumberCache) / sizeof(UsbStack->PipeNumberCache[0]); pos++)
	{
		if (!UsbStack->PipeNumberCache[pos].DeviceHandle)
			UsbStack->PipeNumberCache[pos].DeviceHandle = firstDeviceHandle;

		if (!UsbStack->PipeNumberCache[pos].InterfaceHandle)
			UsbStack->PipeNumberCache[pos].InterfaceHandle = firstInterfaceHandle;
	}
	return TRUE;
}

BOOL UsbStack_GetCache(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __inout_opt PKUSB_DEVICE_EL Device,
    __inout_opt PKUSB_INTERFACE_EL Interface)

{
	if (Interface)
	{
		memcpy(Interface, UsbStack->InterfaceList, sizeof(*Interface));
	}
	if (Device)
	{
		memcpy(Device, UsbStack->DeviceList, sizeof(*Device));
	}

	return TRUE;
}

USB_STACK_HANDLER_RESULT UsbStackHandler_GetDescriptor (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in UCHAR DescriptorType,
    __in UCHAR Index,
    __in USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out PULONG LengthTransferred)
{
	UNREFERENCED_PARAMETER(LanguageID);
	if (!LengthTransferred)
	{
		LusbwError(ERROR_INVALID_PARAMETER);
		return HANDLER_HANDLED_WITH_FALSE;
	}
	if (DescriptorType == USB_CONFIGURATION_DESCRIPTOR_TYPE && Index == 0)
	{
		if (!Buffer)
		{
			*LengthTransferred = UsbStack->DynamicConfigDescriptorSize;
			LusbwError(ERROR_MORE_DATA);
			return HANDLER_HANDLED_WITH_FALSE;
		}

		*LengthTransferred = min(UsbStack->DynamicConfigDescriptorSize , BufferLength);

		memcpy(Buffer, UsbStack->DynamicConfigDescriptor, *LengthTransferred);
		if (BufferLength < UsbStack->DynamicConfigDescriptorSize)
		{
			LusbwError(ERROR_MORE_DATA);
			return HANDLER_HANDLED_WITH_FALSE;
		}
		return HANDLER_HANDLED_WITH_TRUE;
	}

	return  HANDLER_NOT_HANDLED;
}

USB_STACK_HANDLER_RESULT UsbStackHandler_QueryInterfaceSettings (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in  UCHAR AlternateSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	PKUSB_ALT_INTERFACE_EL nextAltInterfaceEL;

	if (!UsbAltInterfaceDescriptor)
	{
		LusbwError(ERROR_INVALID_PARAMETER);
		return HANDLER_HANDLED_WITH_FALSE;
	}

	if (UsbStack->InterfaceList)
	{
		DL_SEARCH_SCALAR(UsbStack->InterfaceList->AltInterfaceList, nextAltInterfaceEL, Number, AlternateSettingNumber);
		if (!nextAltInterfaceEL)
		{
			LusbwError(ERROR_NO_MORE_ITEMS);
			return HANDLER_HANDLED_WITH_FALSE;
		}

		memcpy(UsbAltInterfaceDescriptor, &nextAltInterfaceEL->Descriptor, sizeof(*UsbAltInterfaceDescriptor));
		return HANDLER_HANDLED_WITH_TRUE;

	}
	return HANDLER_NOT_HANDLED;
}

USB_STACK_HANDLER_RESULT UsbStackHandler_QueryPipe (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in  UCHAR AlternateSettingNumber,
    __in  UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	PKUSB_ALT_INTERFACE_EL nextAltInterfaceEL;
	PKUSB_PIPE_EL nextPipeEL;

	if (!PipeInformation)
	{
		LusbwError(ERROR_INVALID_PARAMETER);
		return HANDLER_HANDLED_WITH_FALSE;
	}

	if (UsbStack->InterfaceList)
	{
		DL_SEARCH_SCALAR(UsbStack->InterfaceList->AltInterfaceList, nextAltInterfaceEL, Number, AlternateSettingNumber);
		if (!nextAltInterfaceEL)
		{
			LusbwError(ERROR_NO_MORE_ITEMS);
			return HANDLER_HANDLED_WITH_FALSE;
		}

		DL_SEARCH_SCALAR(nextAltInterfaceEL->PipeList, nextPipeEL, Index, PipeIndex);
		if (!nextPipeEL)
		{
			LusbwError(ERROR_NO_MORE_ITEMS);
			return HANDLER_HANDLED_WITH_FALSE;
		}

		PipeInformation->PipeType = nextPipeEL->Descriptor.bmAttributes & 0x03;
		PipeInformation->MaximumPacketSize = nextPipeEL->Descriptor.wMaxPacketSize;
		PipeInformation->PipeId = nextPipeEL->Descriptor.bEndpointAddress;
		PipeInformation->Interval = nextPipeEL->Descriptor.bInterval;
		return HANDLER_HANDLED_WITH_TRUE;

	}
	return HANDLER_NOT_HANDLED;
}

USB_STACK_HANDLER_RESULT UsbStackHandler_GetConfiguration (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __out PUCHAR ConfigurationNumber)
{
	if (!ConfigurationNumber)
	{
		LusbwError(ERROR_INVALID_PARAMETER);
		return HANDLER_HANDLED_WITH_FALSE;
	}

	*ConfigurationNumber = UsbStack->DynamicConfigDescriptor->bConfigurationValue;
	return HANDLER_HANDLED_WITH_TRUE;
}


USB_STACK_HANDLER_RESULT UsbStackHandler_SetConfiguration (
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in UCHAR ConfigurationNumber)
{
	if (UsbStack->DynamicConfigDescriptor->bConfigurationValue != ConfigurationNumber)
	{
		LusbwError(ERROR_NO_MORE_ITEMS);
		return HANDLER_HANDLED_WITH_FALSE;
	}
	return HANDLER_HANDLED_WITH_TRUE;
}
