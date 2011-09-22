/*!
#
# Copyright (c) 2011 Travis Robinson <libusbdotnet@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS LEE ROBINSON
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
*/
#include "examples.h"

// Example configuration:
#define EP_TRANSFER				0x81
#define EP_PACKET_SIZE			64
#define ISO_PACKETS_PER_XFER	32

#define ISO_CALC_CONTEXT_SIZE(mNumOfIsoPackets) (sizeof(KISO_CONTEXT)+(sizeof(KISO_PACKET)*(mNumOfIsoPackets)))
#define ISO_CALC_DATABUFFER_SIZE(mNumOfIsoPackets, mIsoPacketSize) (mNumOfIsoPackets*mIsoPacketSize)

DWORD __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE handle = NULL;
	DWORD errorCode = ERROR_SUCCESS;
	UCHAR pipeID = EP_TRANSFER;
	BM_TEST_TYPE testType = USB_ENDPOINT_DIRECTION_IN(EP_TRANSFER) ? BM_TEST_TYPE_READ : BM_TEST_TYPE_WRITE;

	BOOL success;
	UCHAR dataBuffer[ISO_CALC_DATABUFFER_SIZE(ISO_PACKETS_PER_XFER, EP_PACKET_SIZE)];
	ULONG transferred = 0;
	PKISO_CONTEXT isoCtx = NULL;
	KOVL_HANDLE ovlkHandle = NULL;
	LONG posPacket;
	ULONG currentFrameNumber;
	KOVL_POOL_HANDLE ovlPool = NULL;

	UNREFERENCED_PARAMETER(currentFrameNumber);

	/*
	Find the test device. Uses "vid=hhhh pid=hhhh" arguments supplied on the
	command line. (default is: vid=04D8 pid=FA2E)
	*/
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	/*
	Initialize the device. This creates the physical usb handle.
	*/
	if (!UsbK_Init(&handle, deviceInfo))
	{
		errorCode = GetLastError();
		printf("UsbK_Init failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

	/*
	Configure the benchmark test device to accept/send data.
	*/
	success = Bench_Configure(handle, BM_COMMAND_SET_TEST, 0, NULL, &testType);
	if (!success)
	{
		errorCode = GetLastError();
		printf("Bench_Configure failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}

	/*
	Initialize a new iso context handle.
	*/
	IsoK_Init(&isoCtx, ISO_PACKETS_PER_XFER, 0);

	/*
	Set the iso packet offsets.
	*/
	IsoK_SetPackets(isoCtx, EP_PACKET_SIZE);

	/*
	Initialize a new OvlK pool handle.
	*/
	OvlK_Init(&ovlPool, handle, 4, 0);
	OvlK_Acquire(&ovlkHandle, ovlPool);

	UsbK_ResetPipe(handle, pipeID);

	if (testType == BM_TEST_TYPE_READ)
		UsbK_IsoReadPipe(handle, pipeID, dataBuffer, sizeof(dataBuffer), ovlkHandle, isoCtx);
	else
		UsbK_IsoWritePipe(handle, pipeID, dataBuffer, sizeof(dataBuffer), ovlkHandle, isoCtx);

	success = OvlK_WaitAndRelease(ovlkHandle, 1000, &transferred);
	if (!success)
	{
		errorCode = GetLastError();
		printf("IsoReadPipe failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}

	printf("ISO StartFrame=%08Xh ErrorCount=%u Transferred=%u\n",
	       isoCtx->StartFrame, isoCtx->ErrorCount, transferred);

	for (posPacket = 0; posPacket < isoCtx->NumberOfPackets; posPacket++)
	{
		LONG posByte;
		PKISO_PACKET isoPacket = &isoCtx->IsoPackets[posPacket];

		printf("  IsoPacket[%d] Length=%u Status=%08Xh\n", posPacket, isoPacket->Length, isoPacket->Status);
		printf("  Data:");

		for (posByte = 0; posByte < isoPacket->Length; posByte++)
		{
			printf("%02Xh ", dataBuffer[isoPacket->Offset + posByte]);
			if ((posByte & 0xF) == 0xF) printf("\n       ");
		}
		printf("\n");
		if ((posByte & 0xF) != 0xF) printf("\n");
	}

Done:
	// Close the device handle.
	UsbK_Free(handle);

	// Free the device list.
	LstK_Free(&deviceList);

	// Free the iso context.
	IsoK_Free(isoCtx);

	// Free the overlapped pool.
	OvlK_Free(ovlPool);

	return errorCode;
}
/*!
Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
Device opened successfully!
ISO StartFrame=24905D5Ch ErrorCount=0 Transferred=2048
  IsoPacket[0] Length=64 Status=00000000h
  Data:00h 00h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[1] Length=64 Status=00000000h
  Data:00h 01h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[2] Length=64 Status=00000000h
  Data:00h 02h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[3] Length=64 Status=00000000h
  Data:00h 03h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[4] Length=64 Status=00000000h
  Data:00h 04h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[5] Length=64 Status=00000000h
  Data:00h 05h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[6] Length=64 Status=00000000h
  Data:00h 06h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[7] Length=64 Status=00000000h
  Data:00h 07h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[8] Length=64 Status=00000000h
  Data:00h 08h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[9] Length=64 Status=00000000h
  Data:00h 09h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[10] Length=64 Status=00000000h
  Data:00h 0Ah 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[11] Length=64 Status=00000000h
  Data:00h 0Bh 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[12] Length=64 Status=00000000h
  Data:00h 0Ch 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[13] Length=64 Status=00000000h
  Data:00h 0Dh 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[14] Length=64 Status=00000000h
  Data:00h 0Eh 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[15] Length=64 Status=00000000h
  Data:00h 0Fh 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[16] Length=64 Status=00000000h
  Data:00h 10h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[17] Length=64 Status=00000000h
  Data:00h 11h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[18] Length=64 Status=00000000h
  Data:00h 12h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[19] Length=64 Status=00000000h
  Data:00h 13h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[20] Length=64 Status=00000000h
  Data:00h 14h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[21] Length=64 Status=00000000h
  Data:00h 15h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[22] Length=64 Status=00000000h
  Data:00h 16h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[23] Length=64 Status=00000000h
  Data:00h 17h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[24] Length=64 Status=00000000h
  Data:00h 18h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[25] Length=64 Status=00000000h
  Data:00h 19h 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[26] Length=64 Status=00000000h
  Data:00h 1Ah 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[27] Length=64 Status=00000000h
  Data:00h 1Bh 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[28] Length=64 Status=00000000h
  Data:00h 1Ch 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[29] Length=64 Status=00000000h
  Data:00h 1Dh 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[30] Length=64 Status=00000000h
  Data:00h 1Eh 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh


  IsoPacket[31] Length=64 Status=00000000h
  Data:00h 1Fh 02h 03h 04h 05h 06h 07h 08h 09h 0Ah 0Bh 0Ch 0Dh 0Eh 0Fh
       10h 11h 12h 13h 14h 15h 16h 17h 18h 19h 1Ah 1Bh 1Ch 1Dh 1Eh 1Fh
       20h 21h 22h 23h 24h 25h 26h 27h 28h 29h 2Ah 2Bh 2Ch 2Dh 2Eh 2Fh
       30h 31h 32h 33h 34h 35h 36h 37h 38h 39h 3Ah 3Bh 3Ch 3Dh 3Eh 3Fh
*/