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
#define EP_RX					0x81
#define EP_TX					0x02
#define EP_PACKET_SIZE			512
#define ISO_PACKETS_PER_XFER	32

#define ISO_CALC_CONTEXT_SIZE(mNumOfIsoPackets) (sizeof(KISO_CONTEXT)+(sizeof(KISO_PACKET)*(mNumOfIsoPackets)))
#define ISO_CALC_DATABUFFER_SIZE(mNumOfIsoPackets, mIsoPacketSize) (mNumOfIsoPackets*mIsoPacketSize)

DWORD __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE handle = NULL;
	DWORD errorCode = ERROR_SUCCESS;

	BM_TEST_TYPE testType = BM_TEST_TYPE_READ;
	UCHAR pipeID = testType == BM_TEST_TYPE_READ ? EP_RX : EP_TX;

	BOOL success;
	UCHAR dataBuffer[ISO_CALC_DATABUFFER_SIZE(ISO_PACKETS_PER_XFER, EP_PACKET_SIZE)];
	ULONG transferred = 0;
	PKISO_CONTEXT isoCtx = NULL;
	KOVL_HANDLE ovlkHandle = NULL;
	ULONG posPacket;
	ULONG currentFrameNumber;
	KOVL_POOL_HANDLE ovlPool = NULL;

	UNREFERENCED_PARAMETER(currentFrameNumber);

	/*!
	Find the test device. Uses "vid=hhhh pid=hhhh" arguments supplied on the
	command line. (default is: vid=04D8 pid=FA2E)
	*/
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	/*!
	Initialize the device. This creates the physical usb handle.
	*/
	if (!UsbK_Init(&handle, deviceInfo))
	{
		errorCode = GetLastError();
		printf("Init device failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

	/*!
	Configure the benchmark test device to accept/send data.
	*/
	success = Bench_Configure(handle, BM_COMMAND_SET_TEST, 0, NULL, &testType);
	if (!success)
	{
		errorCode = GetLastError();
		printf("Bench_Configure failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}

	IsoK_Init(&isoCtx, ISO_PACKETS_PER_XFER, pipeID, 0);
	IsoK_SetPackets(isoCtx, EP_PACKET_SIZE);

	/*!
	Initialize a new OvlK pool handle.
	*/
	OvlK_InitPool(&ovlPool, handle, 4, 0);
	OvlK_Acquire(&ovlkHandle, ovlPool);

	UsbK_ResetPipe(handle, isoCtx->PipeID);

#ifdef USE_STARTFRAME
	success = UsbK_GetCurrentFrameNumber(handle, &currentFrameNumber);
	if (!success)
	{
		errorCode = GetLastError();
		printf("UsbK_GetCurrentFrameNumber failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}

	isoCtx->StartFrame = currentFrameNumber + 8 + (currentFrameNumber % 8);
#endif

	if (testType == BM_TEST_TYPE_READ)
		UsbK_IsoReadPipe(handle, isoCtx, dataBuffer, sizeof(dataBuffer), ovlkHandle);
	else
		UsbK_IsoWritePipe(handle, isoCtx, dataBuffer, sizeof(dataBuffer), ovlkHandle);

	success = OvlK_WaitAndRelease(ovlkHandle, 1000, &transferred);
	if (!success)
	{
		errorCode = GetLastError();
		printf("IsoReadPipe failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}

	printf("ISO StartFrame=%08Xh ErrorCount=%u TransferCounter=%u Transferred=%u\n",
	       isoCtx->StartFrame, isoCtx->ErrorCount, isoCtx->TransferCounter, transferred);

	for (posPacket = 0; posPacket < isoCtx->NumberOfPackets; posPacket++)
	{
		PKISO_PACKET isoPacket = &isoCtx->IsoPackets[posPacket];
		ULONG posData = isoPacket->Offset;
		ULONG posDataMax =  posPacket + 1 < isoCtx->NumberOfPackets ? isoCtx->IsoPackets[posPacket + 1].Offset : sizeof(dataBuffer);

		//if (isoPacket->Length==EP_PACKET_SIZE)
		//	continue;

		printf("  IsoPacket[%d] Length=%u Status=%08Xh\n", posPacket, isoPacket->Length, isoPacket->Status);
		printf("  Data:");

		for (; posData < posDataMax; posData++)
		{
			printf("%02Xh ", dataBuffer[posData]);
			if ((posData & 0xF) == 0xF) printf("\n       ");
		}
		printf("\n");
		if ((posData & 0xF) != 0xF) printf("\n");
	}

Done:
	/*!
	Close the device handle.
	*/
	UsbK_Free(handle);
	/*!
	Free the device list.
	*/
	LstK_Free(&deviceList);
	/*!
	Free the iso context.
	*/
	IsoK_Free(isoCtx);

	/*!
	Free the overlapped pool.
	*/
	OvlK_FreePool(ovlPool);

	return errorCode;
}
