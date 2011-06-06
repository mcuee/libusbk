#include "examples.h"

// Example configuration:
#define EP_WRITE			0x01
#define EP_READ				0x81
#define EP_PACKET_SIZE		32
#define TRANSFER_LENGTH		(EP_PACKET_SIZE*16)
#define TRANSFER_TIMEOUT	1000
#define MAX_OUTSTANDING_IO	3
#define MAX_TEST_LOOPS		12

static DWORD LoopCounter = 0;
static DWORD ErrorCode = ERROR_SUCCESS;

/*
This element represents a user defined transfer context based on a doubly linked list.
*/
typedef struct _MY_XFER_EL
{
	POVERLAPPED_K Ovl;
	UCHAR Buffer[TRANSFER_LENGTH];
	DWORD Length;
	DWORD ErrorCode;

	struct _MY_XFER_EL* prev;
	struct _MY_XFER_EL* next;
} MY_XFER_EL;

/*
OnDataRequested is called by Xfer_Submit when data needs to
be sent to an outging pipe.
*/
VOID OnDataRequested(MY_XFER_EL* xfer, UCHAR PipeID)
{
	xfer->Length = TRANSFER_LENGTH;
	printf("[Tx-#%u] %u bytes to pipe-id=%02Xh\n", LoopCounter, xfer->Length, PipeID);
}

/*
OnDataReceived is called by Xfer_Wait when arrives from
an incomming pipe.
*/
VOID OnDataReceived(MY_XFER_EL* xfer, UCHAR PipeID)
{
	printf("[Rx-#%u] %u bytes from pipe-id=%02Xh\n", LoopCounter, xfer->Length, PipeID);
}

/*
Moves the head MY_XFER_EL element in FromList to the tail of
ToList and starts an asynchronous transfer operation.
*/
BOOL Xfer_Submit(LIBUSBK_INTERFACE_HANDLE handle,
                 MY_XFER_EL** FromList,
                 MY_XFER_EL** ToList,
                 UCHAR PipeID)
{
	MY_XFER_EL* move = *FromList;

	DL_DELETE(*FromList, move);
	DL_APPEND(*ToList, move);
	move->Ovl =  OvlK_Acquire(NULL);

	if (USB_ENDPOINT_DIRECTION_IN(PipeID))
	{
		move->Length = TRANSFER_LENGTH;
		UsbK_ReadPipe(handle, PipeID, move->Buffer, move->Length, NULL, move->Ovl);
	}
	else
	{
		OnDataRequested(move, PipeID);
		UsbK_WritePipe(handle, PipeID, move->Buffer, move->Length, NULL, move->Ovl);
	}
	if ((move->ErrorCode = GetLastError()) != ERROR_IO_PENDING)
	{
		ErrorCode = move->ErrorCode;
		printf("Failed transfer on pipe-id %02Xh.  Win32Error=%u (0x%08X)\n",
		       PipeID, ErrorCode, ErrorCode);
		LoopCounter = MAX_TEST_LOOPS;

		return FALSE;
	}

	return TRUE;
}

/*
Waits for the first element in PendingList (the oldest) to complete. If
the operation completes successfully and a read pipe-id is specified,
calls OnDataReceived.
*/
BOOL Xfer_Wait(MY_XFER_EL* PendingList, ULONG Timeout, UCHAR PipeID)
{
	if (PendingList->ErrorCode != ERROR_IO_PENDING)
		Timeout = 0;

	if (!OvlK_WaitAndRelease(PendingList->Ovl, Timeout, &PendingList->Length))
	{
		if (PendingList->ErrorCode == ERROR_IO_PENDING)
		{
			PendingList->ErrorCode = GetLastError();
			printf("Failed getting i/o results.  Win32Error=%u (0x%08X)\n", PendingList->ErrorCode, PendingList->ErrorCode);
		}
		LoopCounter = MAX_TEST_LOOPS;
		return FALSE;
	}
	if (USB_ENDPOINT_DIRECTION_IN(PipeID))
	{
		OnDataReceived(PendingList, PipeID);
	}
	return TRUE;
}

/*
Moves the head MY_XFER_EL element in FromList to the tail of
ToList and starts an asynchronous transfer operation.
*/
VOID Xfer_Recycle(MY_XFER_EL** FromList, MY_XFER_EL** ToList)
{
	MY_XFER_EL* move = *FromList;
	DL_DELETE(*FromList, move);
	if (LoopCounter < MAX_TEST_LOOPS)
		DL_APPEND(*ToList, move);
}

DWORD __cdecl main(int argc, char* argv[])
{
	PKUSB_DEV_LIST deviceList = NULL;
	PKUSB_DEV_INFO deviceInfo = NULL;
	LIBUSBK_INTERFACE_HANDLE handle = NULL;
	int i;

	MY_XFER_EL xfersTx[MAX_OUTSTANDING_IO];
	MY_XFER_EL xfersRx[MAX_OUTSTANDING_IO];

	MY_XFER_EL* xferListTx = NULL;
	MY_XFER_EL* xferListRx = NULL;

	MY_XFER_EL* xferListWaitTx = NULL;
	MY_XFER_EL* xferListWaitRx = NULL;

	// init vars
	memset(xfersTx, 0, sizeof(xfersTx));
	memset(xfersRx, 0, sizeof(xfersRx));
	for (i = 0; i < MAX_OUTSTANDING_IO; i++)
	{
		DL_APPEND(xferListTx, &xfersTx[i]);
		DL_APPEND(xferListRx, &xfersRx[i]);
	}

	// Find the test device using arguments supplied on the command line.
	// (default is: vid=04D8 pid=FA2E)
	if (!Examples_GetTestDevice(&deviceList, &deviceInfo, argc, argv))
		return GetLastError();

	// Open the device
	if (!UsbK_Open(deviceInfo, &handle))
	{
		ErrorCode = GetLastError();
		printf("Open device failed. Win32Error=%u (0x%08X)\n", ErrorCode, ErrorCode);
		goto Done;
	}
	printf("Device opened successfully!\n");

	do
	{
		while (xferListRx && xferListTx && LoopCounter < MAX_TEST_LOOPS)
		{
			LoopCounter++;
			Xfer_Submit(handle, &xferListRx, &xferListWaitRx, EP_READ);
			Xfer_Submit(handle, &xferListTx, &xferListWaitTx, EP_WRITE);
		}

		if (xferListWaitTx && xferListWaitRx)
		{
			Xfer_Wait(xferListWaitRx, TRANSFER_TIMEOUT, EP_READ);
			Xfer_Wait(xferListWaitTx, TRANSFER_TIMEOUT, EP_WRITE);

			Xfer_Recycle(&xferListWaitTx, &xferListTx);
			Xfer_Recycle(&xferListWaitRx, &xferListRx);
		}
	}
	while(xferListWaitTx && xferListWaitRx);

Done:

	// Close the device handle
	// If handle is invalid (NULL), has no effect
	UsbK_Close(handle);

	// Free the device list
	// If deviceList is invalid (NULL), has no effect
	LstK_Free(&deviceList);

	return ErrorCode;
}
