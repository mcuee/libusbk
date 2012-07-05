/*!
#
# Copyright (c) 2012 Travis Robinson <libusbdotnet@gmail.com>
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

#define SC_GET_STREAM_DATA     0xb1
#define SC_SET_STREAM_DATA     0xb2

// Example configuration:
#define EP_TX			0x00
#define EP_RX			0x80
#define EP_PACKET_SIZE	  16

#define XFER_LENGTH		(EP_PACKET_SIZE)
#define XFER_TIMEOUT	1000

// Max xfer pairs to submit before the test is considered finished.
#define MAX_XFERS	512

// Number of xfer pairs that must be outstanding before wait operations.
#define MAX_PENDING_IO	3

// Globals:

KUSB_DRIVER_API Usb;

/*
The running loop counter. When it hits MAX_XFERS, no more
transfers are submitted and the remaining complete.
*/
static DWORD g_LoopCounter = 0;

/*
Win32 error code maintained by the various functions below and used for
application exit code.
*/
static DWORD g_ErrorCode = ERROR_SUCCESS;

/*
Overlapped (OvlK) pool handle.
*/
KOVL_POOL_HANDLE gOvlPool = NULL;
/*
MY_XFER_EL represents a user defined doubly linked transfer context list.
*/
typedef struct _MY_XFER_EL
{
	// Stores the overlappedK pointer from OvlK_Acquire
	KOVL_HANDLE Ovl;

	// Physical data buffer.
	UCHAR Buffer[XFER_LENGTH];

	// Used as a submit length, and a received length.
	DWORD Length;

	// Win32 error code results for submit/wait operations
	DWORD ErrorCode;

	KUSB_SETUP_PACKET SetupPacket;
	/*
	Doubly linked list elements; required for the utlist macros defined in
	lusbk_linked_list.h. These linked list macros were created by Troy
	D. Hanson and he provides more excellent macros here:
	http://uthash.sourceforge.net/index.html
	*/
	struct _MY_XFER_EL* prev;
	struct _MY_XFER_EL* next;
} MY_XFER_EL;

/*
OnDataNeeded [TX-LIKE EVENT]
- Called by our Xfer_Submit function when data needs to
  be sent to EP_TX.
*/
VOID OnDataNeeded(MY_XFER_EL* xfer, UCHAR PipeID)
{
	static DWORD CounterTx = 0;
	UCHAR chStart;
	int pos;
	/*
	..fill the data buffer here.. (xfer->Buffer,xfer->Length)
	*/
	xfer->Length = XFER_LENGTH;

	if (CounterTx % XFER_LENGTH)
		chStart = CounterTx % XFER_LENGTH;
	else
		chStart = 0;

	for (pos=chStart; pos < XFER_LENGTH+chStart; pos++)
	{
		xfer->Buffer[pos-chStart]=(UCHAR)(pos + (UCHAR)'A');
	}
	printf("[Tx-%03u] PipeID=%02Xh Length=%u\n", ++CounterTx, PipeID, xfer->Length);
}

/*
OnDataArrival [RX-LIKE EVENT]
- Called by our Xfer_Wait function when data arrives
  from EP_RX.
*/
VOID OnDataArrival(MY_XFER_EL* xfer, UCHAR PipeID)
{
	static DWORD CounterRx = 0;
	/*
	..consume the data buffer here.. (xfer->Buffer, xfer->Length)
	*/
	printf("[Rx-%03u] PipeID=%02Xh Length=%u\n", ++CounterRx, PipeID, xfer->Length);
}

/*
Moves the head MY_XFER_EL element in AvailList to the tail of
WaitList and starts an asynchronous transfer operation using
the moved element.
*/
BOOL Xfer_Submit(KUSB_HANDLE usbHandle,
                 MY_XFER_EL** AvailList,
                 MY_XFER_EL** WaitList,
                 UCHAR PipeID)
{
	MY_XFER_EL* move = *AvailList;

	DL_DELETE(*AvailList, move);
	DL_APPEND(*WaitList, move);

	if (!OvlK_Acquire(&move->Ovl, gOvlPool))
	{
		g_ErrorCode = GetLastError();
		printf("OvlK_Acquire failed. ErrorCode: %08Xh\n", g_ErrorCode);
		return FALSE;
	}
	move->SetupPacket.BmRequest.Type = BMREQUEST_TYPE_VENDOR;
	move->SetupPacket.BmRequest.Recipient = BMREQUEST_RECIPIENT_INTERFACE;

	if (USB_ENDPOINT_DIRECTION_IN(PipeID))
	{
		move->Length = XFER_LENGTH;
		move->SetupPacket.BmRequest.Dir = BMREQUEST_DIR_DEVICE_TO_HOST;
		move->SetupPacket.Length = (USHORT)move->Length;
		move->SetupPacket.Request = SC_GET_STREAM_DATA;
	}
	else
	{
		OnDataNeeded(move, PipeID);
		move->SetupPacket.BmRequest.Dir = BMREQUEST_DIR_HOST_TO_DEVICE;
		move->SetupPacket.Length = (USHORT)move->Length;
		move->SetupPacket.Request = SC_SET_STREAM_DATA;
	}
	Usb.ControlTransfer(usbHandle, *((WINUSB_SETUP_PACKET*)&move->SetupPacket), move->Buffer, move->Length, NULL, move->Ovl);

	if ((move->ErrorCode = GetLastError()) != ERROR_IO_PENDING)
	{
		g_ErrorCode = move->ErrorCode;
		printf("Usb.ControlTransfer failed on pipe-id %02Xh. ErrorCode: %08Xh\n",
		       PipeID, g_ErrorCode);

		// Abort the test.
		g_LoopCounter = MAX_XFERS;
		return FALSE;
	}

	return TRUE;
}

/*
Waits for the first element in PendingList (the oldest) to complete. If
the operation completes successfully and a read pipe-id is specified,
calls OnDataArrival.
*/
BOOL Xfer_Wait(MY_XFER_EL* PendingList, ULONG Timeout, UCHAR PipeID)
{
	/*
	If the ErrorCode is not ERROR_IO_PENDING at this point, the xfer did not
	submt properly, so we will not wait time for a completion.
	*/
	if (PendingList->ErrorCode != ERROR_IO_PENDING)
	{
		OvlK_Release(PendingList->Ovl);
		return FALSE;
	}

	/*
	OvlK_WaitAndRelease is called here regardless of whether the xfer
	submitted with or without errors.
	*/
	if (!OvlK_WaitAndRelease(PendingList->Ovl, Timeout, &PendingList->Length))
	{
		g_ErrorCode = PendingList->ErrorCode = GetLastError();
		printf("Failed getting i/o results. ErrorCode: %08Xh\n", g_ErrorCode);

		// Abort the test.
		g_LoopCounter = MAX_XFERS;

		return FALSE;
	}
	if (USB_ENDPOINT_DIRECTION_IN(PipeID))
	{
		OnDataArrival(PendingList, PipeID);
	}
	return TRUE;
}

/*
Moves the head MY_XFER_EL element in WaitList to the tail of
AvailList.
*/
VOID Xfer_Recycle(MY_XFER_EL** WaitList, MY_XFER_EL** AvailList)
{
	MY_XFER_EL* move = *WaitList;
	DL_DELETE(*WaitList, move);

	if (g_LoopCounter < MAX_XFERS)
		DL_APPEND(*AvailList, move);
}

DWORD __cdecl main(int argc, char* argv[])
{
	KLST_HANDLE deviceList = NULL;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;
	KUSB_HANDLE usbHandle = NULL;
	int i;

	/*
	The physical xfer context elements. One for every outstanding i/o
	operation is required.
	*/
	MY_XFER_EL xfersTx[MAX_PENDING_IO];
	MY_XFER_EL xfersRx[MAX_PENDING_IO];

	/*
	These xfer context lists will hold the xfer elements that are available.
	(not in-use)
	*/
	MY_XFER_EL* xferAvailListTx = NULL;
	MY_XFER_EL* xferAvailListRx = NULL;

	/*
	These xfer context lists will hold the xfer elements that are outstanding.
	(in-use)
	*/
	MY_XFER_EL* xferWaitListTx = NULL;
	MY_XFER_EL* xferWaitListRx = NULL;


	/*
	Initially, all xfer elements are zeroed and added to the xferAvailxx lists.
	*/
	memset(xfersTx, 0, sizeof(xfersTx));
	memset(xfersRx, 0, sizeof(xfersRx));
	for (i = 0; i < MAX_PENDING_IO; i++)
	{
		DL_APPEND(xferAvailListTx, &xfersTx[i]);
		DL_APPEND(xferAvailListRx, &xfersRx[i]);
	}

	// Find the test device using arguments supplied on the command line.
	// (default is: vid=04D8 pid=FA2E)
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	/*
	This example will use the dynamic driver api so that it can be used
	with all supported drivers.
	*/
	LibK_LoadDriverAPI(&Usb, deviceInfo->DriverID);

	// Initialize the device
	if (!Usb.Init(&usbHandle, deviceInfo))
	{
		g_ErrorCode = GetLastError();
		printf("Usb.Init failed. ErrorCode: %08Xh\n", g_ErrorCode);
		goto Done;
	}

	/*
	USER-FIXME: ADDITIONAL CONFIG
	If the test is isochronous, there is often more configuration steps.
	e.g. The alternate interface may need to be selected.

	Usb.SetCurrentAlternateSetting(usbHandle, ALT_INTF_SETTING);

	To meet Windows Logo Specifications, when any device has an interface
	that consumes isochronous bandwidth, that device must support multiple
	alternate settings for that interface. In addition, the alternate
	setting zero must not consume isochronous bandwidth.
	(ALT INTF #0 ISO EPs must have wMaxPacketSize=0)
	http://msdn.microsoft.com/en-us/library/ff568641%28v=vs.85%29.aspx
	*/
	printf("Device opened successfully!\n");

	/*!
	Initialize a new OvlK pool handle.
	*/
	if (!OvlK_Init(&gOvlPool, usbHandle, MAX_PENDING_IO * 2, 0))
	{
		g_ErrorCode = GetLastError();
		printf("OvlK_Init failed. ErrorCode: %08Xh\n", g_ErrorCode);
		goto Done;
	}

	/*
	This is the main tranfer loop. It's job is to keep the available lists
	empty and the wait (in-use) lists full. When it has fully emptied the
	available lists, it waits on the oldest read and write xfers of the wait
	(in-use) list. Next, it moves the completed xfer element back to the
	available list where it is immediately re-submitted. This routine
	continues to re-submit transfers until the MAX_XFERS define is
	reached.
	*/
	do
	{
		/*
		While there are xfer elements in the available list and the
		MAX_XFERS threshold has not been reached, continue to submit
		transfer requests.
		*/
		while (xferAvailListRx && xferAvailListTx && g_LoopCounter < MAX_XFERS)
		{
			g_LoopCounter++;
			Xfer_Submit(usbHandle, &xferAvailListTx, &xferWaitListTx, EP_TX);
			Xfer_Submit(usbHandle, &xferAvailListRx, &xferWaitListRx, EP_RX);
		}

		/*
		Unless the test is over, there will always be elements in the wait
		(in-use) lists at this point. This routine waits for the oldest
		read/write operation to complete and "recycles" the xfer element back to
		the available list.
		*/
		if (xferWaitListTx && xferWaitListRx)
		{
			Xfer_Wait(xferWaitListTx, XFER_TIMEOUT, EP_TX);
			Xfer_Wait(xferWaitListRx, XFER_TIMEOUT, EP_RX);

			Xfer_Recycle(&xferWaitListTx, &xferAvailListTx);
			Xfer_Recycle(&xferWaitListRx, &xferAvailListRx);
		}
	}
	while(xferWaitListTx && xferWaitListRx);

Done:
	// Close the usb handle.
	Usb.Free(usbHandle);

	// Free the device list.
	LstK_Free(deviceList);

	// Free the overlapped pool.
	OvlK_Free(gOvlPool);

	return g_ErrorCode;
}
/*!
Looking for device vid/pid 04D8/FA2E..
Using 04D8:FA2E (LUSBW1): Benchmark Device - Microchip Technology, Inc.
Device opened successfully!
[Tx-001] PipeID=00h Length=16
[Tx-002] PipeID=00h Length=16
[Tx-003] PipeID=00h Length=16
[Rx-001] PipeID=80h Length=16
[Tx-004] PipeID=00h Length=16
[Rx-002] PipeID=80h Length=16
[Tx-005] PipeID=00h Length=16
[Rx-003] PipeID=80h Length=16
[Tx-006] PipeID=00h Length=16
[Rx-004] PipeID=80h Length=16
[Tx-007] PipeID=00h Length=16
[Rx-005] PipeID=80h Length=16
[Tx-008] PipeID=00h Length=16
[Rx-006] PipeID=80h Length=16
[Tx-009] PipeID=00h Length=16
[Rx-007] PipeID=80h Length=16
[Tx-010] PipeID=00h Length=16
[Rx-008] PipeID=80h Length=16
[Tx-011] PipeID=00h Length=16
[Rx-009] PipeID=80h Length=16
[Tx-012] PipeID=00h Length=16
[Rx-010] PipeID=80h Length=16
[Rx-011] PipeID=80h Length=16
[Rx-012] PipeID=80h Length=16
*/
