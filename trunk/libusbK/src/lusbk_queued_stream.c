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
#include "lusbk_handles.h"
#include "lusbk_linked_list.h"
#include "lusbk_stack_collection.h"
#include <process.h>

// Default loggging level
extern ULONG DebugLevel;

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#define Stm_Alloc(mStream,mSize) HeapAlloc((mStream)->Heap,HEAP_ZERO_MEMORY,mSize)

#define EXECUTE_USER_CB(KStm_Handle_Internal,KStm_CallbackFn,KStm_Result,KStm_CallbackParams)	\
	if (KStm_Handle_Internal->UserCB->KStm_CallbackFn)   										\
		KStm_Result = KStm_Handle_Internal->UserCB->KStm_CallbackFn##KStm_CallbackParams

static void CALLBACK Stm_ReadAPC(__in ULONG_PTR dwParam)
{
	PKSTM_XFER_LINK_EL xferEL = (PKSTM_XFER_LINK_EL)dwParam;
	PKSTM_HANDLE_INTERNAL handle = (PKSTM_HANDLE_INTERNAL)xferEL->Xfer->StreamHandle;

	DL_APPEND(handle->List.Queued, xferEL);
	PoolHandle_Dec_StmK(handle);
}

static void CALLBACK Stm_WriteAPC(__in ULONG_PTR dwParam)
{
	PKSTM_XFER_LINK_EL xferEL = (PKSTM_XFER_LINK_EL)dwParam;
	PKSTM_HANDLE_INTERNAL handle = (PKSTM_HANDLE_INTERNAL)xferEL->Xfer->StreamHandle;

	DL_APPEND(handle->List.Queued, xferEL);
	PoolHandle_Dec_StmK(handle);
}

static void CALLBACK Stm_StopAPC(__in ULONG_PTR dwParam)
{
	PKSTM_HANDLE_INTERNAL handle = (PKSTM_HANDLE_INTERNAL)dwParam;

	UNREFERENCED_PARAMETER(handle);
//	handle->Info->DriverAPI.FlushPipe(handle->Info->UsbHandle,handle->Info->PipeID);
}

static DWORD KUSB_API Stm_SubmitRead(
    __in PKSTM_INFO StreamInfo,
    __in PKSTM_XFER_CONTEXT XferContext,
    __in LPOVERLAPPED Overlapped)
{
	StreamInfo->DriverAPI.ReadPipe(StreamInfo->UsbHandle, StreamInfo->PipeID, XferContext->Buffer, XferContext->BufferSize, NULL, Overlapped);
	return GetLastError();
}

static DWORD KUSB_API Stm_SubmitWrite(
    __in PKSTM_INFO StreamInfo,
    __in PKSTM_XFER_CONTEXT XferContext,
    __in LPOVERLAPPED Overlapped)
{
	StreamInfo->DriverAPI.WritePipe(StreamInfo->UsbHandle, StreamInfo->PipeID, XferContext->Buffer, XferContext->TransferLength, NULL, Overlapped);
	return GetLastError();
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
} KSTM_THREAD_INTERNAL, *PKSTM_THREAD_INTERNAL;

static BOOL Stm_Thread_Alloc_Ovl(PKSTM_THREAD_INTERNAL stm)
{
	ULONG pos;

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
	ULONG pos;
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

static BOOL Stm_Thread_ProcessQueued(PKSTM_THREAD_INTERNAL stm)
{
	stm->success = TRUE;
	while(stm->ovlList)
	{
		// Continue to submit transfers while PendingIO is less than MaxPendingIO.
		stm->xferNext = stm->handle->List.Queued;
		if (!stm->xferNext)
		{
			// exhausted all transfers
			stm->errorCode = ERROR_NO_MORE_ITEMS;
			stm->success = FALSE;
			return stm->success;
		}

		stm->ovlNext = stm->ovlList;
		DL_DELETE(stm->handle->List.Queued, stm->xferNext);
		DL_DELETE(stm->ovlList, stm->ovlNext);
		stm->xferNext->Xfer->Overlapped = &stm->ovlNext->Overlapped;

		ResetEvent(stm->ovlNext->Overlapped.hEvent);

		stm->xferNext->Xfer->Public.Buffer		= stm->xferNext->Xfer->Buffer;
		stm->xferNext->Xfer->Public.BufferSize	= stm->xferNext->Xfer->BufferSize;

		stm->errorCode = stm->handle->UserCB->Submit(stm->handle->Info, &stm->xferNext->Xfer->Public, stm->xferNext->Xfer->Overlapped);

		if (stm->errorCode != ERROR_IO_PENDING  && stm->errorCode != ERROR_SUCCESS)
		{
			stm->success = FALSE;
			ErrorNoSetAction(!stm->success, NOP_FUNCTION, "Submit failed.");

			SetEvent(stm->ovlNext->Overlapped.hEvent);
			DL_PREPEND(stm->ovlList, stm->ovlNext);
			DL_PREPEND(stm->handle->List.Queued, stm->xferNext);
			return stm->success;
		}
		IncLock(stm->handle->PendingIO);

		DL_APPEND(stm->pendingList, stm->xferNext);
	}
	return stm->success;
}

static BOOL Stm_Thread_ProcessPending(PKSTM_THREAD_INTERNAL stm, DWORD timeoutOverride)
{
	DWORD timeout;
	DWORD waitResult;

	if (!stm->pendingList)
	{
		stm->errorCode = ERROR_NO_MORE_ITEMS;
		return FALSE;
	}

	stm->xferNext = stm->pendingList;
	stm->ovlNext = (PKSTM_OVERLAPPED_EL)stm->xferNext->Xfer->Overlapped;

	if (!timeoutOverride)
		timeout = (stm->handle->PendingIO == (long)stm->handle->Info->MaxPendingIO) ? INFINITE : 0;
	else
		timeout = timeoutOverride;

	stm->success	= TRUE;
	while (stm->success)
	{
		waitResult = WaitForSingleObjectEx(stm->ovlNext->Overlapped.hEvent, timeout, TRUE);
		if (!timeoutOverride)
			timeout = (stm->handle->PendingIO == (long)stm->handle->Info->MaxPendingIO) ? INFINITE : 0;

		if (waitResult == STATUS_USER_APC) continue;
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

	DL_DELETE(stm->pendingList, stm->xferNext);
	DL_DELETE(stm->ovlList, stm->ovlNext);

	stm->success = GetOverlappedResult(stm->handle->Info->DeviceHandle, &stm->ovlNext->Overlapped, &stm->xferNext->Xfer->Public.TransferLength, FALSE);
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

	stm->errorCode = AL_PushTail_XferLink(stm->handle->List.Finished, stm->xferNext);

	DecLock(stm->handle->PendingIO);

	if (USB_ENDPOINT_DIRECTION_IN(stm->handle->Info->PipeID))
		IncLock(stm->handle->PendingTransfers);
	else
		DecLock(stm->handle->PendingTransfers);

	EXECUTE_USER_CB(stm->handle, Complete, stm->errorCode, (stm->handle->Info, &stm->xferNext->Xfer->Public));
	return stm->success;
}

static BOOL Stm_StopInternal(
    __in PKSTM_HANDLE_INTERNAL handle)
{
	BOOL success;

	success = (InterlockedCompareExchange(&handle->Thread.State, KSTM_THREADSTATE_STOPPING, KSTM_THREADSTATE_STARTED) == KSTM_THREADSTATE_STARTED) ? TRUE : FALSE;
	if (!success) return FALSE;

	QueueUserAPC(Stm_StopAPC, handle->Thread.Handle, (ULONG_PTR)handle);
	WaitForSingleObject(handle->Thread.StoppedEvent, INFINITE);
	return TRUE;
}

static unsigned _stdcall Stm_ThreadProc(PKSTM_HANDLE_INTERNAL handle)
{
	DWORD exitCode = ERROR_SUCCESS;
	KSTM_THREAD_INTERNAL stm_thread_internal;
	PKSTM_THREAD_INTERNAL stm;
	BOOL isQueueEmpty, isPendingIoEmpty;
	DWORD maxWaitMS;

	stm = &stm_thread_internal;

	memset(stm, 0, sizeof(*stm));
	stm->handle = handle;

	InterlockedExchange(&handle->Thread.State, KSTM_THREADSTATE_STARTED);
	SetEvent(stm->handle->Thread.StartedEvent);
	ResetEvent(stm->handle->Thread.StoppedEvent);

	if (!Stm_Thread_Alloc_Ovl(stm))
	{
		exitCode = stm->errorCode;
		goto Done;
	}

	while(handle->Thread.State == KSTM_THREADSTATE_STARTED && exitCode == ERROR_SUCCESS)
	{
		isQueueEmpty = FALSE;
		isPendingIoEmpty = FALSE;
		if (!Stm_Thread_ProcessQueued(stm))
		{
			if (stm->errorCode == ERROR_NO_MORE_ITEMS)
			{
				isQueueEmpty = TRUE;
			}
			else
			{
				// fatal stream error; aborting..
				exitCode = stm->errorCode;
				break;
			}
		}
		else
		{
			isPendingIoEmpty = TRUE;
		}

		if (!Stm_Thread_ProcessPending(stm, 0))
		{
			if (stm->errorCode == ERROR_IO_INCOMPLETE) continue;

			if (stm->errorCode == ERROR_NO_MORE_ITEMS)
			{
				if (isQueueEmpty || isPendingIoEmpty)
				{
					if (USB_ENDPOINT_DIRECTION_IN(stm->handle->Info->PipeID))
						SleepEx(stm->handle->PendingTransfers == (long)stm->handle->Info->MaxPendingTransfers ? INFINITE : 0, TRUE);
					else
						SleepEx(stm->handle->PendingTransfers == 0 ? INFINITE : 0, TRUE);
				}
				else
				{
					SleepEx(0, TRUE);
				}
			}
			else
			{
				// fatal stream error; aborting..
				exitCode = stm->errorCode;
				break;
			}
		}
	}

Done:
	InterlockedExchange(&handle->Thread.State, KSTM_THREADSTATE_STOPPING);

	// Wait for all queued APCs
	while (ALLK_GETREF_HANDLE(stm->handle) > 2)
		SleepEx(INFINITE, TRUE);

	if (stm->handle->TimeoutCancelMS)
		maxWaitMS = stm->handle->TimeoutCancelMS;
	else
		maxWaitMS = 1000;

	stm->handle->TimeoutCancelMS = 0;

	while(stm->handle->PendingIO > 0)
	{
		if (!Stm_Thread_ProcessPending(stm, maxWaitMS))
		{
			if (stm->errorCode == ERROR_IO_INCOMPLETE) CancelIo(stm->handle->Info->DeviceHandle);
			maxWaitMS = 10;
		}
	}

	Stm_Thread_Free_Ovl(stm);

	ResetEvent(stm->handle->Thread.StartedEvent);
	SetEvent(stm->handle->Thread.StoppedEvent);

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
		if(handle->Info->UsbHandle) PoolHandle_Dec_UsbK((PKUSB_HANDLE_INTERNAL)handle->Info->UsbHandle);
	}
	if (handle->Thread.StartedEvent) CloseHandle(handle->Thread.StartedEvent);
	if (handle->Thread.StoppedEvent) CloseHandle(handle->Thread.StoppedEvent);

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
    _in ULONG MaxTransferSize,
    _in ULONG MaxPendingTransfers,
    _in ULONG MaxPendingIO,
    _inopt PKSTM_CALLBACK Callbacks,
    _inopt KSTM_FLAG Flags)
{
	DWORD minHeapSize = 0;
	DWORD errorCode = ERROR_SUCCESS;
	PKSTM_HANDLE_INTERNAL handle;
	PKUSB_HANDLE_INTERNAL usbHandle;
	DWORD TransferIndex;
	USB_ENDPOINT_DESCRIPTOR epDescriptor;
	BOOL success;

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

	if (Callbacks)
		memcpy(handle->UserCB, Callbacks, sizeof(*handle->UserCB));

	if (!handle->UserCB->Submit)
	{
		if (USB_ENDPOINT_DIRECTION_IN(handle->Info->PipeID))
			handle->UserCB->Submit = &Stm_SubmitRead;
		else
			handle->UserCB->Submit = &Stm_SubmitWrite;
	}

	handle->Thread.StartedEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
	handle->Thread.StoppedEvent = CreateEventA(NULL, TRUE, TRUE, NULL);

	errorCode = AL_Create_XferLink(&handle->List.Idle, handle->Heap, handle->Info->MaxPendingTransfers);
	ErrorNoSet(errorCode, Error, "AL_Create List.Idle failed.");

	errorCode = AL_Create_XferLink(&handle->List.Finished, handle->Heap, handle->Info->MaxPendingTransfers);
	ErrorNoSet(errorCode, Error, "AL_Create List.Finished failed.");

	for (TransferIndex = 0; TransferIndex < handle->Info->MaxPendingTransfers; TransferIndex++)
	{
		PKSTM_XFER_INTERNAL xfer;

		xfer = Stm_Alloc(handle, sizeof(*xfer));
		ErrorMemory(!xfer, Error);

		xfer->Buffer = Stm_Alloc(handle, handle->Info->MaxTransferSize);
		ErrorMemory(!xfer->Buffer, Error);

		xfer->Link.Xfer			= xfer;
		xfer->StreamHandle		= handle;
		xfer->BufferSize		= handle->Info->MaxTransferSize;
		xfer->Public.Buffer		= xfer->Buffer;
		xfer->Public.BufferSize = xfer->BufferSize;

		EXECUTE_USER_CB(handle, Initialize, errorCode, (handle->Info, &xfer->Public));

		if (USB_ENDPOINT_DIRECTION_IN(handle->Info->PipeID))
		{
			DL_APPEND(handle->List.Queued, &xfer->Link);
		}
		else
		{
			AL_PushTail_XferLink(handle->List.Idle, &xfer->Link);
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

	ResumeThread(handle->Thread.Handle);
	WaitForSingleObject(handle->Thread.StartedEvent, INFINITE);

	PoolHandle_Dec_StmK(handle);
	return TRUE;
Error:
	PoolHandle_Dec_StmK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API StmK_Stop(
    _in KSTM_HANDLE StreamHandle,
    _in ULONG TimeoutCancelMS)
{
	PKSTM_HANDLE_INTERNAL handle;
	BOOL success;

	Pub_To_Priv_StmK(StreamHandle, handle, return FALSE);
	ErrorSetAction(!PoolHandle_Inc_StmK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_StmK");

	handle->TimeoutCancelMS = TimeoutCancelMS ? TimeoutCancelMS : 1000;

	success = Stm_StopInternal(handle);
	ErrorSet(!success, Error, ERROR_ACCESS_DENIED, "stream already stopped");

	PoolHandle_Dec_StmK(handle);
	return TRUE;
Error:
	PoolHandle_Dec_StmK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API StmK_Read(
    _in KSTM_HANDLE StreamHandle,
    _out PUCHAR Buffer,
    _in LONG Offset,
    _in ULONG Length,
    _out PULONG TransferredLength)
{
	PKSTM_HANDLE_INTERNAL handle = NULL;
	PKSTM_XFER_LINK_EL xferEL = NULL;
	UINT transferLength = 0;
	UINT stageSize;

	Pub_To_Priv_StmK(StreamHandle, handle, return FALSE);
	ErrorParamAction(!Buffer, "Buffer", return FALSE);
	ErrorParamAction(Offset < 0, "Offset", return FALSE);
	ErrorParamAction(Length <= 0, "Length", return FALSE);
	ErrorParamAction(!TransferredLength, "TransferredLength", return FALSE);

	ErrorSetAction(!PoolHandle_Inc_StmK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_StmK");
	ErrorSet(!USB_ENDPOINT_DIRECTION_IN(handle->Info->PipeID), Error, ERROR_ACCESS_DENIED, "cannot read from a write stream");
	ErrorParam((Length % handle->Info->MaxTransferSize) > 0, Error, "Length not an interval of MaxTransferSize");
	ErrorSet(handle->Thread.State != KSTM_THREADSTATE_STARTED, Error, ERROR_THREAD_WAS_SUSPENDED, "stream is stopped");

	while(Length > 0)
	{

		if (AL_PopHead_XferLink(handle->List.Finished, &xferEL) != ERROR_SUCCESS)
		{
			if (transferLength == 0)
			{
				LusbwError(ERROR_NO_MORE_ITEMS);
				goto Error;
			}
			else
			{
				goto PartialTransfer;
			}
		}

		DecLock(handle->PendingTransfers);

		stageSize = Length >  xferEL->Xfer->Public.TransferLength ? xferEL->Xfer->Public.TransferLength : Length;
		Length			-= stageSize;
		transferLength	+= stageSize;

		xferEL->Xfer->Public.TransferLength = stageSize;
		memcpy(Buffer + Offset, xferEL->Xfer->Buffer, stageSize);
		Offset += stageSize;

		PoolHandle_Inc_StmK(handle);
		QueueUserAPC(Stm_ReadAPC, handle->Thread.Handle, (ULONG_PTR)xferEL);
	}

PartialTransfer:
	*TransferredLength = transferLength;
	PoolHandle_Dec_StmK(handle);
	return TRUE;
Error:
	PoolHandle_Dec_StmK(handle);
	return FALSE;
}

KUSB_EXP BOOL KUSB_API StmK_Write(
    _in KSTM_HANDLE StreamHandle,
    _in PUCHAR Buffer,
    _in LONG Offset,
    _in ULONG Length,
    _out PULONG TransferredLength)
{
	PKSTM_HANDLE_INTERNAL handle = NULL;
	PKSTM_XFER_LINK_EL xferEL = NULL;
	UINT transferLength = 0;
	UINT stageSize;

	Pub_To_Priv_StmK(StreamHandle, handle, return FALSE);
	ErrorParamAction(!Buffer, "Buffer", return FALSE);
	ErrorParamAction(Offset < 0, "Offset", return FALSE);
	ErrorParamAction(Length <= 0, "Length", return FALSE);
	ErrorParamAction(!TransferredLength, "TransferredLength", return FALSE);

	ErrorSetAction(!PoolHandle_Inc_StmK(handle), ERROR_RESOURCE_NOT_AVAILABLE, return FALSE, "->PoolHandle_Inc_StmK");
	ErrorSet(USB_ENDPOINT_DIRECTION_IN(handle->Info->PipeID), Error, ERROR_ACCESS_DENIED, "cannot write to a read stream");
	ErrorSet(handle->Thread.State != KSTM_THREADSTATE_STARTED, Error, ERROR_THREAD_WAS_SUSPENDED, "stream is stopped");

	while(Length > 0)
	{
		if (AL_PopHead_XferLink(handle->List.Idle, &xferEL) != ERROR_SUCCESS)
		{
			if (AL_PopHead_XferLink(handle->List.Finished, &xferEL) != ERROR_SUCCESS)
			{
				if (transferLength == 0)
				{
					LusbwError(ERROR_NO_MORE_ITEMS);
					goto Error;
				}
				else
				{
					goto PartialTransfer;
				}
			}
		}

		IncLock(handle->PendingTransfers);
		stageSize = Length >  xferEL->Xfer->BufferSize ? xferEL->Xfer->BufferSize : Length;
		Length			-= stageSize;
		transferLength	+= stageSize;

		xferEL->Xfer->Public.TransferLength = stageSize;
		memcpy(xferEL->Xfer->Buffer, Buffer + Offset, stageSize);
		Offset += stageSize;

		PoolHandle_Inc_StmK(handle);
		QueueUserAPC(Stm_WriteAPC, handle->Thread.Handle, (ULONG_PTR)xferEL);
	}

PartialTransfer:
	*TransferredLength = transferLength;
	PoolHandle_Dec_StmK(handle);
	return TRUE;
Error:
	*TransferredLength = transferLength;
	PoolHandle_Dec_StmK(handle);
	return FALSE;
}
