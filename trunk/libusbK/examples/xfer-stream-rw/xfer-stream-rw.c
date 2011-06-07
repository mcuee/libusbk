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
#define EP_TX			0x01
#define EP_RX			0x81
#define EP_PACKET_SIZE	32

#define XFER_LENGTH		(EP_PACKET_SIZE*16)
#define XFER_TIMEOUT	1000

// Max xfer pairs to submit before the test is considered finished.
#define MAX_XFERS	12

// Number of xfer pairs that must be outstanding before wait operations.
#define MAX_PENDING_IO	3

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
MY_XFER_EL represents a user defined doubly linked transfer context list.
*/
typedef struct _MY_XFER_EL
{
	// Stores the overlappedK pointer from OvlK_Acquire
	POVERLAPPED_K Ovl;

	// Physical data buffer.
	UCHAR Buffer[XFER_LENGTH];

	// Used as a submit length, and a received length.
	DWORD Length;

	// Win32 error code results for submit/wait operations
	DWORD ErrorCode;

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
	/*
	..fill the data buffer here.. (xfer->Buffer,xfer->Length)
	*/
	xfer->Length = XFER_LENGTH;
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
BOOL Xfer_Submit(LIBUSBK_INTERFACE_HANDLE handle,
                 MY_XFER_EL** AvailList,
                 MY_XFER_EL** WaitList,
                 UCHAR PipeID)
{
	MY_XFER_EL* move = *AvailList;

	DL_DELETE(*AvailList, move);
	DL_APPEND(*WaitList, move);
	move->Ovl =  OvlK_Acquire(NULL);

	if (USB_ENDPOINT_DIRECTION_IN(PipeID))
	{
		move->Length = XFER_LENGTH;
		UsbK_ReadPipe(handle, PipeID, move->Buffer, move->Length, NULL, move->Ovl);
	}
	else
	{
		OnDataNeeded(move, PipeID);
		UsbK_WritePipe(handle, PipeID, move->Buffer, move->Length, NULL, move->Ovl);
	}
	if ((move->ErrorCode = GetLastError()) != ERROR_IO_PENDING)
	{
		g_ErrorCode = move->ErrorCode;
		printf("Failed transfer on pipe-id %02Xh.  Win32Error=%u (0x%08X)\n",
		       PipeID, g_ErrorCode, g_ErrorCode);

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
		Timeout = 0;

	/*
	OvlK_WaitAndRelease is called here regardless of whether the xfer
	submitted with or without errors.
	*/
	if (!OvlK_WaitAndRelease(PendingList->Ovl, Timeout, &PendingList->Length))
	{
		/*
		An error is expected if the xfer did not submit properly; we will not
		report an error here if it was reported by Xfer_Submit
		*/
		if (PendingList->ErrorCode == ERROR_IO_PENDING)
		{
			g_ErrorCode = PendingList->ErrorCode = GetLastError();
			printf("Failed getting i/o results.  Win32Error=%u (0x%08X)\n", g_ErrorCode, g_ErrorCode);

			// Abort the test.
			g_LoopCounter = MAX_XFERS;
		}

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
	PKUSB_DEV_LIST deviceList = NULL;
	PKUSB_DEV_INFO deviceInfo = NULL;
	LIBUSBK_INTERFACE_HANDLE handle = NULL;
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

	// Open the device
	if (!UsbK_Open(deviceInfo, &handle))
	{
		g_ErrorCode = GetLastError();
		printf("Open device failed. Win32Error=%u (0x%08X)\n", g_ErrorCode, g_ErrorCode);
		goto Done;
	}

	/*
	USER-FIXME: ADDITIONAL CONFIG
	If the test is isochronous, there is often more configuration steps.
	e.g. The alternate interface may need to be selected.

	UsbK_SetCurrentAlternateSetting(handle, ALT_INTF_SETTING);

	To meet Windows Logo Specifications, when any device has an interface
	that consumes isochronous bandwidth, that device must support multiple
	alternate settings for that interface. In addition, the alternate
	setting zero must not consume isochronous bandwidth.
	(ALT INTF #0 ISO EPs must have wMaxPacketSize=0)
	http://msdn.microsoft.com/en-us/library/ff568641%28v=vs.85%29.aspx
	*/
	printf("Device opened successfully!\n");

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
			Xfer_Submit(handle, &xferAvailListRx, &xferWaitListRx, EP_RX);
			Xfer_Submit(handle, &xferAvailListTx, &xferWaitListTx, EP_TX);
		}

		/*
		Unless the test is over, there will always be elements in the wait
		(in-use) lists at this point. This routine waits for the oldest
		read/write operation to complete and "recycles" the xfer element back to
		the available list.
		*/
		if (xferWaitListTx && xferWaitListRx)
		{
			Xfer_Wait(xferWaitListRx, XFER_TIMEOUT, EP_RX);
			Xfer_Wait(xferWaitListTx, XFER_TIMEOUT, EP_TX);

			Xfer_Recycle(&xferWaitListTx, &xferAvailListTx);
			Xfer_Recycle(&xferWaitListRx, &xferAvailListRx);
		}
	}
	while(xferWaitListTx && xferWaitListRx);

Done:
	// Close the device handle
	// If handle is invalid (NULL), has no effect
	UsbK_Close(handle);

	// Free the device list
	// If deviceList is invalid (NULL), has no effect
	LstK_Free(&deviceList);

	return g_ErrorCode;
}
