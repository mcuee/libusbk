
#include "lusbk_bknd.h"

BOOL LibInitialized = FALSE;
volatile LONG LibInitializedLockCount = 0;

// Default loggging level
ULONG DebugLevel = 4;

volatile LONG NextHandlePos = -1;
KUSB_INTERFACE_HANDLE_INTERNAL HandleList[KUSB_MAX_INTERFACE_HANDLES];

HMODULE winusb_dll = NULL;

///////////////////////////////////////////////////////////////////////

VOID CheckLibInitialized()
{
recheck:
	if (!LibInitialized)
	{
		if (InterlockedIncrement(&LibInitializedLockCount) == 1)
		{
			Mem_Zero(HandleList, sizeof(HandleList));
			winusb_dll = LoadLibraryA("winusb.dll");
			LibInitialized = TRUE;
		}
		else
		{
			// this should never happen, but it would be devastating if it did.
			// for this reason a very crude home-rolled spinlock is used.
			Sleep(1);
			InterlockedDecrement(&LibInitializedLockCount);
			goto recheck;
		}
	}
}

PKUSB_INTERFACE_HANDLE_INTERNAL GetNextAvailableHandle()
{
	LONG index = NextHandlePos;
	LONG count = KUSB_MAX_INTERFACE_HANDLES;
	while (count--)
	{
		if ((++index) >= KUSB_MAX_INTERFACE_HANDLES)
			index = 0;

		if (InterlockedIncrement(&HandleList[index].inst.ValidCount) == 1)
		{
			// try and keep NextHandlePos at an index that will work good for the next
			// GetNextAvailableHandle() call.
			InterlockedExchange(&NextHandlePos, index);
			return &HandleList[index];
		}

		// this handle is in use.
		InterlockedDecrement(&HandleList[index].inst.ValidCount);
	}
	USBERR("no more internal interface handles! (max=%d)\n", KUSB_MAX_INTERFACE_HANDLES);
	return NULL;
}

VOID InitInterfaceHandle(__in PKUSB_INTERFACE_HANDLE_INTERNAL InterfaceHandle,
                         __in_opt HANDLE DeviceHandle,
                         __in_opt UCHAR InterfaceIndex,
                         __in_opt libusb_request* DriverVersionRequest,
                         __in_opt PKUSB_INTERFACE_HANDLE_INTERNAL PreviousInterfaceHandle)

{
	memset(InterfaceHandle, 0, sizeof(*InterfaceHandle) - sizeof(InterfaceHandle->inst));
	if (PreviousInterfaceHandle)
	{
		memcpy(InterfaceHandle, PreviousInterfaceHandle, sizeof(*InterfaceHandle) - sizeof(InterfaceHandle->inst));
	}
	else
	{
		InterfaceHandle->DeviceHandle = DeviceHandle;
		if (DriverVersionRequest)
		{
			memcpy(&InterfaceHandle->Version, &DriverVersionRequest->version, sizeof(InterfaceHandle->Version));
		}
	}

	InterfaceHandle->InterfaceIndex = InterfaceIndex;
}

// ErrorCodes:
// ERROR_INVALID_HANDLE
// ERROR_NOT_ENOUGH_MEMORY
// ERROR_BAD_DEVICE
KUSB_EXP BOOL KUSB_API LUsbK_Initialize(
    __in  HANDLE DeviceHandle,
    __out PWINUSB_INTERFACE_HANDLE InterfaceHandle)
{
	DWORD ret = ERROR_SUCCESS;
	BOOL success;
	PKUSB_INTERFACE_HANDLE_INTERNAL interfaceHandle = NULL;
	libusb_request request = {0};
	libusb_request ver_request = {0};
	DWORD transferred = 0;

	CheckLibInitialized();

	if(!DeviceHandle || !InterfaceHandle)
	{
		success = LusbwError(ERROR_INVALID_HANDLE);
		goto Error;
	}
	success = Ioctl_Sync(DeviceHandle, LIBUSB_IOCTL_GET_CACHED_CONFIGURATION,
	                     &request, sizeof(request),
	                     &request, sizeof(request),
	                     &transferred);

	if (!success || transferred != 1)
	{
		success = LusbwError(ERROR_ACCESS_DENIED);
		goto Error;
	}

	if (!((PUCHAR)&request)[0])
	{
		USBWRN("Device not configured. Attempting to set configuration #1.\n");
		request.configuration.configuration = 1;
		success = Ioctl_Sync(DeviceHandle, LIBUSB_IOCTL_SET_CONFIGURATION,
		                     &request, sizeof(request),
		                     &request, sizeof(request),
		                     NULL);
		if (!success)
		{
			goto Error;
		}
	}
	Mem_Zero(&ver_request, sizeof(ver_request));
	Ioctl_Sync(DeviceHandle, LIBUSB_IOCTL_GET_VERSION,
	           &ver_request, sizeof(ver_request),
	           &ver_request, sizeof(ver_request),
	           NULL);

	USBMSG("%s.sys v%u.%u.%u.%u\n",
	       ver_request.version.major <= 1 ? "libusb0" : "libusbK",
	       ver_request.version.major,
	       ver_request.version.minor,
	       ver_request.version.micro,
	       ver_request.version.nano);


	interfaceHandle = GetNextAvailableHandle();
	FailIf(!interfaceHandle, ERROR_OUT_OF_STRUCTURES, Done);

	Mem_Zero(&request, sizeof(request));
	request.intf.useInterfaceIndex = TRUE;
	request.intf.useAltSettingIndex = TRUE;
	success = Ioctl_Sync(DeviceHandle, LIBUSBK_IOCTL_CLAIM_INTERFACE,
	                     &request, sizeof(request),
	                     NULL, 0,
	                     NULL);

	if (!success)
	{
		ret = GetLastError();
		ret = (ret == ERROR_BAD_COMMAND) ? ERROR_BAD_DEVICE : ret;
		goto Done;
	}

	InitInterfaceHandle(interfaceHandle, DeviceHandle, 0, &ver_request, NULL);

Done:
	success = LusbwError(ret);
Error:
	if (success)
	{
		*InterfaceHandle = interfaceHandle;
	}
	else
	{
		InvalidateHandle(interfaceHandle);
	}
	return success;
}

// ErrorCodes: NONE
KUSB_EXP BOOL KUSB_API LUsbK_Free (__in  WINUSB_INTERFACE_HANDLE InterfaceHandle)
{
	libusb_request request = {0};
	HANDLE deviceHandle;
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	if (!handle || !IsHandleValid(handle->DeviceHandle))
		return TRUE;

	request.intf.interfaceIndex = (UCHAR)handle->InterfaceIndex;
	deviceHandle = handle->DeviceHandle;

	if (InterlockedDecrement(&handle->inst.ValidCount) <= -1)
	{
		// handle allready freed.
		InterlockedIncrement(&handle->inst.ValidCount);
		return TRUE;
	}

	request.intf.useInterfaceIndex = TRUE;
	request.intf.useAltSettingIndex = TRUE;
	Ioctl_Sync(deviceHandle, LIBUSBK_IOCTL_RELEASE_INTERFACE,
	           &request, sizeof(request),
	           NULL, 0,
	           NULL);

	return TRUE;
}

// ErrorCodes:
// ERROR_ALREADY_EXISTS
// ERROR_INVALID_HANDLE
// ERROR_INVALID_PARAMETER
// ERROR_NO_MORE_ITEMS
// ERROR_NOT_ENOUGH_MEMORY
KUSB_EXP BOOL KUSB_API LUsbK_GetAssociatedInterface (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AssociatedInterfaceIndex,
    __out PWINUSB_INTERFACE_HANDLE AssociatedInterfaceHandle)
{
	PKUSB_INTERFACE_HANDLE_INTERNAL associatedHandle = NULL;
	BOOL success;
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	if (!handle || !IsHandleValid(handle->DeviceHandle))
		return LusbwError(ERROR_INVALID_HANDLE);

	if(!AssociatedInterfaceHandle)
		return LusbwError(ERROR_INVALID_PARAMETER);

	associatedHandle = GetNextAvailableHandle();
	FailIf(!associatedHandle, ERROR_OUT_OF_STRUCTURES, Done);

	request.intf.useInterfaceIndex = TRUE;
	request.intf.useAltSettingIndex = TRUE;
	request.intf.interfaceIndex = (UCHAR)(handle->InterfaceIndex + AssociatedInterfaceIndex + 1);
	success = Ioctl_Sync(handle->DeviceHandle, LIBUSBK_IOCTL_CLAIM_INTERFACE,
	                     &request, sizeof(request),
	                     NULL, 0,
	                     NULL);

	FailIf(!success, GetLastError(), Done);

	InitInterfaceHandle(associatedHandle, NULL, request.intf.interfaceIndex, NULL, handle);

Done:
	success = LusbwError(ret);
	if (success)
	{
		*AssociatedInterfaceHandle = associatedHandle;
	}
	else
	{
		InvalidateHandle(associatedHandle);
	}

	return success;
}

KUSB_EXP BOOL KUSB_API LUsbK_GetDescriptor (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR DescriptorType,
    __in  UCHAR Index,
    __in  USHORT LanguageID,
    __out_opt PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out PULONG LengthTransferred)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	request.descriptor.type = DescriptorType;
	request.descriptor.index = Index;
	request.descriptor.language_id = LanguageID;

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_GET_DESCRIPTOR,
	                  &request, sizeof(request),
	                  Buffer, BufferLength,
	                  LengthTransferred);

Done:
	return LusbwError(ret);
}

// ERROR_INVALID_HANDLE
// ERROR_NO_MORE_ITEMS
KUSB_EXP BOOL KUSB_API LUsbK_QueryInterfaceSettings (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AlternateInterfaceNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	FailIf(!UsbAltInterfaceDescriptor, ERROR_INVALID_PARAMETER, Done);

	request.intf.useInterfaceIndex = TRUE;
	request.intf.useAltSettingIndex = TRUE;
	request.intf.interfaceIndex = (UCHAR)handle->InterfaceIndex;
	request.intf.altsettingIndex = AlternateInterfaceNumber;

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_QUERY_INTERFACE_SETTINGS,
	                  &request, sizeof(request),
	                  UsbAltInterfaceDescriptor, sizeof(*UsbAltInterfaceDescriptor),
	                  NULL);

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_QueryDeviceInformation (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG InformationType,
    __inout PULONG BufferLength,
    __out PVOID Buffer)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	FailIf(!Buffer || !BufferLength , ERROR_INVALID_PARAMETER, Done);

	request.query_device.information_type = InformationType;

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_QUERY_DEVICE_INFORMATION,
	                  &request, sizeof(request),
	                  Buffer, *BufferLength,
	                  BufferLength);

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_SetCurrentAlternateSetting (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR SettingNumber)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	request.intf.interfaceIndex = (UCHAR)handle->InterfaceIndex;
	request.intf.altsettingIndex = SettingNumber;

	request.intf.useInterfaceIndex = TRUE;
	request.intf.useAltSettingIndex = TRUE;
	return Ioctl_Sync(handle->DeviceHandle, LIBUSBK_IOCTL_SET_INTERFACE,
	                  &request, sizeof(request),
	                  NULL, 0,
	                  NULL);

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_GetCurrentAlternateSetting (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR SettingNumber)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	FailIf(!SettingNumber, ERROR_INVALID_PARAMETER, Done);

	request.intf.interfaceIndex = (UCHAR)handle->InterfaceIndex;

	request.intf.useInterfaceIndex = TRUE;
	request.intf.useAltSettingIndex = TRUE;
	return Ioctl_Sync(handle->DeviceHandle, LIBUSBK_IOCTL_GET_INTERFACE,
	                  &request, sizeof(request),
	                  SettingNumber, sizeof(UCHAR),
	                  NULL);

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_QueryPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AlternateInterfaceNumber,
    __in  UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	PIPE_INFORMATION tempPipeInformation = {0};	// packed!
	BOOL success;

	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	FailIf(!PipeInformation, ERROR_INVALID_PARAMETER, Done);

	request.query_pipe.interface_index = handle->InterfaceIndex;
	request.query_pipe.altsetting_index = AlternateInterfaceNumber;
	request.query_pipe.pipe_index = PipeIndex;

	success = Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_QUERY_PIPE,
	                     &request, sizeof(request),
	                     &tempPipeInformation, sizeof(tempPipeInformation),
	                     NULL);
	if (success)
	{
		// map packed struct to unpacked.
		PipeInformation->Interval = tempPipeInformation.Interval;
		PipeInformation->MaximumPacketSize = tempPipeInformation.MaximumPacketSize;
		PipeInformation->PipeId = tempPipeInformation.PipeId;
		PipeInformation->PipeType = tempPipeInformation.PipeType;

		return TRUE;
	}
	FailIf(!success, GetLastError(), Done);

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_SetPipePolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in  ULONG PolicyType,
    __in  ULONG ValueLength,
    __in PVOID Value)
{
	DWORD ret = ERROR_SUCCESS;
	ULONG requestLength = sizeof(libusb_request) + ValueLength;
	libusb_request* request = Mem_Alloc(requestLength);
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);
	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);
	FailIf(!Value, ERROR_INVALID_PARAMETER, Done);

	memset(request, 0, requestLength);
	request->pipe_policy.interface_index = handle->InterfaceIndex;
	request->pipe_policy.pipe_id = PipeID;
	request->pipe_policy.policy_type = PolicyType;

	if (handle->Version.Major > 1)
	{
		return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_SET_PIPE_POLICY,
		                  request, sizeof(libusb_request) + ValueLength,
		                  NULL, 0, NULL);
	}
	else
	{
		if (!(PipeID & 0x0F))
		{
			// this pipe policy is for the default pipe; they are handled by the driver.
			SetRequestData(request, Value, ValueLength);
			return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_SET_PIPE_POLICY,
			                  request, sizeof(libusb_request) + ValueLength,
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
					InterlockedExchange((PLONG)&GetSetPipePolicy(handle, PipeID)->timeout, (LONG) * ((PULONG)Value));
				}
				break;
			}
		}
	}

Done:
	Mem_Free(&request);
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_GetPipePolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in  ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	FailIf(!Value || !ValueLength, ERROR_INVALID_PARAMETER, Done);

	request.pipe_policy.interface_index = handle->InterfaceIndex;
	request.pipe_policy.pipe_id = PipeID;
	request.pipe_policy.policy_type = PolicyType;
	if (handle->Version.Major > 1)
	{
		return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_GET_PIPE_POLICY,
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
			if (Value && ValueLength && (*ValueLength >= 4))
			{
				((PULONG)Value)[0] = GetSetPipePolicy(handle, PipeID)->timeout;
				*ValueLength = 4;
			}
			break;
		}
	}
Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_ReadPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __out_opt PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	request.endpoint.endpoint = PipeID;

	if (Overlapped)
	{
		return Ioctl_Async(handle->DeviceHandle, LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ,
		                   &request, sizeof(request),
		                   Buffer, BufferLength,
		                   Overlapped);
	}
	else
	{
		return Ioctl_SyncTranfer(handle,
		                         LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ,
		                         &request,
		                         Buffer, BufferLength,
		                         GetSetPipePolicy(handle, PipeID)->timeout,
		                         LengthTransferred);
	}

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_WritePipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	request.endpoint.endpoint = PipeID;

	if (Overlapped)
	{
		return Ioctl_Async(handle->DeviceHandle, LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE,
		                   &request, sizeof(request),
		                   Buffer, BufferLength,
		                   Overlapped);
	}
	else
	{
		return Ioctl_SyncTranfer(handle,
		                         LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE,
		                         &request,
		                         Buffer, BufferLength,
		                         0,
		                         LengthTransferred);
	}

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_ControlTransfer (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  WINUSB_SETUP_PACKET SetupPacket,
    __out_opt PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt  LPOVERLAPPED Overlapped)
{
	INT ioctlCode;
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	memcpy(&request.control, &SetupPacket, sizeof(request.control));

	ioctlCode = (SetupPacket.RequestType & USB_ENDPOINT_DIRECTION_MASK) ? LIBUSB_IOCTL_CONTROL_READ : LIBUSB_IOCTL_CONTROL_WRITE;

	if (Overlapped)
	{
		return Ioctl_Async(handle->DeviceHandle, ioctlCode,
		                   &request, sizeof(request),
		                   Buffer, BufferLength,
		                   Overlapped);
	}
	else
	{
		UCHAR pipeID = (SetupPacket.RequestType & USB_ENDPOINT_DIRECTION_MASK) ? 0x80 : 0x00;
		return Ioctl_SyncTranfer(handle,
		                         ioctlCode,
		                         &request,
		                         Buffer, BufferLength,
		                         GetSetPipePolicy(handle, pipeID)->timeout,
		                         LengthTransferred);
	}

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_ResetPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	request.endpoint.endpoint = PipeID;

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_RESET_ENDPOINT,
	                  &request, sizeof(request),
	                  NULL, 0,
	                  NULL);

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_AbortPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	request.endpoint.endpoint = PipeID;

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_ABORT_ENDPOINT,
	                  &request, sizeof(request),
	                  NULL, 0,
	                  NULL);

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_FlushPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	request.endpoint.endpoint = PipeID;

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_FLUSH_PIPE,
	                  &request, sizeof(request),
	                  NULL, 0,
	                  NULL);

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_SetPowerPolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG PolicyType,
    __in  ULONG ValueLength,
    __in PVOID Value)
{
	DWORD ret = ERROR_SUCCESS;
	BOOL success;
	libusb_request* request;
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);
	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	FailIf(!Value, ERROR_INVALID_PARAMETER, Done);

	request = AllocRequest(ValueLength);

	FailIf(!IsHandleValid(request), ERROR_NOT_ENOUGH_MEMORY, Done);

	request->power_policy.policy_type = PolicyType;
	SetRequestData(request, Value, ValueLength);

	success = Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_SET_POWER_POLICY,
	                     request, sizeof(libusb_request) + ValueLength,
	                     NULL, 0,
	                     NULL);

	FailIf(!success, GetLastError(), Done);

Done:
	Mem_Free(&request);
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_GetPowerPolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG PolicyType,
    __inout PULONG ValueLength,
    __out PVOID Value)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	FailIf(!Value || !ValueLength, ERROR_INVALID_PARAMETER, Done);

	request.power_policy.policy_type = PolicyType;

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_GET_POWER_POLICY,
	                  &request, sizeof(request),
	                  Value, *ValueLength,
	                  ValueLength);

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_GetOverlappedResult (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in  BOOL bWait)
{
	DWORD ret = ERROR_SUCCESS;
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);
	FailIf(!handle || !lpOverlapped || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	return GetOverlappedResult(handle->DeviceHandle, lpOverlapped, lpNumberOfBytesTransferred, bWait);

Done:
	return LusbwError(ret);
}




KUSB_EXP BOOL KUSB_API LUsbK_ResetDevice (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PKUSB_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_RESET_DEVICE,
	                  &request, sizeof(request),
	                  NULL, 0,
	                  NULL);

Done:
	return LusbwError(ret);
}

KUSB_EXP BOOL KUSB_API LUsbK_GetProcAddress(__out KPROC* ProcAddress, __in ULONG DriverID, __in ULONG FunctionID)
{
	CheckLibInitialized();

	switch(DriverID)
	{
	case KUSB_DRVID_LIBUSBK:
		return GetProcAddress_libusbk(ProcAddress, FunctionID);
	case KUSB_DRVID_LIBUSB0_FILTER:
	case KUSB_DRVID_LIBUSB0:
		return GetProcAddress_libusb0(ProcAddress, FunctionID);
	case KUSB_DRVID_WINUSB:
		return GetProcAddress_winusb(ProcAddress, FunctionID);
	}

	return LusbwError(ERROR_NOT_SUPPORTED);
}


KUSB_EXP BOOL KUSB_API LUsbK_LoadDriverApi(
    __inout PKUSB_DRIVER_API DriverAPI,
    __in ULONG DriverID)
{
#define CASE_FNID_LOAD(FunctionName)															\
		case KUSB_FNID_##FunctionName:																\
		if (LUsbK_GetProcAddress((KPROC*)&DriverAPI->FunctionName, DriverID, fnIdIndex) == FALSE)	\
		{																							\
			USBWRN("function id %u for driver id %u does not exist.\n",fnIdIndex,DriverID);			\
		}																							\
		break

	int fnIdIndex;
	if (!IsHandleValid(DriverAPI))
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	for (fnIdIndex = 0; fnIdIndex < KUSB_FNID_COUNT; fnIdIndex++)
	{
		switch(fnIdIndex)
		{
			CASE_FNID_LOAD(Initialize);
			CASE_FNID_LOAD(Free);
			CASE_FNID_LOAD(GetAssociatedInterface);
			CASE_FNID_LOAD(GetDescriptor);
			CASE_FNID_LOAD(QueryInterfaceSettings);
			CASE_FNID_LOAD(QueryDeviceInformation);
			CASE_FNID_LOAD(SetCurrentAlternateSetting);
			CASE_FNID_LOAD(GetCurrentAlternateSetting);
			CASE_FNID_LOAD(QueryPipe);
			CASE_FNID_LOAD(SetPipePolicy);
			CASE_FNID_LOAD(GetPipePolicy);
			CASE_FNID_LOAD(ReadPipe);
			CASE_FNID_LOAD(WritePipe);
			CASE_FNID_LOAD(ControlTransfer);
			CASE_FNID_LOAD(ResetPipe);
			CASE_FNID_LOAD(AbortPipe);
			CASE_FNID_LOAD(FlushPipe);
			CASE_FNID_LOAD(SetPowerPolicy);
			CASE_FNID_LOAD(GetPowerPolicy);
			CASE_FNID_LOAD(GetOverlappedResult);
			CASE_FNID_LOAD(ResetDevice);

		default:
			USBERR("undeclared api function %u!\n", fnIdIndex);
			return LusbwError(ERROR_NOT_SUPPORTED);
		}
	}

	return TRUE;
}

BOOL GetProcAddress_libusbk(__out KPROC* ProcAddress, __in ULONG FunctionID)
{
	DWORD rtn = ERROR_SUCCESS;

	switch(FunctionID)
	{
	case KUSB_FNID_Initialize:
		*ProcAddress = (KPROC)LUsbK_Initialize;
		break;

	case KUSB_FNID_Free:
		*ProcAddress = (KPROC)LUsbK_Free;
		break;

	case KUSB_FNID_GetAssociatedInterface:
		*ProcAddress = (KPROC)LUsbK_GetAssociatedInterface;
		break;

	case KUSB_FNID_GetDescriptor:
		*ProcAddress = (KPROC)LUsbK_GetDescriptor;
		break;

	case KUSB_FNID_QueryInterfaceSettings:
		*ProcAddress = (KPROC)LUsbK_QueryInterfaceSettings;
		break;

	case KUSB_FNID_QueryDeviceInformation:
		*ProcAddress = (KPROC)LUsbK_QueryDeviceInformation;
		break;

	case KUSB_FNID_SetCurrentAlternateSetting:
		*ProcAddress = (KPROC)LUsbK_SetCurrentAlternateSetting;
		break;

	case KUSB_FNID_GetCurrentAlternateSetting:
		*ProcAddress = (KPROC)LUsbK_GetCurrentAlternateSetting;
		break;

	case KUSB_FNID_QueryPipe:
		*ProcAddress = (KPROC)LUsbK_QueryPipe;
		break;

	case KUSB_FNID_SetPipePolicy:
		*ProcAddress = (KPROC)LUsbK_SetPipePolicy;
		break;

	case KUSB_FNID_GetPipePolicy:
		*ProcAddress = (KPROC)LUsbK_GetPipePolicy;
		break;

	case KUSB_FNID_ReadPipe:
		*ProcAddress = (KPROC)LUsbK_ReadPipe;
		break;

	case KUSB_FNID_WritePipe:
		*ProcAddress = (KPROC)LUsbK_WritePipe;
		break;

	case KUSB_FNID_ControlTransfer:
		*ProcAddress = (KPROC)LUsbK_ControlTransfer;
		break;

	case KUSB_FNID_ResetPipe:
		*ProcAddress = (KPROC)LUsbK_ResetPipe;
		break;

	case KUSB_FNID_AbortPipe:
		*ProcAddress = (KPROC)LUsbK_AbortPipe;
		break;

	case KUSB_FNID_FlushPipe:
		*ProcAddress = (KPROC)LUsbK_FlushPipe;
		break;

	case KUSB_FNID_SetPowerPolicy:
		*ProcAddress = (KPROC)LUsbK_SetPowerPolicy;
		break;

	case KUSB_FNID_GetPowerPolicy:
		*ProcAddress = (KPROC)LUsbK_GetPowerPolicy;
		break;

	case KUSB_FNID_GetOverlappedResult:
		*ProcAddress = (KPROC)LUsbK_GetOverlappedResult;
		break;

	case KUSB_FNID_ResetDevice:
		*ProcAddress = (KPROC)LUsbK_ResetDevice;
		break;
	default:
		rtn = ERROR_NOT_SUPPORTED;
		break;

	}
	return LusbwError(rtn);
}
