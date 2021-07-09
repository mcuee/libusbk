/*!********************************************************************
libusbK - Multi-driver USB library.
Copyright (C) 2012 Travis Lee Robinson. All Rights Reserved.
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

const USHORT PollingPeriod_Per_bInterval_HighSpeed[32] =
{
	1, 2, 4, 8, 16, 32, 32,
	32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32,
	32,
};

const USHORT PollingPeriod_Per_bInterval_FullSpeed[32] =
{
	1, 2, 2, 4, 4, 4,
	4, 8, 8, 8, 8, 8,
	8, 8, 8, 16, 16, 16,
	16, 16, 16, 16, 16,
	16, 16, 16, 16, 16,
	16, 16, 16, 32,
};

static void KUSB_API Isoch_Cleanup(PKISOCH_HANDLE_INTERNAL handle)
{
	PoolHandle_Dead_IsochK(handle);
	if (IsHandleValid(handle->UsbHandle))
	{
		INT drvId = handle->UsbHandle->Device->DriverAPI->Info.DriverID;
		switch (drvId)
		{
		case KUSB_DRVID_LIBUSBK:
			if (handle->Context.UsbK != NULL)
				IsoK_Free(handle->Context.UsbK);
			break;
		case KUSB_DRVID_WINUSB:
			if (handle->Context.UsbW.BufferHandle != NULL)
			{
				WinUsb.UnregisterIsochBuffer(handle->Context.UsbW.BufferHandle);
				handle->Context.UsbW.BufferHandle = NULL;
			}
			if (handle->Context.UsbW.Packets != NULL) 
			{
				Mem_Free((PVOID*)&handle->Context.UsbW.Packets);
			}

			break;
		}

		PoolHandle_Dec_UsbK(handle->UsbHandle);
	}
}

KUSB_EXP BOOL KUSB_API IsochK_Init(
	_out KISOCH_HANDLE* IsochHandle,
	_in KUSB_HANDLE InterfaceHandle,
	_in UCHAR PipeId,
	_in UINT MaxNumberOfPackets,
	_in PUCHAR TransferBuffer,
	_in UINT TransferBufferSize)
{
	PKISOCH_HANDLE_INTERNAL handle = NULL;
	PKUSB_HANDLE_INTERNAL usbHandle = NULL;
	UCHAR devSpeed = 1;
	UINT devSpeedLen = 1;
	ErrorParamAction(IsochHandle==NULL, "IsochHandle", goto Error);
	ErrorParamAction(!IsHandleValid(InterfaceHandle), "UsbHandle", goto Error);
	ErrorParamAction(MaxNumberOfPackets <= 0, "MaxNumberOfPackets", goto Error);

	ErrorNoSetAction((!PoolHandle_Inc_UsbK(InterfaceHandle)), goto Error, "UsbHandle is not a KUSB_HANDLE");
	usbHandle = (PKUSB_HANDLE_INTERNAL)InterfaceHandle;
	
	handle = PoolHandle_Acquire_IsochK((PKOBJ_CB)Isoch_Cleanup);
	ErrorNoSetAction(!IsHandleValid(handle), goto Error, "->PoolHandle_Acquire_IsochK");

	if (!usbHandle->Device->DriverAPI->QueryDeviceInformation(usbHandle, DEVICE_SPEED, &devSpeedLen, &devSpeed))
	{
		ErrorNoSetAction(TRUE, goto Error, "QueryDeviceInformation:DEVICE_SPEED failed.");
	}
	if (!UsbStack_QuerySelectedPipe(usbHandle, PipeId, FALSE,&handle->PipeInformation))
	{
		ErrorNoSetAction(TRUE, goto Error, "UsbStack_QuerySelectedPipe failed.");
	}

	handle->UsbHandle = usbHandle;
	handle->PipeID = PipeId;
	handle->PacketCount = MaxNumberOfPackets;
	handle->TransferBuffer = TransferBuffer;
	handle->TransferBufferSize = TransferBufferSize;
	handle->IsHighSpeed = (devSpeed >= 3);

	IsochK_CalcPacketInformation(handle->IsHighSpeed, &handle->PipeInformation, &handle->PacketInformation);
	
	switch(usbHandle->Device->DriverAPI->Info.DriverID)
	{
	case KUSB_DRVID_LIBUSBK:
		ErrorMemory(!IsoK_Init(&handle->Context.UsbK, handle->PacketCount, 0), Error);
		break;
	case KUSB_DRVID_WINUSB:
		if (!WinUsb.RegisterIsochBuffer(Get_PipeInterfaceHandle(usbHandle, handle->PipeID), handle->PipeID, handle->TransferBuffer, handle->TransferBufferSize, &handle->Context.UsbW.BufferHandle))
		{
			ErrorNoSet(TRUE, Error, "WinUsb.RegisterIsochBuffer Failed.");
		}
		handle->Context.UsbW.Packets = Mem_Alloc(MaxNumberOfPackets * sizeof(USBD_ISO_PACKET_DESCRIPTOR));
		handle->Context.UsbW.NumberOfPackets = (SHORT) MaxNumberOfPackets;
		break;
		default:
			ErrorSetAction(TRUE, ERROR_NOT_SUPPORTED, goto Error, "This driver does not support ISO");

	}
	*IsochHandle = handle;
	PoolHandle_Live_IsochK(handle);
	return TRUE;
Error:
	if (IsHandleValid(handle))
	{
		PoolHandle_Dec_IsochK(handle);
	}
	else if (IsHandleValid(usbHandle))
	{
		PoolHandle_Dec_UsbK(usbHandle);
	}
	return FALSE;
}

KUSB_EXP BOOL KUSB_API IsochK_Free(
	_in KISOCH_HANDLE IsochHandle)
{
	ErrorHandle(!IsHandleValid(IsochHandle), Error, "IsochHandle");

	return PoolHandle_Dec_IsochK(IsochHandle);

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API IsochK_SetPacketOffsets(
	_in KISOCH_HANDLE IsochHandle,
	_in UINT PacketSize)
{
	UINT packetIndex;
	UINT nextOffSet = 0;
	PKISOCH_HANDLE_INTERNAL handle;
	INT drvId;
	
	Pub_To_Priv_IsochK(IsochHandle, handle, return FALSE);
	ErrorNoSetAction(!PoolHandle_Inc_IsochK(IsochHandle), return FALSE, "IsochHandle is invalid");
	
	
	drvId = handle->UsbHandle->Device->DriverAPI->Info.DriverID;
	switch(drvId)
	{
	case KUSB_DRVID_LIBUSBK:
		for (packetIndex = 0; packetIndex < handle->PacketCount; packetIndex++)
		{
			// length and status are set by the driver on transfer completion
			handle->Context.UsbK->IsoPackets[packetIndex].Offset = nextOffSet;
			handle->Context.UsbK->IsoPackets[packetIndex].Length = (USHORT)PacketSize;
			handle->Context.UsbK->IsoPackets[packetIndex].Status = 0;
			nextOffSet += PacketSize;
		}
		break;
	case KUSB_DRVID_WINUSB:
		for (packetIndex = 0; packetIndex < handle->PacketCount; packetIndex++)
		{
			// length and status are set by the driver on transfer completion for read endpoints (0x8?)
			// WinUsb does not use packets at all for write endpoints. (0x0?)
			handle->Context.UsbW.Packets[packetIndex].Offset = nextOffSet;
			handle->Context.UsbW.Packets[packetIndex].Length = PacketSize;
			handle->Context.UsbW.Packets[packetIndex].Status = 0;
			nextOffSet += PacketSize;
		}
		break;
	default:
		ErrorSetAction(TRUE, ERROR_NOT_SUPPORTED, goto Error, "This driver does not support ISO");

	}
	
	return PoolHandle_Dec_IsochK(IsochHandle);

Error:
	PoolHandle_Dec_IsochK(IsochHandle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API IsochK_GetPacket(
	_in KISOCH_HANDLE IsochHandle,
	_in UINT PacketIndex,
	_outopt PUINT Offset,
	_outopt PUINT Length,
	_outopt PUINT Status)
{
	PKISOCH_HANDLE_INTERNAL handle;
	INT drvId;

	Pub_To_Priv_IsochK(IsochHandle, handle, return FALSE);
	ErrorNoSetAction(!PoolHandle_Inc_IsochK(IsochHandle), return FALSE, "IsochHandle is invalid");
	
	ErrorParam(PacketIndex >= handle->PacketCount, Error, "PacketIndex");

	drvId = handle->UsbHandle->Device->DriverAPI->Info.DriverID;
	switch (drvId)
	{
	case KUSB_DRVID_LIBUSBK:
		if (Offset)
			*Offset = handle->Context.UsbK->IsoPackets[PacketIndex].Offset;
		if (Length)
			*Length = handle->Context.UsbK->IsoPackets[PacketIndex].Length;
		if (Status)
			*Status = handle->Context.UsbK->IsoPackets[PacketIndex].Status;
		break;
	case KUSB_DRVID_WINUSB:
		if (Offset)
			*Offset = handle->Context.UsbW.Packets[PacketIndex].Offset;
		if (Length)
			*Length = handle->Context.UsbW.Packets[PacketIndex].Length;
		if (Status)
			*Status = handle->Context.UsbW.Packets[PacketIndex].Status;
		break;
	default:
		ErrorSetAction(TRUE, ERROR_NOT_SUPPORTED, goto Error, "This driver does not support ISO");

	}

	return PoolHandle_Dec_IsochK(IsochHandle);

Error:
	PoolHandle_Dec_IsochK(IsochHandle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API IsochK_SetNumberOfPackets(
	_in KISOCH_HANDLE IsochHandle,
	_in UINT NumberOfPackets)
{
	PKISOCH_HANDLE_INTERNAL handle;
	INT drvId;

	Pub_To_Priv_IsochK(IsochHandle, handle, return FALSE);
	ErrorNoSetAction(!PoolHandle_Inc_IsochK(IsochHandle), return FALSE, "IsochHandle is invalid");

	ErrorSetAction(NumberOfPackets > handle->PacketCount, ERROR_OUT_OF_STRUCTURES, goto Error, "NumberOfPackets must be less than or equal to MaxNumberOfPackets");
	drvId = handle->UsbHandle->Device->DriverAPI->Info.DriverID;
	switch (drvId)
	{
	case KUSB_DRVID_LIBUSBK:
		handle->Context.UsbK->NumberOfPackets =(SHORT)NumberOfPackets;
		break;
	case KUSB_DRVID_WINUSB:
		handle->Context.UsbW.NumberOfPackets = NumberOfPackets;
		break;
	default:
		ErrorSetAction(TRUE, ERROR_NOT_SUPPORTED, goto Error, "This driver does not support ISO");

	}

	return PoolHandle_Dec_IsochK(IsochHandle);

Error:
	PoolHandle_Dec_IsochK(IsochHandle);
	return FALSE;

}

KUSB_EXP BOOL KUSB_API IsochK_GetNumberOfPackets(
	_in KISOCH_HANDLE IsochHandle,
	_out PUINT NumberOfPackets)
{
	PKISOCH_HANDLE_INTERNAL handle;
	INT drvId;

	Pub_To_Priv_IsochK(IsochHandle, handle, return FALSE);
	ErrorNoSetAction(!PoolHandle_Inc_IsochK(IsochHandle), return FALSE, "IsochHandle is invalid");
	
	ErrorParam(!IsHandleValid(NumberOfPackets), Error, NumberOfPackets);

	drvId = handle->UsbHandle->Device->DriverAPI->Info.DriverID;
	switch (drvId)
	{
	case KUSB_DRVID_LIBUSBK:
		*NumberOfPackets = handle->Context.UsbK->NumberOfPackets;
		break;
	case KUSB_DRVID_WINUSB:
		*NumberOfPackets = handle->Context.UsbW.NumberOfPackets;
		break;
	default:
		ErrorSetAction(TRUE, ERROR_NOT_SUPPORTED, goto Error, "This driver does not support ISO");

	}

	return PoolHandle_Dec_IsochK(IsochHandle);

Error:
	PoolHandle_Dec_IsochK(IsochHandle);
	return FALSE;

}

KUSB_EXP BOOL KUSB_API IsochK_SetPacket(
	_in KISOCH_HANDLE IsochHandle,
	_in UINT PacketIndex,
	_in UINT Offset,
	_in UINT Length,
	_in UINT Status)
{
	PKISOCH_HANDLE_INTERNAL handle;
	INT drvId;

	Pub_To_Priv_IsochK(IsochHandle, handle, return FALSE);
	ErrorNoSetAction(!PoolHandle_Inc_IsochK(IsochHandle), return FALSE, "IsochHandle is invalid");

	ErrorParam(PacketIndex >= handle->PacketCount, Error, "PacketIndex");

	drvId = handle->UsbHandle->Device->DriverAPI->Info.DriverID;
	switch (drvId)
	{
	case KUSB_DRVID_LIBUSBK:
		handle->Context.UsbK->IsoPackets[PacketIndex].Offset = Offset;
		handle->Context.UsbK->IsoPackets[PacketIndex].Length = (USHORT)Length;
		handle->Context.UsbK->IsoPackets[PacketIndex].Status = (USHORT)Status;
		break;
	case KUSB_DRVID_WINUSB:
		handle->Context.UsbW.Packets[PacketIndex].Offset = Offset;
		handle->Context.UsbW.Packets[PacketIndex].Length = Length;
		handle->Context.UsbW.Packets[PacketIndex].Status = Status;
		break;
	default:
		ErrorSetAction(TRUE, ERROR_NOT_SUPPORTED, goto Error, "This driver does not support ISO");

	}

	return PoolHandle_Dec_IsochK(IsochHandle);

Error:
	PoolHandle_Dec_IsochK(IsochHandle);
	return FALSE;
}


KUSB_EXP BOOL KUSB_API IsochK_EnumPackets(
	_in KISOCH_HANDLE IsochHandle,
	_in KISOCH_ENUM_PACKETS_CB* EnumPackets,
	_inopt UINT StartPacketIndex,
	_inopt PVOID UserState)
{
	PKISOCH_HANDLE_INTERNAL handle;
	INT drvId;
	UINT offset, length, status;
	BOOL cont;
	
	Pub_To_Priv_IsochK(IsochHandle, handle, return FALSE);
	ErrorParamAction(EnumPackets == NULL, "EnumPackets", return FALSE);
	ErrorNoSetAction(!PoolHandle_Inc_IsochK(IsochHandle), return FALSE, "IsochHandle is invalid");
	ErrorParam(StartPacketIndex >= handle->PacketCount, Error, "StartPacketIndex");

	drvId = handle->UsbHandle->Device->DriverAPI->Info.DriverID;
	switch (drvId)
	{
	case KUSB_DRVID_LIBUSBK:
		for (; StartPacketIndex < (UINT)handle->Context.UsbK->NumberOfPackets; StartPacketIndex++)
		{
			offset = handle->Context.UsbK->IsoPackets[StartPacketIndex].Offset;
			length = handle->Context.UsbK->IsoPackets[StartPacketIndex].Length;
			status = handle->Context.UsbK->IsoPackets[StartPacketIndex].Status;
			cont = EnumPackets(StartPacketIndex, &offset, &length, &status, UserState);
			handle->Context.UsbK->IsoPackets[StartPacketIndex].Offset = offset;
			handle->Context.UsbK->IsoPackets[StartPacketIndex].Length = (USHORT)length;
			handle->Context.UsbK->IsoPackets[StartPacketIndex].Status = (USHORT)status;
			if (!cont) break;
		}
		break;
	case KUSB_DRVID_WINUSB:
		for (; StartPacketIndex < handle->Context.UsbW.NumberOfPackets; StartPacketIndex++)
		{
			offset = handle->Context.UsbW.Packets[StartPacketIndex].Offset;
			length = handle->Context.UsbW.Packets[StartPacketIndex].Length;
			status = handle->Context.UsbW.Packets[StartPacketIndex].Status;
			cont = EnumPackets(StartPacketIndex, &offset, &length, &status, UserState);
			handle->Context.UsbW.Packets[StartPacketIndex].Offset = offset;
			handle->Context.UsbW.Packets[StartPacketIndex].Length = length;
			handle->Context.UsbW.Packets[StartPacketIndex].Status = status;
			if (!cont) break;
		}
		break;
	default:
		ErrorSetAction(TRUE, ERROR_NOT_SUPPORTED, goto Error, "This driver does not support ISO");

	}

	return PoolHandle_Dec_IsochK(IsochHandle);

Error:
	PoolHandle_Dec_IsochK(IsochHandle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API IsochK_CalcPacketInformation(
	_in BOOL IsHighSpeed,
	_in PWINUSB_PIPE_INFORMATION_EX pipeInformationEx,
	_out PKISOCH_PACKET_INFORMATION PacketInformation)
{
	UINT pollingPeriod;

	ErrorParamAction(PacketInformation == NULL, "PacketInformation", return FALSE);
	
	if (IsHighSpeed)
	{
		pollingPeriod = PollingPeriod_Per_bInterval_HighSpeed[(pipeInformationEx->Interval - 1) % _countof(PollingPeriod_Per_bInterval_HighSpeed)];
		PacketInformation->PacketsPerFrame = 8 / pollingPeriod;
		
		PacketInformation->PollingPeriodMicroseconds = pollingPeriod * 125;
		PacketInformation->BytesPerMillisecond = (pipeInformationEx->MaximumBytesPerInterval * PacketInformation->PacketsPerFrame);
	}
	else
	{
		pollingPeriod = PollingPeriod_Per_bInterval_FullSpeed[(pipeInformationEx->Interval - 1) % _countof(PollingPeriod_Per_bInterval_HighSpeed)];
		PacketInformation->PacketsPerFrame = 1;
		PacketInformation->PollingPeriodMicroseconds = pollingPeriod * 1000;
		PacketInformation->BytesPerMillisecond = pipeInformationEx->MaximumPacketSize / pollingPeriod;
	}

	return TRUE;
}
