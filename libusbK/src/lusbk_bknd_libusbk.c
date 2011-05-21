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

#include "lusbk_bknd.h"
#include "lusbk_stack_collection.h"
#pragma warning(disable: 4127)

extern ULONG DebugLevel;
extern KUSB_INTERFACE_HANDLE_INTERNAL InternalHandlePool[KUSB_MAX_INTERFACE_HANDLES];

typedef struct _LIBUSBK_BKND_CONTEXT
{
	KUSB_INTERFACE_STACK UsbStack;

	USER_PIPE_POLICY PipePolicies[32];
	version_t Version;
}* PLIBUSBK_BKND_CONTEXT, LIBUSBK_BKND_CONTEXT;

#define K_CTX(BackendContextPtr, InterfaceHandle) \
	GET_BACKEND_CONTEXT((BackendContextPtr), ((PKUSB_INTERFACE_HANDLE_INTERNAL)(InterfaceHandle)), LIBUSBK_BKND_CONTEXT)

#define K_CTXJ(BackendContextPtr, InterfaceHandle, ErrorJump) \
	GET_BACKEND_CONTEXT_EJUMP((BackendContextPtr), ((PKUSB_INTERFACE_HANDLE_INTERNAL)(InterfaceHandle)), LIBUSBK_BKND_CONTEXT, ErrorJump)

#define SubmitSimpleSyncRequest(DeviceHandle, IoControlCode) \
	Ioctl_Sync(DeviceHandle, IoControlCode, &request, sizeof(request), &request, sizeof(request), NULL)


#define LUSBKFN_CTX_PREFIX()							\
	PKUSB_INTERFACE_HANDLE_INTERNAL handle=NULL;		\
	BOOL success=FALSE;									\
	DWORD errorCode=ERROR_SUCCESS;						\
	PLIBUSBK_BKND_CONTEXT backendContext=NULL;			\
														\
	UNREFERENCED_PARAMETER(success);					\
	UNREFERENCED_PARAMETER(errorCode);					\
														\
	GET_INTERNAL_HANDLE(handle);						\
	K_CTX(backendContext, handle)

#define LUSBKFN_CTX_DEVICE_PREFIX()						\
	HANDLE deviceHandle=NULL;							\
	KUSB_DEVICE_EL deviceEL;							\
	LUSBKFN_CTX_PREFIX()

#define LUSBKFN_CTX_INTERFACE_PREFIX()					\
	HANDLE deviceHandle=NULL;							\
	KUSB_INTERFACE_EL interfaceEL;						\
	LUSBKFN_CTX_PREFIX()

#define LUSBKFN_CTX_PIPE_PREFIX()						\
	LUSBKFN_CTX_PREFIX()

#define LUSBKFN_CTX_ALL_PREFIX()						\
	HANDLE deviceHandle=NULL;							\
	KUSB_DEVICE_EL deviceEL;							\
	KUSB_INTERFACE_EL interfaceEL;						\
	LUSBKFN_CTX_PREFIX()

#define GetStackDeviceCache()	 {success=UsbStack_GetCache(&backendContext->UsbStack, &(deviceEL), NULL); if (success) deviceHandle = deviceEL.Device->MasterDeviceHandle; }
#define GetStackInterfaceCache() {success=UsbStack_GetCache(&backendContext->UsbStack, NULL, &(interfaceEL)); if (success) deviceHandle = interfaceEL.ParentDevice->Device->MasterDeviceHandle; }

#define DeviceHandleByPipeID(PipeID)	 GetPipeDeviceHandle(backendContext->UsbStack,PipeID)

#define ErrorStackDeviceCache()		ErrorSet(!success, Error, ERROR_DEVICE_NOT_AVAILABLE, "no devices found.")
#define ErrorStackInterfaceCache()	ErrorSet(!success, Error, ERROR_DEVICE_NOT_AVAILABLE, "no interfaces found.")

///////////////////////////////////////////////////////////////////////
// private libusbk/0 backend functions
///////////////////////////////////////////////////////////////////////

LONG k_ClosedBySetConfigCB(__in PKUSB_INTERFACE_STACK UsbStack,
                           __in PKUSB_DEVICE_EL DeviceElement,
                           __in PVOID Context)
{
	UNREFERENCED_PARAMETER(UsbStack);
	UNREFERENCED_PARAMETER(DeviceElement);
	UNREFERENCED_PARAMETER(Context);

	return ERROR_SUCCESS;
}

static BOOL k_GetVersion(__in HANDLE DeviceHandle, version_t* Version)
{
	libusb_request request;
	BOOL success;

	ErrorParam(!DeviceHandle, Error, "DeviceHandle");
	ErrorParam(!Version, Error, "Version");

	Mem_Zero(&request, sizeof(request));

	success = Ioctl_Sync(DeviceHandle, LIBUSB_IOCTL_GET_VERSION,
	                     &request, sizeof(request),
	                     &request, sizeof(request),
	                     NULL);

	ErrorNoSet(!success, Error, "failed get version request.");
	memcpy(Version, &request.version, sizeof(*Version));

	USBMSG("%s.sys v%u.%u.%u.%u\n",
	       Version->major <= 1 ? "libusb0" : "libusbK",
	       Version->major,
	       Version->minor,
	       Version->micro,
	       Version->nano);

	return TRUE;
Error:
	return FALSE;
}

static LONG k_GetCurrentConfigDescriptorCB(
    __in PKUSB_INTERFACE_STACK UsbStack,
    __in_opt LPCSTR DevicePath,
    __in_opt HANDLE DeviceHandle,
    __in_opt HANDLE InterfaceHandle,
    __out PUSB_CONFIGURATION_DESCRIPTOR* ConfigDescriptorRef,
    __in_opt PVOID Context)
{
	libusb_request request;
	DWORD transferred = 0;
	USB_CONFIGURATION_DESCRIPTOR configCheck;
	BOOL success;
	UCHAR lastConfigValue = 0;
	UCHAR currentConfigNumber = 0;
	UCHAR configIndex = 0;
	PLIBUSBK_BKND_CONTEXT backendContext = (PLIBUSBK_BKND_CONTEXT)Context;

	UNREFERENCED_PARAMETER(UsbStack);
	UNREFERENCED_PARAMETER(DevicePath);
	UNREFERENCED_PARAMETER(InterfaceHandle);

	success = k_GetVersion(DeviceHandle, &backendContext->Version);
	ErrorNoSet(!success, Error, "->k_GetVersion");

	Mem_Zero(&request, sizeof(request));
	success = Ioctl_Sync(DeviceHandle, LIBUSB_IOCTL_GET_CACHED_CONFIGURATION,
	                     &request, sizeof(request),
	                     &request, sizeof(request),
	                     &transferred);

	currentConfigNumber = ((PUCHAR)(&request))[0];
	if (!success || !transferred || !currentConfigNumber)
	{
		currentConfigNumber = 0;
		Mem_Zero(&request, sizeof(request));
		request.configuration.configuration = (unsigned int) - 1;
		success = Ioctl_Sync(DeviceHandle, LIBUSB_IOCTL_SET_CONFIGURATION,
		                     &request, (DWORD)sizeof(request),
		                     &request, (DWORD)sizeof(request),
		                     NULL);

		if (!success)
		{
			USBERR("failed configuring device. ErrorCode = %08Xh\n", GetLastError());
			goto Error;
		}
	}

	Mem_Zero(&configCheck, sizeof(configCheck));

	while(success)
	{
		Mem_Zero(&request, sizeof(request));
		request.descriptor.type = USB_CONFIGURATION_DESCRIPTOR_TYPE;
		request.descriptor.index = configIndex++;

		if (configIndex == UCHAR_MAX)
		{
			success = LusbwError(ERROR_NO_MORE_ITEMS);
			goto Error;
		}

		success = Ioctl_Sync(DeviceHandle, LIBUSB_IOCTL_GET_DESCRIPTOR,
		                     &request, sizeof(request),
		                     &configCheck,
		                     sizeof(configCheck),
		                     NULL);

		if (success)
		{
			if (!currentConfigNumber ||
			        configCheck.bConfigurationValue == currentConfigNumber ||
			        configCheck.bConfigurationValue == lastConfigValue)
			{
				break;
			}

			lastConfigValue = configCheck.bConfigurationValue;
		}
	}

	if (success)
	{
		*ConfigDescriptorRef = Mem_Alloc(configCheck.wTotalLength);
		if (!*ConfigDescriptorRef)
			return FALSE;

		success = Ioctl_Sync(DeviceHandle, LIBUSB_IOCTL_GET_DESCRIPTOR,
		                     &request, sizeof(request),
		                     *ConfigDescriptorRef,
		                     configCheck.wTotalLength,
		                     &transferred);

		if (!success)
		{
			Mem_Free(ConfigDescriptorRef);
		}
	}

Error:
	return success ? ERROR_SUCCESS : GetLastError();
}

static BOOL k_DestroyContext(__inout PKUSB_INTERFACE_HANDLE_INTERNAL InternalHandle)
{
	PLIBUSBK_BKND_CONTEXT backendContext = NULL;

	// handle uninitialized.
	if (!IsHandleValid(InternalHandle))
		return LusbwError(ERROR_INVALID_HANDLE);

	if (InternalHandle->Instance.UsageCount < 1)
		return LusbwError(ERROR_INVALID_HANDLE);

	backendContext = (PLIBUSBK_BKND_CONTEXT)InternalHandle->BackendContext;
	if (!IsHandleValid(backendContext))
		goto Done;

	// free the device/interface stack
	UsbStack_Free(&backendContext->UsbStack, backendContext);

Done:
	// destroy the main context semaphore lock.
	DestroyLock(&InternalHandle->Instance.Lock);

	// Release the handle back to the pool.
	DecUsageCount(InternalHandle);

	Mem_Free(&backendContext);

	return TRUE;
}

LONG k_ClaimReleaseCB (__in PKUSB_INTERFACE_STACK UsbStack,
                       __in PKUSB_INTERFACE_EL InterfaceElement,
                       __in BOOL IsClaim,
                       __in INT InterfaceIndexOrNumber,
                       __in BOOL IsIndex,
                       __in PVOID Context)
{
	libusb_request request;
	BOOL success;
	LONG errorCode = ERROR_SUCCESS;
	INT ioControlCode = IsClaim ? LIBUSBK_IOCTL_CLAIM_INTERFACE : LIBUSBK_IOCTL_RELEASE_INTERFACE;

	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(IsIndex);
	UNREFERENCED_PARAMETER(InterfaceIndexOrNumber);

	if (!InterfaceElement)
		return ERROR_NO_MORE_ITEMS;

	Mem_Zero(&request, sizeof(request));
	request.intf.interface_number = (UCHAR)InterfaceElement->Number;
	success = Ioctl_Sync(InterfaceElement->ParentDevice->Device->MasterDeviceHandle, ioControlCode,
	                     &request, sizeof(request),
	                     &request, sizeof(request),
	                     NULL);

	if (!success)
	{
		errorCode = GetLastError();
		USBWRN("failed %s interface #%d. ret=%d\n",
		       IsClaim ? "claiming" : "releasing",
		       InterfaceElement->Number,
		       errorCode);
	}
	else
	{
		// The most recently claimed interface is always at the list head.
		DL_DELETE(UsbStack->InterfaceList, InterfaceElement);
		if (IsClaim)
		{
			DL_PREPEND(UsbStack->InterfaceList, InterfaceElement);
		}
		else
		{
			DL_APPEND(UsbStack->InterfaceList, InterfaceElement);
		}
		// All device handles and interface handles are cached.
		UsbStack_BuildCache(UsbStack);
	}

	return errorCode;
}

static BOOL k_CreateContext(__out PKUSB_INTERFACE_HANDLE_INTERNAL* InternalHandleRef)
{
	BOOL success;
	PLIBUSBK_BKND_CONTEXT backendContext = NULL;
	PKUSB_INTERFACE_HANDLE_INTERNAL internalHandle = NULL;

	ErrorHandle(!InternalHandleRef, Error, "InternalHandleRef");

	internalHandle = GetInternalPoolHandle();
	ErrorHandle(!internalHandle, Error, "internalHandle");

	memset(internalHandle, 0, sizeof(*internalHandle) - sizeof(internalHandle->Instance));

	backendContext = Mem_Alloc(sizeof(LIBUSBK_BKND_CONTEXT));
	ErrorMemory(!IsHandleValid(backendContext), Error);
	internalHandle->BackendContext = backendContext;

	success = UsbStack_Init(&backendContext->UsbStack, k_GetCurrentConfigDescriptorCB, k_ClaimReleaseCB, NULL, NULL, NULL);
	ErrorNoSet(!success, Error, "->UsbStack_Init");

	success = InitInterfaceLock(&internalHandle->Instance.Lock);
	ErrorNoSet(!success, Error, "->InitInterfaceLock");

	*InternalHandleRef = internalHandle;
	return TRUE;

Error:
	k_DestroyContext(internalHandle);
	*InternalHandleRef = NULL;
	return FALSE;
}

BOOL k_ClaimOrReleaseInterface(__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
                               __in INT InterfaceNumberOrIndex,
                               __in UCHAR IsClaim,
                               __in UCHAR IsIndex)
{
	LONG action;
	LUSBKFN_CTX_PREFIX();

	if (InterfaceNumberOrIndex < 0)
		return LusbwError(ERROR_NO_MORE_ITEMS);

	AcquireSyncLockWrite(&handle->Instance.Lock);

	GET_BACKEND_CONTEXT(backendContext, handle, LIBUSBK_BKND_CONTEXT);

	action = UsbStack_ClaimOrRelease(&backendContext->UsbStack,
	                                 IsClaim,
	                                 InterfaceNumberOrIndex,
	                                 IsIndex,
	                                 handle);

	ReleaseSyncLockWrite(&handle->Instance.Lock);

	if (action == ERROR_SUCCESS)
		return TRUE;

	return LusbwError(action);
}

KUSB_EXP BOOL KUSB_API UsbK_Initialize(
    __in HANDLE DeviceHandle,
    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	BOOL success;
	PKUSB_INTERFACE_HANDLE_INTERNAL interfaceHandle = NULL;
	PLIBUSBK_BKND_CONTEXT backendContext = NULL;

	CheckLibInitialized();

	ErrorHandle(!IsHandleValid(DeviceHandle), Error, "DeviceHandle");
	ErrorHandle(!IsHandleValid(InterfaceHandle), Error, "InterfaceHandle");

	success = k_CreateContext(&interfaceHandle);
	ErrorNoSet(!success, Error, "->k_CreateContext");

	K_CTX(backendContext, interfaceHandle);
	ErrorHandle(!backendContext, Error, "backendContext");

	success = UsbStack_AddDevice(
	              &backendContext->UsbStack,
	              NULL,
	              DeviceHandle,
	              backendContext);

	ErrorNoSet(!success, Error, "->UsbStack_AddDevice");

	*InterfaceHandle = interfaceHandle;
	return TRUE;

Error:
	k_DestroyContext(interfaceHandle);
	*InterfaceHandle = NULL;
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_SetConfiguration(
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR ConfigurationNumber)
{
	libusb_request request;
	USB_STACK_HANDLER_RESULT result;
	CHAR devicePath[MAX_PATH];

	LUSBKFN_CTX_DEVICE_PREFIX();

	if (backendContext->UsbStack.DeviceCount > 1 ||
	        backendContext->Version.major > 1)
	{
		// The best thing we can do to for composite device is nothing.
		// there is no support for this in usbccgp.

		// currently this is only supported by libusb0; so the same is true for K.
		AcquireSyncLockRead(&handle->Instance.Lock);
		result = UsbStackHandler_SetConfiguration(&backendContext->UsbStack, ConfigurationNumber);
		ReleaseSyncLockRead(&handle->Instance.Lock);

		return (BOOL)(result & 1);
	}

	// we will need to rebuild the interface stack completely, so get the write lock now.
	AcquireSyncLockWrite(&handle->Instance.Lock);

	GetStackDeviceCache();
	ErrorStackDeviceCache();

	Mem_Zero(&request, sizeof(request));
	request.configuration.configuration = ConfigurationNumber;
	success = Ioctl_Sync(deviceEL.Device->MasterDeviceHandle, LIBUSB_IOCTL_SET_CONFIGURATION,
	                     &request, sizeof(request),
	                     &request, sizeof(request),
	                     NULL);
	if (!success)
	{
		USBERR("failed setting configuration #%d\n", ConfigurationNumber);
		goto Error;
	}

	if (!Str_IsNullOrEmpty(deviceEL.Device->DevicePath))
		strcpy_s(devicePath, sizeof(devicePath) - 1, deviceEL.Device->DevicePath);
	else
		Mem_Zero(devicePath, sizeof(devicePath));

	deviceHandle = deviceEL.Device->MasterDeviceHandle;

	// rebuild the interface list.
	backendContext->UsbStack.Cb.CloseDevice = k_ClosedBySetConfigCB;
	UsbStack_Free(&backendContext->UsbStack, backendContext);

	UsbStack_Init(&backendContext->UsbStack, k_GetCurrentConfigDescriptorCB, k_ClaimReleaseCB, NULL, NULL, NULL);

	success = UsbStack_AddDevice(
	              &backendContext->UsbStack,
	              devicePath,
	              deviceHandle,
	              backendContext);

Error:
	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_GetConfiguration(
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR ConfigurationNumber)
{
	libusb_request request;
	USB_STACK_HANDLER_RESULT handerResult;
	DWORD transferred;
	UCHAR PipeID = 0;
	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	handerResult = UsbStackHandler_GetConfiguration(&backendContext->UsbStack, ConfigurationNumber);

	if (handerResult != HANDLER_NOT_HANDLED)
	{
		ReleaseSyncLockRead(&handle->Instance.Lock);
		return(BOOL)handerResult & 1;
	}

	Mem_Zero(&request, sizeof(request));
	success = Ioctl_Sync(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_GET_CACHED_CONFIGURATION,
	                     &request, sizeof(request),
	                     &request, sizeof(request),
	                     &transferred);

	if (!success && !transferred)
	{
		USBERR("failed getting cached configration number\n");
	}
	else
	{
		*ConfigurationNumber = ((PUCHAR)&request)[0];
	}

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_Free (__in LIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	PKUSB_INTERFACE_HANDLE_INTERNAL handle;

	GET_INTERNAL_HANDLE(handle);

	AcquireSyncLockWrite(&handle->Instance.Lock);

	k_DestroyContext(handle);

	// Free always returns true;  The Close method does not. This is the primary difference.
	SetLastError(ERROR_SUCCESS);
	return TRUE;
}

KUSB_EXP BOOL KUSB_API UsbK_GetAssociatedInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AssociatedInterfaceIndex,
    __out PLIBUSBK_INTERFACE_HANDLE AssociatedInterfaceHandle)
{
	PKUSB_INTERFACE_HANDLE_INTERNAL assocHandle = NULL;
	PLIBUSBK_BKND_CONTEXT assocContext = NULL;
	PKUSB_INTERFACE_EL assocEL = NULL;
	INT findVirtualIndex;

	LUSBKFN_CTX_PREFIX();

	AcquireSyncLockWrite(&handle->Instance.Lock);

	ErrorHandle(!backendContext->UsbStack.InterfaceList, Error, "UsbStack.InterfaceList");

	// this is where we are right now, we must find [this] + 1 + AssociatedInterfaceIndex
	findVirtualIndex = backendContext->UsbStack.InterfaceList->VirtualIndex;
	findVirtualIndex = (findVirtualIndex + 1) + (INT)AssociatedInterfaceIndex;

	DL_SEARCH_SCALAR(backendContext->UsbStack.InterfaceList, assocEL, VirtualIndex, findVirtualIndex);
	if (!assocEL)
	{
		LusbwError(ERROR_NO_MORE_ITEMS);
		goto Error;
	}

	success = k_CreateContext(&assocHandle);
	ErrorNoSet(!success, Error, "->k_CreateContext");

	K_CTX(assocContext, assocHandle);
	ErrorHandle(!assocContext, Error, "assocContext");

	success = UsbStack_Init(&assocContext->UsbStack, k_GetCurrentConfigDescriptorCB, k_ClaimReleaseCB, NULL, NULL, NULL);
	ErrorNoSet(!success, Error, "->UsbStack_Init");

	UsbStack_Clone(&backendContext->UsbStack, &assocContext->UsbStack);
	ErrorNoSet(!success, Error, "->UsbStack_Clone");

	// move this interface to top if stack for the cloned handle.
	DL_SEARCH_SCALAR(assocContext->UsbStack.InterfaceList, assocEL, VirtualIndex, findVirtualIndex);
	DL_DELETE(assocContext->UsbStack.InterfaceList, assocEL);
	DL_PREPEND(assocContext->UsbStack.InterfaceList, assocEL);

	// clone the backend
	memcpy(&assocContext->Version, &backendContext->Version, sizeof(assocContext->Version));
	memcpy(&assocContext->PipePolicies, &backendContext->PipePolicies, sizeof(assocContext->PipePolicies));

	// clone the users context
	memcpy(&assocHandle->UserContext, &handle->UserContext, sizeof(assocHandle->UserContext));

	// rebuild the pipe caches
	success = UsbStack_BuildCache(&assocContext->UsbStack);
	ErrorNoSet(!success, Error, "->UsbStack_BuildCache");

	*AssociatedInterfaceHandle = assocHandle;

	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return TRUE;

Error:
	k_DestroyContext(assocHandle);

	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_GetDescriptor (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR DescriptorType,
    __in UCHAR Index,
    __in USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out PULONG LengthTransferred)
{
	libusb_request request;
	USB_STACK_HANDLER_RESULT handerResult;
	UCHAR PipeID = 0;
	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	handerResult = UsbStackHandler_GetDescriptor(
	                   &backendContext->UsbStack,
	                   DescriptorType,
	                   Index,
	                   LanguageID,
	                   Buffer,
	                   BufferLength,
	                   LengthTransferred);


	if (handerResult != HANDLER_NOT_HANDLED)
	{
		ReleaseSyncLockRead(&handle->Instance.Lock);
		return(BOOL)handerResult & 1;
	}

	Mem_Zero(&request, sizeof(request));
	request.descriptor.type = DescriptorType;
	request.descriptor.index = Index;
	request.descriptor.language_id = LanguageID;

	success = Ioctl_Sync(
	              DeviceHandleByPipeID(PipeID),
	              LIBUSB_IOCTL_GET_DESCRIPTOR,
	              &request, sizeof(request),
	              Buffer, BufferLength,
	              LengthTransferred);

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_QueryInterfaceSettings (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AlternateSettingNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	USB_STACK_HANDLER_RESULT handerResult;

	LUSBKFN_CTX_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	handerResult = UsbStackHandler_QueryInterfaceSettings(
	                   &backendContext->UsbStack,
	                   AlternateSettingNumber,
	                   UsbAltInterfaceDescriptor);


	if (handerResult != HANDLER_NOT_HANDLED)
	{
		ReleaseSyncLockRead(&handle->Instance.Lock);
		return(BOOL)handerResult & 1;
	}

	success = LusbwError(ERROR_NOT_SUPPORTED);
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_QueryDeviceInformation (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG InformationType,
    __inout PULONG BufferLength,
    __out PVOID Buffer)
{

	libusb_request request;
	LUSBKFN_CTX_INTERFACE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	GetStackInterfaceCache();
	ErrorStackInterfaceCache();

	Mem_Zero(&request, sizeof(request));
	request.query_device.information_type = InformationType;

	success = Ioctl_Sync(deviceHandle, LIBUSB_IOCTL_QUERY_DEVICE_INFORMATION,
	                     &request, sizeof(request),
	                     Buffer, *BufferLength,
	                     BufferLength);

Error:
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_SetCurrentAlternateSetting (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AlternateSettingNumber)
{


	libusb_request request;
	LUSBKFN_CTX_INTERFACE_PREFIX();

	AcquireSyncLockWrite(&handle->Instance.Lock);

	GetStackInterfaceCache();
	ErrorStackInterfaceCache();

	Mem_Zero(&request, sizeof(request));
	request.intf.interface_number = interfaceEL.Number;
	request.intf.altsetting_number = AlternateSettingNumber;
	request.intf.intf_use_index = FALSE;
	request.intf.altf_use_index = FALSE;

	success = SubmitSimpleSyncRequest(deviceHandle, LIBUSBK_IOCTL_SET_INTERFACE);
	if (success)
	{
		success = (BOOL)UsbStackHandler_SetAltInterface(
		              &backendContext->UsbStack,
		              request.intf.interface_number, FALSE,
		              request.intf.altsetting_number, FALSE);
	}

Error:
	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return success;

}

KUSB_EXP BOOL KUSB_API UsbK_GetCurrentAlternateSetting (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR AlternateSettingNumber)
{
	libusb_request request;
	LUSBKFN_CTX_INTERFACE_PREFIX();

	AcquireSyncLockWrite(&handle->Instance.Lock);

	GetStackInterfaceCache();
	ErrorStackInterfaceCache();

	Mem_Zero(&request, sizeof(request));
	request.intf.interface_number = interfaceEL.Number;
	request.intf.intf_use_index = FALSE;
	request.intf.altf_use_index = FALSE;

	success = SubmitSimpleSyncRequest(deviceHandle, LIBUSBK_IOCTL_GET_INTERFACE);

	if (success)
	{
		*AlternateSettingNumber = (UCHAR)request.intf.altsetting_number;

		success = (BOOL)UsbStackHandler_SetAltInterface(
		              &backendContext->UsbStack,
		              request.intf.interface_number, FALSE,
		              request.intf.altsetting_number, FALSE);

	}

Error:
	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_QueryPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR AlternateSettingNumber,
    __in UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	USB_STACK_HANDLER_RESULT handerResult;

	LUSBKFN_CTX_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	handerResult = UsbStackHandler_QueryPipe(
	                   &backendContext->UsbStack,
	                   AlternateSettingNumber,
	                   PipeIndex,
	                   PipeInformation);


	if (handerResult != HANDLER_NOT_HANDLED)
	{
		ReleaseSyncLockRead(&handle->Instance.Lock);
		return(BOOL)handerResult & 1;
	}

	success = LusbwError(ERROR_NOT_SUPPORTED);
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_SetPipePolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value)
{
	ULONG requestLength = sizeof(libusb_request) + ValueLength;
	libusb_request* request = NULL;

	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	request = AllocRequest(ValueLength);
	ErrorSet(!IsHandleValid(request), Error, ERROR_NOT_ENOUGH_MEMORY, "ValueLength=%u", ValueLength);

	request->pipe_policy.pipe_id = PipeID;
	request->pipe_policy.policy_type = PolicyType;

	if (backendContext->Version.major > 1)
	{
		// this is a libusbK driver; it has full support for policies
		success = Ioctl_Sync(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_SET_PIPE_POLICY,
		                     request, requestLength,
		                     NULL, 0, NULL);
	}
	else
	{
		if (!(PipeID & 0x0F))
		{
			// libusb-win32 must do a little in the driver and a little in the dll.
			// this pipe policy is for the default pipe; they are handled by the driver.
			SetRequestData(request, Value, ValueLength);
			success = Ioctl_Sync(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_SET_PIPE_POLICY,
			                     request, requestLength,
			                     NULL, 0, NULL);
		}
		else
		{
			// sync pipe transfer timeouts must be handled in user mode for libusb0.
			// setting the user pipe policy timeout causes the SyncTransfer function
			// to wait and cancel if the timeout expires.
			switch(PolicyType)
			{
			case PIPE_TRANSFER_TIMEOUT:
				if (Value && (ValueLength >= 4))
				{
					InterlockedExchange((PLONG)&GetSetPipePolicy(backendContext, PipeID).timeout, (LONG) * ((PULONG)Value));
					success = TRUE;
				}
				break;
			default:
				success = LusbwError(ERROR_NOT_SUPPORTED);
				break;
			}
		}
	}

Error:
	Mem_Free(&request);
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_GetPipePolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{

	libusb_request request;

	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	request.pipe_policy.pipe_id = PipeID;
	request.pipe_policy.policy_type = PolicyType;

	if (backendContext->Version.major > 1)
	{
		// this is a libusbK driver; it has full support for policies
		success = Ioctl_Sync(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_GET_PIPE_POLICY,
		                     &request, sizeof(request),
		                     Value, *ValueLength,
		                     ValueLength);
	}
	else
	{
		if (!(PipeID & 0x0F))
		{
			// libusb-win32 must do a little in the driver and a little in the dll.
			// this pipe policy is for the default pipe; they are handled by the driver.
			success = Ioctl_Sync(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_GET_PIPE_POLICY,
			                     &request, sizeof(request),
			                     Value, *ValueLength,
			                     ValueLength);
		}
		else
		{
			// sync pipe transfer timeouts must be handled in user mode for libusb0.
			// setting the user pipe policy timeout causes the SyncTransfer function
			// to wait and cancel if the timeout expires.
			switch(PolicyType)
			{
			case PIPE_TRANSFER_TIMEOUT:
				if (*ValueLength >= 4)
				{
					((PULONG)Value)[0] = GetSetPipePolicy(backendContext, PipeID).timeout;
					*ValueLength = 4;
					success = TRUE;
				}
				break;
			default:
				success = LusbwError(ERROR_NOT_SUPPORTED);
				break;
			}
		}
	}

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_ReadPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	libusb_request request;

	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	Mem_Zero(&request, sizeof(request));
	request.endpoint.endpoint = PipeID;

	if (Overlapped)
	{
		success = Ioctl_Async(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ,
		                      &request, sizeof(request),
		                      Buffer, BufferLength,
		                      Overlapped);
	}
	else
	{
		PLIBUSBK_BKND_CONTEXT backendContext;
		GET_BACKEND_CONTEXT(backendContext, handle, LIBUSBK_BKND_CONTEXT);

		success = Ioctl_SyncWithTimeout(DeviceHandleByPipeID(PipeID),
		                                LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ,
		                                &request,
		                                Buffer, BufferLength,
		                                GetSetPipePolicy(backendContext, PipeID).timeout,
		                                PipeID,
		                                LengthTransferred);
	}

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_WritePipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	libusb_request request;

	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	Mem_Zero(&request, sizeof(request));
	request.endpoint.endpoint = PipeID;

	if (Overlapped)
	{
		success = Ioctl_Async(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE,
		                      &request, sizeof(request),
		                      Buffer, BufferLength,
		                      Overlapped);
	}
	else
	{
		success = Ioctl_SyncWithTimeout(DeviceHandleByPipeID(PipeID),
		                                LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE,
		                                &request,
		                                Buffer, BufferLength,
		                                GetSetPipePolicy(backendContext, PipeID).timeout,
		                                PipeID,
		                                LengthTransferred);
	}

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_ControlTransfer (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in WINUSB_SETUP_PACKET SetupPacket,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt  LPOVERLAPPED Overlapped)
{
	libusb_request request;
	INT ioctlCode;
	UCHAR PipeID = 0;
	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);


	Mem_Zero(&request, sizeof(request));
	memcpy(&request.control, &SetupPacket, sizeof(request.control));

	ioctlCode = (SetupPacket.RequestType & USB_ENDPOINT_DIRECTION_MASK) ? LIBUSB_IOCTL_CONTROL_READ : LIBUSB_IOCTL_CONTROL_WRITE;

	if (Overlapped)
	{
		success = Ioctl_Async(DeviceHandleByPipeID(PipeID), ioctlCode, &request, sizeof(request), Buffer, BufferLength, Overlapped);
	}
	else
	{
		UCHAR pipeID = (SetupPacket.RequestType & USB_ENDPOINT_DIRECTION_MASK) ? 0x80 : 0x00;
		request.timeout = GetSetPipePolicy(backendContext, pipeID).timeout;

		success = Ioctl_Sync(DeviceHandleByPipeID(PipeID), ioctlCode, &request, sizeof(request), Buffer, BufferLength, LengthTransferred);
	}

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_ResetPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	libusb_request request;

	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	Mem_Zero(&request, sizeof(request));
	request.endpoint.endpoint = PipeID;

	success = Ioctl_Sync(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_RESET_ENDPOINT,
	                     &request, sizeof(request),
	                     NULL, 0,
	                     NULL);

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_AbortPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	libusb_request request;

	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	Mem_Zero(&request, sizeof(request));
	request.endpoint.endpoint = PipeID;

	success = Ioctl_Sync(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_ABORT_ENDPOINT,
	                     &request, sizeof(request),
	                     NULL, 0,
	                     NULL);

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_FlushPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID)
{
	libusb_request request;

	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	Mem_Zero(&request, sizeof(request));
	request.endpoint.endpoint = PipeID;

	success = Ioctl_Sync(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_FLUSH_PIPE,
	                     &request, sizeof(request),
	                     NULL, 0,
	                     NULL);

	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_SetPowerPolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __in ULONG ValueLength,
    __in PVOID Value)
{
	libusb_request* request;
	UCHAR PipeID = 0;
	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);
	ErrorParam(Value == NULL, Error, "Value");

	request = AllocRequest(ValueLength);
	ErrorSet(!IsHandleValid(request), Error, ERROR_NOT_ENOUGH_MEMORY, "ValueLength=%u", ValueLength);

	request->power_policy.policy_type = PolicyType;
	SetRequestData(request, Value, ValueLength);

	success = Ioctl_Sync(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_SET_POWER_POLICY,
	                     request, sizeof(libusb_request) + ValueLength,
	                     NULL, 0,
	                     NULL);

Error:
	Mem_Free(&request);
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_GetPowerPolicy (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	libusb_request request;
	UCHAR PipeID = 0;
	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);
	ErrorParam(!Value, Error, "Value");
	ErrorParam(!ValueLength, Error, "ValueLength");

	Mem_Zero(&request, sizeof(request));
	request.power_policy.policy_type = PolicyType;

	success = Ioctl_Sync(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_GET_POWER_POLICY,
	                     &request, sizeof(request),
	                     Value, *ValueLength,
	                     ValueLength);

Error:
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_GetOverlappedResult (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in BOOL bWait)
{
	LUSBKFN_CTX_INTERFACE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	GetStackInterfaceCache();
	ErrorStackInterfaceCache();

	success = GetOverlappedResult(deviceHandle, lpOverlapped, lpNumberOfBytesTransferred, bWait);

Error:
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_Open(
    __in PKUSB_DEV_LIST DeviceListItem,
    __out PLIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	BOOL success;
	PKUSB_INTERFACE_HANDLE_INTERNAL interfaceHandle = NULL;
	PLIBUSBK_BKND_CONTEXT backendContext = NULL;

	PKUSB_DEV_LIST nextCompositeEL;

	CheckLibInitialized();

	if(!IsHandleValid(DeviceListItem) || !IsHandleValid(InterfaceHandle))
	{
		success = LusbwError(ERROR_INVALID_HANDLE);
		goto Error;
	}

	success = k_CreateContext(&interfaceHandle);
	ErrorNoSet(!success, Error, "->k_CreateContext");

	K_CTX(backendContext, interfaceHandle);
	ErrorHandle(!backendContext, Error, "backendContext");

	if (DeviceListItem->CompositeList)
	{
		DL_FOREACH(DeviceListItem->CompositeList, nextCompositeEL)
		{
			success = UsbStack_AddDevice(
			              &backendContext->UsbStack,
			              nextCompositeEL->DevicePath,
			              NULL,
			              backendContext);

			if (!success)
			{
				USBWRN("skipping composite device %s. ErrorCode=%08Xh\n",
				       nextCompositeEL->DeviceDesc, GetLastError());
			}
		}
	}
	else
	{

		success = UsbStack_AddDevice(
		              &backendContext->UsbStack,
		              DeviceListItem->DevicePath,
		              NULL,
		              NULL);
	}

	ErrorNoSet(!backendContext->UsbStack.DeviceCount, Error, "failed opening %s device. ErrorCode=%d",
	           DeviceListItem->DeviceDesc, GetLastError());

	*InterfaceHandle = interfaceHandle;
	return TRUE;

Error:
	k_DestroyContext(interfaceHandle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API UsbK_Close(
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	LUSBKFN_CTX_PREFIX();

	// k_DestroyContext will also destroy the lock.
	AcquireSyncLockWrite(&handle->Instance.Lock);

	success = k_DestroyContext(handle);
	if (!success)
	{
		USBERR("->k_DestroyContext\n");
	}

	return success;

}


KUSB_EXP BOOL KUSB_API UsbK_ClaimInterface (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex)
{

	return k_ClaimOrReleaseInterface(InterfaceHandle, InterfaceNumberOrIndex, TRUE, (UCHAR)IsIndex);
}


KUSB_EXP BOOL KUSB_API UsbK_ReleaseInterface(
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex)
{
	return k_ClaimOrReleaseInterface(InterfaceHandle, InterfaceNumberOrIndex, FALSE, (UCHAR)IsIndex);
}


KUSB_EXP BOOL KUSB_API UsbK_SetAltInterface(
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex,
    __in UCHAR AltInterfaceNumber)
{
	PKUSB_INTERFACE_EL interfaceEL = NULL;
	libusb_request request;
	LUSBKFN_CTX_PREFIX();

	AcquireSyncLockWrite(&handle->Instance.Lock);

	if (IsIndex)
	{
		DL_SEARCH_SCALAR(backendContext->UsbStack.InterfaceList, interfaceEL, VirtualIndex, InterfaceNumberOrIndex);
	}
	else
	{
		DL_SEARCH_SCALAR(backendContext->UsbStack.InterfaceList, interfaceEL, Number, InterfaceNumberOrIndex);
	}
	if (!interfaceEL)
	{
		USBWRN("interface #%u not found.\n", InterfaceNumberOrIndex);
		success = LusbwError(ERROR_NO_MORE_ITEMS);
		goto Error;
	}

	Mem_Zero(&request, sizeof(request));
	request.intf.interface_number = interfaceEL->Number;
	request.intf.altsetting_number = AltInterfaceNumber;

	request.intf.intf_use_index = FALSE;
	request.intf.altf_use_index = FALSE;

	success = SubmitSimpleSyncRequest(interfaceEL->ParentDevice->Device->MasterDeviceHandle, LIBUSBK_IOCTL_SET_INTERFACE);
	if (success)
	{
		success = (BOOL)UsbStackHandler_SetAltInterface(
		              &backendContext->UsbStack,
		              request.intf.interface_number, FALSE,
		              request.intf.altsetting_number, FALSE);
		if (!success)
		{
			USBERR("->UsbStackHandler_SetAltInterface\n");
		}
	}

Error:
	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return success;
}


KUSB_EXP BOOL KUSB_API UsbK_GetAltInterface(
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR InterfaceNumberOrIndex,
    __in BOOL IsIndex,
    __out PUCHAR AltInterfaceNumber)
{
	PKUSB_INTERFACE_EL interfaceEL = NULL;
	libusb_request request;
	LUSBKFN_CTX_PREFIX();

	AcquireSyncLockWrite(&handle->Instance.Lock);

	ErrorParam(!AltInterfaceNumber, Error, "AltInterfaceNumber");

	if (IsIndex)
	{
		DL_SEARCH_SCALAR(backendContext->UsbStack.InterfaceList, interfaceEL, VirtualIndex, InterfaceNumberOrIndex);
	}
	else
	{
		DL_SEARCH_SCALAR(backendContext->UsbStack.InterfaceList, interfaceEL, Number, InterfaceNumberOrIndex);
	}
	if (!interfaceEL)
	{
		USBWRN("interface #%u not found.\n", InterfaceNumberOrIndex);
		success = LusbwError(ERROR_NO_MORE_ITEMS);
		goto Error;
	}

	Mem_Zero(&request, sizeof(request));
	request.intf.interface_number = interfaceEL->Number;
	request.intf.intf_use_index = FALSE;

	success = SubmitSimpleSyncRequest(interfaceEL->ParentDevice->Device->MasterDeviceHandle, LIBUSBK_IOCTL_GET_INTERFACE);
	if (success)
	{
		UsbStackHandler_SetAltInterface(
		    &backendContext->UsbStack,
		    request.intf.interface_number, FALSE,
		    request.intf.altsetting_number, FALSE);

		*AltInterfaceNumber = (UCHAR)request.intf.altsetting_number;
	}

Error:
	ReleaseSyncLockWrite(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_IsoReadPipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in ULONG BufferLength,
    __in ULONG IsoPacketSize,
    __in LPOVERLAPPED Overlapped)
{
	libusb_request request;

	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	if (!IsHandleValid(Overlapped))
	{
		USBERR("isochronous transfers require an 'Overlapped' parameter.\n");
		success = LusbwError(ERROR_INVALID_PARAMETER);
		goto Error;
	}

	Mem_Zero(&request, sizeof(request));
	request.endpoint.endpoint = PipeID;
	request.endpoint.packet_size = IsoPacketSize;

	success = Ioctl_Async(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_ISOCHRONOUS_READ,
	                      &request, sizeof(request),
	                      Buffer, BufferLength,
	                      Overlapped);

Error:
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_IsoWritePipe (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle,
    __in UCHAR PipeID,
    __in PUCHAR Buffer,
    __in ULONG BufferLength,
    __in ULONG IsoPacketSize,
    __in LPOVERLAPPED Overlapped)
{
	libusb_request request;

	LUSBKFN_CTX_PIPE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	if (!IsHandleValid(Overlapped))
	{
		USBERR("isochronous transfers require an 'Overlapped' parameter.\n");
		success = LusbwError(ERROR_INVALID_PARAMETER);
		goto Error;
	}

	Mem_Zero(&request, sizeof(request));
	request.endpoint.endpoint = PipeID;
	request.endpoint.packet_size = IsoPacketSize;

	success = Ioctl_Async(DeviceHandleByPipeID(PipeID), LIBUSB_IOCTL_ISOCHRONOUS_WRITE,
	                      &request, sizeof(request),
	                      Buffer, BufferLength,
	                      Overlapped);


Error:
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

KUSB_EXP BOOL KUSB_API UsbK_ResetDevice (
    __in LIBUSBK_INTERFACE_HANDLE InterfaceHandle)
{
	libusb_request request;
	LUSBKFN_CTX_INTERFACE_PREFIX();

	AcquireSyncLockRead(&handle->Instance.Lock);

	GetStackInterfaceCache();
	ErrorStackInterfaceCache();

	Mem_Zero(&request, sizeof(request));
	success = Ioctl_Sync(deviceHandle, LIBUSB_IOCTL_RESET_DEVICE,
	                     &request, sizeof(request),
	                     NULL, 0,
	                     NULL);

Error:
	ReleaseSyncLockRead(&handle->Instance.Lock);
	return success;
}

BOOL GetProcAddress_UsbK(__out KPROC* ProcAddress, __in ULONG FunctionID)
{
	DWORD rtn = ERROR_SUCCESS;

	switch(FunctionID)
	{
	case KUSB_FNID_Initialize:
		*ProcAddress = (KPROC)UsbK_Initialize;
		break;
	case KUSB_FNID_Free:
		*ProcAddress = (KPROC)UsbK_Free;
		break;
	case KUSB_FNID_GetAssociatedInterface:
		*ProcAddress = (KPROC)UsbK_GetAssociatedInterface;
		break;
	case KUSB_FNID_GetDescriptor:
		*ProcAddress = (KPROC)UsbK_GetDescriptor;
		break;
	case KUSB_FNID_QueryInterfaceSettings:
		*ProcAddress = (KPROC)UsbK_QueryInterfaceSettings;
		break;
	case KUSB_FNID_QueryDeviceInformation:
		*ProcAddress = (KPROC)UsbK_QueryDeviceInformation;
		break;
	case KUSB_FNID_SetCurrentAlternateSetting:
		*ProcAddress = (KPROC)UsbK_SetCurrentAlternateSetting;
		break;
	case KUSB_FNID_GetCurrentAlternateSetting:
		*ProcAddress = (KPROC)UsbK_GetCurrentAlternateSetting;
		break;
	case KUSB_FNID_QueryPipe:
		*ProcAddress = (KPROC)UsbK_QueryPipe;
		break;
	case KUSB_FNID_SetPipePolicy:
		*ProcAddress = (KPROC)UsbK_SetPipePolicy;
		break;
	case KUSB_FNID_GetPipePolicy:
		*ProcAddress = (KPROC)UsbK_GetPipePolicy;
		break;
	case KUSB_FNID_ReadPipe:
		*ProcAddress = (KPROC)UsbK_ReadPipe;
		break;
	case KUSB_FNID_WritePipe:
		*ProcAddress = (KPROC)UsbK_WritePipe;
		break;
	case KUSB_FNID_ControlTransfer:
		*ProcAddress = (KPROC)UsbK_ControlTransfer;
		break;
	case KUSB_FNID_ResetPipe:
		*ProcAddress = (KPROC)UsbK_ResetPipe;
		break;
	case KUSB_FNID_AbortPipe:
		*ProcAddress = (KPROC)UsbK_AbortPipe;
		break;
	case KUSB_FNID_FlushPipe:
		*ProcAddress = (KPROC)UsbK_FlushPipe;
		break;
	case KUSB_FNID_SetPowerPolicy:
		*ProcAddress = (KPROC)UsbK_SetPowerPolicy;
		break;
	case KUSB_FNID_GetPowerPolicy:
		*ProcAddress = (KPROC)UsbK_GetPowerPolicy;
		break;
	case KUSB_FNID_GetOverlappedResult:
		*ProcAddress = (KPROC)UsbK_GetOverlappedResult;
		break;
	case KUSB_FNID_ResetDevice:
		*ProcAddress = (KPROC)UsbK_ResetDevice;
		break;
	case KUSB_FNID_Open:
		*ProcAddress = (KPROC)UsbK_Open;
		break;
	case KUSB_FNID_Close:
		*ProcAddress = (KPROC)UsbK_Close;
		break;
	case KUSB_FNID_SetConfiguration:
		*ProcAddress = (KPROC)UsbK_SetConfiguration;
		break;
	case KUSB_FNID_GetConfiguration:
		*ProcAddress = (KPROC)UsbK_GetConfiguration;
		break;
	case KUSB_FNID_ClaimInterface:
		*ProcAddress = (KPROC)UsbK_ClaimInterface;
		break;
	case KUSB_FNID_ReleaseInterface:
		*ProcAddress = (KPROC)UsbK_ReleaseInterface;
		break;
	case KUSB_FNID_SetAltInterface:
		*ProcAddress = (KPROC)UsbK_SetAltInterface;
		break;
	case KUSB_FNID_GetAltInterface:
		*ProcAddress = (KPROC)UsbK_GetAltInterface;
		break;
	case KUSB_FNID_IsoReadPipe:
		*ProcAddress = (KPROC)UsbK_IsoReadPipe;
		break;
	case KUSB_FNID_IsoWritePipe:
		*ProcAddress = (KPROC)UsbK_IsoWritePipe;
		break;

	default:
		rtn = ERROR_NOT_SUPPORTED;
		*ProcAddress = (KPROC)NULL;
		USBERR("Unrecognized function id! FunctionID=%u\n", FunctionID);
		break;

	}
	return LusbwError(rtn);
}
