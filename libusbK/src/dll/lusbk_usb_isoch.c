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
			}
			if (handle->Context.UsbW.Packets != NULL) Mem_Free((PVOID)handle->Context.UsbW.Packets);

			break;
		}

		PoolHandle_Dec_UsbK(handle->UsbHandle);
	}
}

KUSB_EXP BOOL KUSB_API IsochK_Init(
	_out KUSB_ISOCH_HANDLE* IsochHandle,
	_in KUSB_HANDLE InterfaceHandle,
	_in UCHAR PipeId,
	_in UINT MaxNumberOfPackets,
	_in PUCHAR TransferBuffer,
	_in UINT TransferBufferSize)
{
	PKISOCH_HANDLE_INTERNAL handle = NULL;
	PKUSB_HANDLE_INTERNAL usbHandle = NULL;
	
	ErrorParamAction(IsochHandle==NULL, "IsochHandle", goto Error);
	ErrorParamAction(!IsHandleValid(InterfaceHandle), "UsbHandle", goto Error);
	ErrorParamAction(MaxNumberOfPackets <= 0, "MaxNumberOfPackets", goto Error);

	ErrorNoSetAction((!PoolHandle_Inc_UsbK(InterfaceHandle)), goto Error, "UsbHandle is not a KUSB_HANDLE");
	usbHandle = (PKUSB_HANDLE_INTERNAL)InterfaceHandle;
	
	handle = PoolHandle_Acquire_IsochK((PKOBJ_CB)Isoch_Cleanup);
	ErrorNoSetAction(!IsHandleValid(handle), goto Error, "->PoolHandle_Acquire_IsochK");
	

	handle->UsbHandle = usbHandle;
	handle->PipeID = PipeId;
	handle->PacketCount = MaxNumberOfPackets;
	handle->TransferBuffer = TransferBuffer;
	handle->TransferBufferSize = TransferBufferSize;

	switch(usbHandle->Device->DriverAPI->Info.DriverID)
	{
	case KUSB_DRVID_LIBUSBK:
		ErrorMemory(!IsoK_Init(&handle->Context.UsbK, handle->PacketCount, 0), Error);
		handle->Context.UsbK->Flags = KISO_FLAG_SET_START_FRAME;
		break;
	case KUSB_DRVID_WINUSB:
		if (!WinUsb.RegisterIsochBuffer(Get_PipeInterfaceHandle(usbHandle, handle->PipeID), handle->PipeID, handle->TransferBuffer, handle->TransferBufferSize, &handle->Context.UsbW.BufferHandle))
		{
			ErrorNoSet(TRUE, Error, "WinUsb.RegisterIsochBuffer Failed.");
		}
		handle->Context.UsbW.Packets = Mem_Alloc(MaxNumberOfPackets * sizeof(USBD_ISO_PACKET_DESCRIPTOR));
		handle->Context.UsbK->NumberOfPackets = (SHORT) MaxNumberOfPackets;
		break;
		default:
			ErrorSetAction(TRUE, ERROR_NOT_SUPPORTED, goto Error, "This driver does not support ISO");

	}
	
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
	_in KUSB_ISOCH_HANDLE IsochHandle)
{
	ErrorHandle(!IsHandleValid(IsochHandle), Error, "IsochHandle");

	return PoolHandle_Dec_IsochK(IsochHandle);

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API IsochK_SetPacketOffsets(
	_in KUSB_ISOCH_HANDLE IsochHandle,
	_in UINT NumberOfPackets,
	_in UINT PacketSize)
{
	UINT packetIndex;
	UINT nextOffSet = 0;
	PKISOCH_HANDLE_INTERNAL handle;
	INT drvId;
	
	Pub_To_Priv_IsochK(IsochHandle, handle, return FALSE);
	ErrorNoSetAction(!PoolHandle_Inc_IsochK(IsochHandle), return FALSE, "IsochHandle is invalid");
	
	ErrorParam(NumberOfPackets > handle->PacketCount, Error, "NumberOfPackets");
	
	drvId = handle->UsbHandle->Device->DriverAPI->Info.DriverID;
	switch(drvId)
	{
	case KUSB_DRVID_LIBUSBK:
		handle->Context.UsbK->NumberOfPackets = (SHORT)NumberOfPackets;
		for (packetIndex = 0; packetIndex < NumberOfPackets; packetIndex++)
		{
			handle->Context.UsbK->IsoPackets[packetIndex].Offset = nextOffSet;
			nextOffSet += PacketSize;
		}
		break;
	case KUSB_DRVID_WINUSB:
		handle->Context.UsbW.NumberOfPackets = (UINT)NumberOfPackets;
		for (packetIndex = 0; packetIndex < NumberOfPackets; packetIndex++)
		{
			handle->Context.UsbW.Packets[packetIndex].Offset = nextOffSet;
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
	_in KUSB_ISOCH_HANDLE IsochHandle,
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

KUSB_EXP BOOL KUSB_API IsochK_SetPacket(
	_in KUSB_ISOCH_HANDLE IsochHandle,
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
	_in KUSB_ISOCH_HANDLE IsochHandle,
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


