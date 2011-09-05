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
#define EP_ADDRESS				0x81
#define MAX_TRANSFER_SIZE		4096
#define MAX_PENDING_TRANSFERS	128
#define MAX_PENDING_IO			3

// Gobals:
KUSB_DRIVER_API		Usb;
DATA_COUNTER_STATS	Dcs;

DWORD __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE usbHandle = NULL;
	KSTM_HANDLE streamHandle = NULL;
	DWORD errorCode = ERROR_SUCCESS;
	DWORD transferLength = 0;
	BOOL success;
	UCHAR myBuffer[MAX_TRANSFER_SIZE * MAX_PENDING_IO];
	BM_TEST_TYPE testType = USB_ENDPOINT_DIRECTION_IN(EP_ADDRESS) ? BM_TEST_TYPE_READ : BM_TEST_TYPE_WRITE;

	/*
	Find the test device. Uses "vid=hhhh pid=hhhh" arguments supplied on the
	command line. (default is: vid=04D8 pid=FA2E)
	*/
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	LibK_LoadDriverAPI(&Usb, deviceInfo->DriverID);

	/*
	Initialize the device. This creates the physical usb handle.
	*/
	if (!Usb.Init(&usbHandle, deviceInfo))
	{
		errorCode = GetLastError();
		printf("Init device failed. ErrorCode: %08Xh\n",  errorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

	/*
	Configure the benchmark test test type.
	*/
	success = Bench_Configure(usbHandle, BM_COMMAND_SET_TEST, 0, &Usb, &testType);
	if (!success) goto Done;

	/*
	Initialize a new stream handle.
	*/
	success = StmK_Init(
	              &streamHandle,
	              usbHandle,
	              EP_ADDRESS,
	              MAX_TRANSFER_SIZE,
	              MAX_PENDING_TRANSFERS,
	              MAX_PENDING_IO,
	              NULL,
	              KSTM_FLAG_NONE);
	errorCode = (success) ? ERROR_SUCCESS  : GetLastError();
	printf("[Init  Stream] result = %08Xh\n", errorCode);
	if (!success) goto Done;

	/*
	Start the stream.
	*/
	success = StmK_Start(streamHandle);
	errorCode = (success) ? ERROR_SUCCESS  : GetLastError();
	printf("[Start Stream] result = %08Xh\n", errorCode);
	if (!success) goto Done;

	mDcs_Init(&Dcs);

	while(_kbhit()) _getch();
	do
	{
		ULONG length;
		if (USB_ENDPOINT_DIRECTION_IN(EP_ADDRESS))
			success = StmK_Read(streamHandle, myBuffer, 0, sizeof(myBuffer), &length);
		else
			success = StmK_Write(streamHandle, myBuffer, 0, sizeof(myBuffer), &length);

		if (success)
		{
			transferLength += length;
		}
		else
		{
			// If the return result is ERROR_NO_MORE_ITEMS then there is no more data
			// to read.  Other errors indicate a problem.
			if (GetLastError() != ERROR_NO_MORE_ITEMS)
			{
				break;
			}
			if (transferLength >= sizeof(myBuffer) * 8)
			{
				mDcs_MarkStop(&Dcs, transferLength);
				printf("Transferred bytes:%d BPS:%.2f\n", transferLength, Dcs.Bps);
				transferLength = 0;
			}
			if ((Dcs.TotalBytes >= (sizeof(myBuffer) * 128)))
			{
				if (transferLength)
				{
					mDcs_MarkStop(&Dcs, transferLength);
					transferLength = 0;
				}

				// Stop and start the stream; This is done here to excercise the API only.
				success = StmK_Stop(streamHandle, 0);
				errorCode = (success) ? ERROR_SUCCESS  : GetLastError();
				if (!success) printf("[Stop  Stream] result = %08Xh\n", errorCode);
				if (!success) goto Done;

				printf("[StreamResult] "
				       "Bytes-total:%I64d Seconds-total:%.3f BPS-average:%.2f\n",
				       Dcs.TotalBytes, Dcs.Duration, Dcs.Bps);

				success = StmK_Start(streamHandle);
				errorCode = (success) ? ERROR_SUCCESS  : GetLastError();
				if (!success) printf("[Start Stream] result = %08Xh\n", errorCode);
				if (!success) goto Done;
				//////////////////////////////////////////////////////////////////////////

				mDcs_Init(&Dcs);
			}
			else
			{
				// ..Emulate some work..
				Sleep(100);
			}
		}

	}
	while(!_kbhit());

	/*
	Stop the stream.
	*/
	success = StmK_Stop(streamHandle, 0);
	errorCode = (success) ? ERROR_SUCCESS  : GetLastError();
	printf("[Stop  Stream] result = %08Xh\n", errorCode);
	if (!success) goto Done;

Done:
	// Free the stream handle.
	StmK_Free(streamHandle);

	// Free the usb handle.
	UsbK_Free(usbHandle);

	// Free the device list.
	LstK_Free(deviceList);

	return errorCode;
}
/*!
Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
Device opened successfully!
[Init  Stream] result = 00000000h
[Start Stream] result = 00000000h
Transferred bytes:524288 BPS:185559705.08
Transferred bytes:159744 BPS:3444001.36
Transferred bytes:159744 BPS:2128927.06
Transferred bytes:167936 BPS:1694587.83
Transferred bytes:163840 BPS:1473756.41
Transferred bytes:163840 BPS:1343928.97
Transferred bytes:167936 BPS:1259668.21
Transferred bytes:167936 BPS:1199183.22
[StreamResult] Bytes-total:1675264 Seconds-total:1.397 BPS-average:1199183.22
Transferred bytes:172032 BPS:876132.77
Transferred bytes:167936 BPS:858126.93
Transferred bytes:163840 BPS:843762.19
Transferred bytes:163840 BPS:838453.05
Transferred bytes:163840 BPS:834713.10
Transferred bytes:163840 BPS:832090.60
Transferred bytes:163840 BPS:830226.44
Transferred bytes:163840 BPS:828502.37
Transferred bytes:167936 BPS:829741.23
Transferred bytes:163840 BPS:828706.57
[StreamResult] Bytes-total:1654784 Seconds-total:1.997 BPS-average:828706.57
Transferred bytes:167936 BPS:860579.17
Transferred bytes:159744 BPS:828717.06
Transferred bytes:159744 BPS:819513.85
Transferred bytes:163840 BPS:819588.39
Transferred bytes:159744 BPS:815706.23
Transferred bytes:163840 BPS:816251.25
Transferred bytes:159744 BPS:813581.03
Transferred bytes:163840 BPS:814108.63
Transferred bytes:163840 BPS:815067.72
Transferred bytes:163840 BPS:815427.14
[StreamResult] Bytes-total:1626112 Seconds-total:1.994 BPS-average:815427.14
[Stop  Stream] result = 00000000h
*/
