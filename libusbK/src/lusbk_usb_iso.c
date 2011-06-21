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

KUSB_EXP BOOL KUSB_API IsoK_Create (
    __deref_out PKUSB_ISO_CONTEXT* IsoContext,
    __in ULONG NumberOfPackets,
    __in_opt UCHAR PipeID,
    __in_opt ULONG StartFrame)
{
	PKUSB_ISO_CONTEXT isoCtx = NULL;

	ErrorHandle(!IsHandleValid(IsoContext), Error, "IsoContext");

	isoCtx = Mem_Alloc(sizeof(KUSB_ISO_CONTEXT) + sizeof(KUSB_ISO_PACKET) * NumberOfPackets);
	if (!isoCtx)
	{
		ErrorMemory(!IsHandleValid(isoCtx), Error);
	}

	isoCtx->NumberOfPackets = NumberOfPackets;
	isoCtx->PipeID = PipeID;
	isoCtx->StartFrame = StartFrame;

	*IsoContext = isoCtx;
	return TRUE;

Error:
	Mem_Free(&isoCtx);
	if (IsoContext)
		*IsoContext = NULL;

	return FALSE;
}

KUSB_EXP BOOL KUSB_API IsoK_Destroy(
    __deref_inout PKUSB_ISO_CONTEXT* IsoContext)
{
	ErrorHandle(!IsHandleValid(IsoContext), Error, "IsoContext");
	ErrorHandle(!IsHandleValid(*IsoContext), Error, "IsoContext");

	Mem_Free(IsoContext);
	return TRUE;

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API IsoK_InitPackets(
    __inout PKUSB_ISO_CONTEXT IsoContext,
    __in ULONG PacketSize)
{
	ULONG packetIndex;
	ULONG nextOffSet = 0;

	ErrorHandle(!IsHandleValid(IsoContext), Error, "IsoContext");

	for (packetIndex = 0; packetIndex < IsoContext->NumberOfPackets; packetIndex++)
	{
		IsoContext->IsoPackets[packetIndex].Offset = nextOffSet;
		nextOffSet += PacketSize;
	}
	return TRUE;

Error:
	return FALSE;
}


KUSB_EXP BOOL KUSB_API IsoK_SetPacket(
    __in PKUSB_ISO_CONTEXT IsoContext,
    __in ULONG PacketIndex,
    __in PKUSB_ISO_PACKET IsoPacket)
{
	ErrorHandle(!IsHandleValid(IsoContext), Error, "IsoContext");
	ErrorParam(PacketIndex >= IsoContext->NumberOfPackets, Error, "PacketIndex");
	ErrorParam(!IsHandleValid(IsoPacket), Error, "IsoPacket");

	memcpy(&IsoContext->IsoPackets[PacketIndex], IsoPacket, sizeof(*IsoPacket));
	return TRUE;

Error:
	return FALSE;
}


KUSB_EXP BOOL KUSB_API IsoK_GetPacket(
    __in PKUSB_ISO_CONTEXT IsoContext,
    __in ULONG PacketIndex,
    __out PKUSB_ISO_PACKET IsoPacket)
{
	ErrorHandle(!IsHandleValid(IsoContext), Error, "IsoContext");
	ErrorParam(PacketIndex >= IsoContext->NumberOfPackets, Error, "PacketIndex");
	ErrorParam(!IsHandleValid(IsoPacket), Error, "IsoPacket");

	memcpy(IsoPacket, &IsoContext->IsoPackets[PacketIndex], sizeof(*IsoPacket));
	return TRUE;
Error:
	return FALSE;
}


KUSB_EXP BOOL KUSB_API IsoK_EnumPackets(
    __in PKUSB_ISO_CONTEXT IsoContext,
    __in ISO_ENUM_PACKETS_CB* EnumPackets,
    __in_opt ULONG StartPacketIndex,
    __in_opt PVOID UserContext)
{
	ErrorHandle(!IsHandleValid(IsoContext), Error, "IsoContext");
	ErrorParam(!IsHandleValid(EnumPackets), Error, "EnumPackets");
	ErrorParam(StartPacketIndex >= IsoContext->NumberOfPackets, Error, "StartPacketIndex");

	for (StartPacketIndex; StartPacketIndex < IsoContext->NumberOfPackets; StartPacketIndex++)
	{
		if (!EnumPackets(StartPacketIndex, &IsoContext->IsoPackets[StartPacketIndex], UserContext))
			break;
	}
	return TRUE;

Error:
	return FALSE;
}


KUSB_EXP BOOL KUSB_API IsoK_Reuse(
    __inout PKUSB_ISO_CONTEXT IsoContext)
{
	ULONG packetIndex;

	ErrorHandle(!IsHandleValid(IsoContext), Error, "IsoContext");

	for (packetIndex = 0; packetIndex < IsoContext->NumberOfPackets; packetIndex++)
	{
		IsoContext->IsoPackets[packetIndex].Length = 0;
		IsoContext->IsoPackets[packetIndex].Status = 0;
	}
	IsoContext->StartFrame = 0;
	IsoContext->ErrorCount = 0;

	return TRUE;

Error:
	return FALSE;
}

