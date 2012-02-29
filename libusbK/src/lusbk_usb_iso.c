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

KUSB_EXP BOOL KUSB_API IsoK_Init(
    _out PKISO_CONTEXT* IsoContext,
    _in INT NumberOfPackets,
    _inopt INT StartFrame)
{
	PKISO_CONTEXT isoCtx = NULL;

	ErrorHandle(!IsHandleValid(IsoContext), Error, "IsoContext");

	isoCtx = Mem_Alloc(sizeof(KISO_CONTEXT) + sizeof(KISO_PACKET) * NumberOfPackets);
	if (!isoCtx)
	{
		ErrorMemory(!IsHandleValid(isoCtx), Error);
	}

	isoCtx->NumberOfPackets = (SHORT)NumberOfPackets;
	isoCtx->StartFrame = StartFrame;

	*IsoContext = isoCtx;
	return TRUE;

Error:
	Mem_Free(&isoCtx);
	if (IsoContext)
		*IsoContext = NULL;

	return FALSE;
}

KUSB_EXP BOOL KUSB_API IsoK_Free(
    _in PKISO_CONTEXT IsoContext)
{
	ErrorHandle(!IsHandleValid(IsoContext), Error, "IsoContext");
	Mem_Free(&IsoContext);

	return TRUE;

Error:
	return FALSE;
}

KUSB_EXP BOOL KUSB_API IsoK_SetPackets(
    _in PKISO_CONTEXT IsoContext,
    _in INT PacketSize)
{
	LONG packetIndex;
	LONG nextOffSet = 0;

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
    _in PKISO_CONTEXT IsoContext,
    _in INT PacketIndex,
    _in PKISO_PACKET IsoPacket)
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
    _in PKISO_CONTEXT IsoContext,
    _in INT PacketIndex,
    _out PKISO_PACKET IsoPacket)
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
    _in PKISO_CONTEXT IsoContext,
    _in KISO_ENUM_PACKETS_CB* EnumPackets,
    _inopt INT StartPacketIndex,
    _inopt PVOID UserState)
{
	ErrorHandle(!IsHandleValid(IsoContext), Error, "IsoContext");
	ErrorParam(!IsHandleValid(EnumPackets), Error, "EnumPackets");
	ErrorParam(StartPacketIndex >= IsoContext->NumberOfPackets, Error, "StartPacketIndex");

	for (StartPacketIndex; StartPacketIndex < IsoContext->NumberOfPackets; StartPacketIndex++)
	{
		if (!EnumPackets(StartPacketIndex, &IsoContext->IsoPackets[StartPacketIndex], UserState))
			break;
	}
	return TRUE;

Error:
	return FALSE;
}


KUSB_EXP BOOL KUSB_API IsoK_ReUse(
    _ref PKISO_CONTEXT IsoContext)
{
	LONG packetIndex;

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

