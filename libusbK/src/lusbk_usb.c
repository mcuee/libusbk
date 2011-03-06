/* libusb-win32 WDF, Generic KMDF Windows USB Driver
 * Copyright (c) 2010-2011 Travis Robinson <libusbdotnet@gmail.com>
 * Copyright (c) 2002-2005 Stephan Meyer <ste_meyer@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "lusbk_private.h"

// Default loggging level
ULONG DebugLevel = 4;

volatile LONG NextHandlePos = -1;
LUSBW_INTERFACE_HANDLE_INTERNAL HandleList[LUSBW_MAX_INTERFACE_HANDLES] = {0};

#define InvalidateHandle(InterfaceHandleInternalPtr) \
	if (InterfaceHandleInternalPtr) InterlockedDecrement(&InterfaceHandleInternalPtr->ValidCount)

///////////////////////////////////////////////////////////////////////

PLUSBW_INTERFACE_HANDLE_INTERNAL GetNextAvailableHandle()
{
	LONG index = NextHandlePos;
	LONG count = LUSBW_MAX_INTERFACE_HANDLES;
	while (count--)
	{
		if ((++index) >= LUSBW_MAX_INTERFACE_HANDLES)
			index = 0;

		if (InterlockedIncrement(&HandleList[index].ValidCount) == 1)
		{
			InterlockedExchange(&NextHandlePos, index);
			return &HandleList[index];
		}

		// this handle is in use.
		InterlockedDecrement(&HandleList[index].ValidCount);
	}
	USBERR("no more internal interface handles! (max=%d)\n", LUSBW_MAX_INTERFACE_HANDLES);
	return NULL;
}

// ErrorCodes:
// ERROR_INVALID_HANDLE
// ERROR_NOT_ENOUGH_MEMORY
// ERROR_BAD_DEVICE
LUSBW_EXP BOOL LUSBW_API LUsbK_Initialize
(
    __in  HANDLE DeviceHandle,
    __out PWINUSB_INTERFACE_HANDLE InterfaceHandle)
{
	DWORD ret = ERROR_SUCCESS;
	BOOL success;
	PLUSBW_INTERFACE_HANDLE_INTERNAL interfaceHandle = NULL;
	libusb_request request = {0};

	if(!DeviceHandle || !InterfaceHandle)
		return LusbwError(ERROR_INVALID_HANDLE);

	interfaceHandle = GetNextAvailableHandle();
	FailIf(!interfaceHandle, ERROR_OUT_OF_STRUCTURES, Done);

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


	// Initialize the internal interface structure.
	interfaceHandle->DeviceHandle = DeviceHandle;
	interfaceHandle->InterfaceIndex = 0;

Done:
	success = LusbwError(ret);
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
LUSBW_EXP BOOL LUSBW_API LUsbK_Free (__in  WINUSB_INTERFACE_HANDLE InterfaceHandle)
{
	libusb_request request = {0};
	HANDLE deviceHandle;
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	if (!handle || !IsHandleValid(handle->DeviceHandle))
		return TRUE;

	request.intf.interfaceIndex = (UCHAR)handle->InterfaceIndex;
	deviceHandle = handle->DeviceHandle;

	if (InterlockedDecrement(&handle->ValidCount) <= -1)
	{
		// handle allready freed.
		InterlockedIncrement(&handle->ValidCount);
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
LUSBW_EXP BOOL LUSBW_API LUsbK_GetAssociatedInterface (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AssociatedInterfaceIndex,
    __out PWINUSB_INTERFACE_HANDLE AssociatedInterfaceHandle)
{
	PLUSBW_INTERFACE_HANDLE_INTERNAL associatedHandle = NULL;
	BOOL success;
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

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

	// Initialize the internal interface structure.
	associatedHandle->DeviceHandle = handle->DeviceHandle;
	associatedHandle->InterfaceIndex = request.intf.interfaceIndex;

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

LUSBW_EXP BOOL LUSBW_API LUsbK_GetDescriptor (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR DescriptorType,
    __in  UCHAR Index,
    __in  USHORT LanguageID,
    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out PULONG LengthTransferred)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

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
LUSBW_EXP BOOL LUSBW_API LUsbK_QueryInterfaceSettings (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AlternateInterfaceNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

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

LUSBW_EXP BOOL LUSBW_API LUsbK_QueryDeviceInformation (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG InformationType,
    __inout PULONG BufferLength,
    __out_bcount(*BufferLength) PVOID Buffer)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

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

LUSBW_EXP BOOL LUSBW_API LUsbK_SetCurrentAlternateSetting (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR SettingNumber)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

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

LUSBW_EXP BOOL LUSBW_API LUsbK_GetCurrentAlternateSetting (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR SettingNumber)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

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

LUSBW_EXP BOOL LUSBW_API LUsbK_QueryPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AlternateInterfaceNumber,
    __in  UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	PIPE_INFORMATION tempPipeInformation = {0};	// packed!
	BOOL success;

	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

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

LUSBW_EXP BOOL LUSBW_API LUsbK_SetPipePolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in  ULONG PolicyType,
    __in  ULONG ValueLength,
    __in_bcount(ValueLength) PVOID Value)
{
	DWORD ret = ERROR_SUCCESS;
	ULONG requestLength = sizeof(libusb_request) + ValueLength;
	libusb_request* request = Mem_Alloc(requestLength);
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);
	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);
	FailIf(!Value, ERROR_INVALID_PARAMETER, Done);

	memset(request, 0, requestLength);
	request->pipe_policy.interface_index = handle->InterfaceIndex;
	request->pipe_policy.pipe_id = PipeID;
	request->pipe_policy.policy_type = PolicyType;

	SetRequestData(request, Value, ValueLength);

	if (ValueLength == 4)
		USBDBG("ValueLength=%u ULONG Value=%u\n", ValueLength, *((PULONG)GetRequestDataPtr(request)));
	else if (ValueLength == 1)
		USBDBG("ValueLength=%u UCHAR Value=%u\n", ValueLength, *((PUCHAR)GetRequestDataPtr(request)));


	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_SET_PIPE_POLICY,
	                  request, sizeof(libusb_request) + ValueLength,
	                  NULL, 0, NULL);

Done:
	Mem_Free(&request);
	return LusbwError(ret);
}

LUSBW_EXP BOOL LUSBW_API LUsbK_GetPipePolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in  ULONG PolicyType,
    __inout PULONG ValueLength,
    __out_bcount(*ValueLength) PVOID Value)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	FailIf(!Value || !ValueLength, ERROR_INVALID_PARAMETER, Done);

	request.pipe_policy.interface_index = handle->InterfaceIndex;
	request.pipe_policy.pipe_id = PipeID;
	request.pipe_policy.policy_type = PolicyType;

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_GET_PIPE_POLICY,
	                  &request, sizeof(request),
	                  Value, *ValueLength,
	                  ValueLength);

Done:
	return LusbwError(ret);
}

LUSBW_EXP BOOL LUSBW_API LUsbK_ReadPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

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
		return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_INTERRUPT_OR_BULK_READ,
		                  &request, sizeof(request),
		                  Buffer, BufferLength,
		                  LengthTransferred);
	}

Done:
	return LusbwError(ret);
}

LUSBW_EXP BOOL LUSBW_API LUsbK_WritePipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in_bcount(BufferLength) PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

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
		return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_INTERRUPT_OR_BULK_WRITE,
		                  &request, sizeof(request),
		                  Buffer, BufferLength,
		                  LengthTransferred);
	}

Done:
	return LusbwError(ret);
}

LUSBW_EXP BOOL LUSBW_API LUsbK_ControlTransfer (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  WINUSB_SETUP_PACKET SetupPacket,
    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt  LPOVERLAPPED Overlapped)
{
	INT ioctlCode;
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

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
		return Ioctl_Sync(handle->DeviceHandle, ioctlCode,
		                  &request, sizeof(request),
		                  Buffer, BufferLength,
		                  LengthTransferred);
	}

Done:
	return LusbwError(ret);
}

LUSBW_EXP BOOL LUSBW_API LUsbK_ResetPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	request.endpoint.endpoint = PipeID;

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_RESET_ENDPOINT,
	                  &request, sizeof(request),
	                  NULL, 0,
	                  NULL);

Done:
	return LusbwError(ret);
}

LUSBW_EXP BOOL LUSBW_API LUsbK_AbortPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	request.endpoint.endpoint = PipeID;

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_ABORT_ENDPOINT,
	                  &request, sizeof(request),
	                  NULL, 0,
	                  NULL);

Done:
	return LusbwError(ret);
}

LUSBW_EXP BOOL LUSBW_API LUsbK_FlushPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	request.endpoint.endpoint = PipeID;

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_FLUSH_PIPE,
	                  &request, sizeof(request),
	                  NULL, 0,
	                  NULL);

Done:
	return LusbwError(ret);
}

LUSBW_EXP BOOL LUSBW_API LUsbK_SetPowerPolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG PolicyType,
    __in  ULONG ValueLength,
    __in_bcount(ValueLength) PVOID Value)
{
	DWORD ret = ERROR_SUCCESS;
	BOOL success;
	libusb_request* request;
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);
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

LUSBW_EXP BOOL LUSBW_API LUsbK_GetPowerPolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG PolicyType,
    __inout PULONG ValueLength,
    __out_bcount(*ValueLength) PVOID Value)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

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

LUSBW_EXP BOOL LUSBW_API LUsbK_GetOverlappedResult (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in  BOOL bWait)
{
	DWORD ret = ERROR_SUCCESS;
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);
	FailIf(!handle || !lpOverlapped || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	return GetOverlappedResult(handle->DeviceHandle, lpOverlapped, lpNumberOfBytesTransferred, bWait);

Done:
	return LusbwError(ret);
}

LUSBW_EXP BOOL LUSBW_API LUsbK_ResetDevice (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle)
{
	DWORD ret = ERROR_SUCCESS;
	libusb_request request = {0};
	PLUSBW_INTERFACE_HANDLE_INTERNAL handle = PublicToPrivateHandle(InterfaceHandle);

	FailIf(!handle || !IsHandleValid(handle->DeviceHandle), ERROR_INVALID_HANDLE, Done);

	return Ioctl_Sync(handle->DeviceHandle, LIBUSB_IOCTL_RESET_DEVICE,
	                  &request, sizeof(request),
	                  NULL, 0,
	                  NULL);

Done:
	return LusbwError(ret);
}

//////////////////////////////////////////////////////////////////////////////
///////////////  W I N U S B   W R A P P E R   S E C T I O N   ///////////////
//////////////////////////////////////////////////////////////////////////////
#ifndef EXCLUDE_WINUSB_WRAPPER

LUSBW_EXP BOOL LUSBW_API WinUsb_Initialize (
    __in  HANDLE DeviceHandle,
    __out PWINUSB_INTERFACE_HANDLE InterfaceHandle)
{
	return LUsbK_Initialize(DeviceHandle, InterfaceHandle);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_Free (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle)
{
	return LUsbK_Free(InterfaceHandle);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_GetAssociatedInterface (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AssociatedInterfaceIndex,
    __out PWINUSB_INTERFACE_HANDLE AssociatedInterfaceHandle)
{
	return LUsbK_GetAssociatedInterface(InterfaceHandle, AssociatedInterfaceIndex, AssociatedInterfaceHandle);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_GetDescriptor (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR DescriptorType,
    __in  UCHAR Index,
    __in  USHORT LanguageID,
    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out PULONG LengthTransferred)
{
	return LUsbK_GetDescriptor(InterfaceHandle, DescriptorType, Index, LanguageID, Buffer, BufferLength, LengthTransferred);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_QueryInterfaceSettings (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AlternateInterfaceNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
{
	return LUsbK_QueryInterfaceSettings(InterfaceHandle, AlternateInterfaceNumber, UsbAltInterfaceDescriptor);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_QueryDeviceInformation (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG InformationType,
    __inout PULONG BufferLength,
    __out_bcount(*BufferLength) PVOID Buffer)
{
	return LUsbK_QueryDeviceInformation(InterfaceHandle, InformationType, BufferLength, Buffer);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_SetCurrentAlternateSetting (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR SettingNumber)
{
	return LUsbK_SetCurrentAlternateSetting(InterfaceHandle, SettingNumber);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_GetCurrentAlternateSetting (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR SettingNumber)
{
	return LUsbK_GetCurrentAlternateSetting(InterfaceHandle, SettingNumber);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_QueryPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AlternateInterfaceNumber,
    __in  UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation)
{
	return LUsbK_QueryPipe(InterfaceHandle, AlternateInterfaceNumber, PipeIndex, PipeInformation);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_SetPipePolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in  ULONG PolicyType,
    __in  ULONG ValueLength,
    __in_bcount(ValueLength) PVOID Value)
{
	return LUsbK_SetPipePolicy(InterfaceHandle, PipeID, PolicyType, ValueLength, Value);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_GetPipePolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in  ULONG PolicyType,
    __inout PULONG ValueLength,
    __out_bcount(*ValueLength) PVOID Value)
{
	return LUsbK_GetPipePolicy(InterfaceHandle, PipeID, PolicyType, ValueLength, Value);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_ReadPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	return LUsbK_ReadPipe(InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_WritePipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in_bcount(BufferLength) PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped)
{
	return LUsbK_WritePipe(InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_ControlTransfer (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  WINUSB_SETUP_PACKET SetupPacket,
    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt  LPOVERLAPPED Overlapped)
{
	return LUsbK_ControlTransfer(InterfaceHandle, SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_ResetPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID)
{
	return LUsbK_ResetPipe(InterfaceHandle, PipeID);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_AbortPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID)
{
	return LUsbK_AbortPipe(InterfaceHandle, PipeID);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_FlushPipe (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID)
{
	return LUsbK_FlushPipe(InterfaceHandle, PipeID);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_SetPowerPolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG PolicyType,
    __in  ULONG ValueLength,
    __in_bcount(ValueLength) PVOID Value)
{
	return LUsbK_SetPowerPolicy(InterfaceHandle, PolicyType, ValueLength, Value);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_GetPowerPolicy (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG PolicyType,
    __inout PULONG ValueLength,
    __out_bcount(*ValueLength) PVOID Value)
{
	return LUsbK_GetPowerPolicy(InterfaceHandle, PolicyType, ValueLength, Value);
}

LUSBW_EXP BOOL LUSBW_API WinUsb_GetOverlappedResult (
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in  BOOL bWait)
{
	return LUsbK_GetOverlappedResult(InterfaceHandle, lpOverlapped, lpNumberOfBytesTransferred, bWait);
}
/*
	LUSBW_EXP PUSB_INTERFACE_DESCRIPTOR WinUsb_ParseConfigurationDescriptor (
	    __in  PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
	    __in  PVOID StartPosition,
	    __in  LONG InterfaceNumber,
	    __in  LONG AlternateSetting,
	    __in  LONG InterfaceClass,
	    __in  LONG InterfaceSubClass,
	    __in  LONG InterfaceProtocol
		) {return LUsbK_ParseConfigurationDescriptor(ConfigurationDescriptor,StartPosition,InterfaceNumber,AlternateSetting,InterfaceClass,InterfaceSubClass,InterfaceProtocol); }

	LUSBW_EXP PUSB_COMMON_DESCRIPTOR WinUsb_ParseDescriptors (
	    __in_bcount(TotalLength) PVOID    DescriptorBuffer,
	    __in  ULONG    TotalLength,
	    __in  PVOID    StartPosition,
	    __in  LONG     DescriptorType
		) {return LUsbK_ParseDescriptors(DescriptorBuffer,TotalLength,StartPosition,DescriptorType); }
*/

#endif // EXCLUDE_WINUSB_WRAPPER
