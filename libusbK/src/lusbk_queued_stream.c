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
#include "lusbk_linked_list.h"
#include "lusbk_stack_collection.h"
#include <process.h>

// Default loggging level
extern ULONG DebugLevel;

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#define Stm_Alloc(mStream,mSize) HeapAlloc((mStream)->Heap,HEAP_ZERO_MEMORY,mSize)

#define mStm_QueueTransferList(mStreamHandle, mXferSubmitList, mXferEL, mXferTempEL, mErrorJump)do { \
		if (mXferSubmitList && mStreamHandle->Thread.State == KSTM_THREADSTATE_STARTED)  						\
		{																										\
			PoolHandle_Inc_StmK(mStreamHandle);  																\
			if (!QueueUserAPC(Stm_TransferListAPC, mStreamHandle->Thread.Handle, (ULONG_PTR)mXferSubmitList))	\
			{																									\
				PoolHandle_Dec_StmK(mStreamHandle);  															\
				USBERRN("QueueUserAPC failed. ErrorCode=%08Xh", GetLastError()); 								\
				goto mErrorJump;  																				\
			}																									\
			IncLock(mStreamHandle->APCTransferQueueCount);   													\
		}																										\
		else 																									\
		{																										\
			/* Stream is not started; add items added to the Queued list. */  									\
			DL_FOREACH_SAFE(mXferSubmitList, mXferEL, mXferTempEL)   											\
			{																									\
				DL_DELETE(mXferSubmitList, mXferEL); 															\
				DL_APPEND(mStreamHandle->List.Queued, mXferEL);  												\
			}																									\
		}																										\
	}																											\
	while(0)

#define mStm_SpinLockForTransferRequest(mStreamHandle, mErrorJump)do { \
		if (mStreamHandle->SemReady)   																				\
		{  																											\
			if  (WaitForSingleObject(mStreamHandle->SemReady, mStreamHandle->WaitTimeout) != WAIT_OBJECT_0)			\
			{  																										\
				USBWRNN("[WaitTimeout] No more pending transfer slots. PipeID=%02Xh", mStreamHandle->Info->PipeID);	\
				SetLastError(ERROR_NO_MORE_ITEMS); 																	\
				goto mErrorJump;																					\
			}  																										\
		}  																											\
		else if (mStreamHandle->List.Finished == NULL) 																\
		{  																											\
			USBWRNN("No more pending transfer slots. PipeID=%02Xh", mStreamHandle->Info->PipeID);  					\
			SetLastError(ERROR_NO_MORE_ITEMS); 																		\
			goto mErrorJump;																						\
		}  																											\
		\
		mSpin_Acquire(&mStreamHandle->List.FinishedLock);  															\
		if (mStreamHandle->List.Finished == NULL)  																	\
		{  																											\
			mSpin_Release(&mStreamHandle->List.FinishedLock);  														\
			\
			if (mStreamHandle->SemReady) ReleaseSemaphore(mStreamHandle->SemReady, 1, NULL);   						\
			USBWRNN("No more pending transfer slots. PipeID=%02Xh", mStreamHandle->Info->PipeID);  					\
			SetLastError(ERROR_NO_MORE_ITEMS); 																		\
			goto mErrorJump;																						\
		}  																											\
	}  																												\
	while(0)

#define mStm_CheckPartialTransfer(mStreamHandle, mLength, mTransferredLengthRef, mTransferLength, mErrorJump)do { \
		if (mLength > 0) 																									\
		{																													\
			if (((mStreamHandle)->Flags & KSTM_FLAG_NO_PARTIAL_XFERS) || !(mTransferredLengthRef))  						\
			{																												\
				USBERRN("No more pending transfer slots. PipeID=%02Xh", (mStreamHandle)->Info->PipeID);  					\
				SetLastError(ERROR_NO_MORE_ITEMS);   																		\
				goto mErrorJump; 																							\
			}																												\
			else 																											\
			{																												\
				USBWRNN("[PartialTransfer] PipeID=%02Xh Transferred=%u", (mStreamHandle)->Info->PipeID, mTransferLength);	\
			}																												\
		}																													\
	}																														\
	while(0)

static BOOL Stm_SynchronizeFinishedList(PKSTM_HANDLE_INTERNAL handle, BOOL requireLock)
{
	PKSTM_XFER_LINK_EL xferEL, xferTempEL;

	if (requireLock)
	{
		// SPIN-LOCKED : List.FinishedLock
		mSpin_Acquire(&handle->List.FinishedLock);
	}

	DL_FOREACH_SAFE(handle->List.FinishedTemp, xferEL, xferTempEL)
	{
		DL_DELETE(handle->List.FinishedTemp, xferEL);
		DL_APPEND(handle->List.Finished, xferEL);
	}

	// SPIN-LOCK-RELEASE : List.FinishedLock
	if (requireLock) mSpin_Release(&handle->List.FinishedLock);
	return TRUE;

}

static void CALLBACK Stm_TransferListAPC(__in ULONG_PTR dwParam)
{
	PKSTM_XFER_LINK_EL xferListEL = (PKSTM_XFER_LINK_EL)dwParam;
	PKSTM_XFER_LINK_EL xferEL, xferTempEL;
	PKSTM_HANDLE_INTERNAL handle = (PKSTM_HANDLE_INTERNAL)xferListEL->Xfer->StreamHandle;

	DL_FOREACH_SAFE(xferListEL, xferEL, xferTempEL)
	{
		DL_DELETE(xferListEL, xferEL);
		DL_APPEND(handle->List.Queued, xferEL);
	}
	DecLock(handle->APCTransferQueueCount);
	PoolHandle_Dec_StmK(handle);
}

static void CALLBACK Stm_StopAPC(__in ULONG_PTR dwParam)
{
	PKSTM_HANDLE_INTERNAL handle = (PKSTM_HANDLE_INTERNAL)dwParam;

	UNREFERENCED_PARAMETER(handle);
}

static INT KUSB_API Stm_SubmitRead(
    __in PKSTM_INFO StreamInfo,
    __in PKSTM_XFER_CONTEXT XferContext,
    __in INT XferContextIndex,
    __in LPOVERLAPPED Overlapped)
{
	UNREFERENCED_PARAMETER(XferContextIndex);

	StreamInfo->DriverAPI.ReadPipe(StreamInfo->UsbHandle, StreamInfo->PipeID, XferContext->Buffer, XferContext->BufferSize, NULL, Overlapped);
	return (INT)GetLastError();
}

static INT KUSB_API Stm_SubmitWrite(
    __in PKSTM_INFO StreamInfo,
    __in PKSTM_XFER_CONTEXT XferContext,
    __in INT XferContextIndex,
    __in LPOVERLAPPED Overlapped)
{
	UNREFERENCED_PARAMETER(XferContextIndex);

	StreamInfo->DriverAPI.WritePipe(StreamInfo->UsbHandle, StreamInfo->PipeID, XferContext->Buffer, XferContext->TransferLength, NULL, Overlapped);
	return (INT)GetLastError();
}

typedef struct _KSTM_THREAD_INTERNAL
{
	PKSTM_HANDLE_INTERNAL handle;

	HANDLE* ovlEvents;
	PKSTM_OVERLAPPED_EL ovlList;
	PKSTM_OVERLAPPED_EL ovlNext;

	PKSTM_XFER_LINK_EL pendingList;
	PKSTM_XFER_LINK_EL xferNext;
	PKSTM_XFER_LINK_EL xferTemp;

	DWORD errorCode;
	BOOL success;
	BOOL NoWaiting;

} KSTM_THREAD_INTERNAL, *PKSTM_THREAD_INTERNAL;

static BOOL Stm_Thread_Alloc_Ovl(PKSTM_THREAD_INTERNAL stm)
{
	LONG pos;

	stm->ovlEvents = Stm_Alloc(stm->handle, sizeof(HANDLE) * stm->handle->Info->MaxPendingIO);
	if (!stm->ovlEvents)
	{
		stm->errorCode = GetLastError();
		return FALSE;
	}

	for (pos = 0; pos < stm->handle->Info->MaxPendingIO; pos++)
	{
		stm->ovlEvents[pos] = CreateEventA(NULL, TRUE, TRUE, NULL);
		stm->ovlNext = Stm_Alloc(stm->handle, sizeof(*stm->ovlNext));
		if (!stm->ovlNext)
		{
			stm->errorCode = GetLastError();
			return FALSE;
		}

		stm->ovlNext->Overlapped.hEvent = stm->ovlEvents[pos];
		DL_APPEND(stm->ovlList, stm->ovlNext);
	}

	return TRUE;
}

static BOOL Stm_Thread_Free_Ovl(PKSTM_THREAD_INTERNAL stm)
{
	LONG pos;
	PKSTM_OVERLAPPED_EL ovlTemp;

	if (!stm->ovlEvents) return TRUE;

	for (pos = 0; pos < stm->handle->Info->MaxPendingIO; pos++)
		if (stm->ovlEvents[pos]) CloseHandle(stm->ovlEvents[pos]);

	stm->ovlEvents = NULL;

	DL_FOREACH_SAFE(stm->ovlList, stm->ovlNext, ovlTemp)
	{
		DL_DELETE(stm->ovlList, stm->ovlNext);
		HeapFree(stm->handle->Heap, 0, stm->ovlNext);
	}

	return TRUE;
}

typedef enum _KSTM_THREAD_RESULT
{
    KSTM_THREAD_RESULT_ITEM_PROCESSED,
    KSTM_THREAD_RESULT_OVERLAPPED_EMPTY,
    KSTM_THREAD_RESULT_QUEUE_EMPTY,
    KSTM_THREAD_RESULT_SUMBIT_ERROR,

} KSTM_THREAD_RESULT;

static BOOL Stm_Thread_ProcessQueued(PKSTM_THREAD_INTERNAL stm)
{
	// No more pending IO slots
	if (!stm->ovlList)
		return KSTM_THREAD_RESULT_OVERLAPPED_EMPTY;

	/* Nothing queued.
	   - Read pipes need the stream thread to submit more in 'ProcessPending'
	   - Write pipes need the user to submit more requests.
	*/
	if (!stm->handle->List.Queued)
		return KSTM_THREAD_RESULT_QUEUE_EMPTY;

	// Get the next xfer item and link it with the next overlapped
	stm->xferNext	= stm->handle->List.Queued;
	stm->ovlNext	= stm->ovlList;
	DL_DELETE(stm->handle->List.Queued, stm->xferNext);
	DL_DELETE(stm->ovlList, stm->ovlNext);
	stm->xferNext->Xfer->Overlapped = &stm->ovlNext->Overlapped;

	// Reset the overlapped event, buffer pointers and sizes each time.
	ResetEvent(stm->ovlNext->Overlapped.hEvent);
	stm->xferNext->Xfer->Public.Buffer		= stm->xferNext->Xfer->Buffer;
	stm->xferNext->Xfer->Public.BufferSize	= stm->xferNext->Xfer->BufferSize;

	// For Read pipes, reset the TransferLength to the buffer size.
	if (USB_ENDPOINT_DIRECTION_IN(stm->handle->Info->PipeID))
		stm->xferNext->Xfer->Public.TransferLength	= stm->xferNext->Xfer->BufferSize;

	// Submit
	stm->errorCode = stm->handle->UserCB->Submit(stm->handle->Info, &stm->xferNext->Xfer->Public, stm->xferNext->Xfer->Index, stm->xferNext->Xfer->Overlapped);

	// Accept ERROR_IO_PENDING or ERROR_SUCCESS equally.
	if (stm->errorCode != ERROR_IO_PENDING  && stm->errorCode != ERROR_SUCCESS)
	{
		ErrorNoSetAction(!stm->success, NOP_FUNCTION, "Submit failed.");

		SetEvent(stm->ovlNext->Overlapped.hEvent);
		DL_PREPEND(stm->ovlList, stm->ovlNext);
		DL_PREPEND(stm->handle->List.Queued, stm->xferNext);
		return KSTM_THREAD_RESULT_SUMBIT_ERROR;
	}

	// Increment the IO count and add this to the pending list.
	IncLock(stm->handle->PendingIO);
	DL_APPEND(stm->pendingList, stm->xferNext);
	return KSTM_THREAD_RESULT_ITEM_PROCESSED;
}

static BOOL Stm_Thread_ProcessPending(PKSTM_THREAD_INTERNAL stm, DWORD timeoutOverride)
{
	DWORD timeout;
	DWORD waitResult;
	KSTM_COMPLETE_RESULT completeResult;

	if (!stm->pendingList)
	{
		stm->errorCode = ERROR_NO_MORE_ITEMS;
		return FALSE;
	}

	stm->xferNext = stm->pendingList;
	stm->ovlNext = (PKSTM_OVERLAPPED_EL)stm->xferNext->Xfer->Overlapped;

	timeout = timeoutOverride;

	stm->success	= TRUE;

	if (!stm->NoWaiting)
	{
		while (stm->success)
		{
			waitResult = WaitForSingleObjectEx(stm->ovlNext->Overlapped.hEvent, timeout, TRUE);
			if (waitResult == STATUS_USER_APC)
			{
				// User APC; we need no fallback to the thread proc so it can take action.
				stm->success	= FALSE;
				stm->errorCode  = waitResult;
				return FALSE;
			}
			if (waitResult == WAIT_OBJECT_0)
			{
				stm->errorCode = ERROR_SUCCESS;
				break;
			}
			else if (waitResult == WAIT_TIMEOUT)
			{
				stm->success	= FALSE;
				stm->errorCode	= ERROR_IO_INCOMPLETE;
				break;
			}
		}
		if (!stm->success) return FALSE;
	}

	DL_DELETE(stm->pendingList, stm->xferNext);
	DL_DELETE(stm->ovlList, stm->ovlNext);

	stm->success = GetOverlappedResult(stm->handle->Info->DeviceHandle, &stm->ovlNext->Overlapped, (LPDWORD)&stm->xferNext->Xfer->Public.TransferLength, FALSE);
	if (!stm->success)
	{
		stm->errorCode = GetLastError();
		if (stm->errorCode == ERROR_OPERATION_ABORTED || stm->errorCode == ERROR_CANCELLED)
		{
			MsgErrorNoSetAction(!stm->success, NOP_FUNCTION, "I/O request cancelled.");
			stm->success = TRUE;
		}
		else
		{
			ErrorNoSetAction(!stm->success, NOP_FUNCTION, "GetOverlappedResult failed.");
			SetEvent(stm->ovlNext->Overlapped.hEvent);
		}
	}
	else
	{
		stm->errorCode = ERROR_SUCCESS;
	}
	DL_APPEND(stm->ovlList, stm->ovlNext);

	completeResult = KSTM_COMPLETE_RESULT_VALID;

	if (!stm->success && stm->handle->UserCB->Error)
	{
		stm->errorCode = stm->handle->UserCB->Error(stm->handle->Info, &stm->xferNext->Xfer->Public, stm->xferNext->Xfer->Index, stm->errorCode);
		stm->success = (stm->errorCode == ERROR_SUCCESS);
	}

	if (stm->handle->UserCB->BeforeComplete)
	{
		completeResult = stm->handle->UserCB->BeforeComplete(stm->handle->Info, &stm->xferNext->Xfer->Public, stm->xferNext->Xfer->Index, (PINT)&stm->errorCode);
		if (completeResult == KSTM_COMPLETE_RESULT_INVALID)
			stm->success = FALSE;
	}

	if (completeResult == KSTM_COMPLETE_RESULT_INVALID)
	{
		// This can only happen if using a custom BeforeComplete callback; user code has instructed to place this xfer item back in the queue
		// for re-processing.
		DL_APPEND(stm->handle->List.Queued, stm->xferNext);
		DecLock(stm->handle->PendingIO);
	}
	else
	{
		// Place the xfer item in the finished list.

		// SPIN-LOCKED-TRY ///////////////////////////////////////////
		if (mSpin_Try_Acquire(&(stm->handle->List.FinishedLock)))
		{
			Stm_SynchronizeFinishedList(stm->handle, FALSE);
			DL_APPEND(stm->handle->List.Finished, stm->xferNext);

			// SPIN-LOCK-RELEASE : List.FinishedLock
			mSpin_Release(&(stm->handle->List.FinishedLock));
		}
		else
		{
			DL_APPEND(stm->handle->List.FinishedTemp, stm->xferNext);
		}
		//////////////////////////////////////////////////////////////

		DecLock(stm->handle->PendingIO);

		if (stm->handle->SemReady)
			ReleaseSemaphore(stm->handle->SemReady, 1, NULL);

		if (stm->handle->UserCB->Complete)
		{
			stm->errorCode = stm->handle->UserCB->Complete(stm->handle->Info, &stm->xferNext->Xfer->Public, stm->xferNext->Xfer->Index, stm->errorCode);
			stm->success = (stm->errorCode == ERROR_SUCCESS);
		}
	}

	return stm->success;
}

static BOOL Stm_StopInternal(
    __in PKSTM_HANDLE_INTERNAL handle)
{
	BOOL success;

	success = (InterlockedCompareExchange(&handle->Thread.State, KSTM_THREADSTATE_STOPPING, KSTM_THREADSTATE_STARTED) == KSTM_THREADSTATE_STARTED) ? TRUE : FALSE;
	ErrorSet(!success, Error, ERROR_ACCESS_DENIED, "stream already stopped");

	ErrorNoSetAction(!QueueUserAPC(Stm_StopAPC, handle->Thread.Handle, (ULONG_PTR)handle), goto Error, "QueueUserAPC failed.");

	Sleep(0);
	while (InterlockedCompareExchange(&handle->Thread.State, KSTM_THREADSTATE_STOPPED, KSTM_THREADSTATE_STOPPED) != KSTM_THREADSTATE_STOPPED)
		if (!SwitchToThread()) Sleep(0);

	return TRUE;
Error:
	return FALSE;
}

static unsigned _stdcall Stm_ThreadProc(PKSTM_HANDLE_INTERNAL handle)
{
	DWORD exitCode = ERROR_SUCCESS;
	KSTM_THREAD_INTERNAL stm_thread_internal;
	PKSTM_THREAD_INTERNAL stm;
	//BOOL isQueueEmpty, isPendingIoEmpty;
	DWORD maxWaitMS;
	KSTM_THREAD_RESULT threadResult;

	stm = &stm_thread_internal;

	memset(stm, 0, sizeof(*stm));
	stm->handle = handle;

	if (!Stm_Thread_Alloc_Ovl(stm))
	{
		exitCode = stm->errorCode;
		goto Done;
	}

	if (handle->UserCB->Started)
	{
		// Execute the user callback for all of the xfer items.
		int listIndex;
		PKSTM_XFER_INTERNAL xferItem;

		for (listIndex = 0; listIndex < handle->XferItemsCount; listIndex++)
		{
			xferItem = &handle->XferItems[listIndex];
			handle->UserCB->Started(handle->Info, &xferItem->Public, xferItem->Index);
		}
	}

	// Notify the Start function that we are ready.
	InterlockedExchange(&handle->Thread.State, KSTM_THREADSTATE_STARTED);
	SwitchToThread();

	while(handle->Thread.State == KSTM_THREADSTATE_STARTED && exitCode == ERROR_SUCCESS)
	{
		if (handle->List.FinishedTemp) Stm_SynchronizeFinishedList(handle, FALSE);

		// Continue processing the list while there are xfer items queued.
		while ((threadResult = Stm_Thread_ProcessQueued(stm)) == KSTM_THREAD_RESULT_ITEM_PROCESSED);

		if (threadResult == KSTM_THREAD_RESULT_OVERLAPPED_EMPTY || threadResult == KSTM_THREAD_RESULT_QUEUE_EMPTY)
		{
			// - No more pending IO slots; we need to process pending.
			// - Nothing left in the queue.
			//   - For IN pipes, all of the xfer items are sitting in the finished list. User needs to call StmK_Read before we can proceed.
			//   - For OUT pipes, the user has not given the stream any more data to send. User needs to call StmK_Write before we can proceed.

			if (!Stm_Thread_ProcessPending(stm, (handle->List.FinishedTemp) ? 1 : INFINITE))
			{
				// User APC.  (Read, Write, Stop, etc)
				if (stm->errorCode == STATUS_USER_APC) continue;

				// A timout accured; (should not be possible because we wait INFINITE)
				if (stm->errorCode == ERROR_IO_INCOMPLETE) continue;

				if (stm->errorCode == ERROR_NO_MORE_ITEMS)
				{
					// No more pending *transfer* slots.
					USBDEVN("KSTM_THREAD_RESULT_OVERLAPPED_EMPTY");
					SleepEx(INFINITE, TRUE);
					continue;
				}

				USBERRN("Un-handled stream error; aborting.. ErrorCode=%08Xh", stm->errorCode);
				exitCode = stm->errorCode;
			}

		}
		else if (threadResult == KSTM_THREAD_RESULT_SUMBIT_ERROR)
		{
			USBDEVN("KSTM_THREAD_RESULT_SUMBIT_ERROR");
			// An Error occured or was returned by the user submit callback.
			exitCode = stm->errorCode;
			goto Done;
		}
		else
		{
			USBERRN("Fatal Error: UNHANDLED KSTM_THREAD_RESULT");
			exitCode = ERROR_FUNCTION_FAILED;
			goto Done;
		}
	}

Done:
	// Set the stopping state.
	InterlockedExchange(&handle->Thread.State, KSTM_THREADSTATE_STOPPING);


	// TimeoutCancelMS is set be the Stop() function.
	maxWaitMS = stm->handle->TimeoutCancelMS;
	stm->handle->TimeoutCancelMS = 0;

	while(stm->handle->PendingIO > 0)
	{
		SleepEx(0, TRUE);
		// Complete or cancel all of the pending IO.
		if (!Stm_Thread_ProcessPending(stm, maxWaitMS))
		{
			if (stm->errorCode == ERROR_IO_INCOMPLETE)
			{
				// Disable waiting in Stm_Thread_ProcessPending
				if (!stm->NoWaiting) stm->NoWaiting = TRUE;

				// A timout occured; cancel *all* IO on the stream thread.
				if (!CancelIo(stm->handle->Info->DeviceHandle))
				{
					USBERRN("CancelIo Failed. ErrorCode=%08Xh", GetLastError());
				}
			}
		}
	}

	if (handle->UserCB->Stopped)
	{
		int listIndex;
		PKSTM_XFER_INTERNAL xferItem;

		for (listIndex = 0; listIndex < handle->XferItemsCount; listIndex++)
		{
			xferItem = &handle->XferItems[listIndex];
			handle->UserCB->Stopped(handle->Info, &xferItem->Public, xferItem->Index);
		}
	}

	Stm_Thread_Free_Ovl(stm);

	InterlockedExchange(&handle->Thread.State, KSTM_THREADSTATE_STOPPED);
	_endthreadex(exitCode);
	return exitCode;
}

static BOOL Stm_Create_Thread(PKSTM_HANDLE_INTERNAL handle)
{
	handle->Thread.Handle = (HANDLE)_beginthreadex( NULL, 0, &Stm_ThreadProc, handle, CREATE_SUSPENDED, &handle->Thread.Id);
	ErrorNoSetAction(!IsHandleValid(handle->Thread.Handle), return FALSE, "_beginthreadex failed.");
	return TRUE;
}

static void KUSB_API Stm_Cleanup(PKSTM_HANDLE_INTERNAL handle)
{
	PoolHandle_Dead_StmK(handle);
	if (handle->Info)
	{
		if(handle->Info->UsbHandle)
			PoolHandle_Dec_UsbK((PKUSB_HANDLE_INTERNAL)handle->Info->UsbHandle);
	}
	if (handle->SemReady) CloseHandle(handle->SemReady);

	if (handle->Heap)
	{
		HeapDestroy(handle->Heap);
		handle->Heap = NULL;
	}
}

KUSB_EXP BOOL KUSB_API StmK_Init(
    _out KSTM_HANDLE* StreamHandle,
    _in KUSB_HANDLE UsbHandle,
    _in UCHAR PipeID,
    _in INT MaxTransferSize,
    _in INT MaxPendingTransfers,
    _in INT MaxPendingIO,
    _inopt PKSTM_CALLBACK Callbacks,
    _inopt KSTM_FLAG Flags)
{
	DWORD minHeapSize = 0;
	PKSTM_HANDLE_INTERNAL handle;
	PKUSB_HANDLE_INTERNAL usbHandle;
	LONG xferIndex;
	USB_ENDPOINT_DESCRIPTOR epDescriptor;
	BOOL success;
	PUCHAR bufferMemory;

	usbHandle = (PKUSB_HANDLE_INTERNAL)UsbHandle;

	ErrorParamAction(!(PipeID & 0x8F), "PipeID", return FALSE);
	ErrorParamAction(MaxPendingIO < 1, "MaxPendingIO < 1", return FALSE);
	ErrorParamAction(MaxPendingTransfers < MaxPendingIO, "MaxPendingTransfers < MaxPendingIO", return FALSE);

	success = UsbStack_QuerySelectedEndpoint(UsbHandle, PipeID, FALSE, &epDescriptor);
	ErrorNoSetAction(!success, return FALSE, "PipeID not found on selected interface");

	ErrorParamAction(epDescriptor.wMaxPacketSize == 0, "endpoint wMaxPacketSize = 0", return FALSE);
	ErrorParamAction((MaxTransferSize % epDescriptor.wMaxPacketSize) > 0, "MaxTransferSize not an interval of wMaxPacketSize", return FALSE);

	handle = PoolHandle_Acquire_StmK(Stm_Cleanup);
	ErrorNoSetAction(!IsHandleValid(handle), return FALSE, "->PoolHandle_Acquire_StmK");

	minHeapSize = 0;

	handle->Heap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE | HEAP_NO_SERIALIZE, minHeapSize, 0);
	ErrorNoSet(!handle->Heap, Error, "HeapCreate failed.");

	handle->Info = Stm_Alloc(handle, sizeof(*handle->Info));
	ErrorMemory(!handle->Info, Error);

	handle->UserCB = Stm_Alloc(handle, sizeof(*handle->UserCB));
	ErrorMemory(!handle->UserCB, Error);

	handle->XferItems = Stm_Alloc(handle, sizeof(KSTM_XFER_INTERNAL) * MaxPendingTransfers);
	ErrorMemory(!handle->XferItems, Error);

	bufferMemory = Stm_Alloc(handle, MaxTransferSize * MaxPendingTransfers);
	ErrorMemory(!bufferMemory, Error);

	if (Flags & KSTM_FLAG_USE_TIMEOUT)
	{
		handle->SemReady = CreateSemaphoreA(NULL, USB_ENDPOINT_DIRECTION_IN(PipeID) ? 0 : MaxPendingTransfers, MaxPendingTransfers, NULL);
		ErrorNoSetAction(!handle->SemReady, goto Error, "CreateSemaphoreA failed.");

		handle->WaitTimeout = (Flags & KSTM_FLAG_TIMEOUT_MASK) == KSTM_FLAG_TIMEOUT_MASK ? INFINITE : Flags & KSTM_FLAG_TIMEOUT_MASK;
	}
	else
		handle->SemReady = NULL;

	handle->Flags = Flags;

	ErrorSet(!PoolHandle_Inc_UsbK(usbHandle), Error, ERROR_RESOURCE_NOT_AVAILABLE, "->PoolHandle_Inc_UsbK");

	memcpy(&handle->Info->DriverAPI, usbHandle->Device->DriverAPI, sizeof(handle->Info->DriverAPI));
	memcpy(&handle->Info->EndpointDescriptor, &epDescriptor, sizeof(handle->Info->EndpointDescriptor));
	handle->Info->UsbHandle				= UsbHandle;
	handle->Info->DeviceHandle			= usbHandle->Device->MasterDeviceHandle;
	handle->Info->MaxPendingIO			= MaxPendingIO;
	handle->Info->MaxPendingTransfers	= MaxPendingTransfers;
	handle->Info->MaxTransferSize		= MaxTransferSize;
	handle->Info->PipeID				= PipeID;
	handle->Info->StreamHandle          = handle;
	handle->TimeoutCancelMS				= 1;

	if (Callbacks)
		memcpy(handle->UserCB, Callbacks, sizeof(*handle->UserCB));

	if (!handle->UserCB->Submit)
	{
		if (USB_ENDPOINT_DIRECTION_IN(handle->Info->PipeID))
			handle->UserCB->Submit = &Stm_SubmitRead;
		else
			handle->UserCB->Submit = &Stm_SubmitWrite;
	}

	for (xferIndex = 0; xferIndex < MaxPendingTransfers; xferIndex++)
	{
		PKSTM_XFER_INTERNAL xfer;

		xfer = &handle->XferItems[xferIndex];

		xfer->Buffer			= &bufferMemory[xferIndex * MaxTransferSize];
		xfer->Link.Xfer			= xfer;
		xfer->Index				= xferIndex;
		xfer->StreamHandle		= handle;
		xfer->BufferSize		= MaxTransferSize;
		xfer->Public.Buffer		= xfer->Buffer;
		xfer->Public.BufferSize = xfer->BufferSize;

		if (USB_ENDPOINT_DIRECTION_IN(handle->Info->PipeID))
		{
			DL_APPEND(handle->List.Queued, &xfer->Link);
		}
		else
		{
			DL_APPEND(handle->List.Finished, &xfer->Link);
		}
	}

	*StreamHandle = (KSTM_HANDLE)handle;
	PoolHandle_Live_StmK(handle);
	return TRUE;
Error:
	if (handle)
		PoolHandle_Dec_StmK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API StmK_Free(
    _in KSTM_HANDLE StreamHandle)
{
	PKSTM_HANDLE_INTERNAL handle;

	Pub_To_Priv_StmK(StreamHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_StmK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_StmK");

	Stm_StopInternal(handle);

	PoolHandle_Dec_StmK(handle);
	PoolHandle_Dec_StmK(handle);
	return TRUE;
}

KUSB_EXP BOOL KUSB_API StmK_Start(
    _in KSTM_HANDLE StreamHandle)
{
	PKSTM_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_StmK(StreamHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_StmK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_StmK");

	success = (InterlockedCompareExchange(&handle->Thread.State, KSTM_THREADSTATE_STARTING, KSTM_THREADSTATE_STOPPED) == KSTM_THREADSTATE_STOPPED) ? TRUE : FALSE;
	ErrorSet(!success, Error, ERROR_ACCESS_DENIED, "stream already started");

	success = Stm_Create_Thread(handle);
	if (!success) InterlockedExchange(&handle->Thread.State, KSTM_THREADSTATE_STOPPED);
	ErrorNoSet(!success, Error, "->Stm_Create_Thread");

	success = ResumeThread(handle->Thread.Handle) != 0xFFFFFFFF;
	if (!success)
	{
		TerminateThread(handle->Thread.Handle, GetLastError());
		InterlockedExchange(&handle->Thread.State, KSTM_THREADSTATE_STOPPED);
		ErrorNoSet(!success, Error, "->ResumeThread");
	}

	Sleep(0);
	while (InterlockedCompareExchange(&handle->Thread.State, KSTM_THREADSTATE_STARTED, KSTM_THREADSTATE_STARTED) != KSTM_THREADSTATE_STARTED)
		if (!SwitchToThread()) Sleep(0);

	USBMSGN("Stream Started.  ThreadID=%08Xh", handle->Thread.Id);
	PoolHandle_Dec_StmK(handle);
	return TRUE;
Error:
	PoolHandle_Dec_StmK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API StmK_Stop(
    _in KSTM_HANDLE StreamHandle,
    _in INT TimeoutCancelMS)
{
	PKSTM_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_StmK(StreamHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_StmK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_StmK");

	handle->TimeoutCancelMS = TimeoutCancelMS;

	success = Stm_StopInternal(handle);
	ErrorSet(!success, Error, ERROR_ACCESS_DENIED, "stream already stopped");

	USBMSGN("Stream Stopped.  ThreadID=%08Xh", handle->Thread.Id);
	PoolHandle_Dec_StmK(handle);
	return TRUE;
Error:
	PoolHandle_Dec_StmK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API StmK_Read(
    _in KSTM_HANDLE StreamHandle,
    _in PUCHAR Buffer,
    _in INT Offset,
    _in INT Length,
    _out PUINT TransferredLength)
{
	PKSTM_HANDLE_INTERNAL handle = NULL;
	PKSTM_XFER_LINK_EL xferEL, xferTempEL;
	PKSTM_XFER_LINK_EL xferSubmitList = NULL;

	UINT transferLength = 0;
	UINT stageSize;
	int iTemp;

	INT backupTransferLength = 0;
	INT backupBufferSize = 0;
	PUCHAR backupBuffer = NULL;
	PKSTM_XFER_LINK_EL backupXferItem = NULL;

	Pub_To_Priv_StmK(StreamHandle, handle, return FALSE);
	ErrorParamAction(!Buffer, "Buffer", return FALSE);
	ErrorParamAction(Offset < 0, "Offset", return FALSE);
	ErrorParamAction(Length <= 0, "Length", return FALSE);
	ErrorParamAction(!TransferredLength, "TransferredLength", return FALSE);

	ErrorSetAction(!PoolHandle_Inc_StmK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_StmK");
	ErrorSet(!USB_ENDPOINT_DIRECTION_IN(handle->Info->PipeID), Error, ERROR_ACCESS_DENIED, "cannot read from a write stream");
	ErrorSet(handle->Thread.State > KSTM_THREADSTATE_STARTED, Error, ERROR_ACCESS_DENIED, "stream is stopping or starting");

	// Wait on the semaphore (if using a timeout) and acquire the Finished list lock.
	mStm_SpinLockForTransferRequest(handle, Error);

	DL_FOREACH_SAFE(handle->List.Finished, xferEL, xferTempEL)
	{
		if ((handle->SemReady) && xferEL != handle->List.Finished)
		{
			if (WaitForSingleObject(handle->SemReady, 0) != WAIT_OBJECT_0)
				break;
		}

		stageSize		= (Length > xferEL->Xfer->Public.TransferLength) ? xferEL->Xfer->Public.TransferLength : Length;
		Length			-= stageSize;
		transferLength	+= stageSize;

		memcpy(Buffer + Offset, xferEL->Xfer->Buffer, stageSize);
		Offset += stageSize;

		if ((xferEL->Xfer->Public.TransferLength - stageSize) > 0)
		{
			// We need to backup these original values up before modifying incase an error occurs queueing the APC.
			backupXferItem			= xferEL;
			backupTransferLength	= xferEL->Xfer->Public.TransferLength;
			backupBuffer			= xferEL->Xfer->Public.Buffer;
			backupBufferSize		= xferEL->Xfer->Public.BufferSize;

			// There are still bytes remaining in this xfer context; update the conext and push in back to the list head.
			// This is an inefficient way to use the stream and is not entirely thread safe because we need no put
			// everything back after the fact.
			xferEL->Xfer->Public.TransferLength -= stageSize;
			xferEL->Xfer->Public.Buffer += stageSize;
			xferEL->Xfer->Public.BufferSize -= stageSize;

			if (handle->SemReady) ReleaseSemaphore(handle->SemReady, 1, NULL);
			break;
		}
		else
		{
			DL_DELETE(handle->List.Finished, xferEL);
			DL_APPEND(xferSubmitList, xferEL);
		}

		if (Length == 0) break;
	}
	// SPIN-LOCK-RELEASE : List.FinishedLock
	mSpin_Release(&handle->List.FinishedLock);

	mStm_CheckPartialTransfer(handle, Length, TransferredLength, transferLength, Error);

	mStm_QueueTransferList(handle, xferSubmitList, xferEL, xferTempEL, Error);

	if (TransferredLength)
		*TransferredLength = transferLength;

	PoolHandle_Dec_StmK(handle);
	return TRUE;

Error:
	if (xferSubmitList)
	{
		// SPIN-LOCKED : List.FinishedLock
		mSpin_Acquire(&handle->List.FinishedLock);

		if (backupXferItem)
		{
			// This was a partial transfer that is still in the finished list.
			backupXferItem->Xfer->Public.Buffer = backupBuffer;
			backupXferItem->Xfer->Public.BufferSize = backupBufferSize;
			backupXferItem->Xfer->Public.TransferLength = backupTransferLength;
		}

		// Add the elements back to the finished list.
		iTemp = 0;
		DL_FOREACH(xferSubmitList, xferEL)
		{
			iTemp++;
			DL_PREPEND(handle->List.Finished, xferEL);
		}

		// Release xferSubmitList semaphores
		if (iTemp && handle->SemReady) ReleaseSemaphore(handle->SemReady, iTemp, NULL);

		// SPIN-LOCK-RELEASE : List.FinishedLock
		mSpin_Release(&handle->List.FinishedLock);
	}

	if (TransferredLength)
		*TransferredLength = 0;

	PoolHandle_Dec_StmK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API StmK_Write(
    _in KSTM_HANDLE StreamHandle,
    _in PUCHAR Buffer,
    _in INT Offset,
    _in INT Length,
    _out PUINT TransferredLength)
{
	PKSTM_HANDLE_INTERNAL handle = NULL;
	PKSTM_XFER_LINK_EL xferEL, xferTempEL;
	PKSTM_XFER_LINK_EL xferSubmitList = NULL;

	UINT transferLength = 0;
	UINT stageSize;
	int iTemp;

	Pub_To_Priv_StmK(StreamHandle, handle, return FALSE);
	ErrorParamAction(!Buffer, "Buffer", return FALSE);
	ErrorParamAction(Offset < 0, "Offset", return FALSE);
	ErrorParamAction(Length <= 0, "Length", return FALSE);

	ErrorSetAction(!PoolHandle_Inc_StmK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_StmK");
	ErrorSet(USB_ENDPOINT_DIRECTION_IN(handle->Info->PipeID), Error, ERROR_ACCESS_DENIED, "cannot write to a read stream");
	ErrorSet(handle->Thread.State > KSTM_THREADSTATE_STARTED, Error, ERROR_ACCESS_DENIED, "stream is stopping or starting");

	// Wait on the semaphore (if using a timeout) and acquire the Finished list lock.
	mStm_SpinLockForTransferRequest(handle, Error);

	DL_FOREACH_SAFE(handle->List.Finished, xferEL, xferTempEL)
	{
		if ((handle->SemReady) && xferEL != handle->List.Finished)
		{
			if (WaitForSingleObject(handle->SemReady, 0) != WAIT_OBJECT_0)
				break;
		}

		stageSize		= (Length > xferEL->Xfer->BufferSize) ? xferEL->Xfer->BufferSize : Length;
		Length			-= stageSize;
		transferLength	+= stageSize;

		xferEL->Xfer->Public.TransferLength = stageSize;
		memcpy(xferEL->Xfer->Buffer, Buffer + Offset, stageSize);
		Offset += stageSize;

		DL_DELETE(handle->List.Finished, xferEL);
		DL_APPEND(xferSubmitList, xferEL);

		if (Length == 0) break;
	}

	// SPIN-LOCK-RELEASE : List.FinishedLock
	mSpin_Release(&handle->List.FinishedLock);

	mStm_CheckPartialTransfer(handle, Length, TransferredLength, transferLength, Error);

	mStm_QueueTransferList(handle, xferSubmitList, xferEL, xferTempEL, Error);

	if (TransferredLength)
		*TransferredLength = transferLength;

	PoolHandle_Dec_StmK(handle);
	return TRUE;

Error:
	if (xferSubmitList)
	{
		iTemp = 0;

		// SPIN-LOCKED : List.FinishedLock
		mSpin_Acquire(&handle->List.FinishedLock);
		DL_FOREACH(xferSubmitList, xferEL)
		{
			iTemp++;
			DL_PREPEND(handle->List.Finished, xferEL);
		}
		// SPIN-LOCK-RELEASE : List.FinishedLock
		mSpin_Release(&handle->List.FinishedLock);

		// Release xferSubmitList semaphores
		if (iTemp && handle->SemReady) ReleaseSemaphore(handle->SemReady, iTemp, NULL);
	}

	*TransferredLength = 0;
	PoolHandle_Dec_StmK(handle);
	return FALSE;
}
