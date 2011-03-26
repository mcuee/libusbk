/* USB Benchmark for libusb-win32

 Copyright © 2010 Travis Robinson. <libusbdotnet@gmail.com>
 website: http://sourceforge.net/projects/libusb-win32

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, please visit www.gnu.org.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include "lusbk_usb.h"
#include "lusbk_version.h"


#define MAX_OUTSTANDING_TRANSFERS 10

#define USB_ENDPOINT_ADDRESS_MASK 0x0F

// This is used only in VerifyData() for display information
// about data validation mismatches.
#define CONVDAT(format,...) printf("[data-mismatch] " format,__VA_ARGS__)

// All output is directed through these macros.
//
#define LOG(LogTypeString,format,...)  printf("%s[" __FUNCTION__ "] "format, LogTypeString, __VA_ARGS__)
#define LOG_NO_FN(LogTypeString,format,...)  printf("%s" format "%s",LogTypeString,__VA_ARGS__,"")
#define CONERR(format,...) LOG("Error:",format,__VA_ARGS__)
#define CONMSG(format,...) LOG_NO_FN("",format,__VA_ARGS__)
#define CONWRN(format,...) LOG("Warn:",format,__VA_ARGS__)
#define CONDBG(format,...) LOG_NO_FN("",format,__VA_ARGS__)

#define CONERR0(message) CONERR("%s", message)
#define CONMSG0(message) CONMSG("%s", message)
#define CONWRN0(message) CONWRN("%s", message)
#define CONDBG0(message) CONDBG("%s", message)

static LPCSTR DrvIdNames[8] = {"libusbK", "libusb0", "libusb0 filter", "WinUSB", "Unknown", "Unknown", "Unknown", "Unknown"};
#define GetDrvIdString(DrvId)	(DrvIdNames[((((LONG)(DrvId))<0) || ((LONG)(DrvId)) >= KUSB_DRVID_COUNT)?KUSB_DRVID_COUNT:(DrvId)])

KUSB_DRIVER_API K;

// Custom vendor requests that must be implemented in the benchmark firmware.
// Test selection can be bypassed with the "notestselect" argument.
//
typedef enum _BENCHMARK_DEVICE_COMMANDS
{
	SET_TEST = 0x0E,
	GET_TEST = 0x0F,
} BENCHMARK_DEVICE_COMMANDS;

// Tests supported by the official benchmark firmware.
//
typedef enum _BENCHMARK_DEVICE_TEST_TYPE
{
	TestTypeNone	= 0x00,
	TestTypeRead	= 0x01,
	TestTypeWrite	= 0x02,
	TestTypeLoop	= TestTypeRead | TestTypeWrite,
} BENCHMARK_DEVICE_TEST_TYPE;

// This software was mainly created for testing the libusb-win32 kernel & user driver.
typedef enum _BENCHMARK_TRANSFER_MODE
{
	// Tests for the libusb-win32 sync transfer function.
	TRANSFER_MODE_SYNC,

	// Test for async function, iso transfers, and queued transfers
	TRANSFER_MODE_ASYNC,
} BENCHMARK_TRANSFER_MODE;

// Holds all of the information about a test.
typedef struct _BENCHMARK_TEST_PARAM
{
	// User configurable value set from the command line.
	//
	INT Vid;			// Vendor ID
	INT Pid;			// Porduct ID
	INT Intf;			// Interface number
	INT	Altf;			// Alt Interface number
	INT Ep;				// Endpoint number (1-15)
	INT Refresh;		// Refresh interval (ms)
	INT Timeout;		// Transfer timeout (ms)
	INT Retry;			// Number for times to retry a timed out transfer before aborting
	INT BufferSize;		// Number of bytes to transfer
	INT BufferCount;	// Number of outstanding asynchronous transfers
	BOOL NoTestSelect;	// If true, don't send control message to select the test type.
	BOOL UseList;		// Show the user a device list and let them choose a benchmark device.
	INT IsoPacketSize; // Isochronous packet size (defaults to the endpoints max packet size)
	INT Priority;		// Priority to run this thread at.
	BOOL Verify;		// Only for loop and read test. If true, verifies data integrity.
	BOOL VerifyDetails;	// If true, prints detailed information for each invalid byte.
	enum BENCHMARK_DEVICE_TEST_TYPE TestType;	// The benchmark test type.
	enum BENCHMARK_TRANSFER_MODE TransferMode;	// Sync or Async

	// Internal value use during the test.
	//
	PKUSB_DEV_LIST DeviceList;
	PKUSB_DEV_LIST SelectedDeviceProfile;
	HANDLE DeviceHandle;
	WINUSB_INTERFACE_HANDLE InterfaceHandle;
	USB_DEVICE_DESCRIPTOR DeviceDescriptor;
	USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
	WINUSB_PIPE_INFORMATION PipeInformation[32];
	BOOL IsCancelled;
	BOOL IsUserAborted;

	BYTE* VerifyBuffer;		// Stores the verify test pattern for 1 packet.
	WORD VerifyBufferSize;	// Size of VerifyBuffer
} BENCHMARK_TEST_PARAM, *PBENCHMARK_TEST_PARAM;

// The benchmark transfer context used for asynchronous transfers.  see TransferAsync().
typedef struct _BENCHMARK_TRANSFER_HANDLE
{
	OVERLAPPED Overlapped;
	BOOL InUse;
	PUCHAR Data;
	INT DataMaxLength;
	INT ReturnCode;
} BENCHMARK_TRANSFER_HANDLE, *PBENCHMARK_TRANSFER_HANDLE;
#pragma warning(disable:4200)
// Holds all of the information about a transfer.
typedef struct _BENCHMARK_TRANSFER_PARAM
{
	PBENCHMARK_TEST_PARAM Test;

	HANDLE ThreadHandle;
	DWORD ThreadID;
	WINUSB_PIPE_INFORMATION Ep;
	INT IsoPacketSize;
	BOOL IsRunning;

	LONGLONG TotalTransferred;
	LONG LastTransferred;

	LONG Packets;
	DWORD StartTick;
	DWORD LastTick;
	DWORD LastStartTick;

	INT TotalTimeoutCount;
	INT RunningTimeoutCount;

	INT TotalErrorCount;
	INT RunningErrorCount;

	INT ShortTransferCount;

	INT TransferHandleNextIndex;
	INT TransferHandleWaitIndex;
	INT OutstandingTransferCount;

	BENCHMARK_TRANSFER_HANDLE TransferHandles[MAX_OUTSTANDING_TRANSFERS];

	// Placeholder for end of structure; this is where the raw data for the
	// transfer buffer is allocated.
	//
	UCHAR Buffer[0];
} BENCHMARK_TRANSFER_PARAM, *PBENCHMARK_TRANSFER_PARAM;
#pragma warning(default:4200)

// Benchmark device api.
BOOL Bench_Open(PBENCHMARK_TEST_PARAM test);
BOOL Bench_SetTestType(WINUSB_INTERFACE_HANDLE handle, BENCHMARK_DEVICE_TEST_TYPE testType, int intf);
BOOL Bench_GetTestType(WINUSB_INTERFACE_HANDLE handle, BENCHMARK_DEVICE_TEST_TYPE* testType, int intf);

// Critical section for running status.
CRITICAL_SECTION DisplayCriticalSection;

// Finds the interface for [interface_number] in a libusb-win32 config descriptor.
// If first_interface is not NULL, it is set to the first interface in the config.
//

// Internal function used by the benchmark application.
void ShowHelp(void);
void ShowCopyright(void);

void SetTestDefaults(PBENCHMARK_TEST_PARAM test);
int GetTestDeviceFromArgs(PBENCHMARK_TEST_PARAM test);
int GetTestDeviceFromList(PBENCHMARK_TEST_PARAM test);

char* GetParamStrValue(const char* src, const char* paramName);
BOOL GetParamIntValue(const char* src, const char* paramName, INT* returnValue);
int ValidateBenchmarkArgs(PBENCHMARK_TEST_PARAM test);
int ParseBenchmarkArgs(PBENCHMARK_TEST_PARAM testParams, int argc, char** argv);
void FreeTransferParam(PBENCHMARK_TRANSFER_PARAM* testTransferRef);
PBENCHMARK_TRANSFER_PARAM CreateTransferParam(PBENCHMARK_TEST_PARAM test, int endpointID);
void GetAverageBytesSec(PBENCHMARK_TRANSFER_PARAM transferParam, DOUBLE* bps);
void GetCurrentBytesSec(PBENCHMARK_TRANSFER_PARAM transferParam, DOUBLE* bps);
void ShowRunningStatus(PBENCHMARK_TRANSFER_PARAM transferParam);
void ShowTestInfo(PBENCHMARK_TEST_PARAM test);
void ShowTransferInfo(PBENCHMARK_TRANSFER_PARAM transferParam);

void WaitForTestTransfer(PBENCHMARK_TRANSFER_PARAM transferParam);
void ResetRunningStatus(PBENCHMARK_TRANSFER_PARAM transferParam);

// The thread transfer routine.
DWORD TransferThreadProc(PBENCHMARK_TRANSFER_PARAM transferParams);

#define TRANSFER_DISPLAY(TransferParam, ReadingString, WritingString) \
	((TransferParam->Ep.PipeId & USB_ENDPOINT_DIRECTION_MASK) ? ReadingString : WritingString)

#define INC_ROLL(IncField, RollOverValue) if ((++IncField) >= RollOverValue) IncField = 0

#define ENDPOINT_TYPE(TransferParam) (TransferParam->Ep.PipeType & 3)
const char* TestDisplayString[] = {"None", "Read", "Write", "Loop", NULL};
const char* EndpointTypeDisplayString[] = {"Control", "Isochronous", "Bulk", "Interrupt", NULL};

LONG WinError(__in_opt DWORD errorCode)
{
	LPSTR buffer = NULL;

	errorCode = errorCode ? labs(errorCode) : GetLastError();
	if (!errorCode) return errorCode;

	if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
	                   NULL, errorCode, 0, (LPSTR)&buffer, 0, NULL) > 0)
	{
		CONERR("%s\n", buffer);
	}
	else
	{
		CONERR("FormatMessage error!\n");
	}

	if (buffer)
		LocalFree(buffer);

	return -labs(errorCode);
}

void SetTestDefaults(PBENCHMARK_TEST_PARAM test)
{
	memset(test, 0, sizeof(*test));

	test->Ep			= 0x00;
	test->Vid			= 0x0666;
	test->Pid			= 0x0001;
	test->Refresh		= 1000;
	test->Timeout		= 5000;
	test->TestType		= TestTypeLoop;
	test->BufferSize	= 4096;
	test->BufferCount   = 1;
	test->Priority		= THREAD_PRIORITY_NORMAL;
	test->Intf = -1;
	test->Altf = -1;

}

BOOL Bench_Open(PBENCHMARK_TEST_PARAM test)
{
	UCHAR altSetting;
	UCHAR interfaceIndex = UCHAR_MAX;
	WINUSB_INTERFACE_HANDLE associatedHandle;
	ULONG transferred;
	PKUSB_DEV_LIST list, next;

	test->SelectedDeviceProfile = NULL;

	next = test->DeviceList;
	while (next)
	{
		list = next;			// the one we are using now
		next = list->next;	// the one we may use next

		if (!list->My.Byte[0])
			continue;

		memset(&K, 0, sizeof(K));
		if (!LUsbK_LoadDriverApi(&K, list->DrvId))
		{
			WinError(0);
			CONERR("failed loading driver api %s.\n", GetDrvIdString(list->DrvId));
			return FALSE;
		}

		test->DeviceHandle = CreateFileA(list->DevicePath,
		                                 GENERIC_READ | GENERIC_WRITE,
		                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
		                                 NULL,
		                                 OPEN_EXISTING,
		                                 FILE_FLAG_OVERLAPPED,
		                                 NULL);

		if (!test->DeviceHandle || test->DeviceHandle == INVALID_HANDLE_VALUE)
		{
			test->DeviceHandle = NULL;
			CONERR0("failed creating device handle.\n");
			return FALSE;
		}
		if (!K.Initialize(test->DeviceHandle, &test->InterfaceHandle))
		{
			WinError(0);
			return FALSE;
		}

		if (!K.GetDescriptor(test->InterfaceHandle,
		                     USB_DEVICE_DESCRIPTOR_TYPE,
		                     0, 0,
		                     (PUCHAR)&test->DeviceDescriptor,
		                     sizeof(test->DeviceDescriptor),
		                     &transferred))
		{
			WinError(0);
			return FALSE;
		}
		test->Vid = (INT)test->DeviceDescriptor.idVendor;
		test->Pid = (INT)test->DeviceDescriptor.idProduct;

NextInterface:

		// While searching for hardware specifics we are also gathering information and storing it
		// in our test.
		memset(&test->InterfaceDescriptor, 0, sizeof(test->InterfaceDescriptor));
		altSetting = 0;
		while(K.QueryInterfaceSettings(test->InterfaceHandle, altSetting, &test->InterfaceDescriptor))
		{
			// found an interface
			UCHAR pipeIndex = 0;
			memset(&test->PipeInformation, 0, sizeof(test->PipeInformation));
			while(K.QueryPipe(test->InterfaceHandle, altSetting, pipeIndex, &test->PipeInformation[pipeIndex]))
			{
				// found a pipe
				pipeIndex++;
			}
			if ( (( test->Intf == -1) || (test->Intf == test->InterfaceDescriptor.bInterfaceNumber)) &&
			        (( test->Altf == -1) || (test->Altf == test->InterfaceDescriptor.bAlternateSetting)) )
			{
				// this is the one we are looking for.
				test->Intf = test->InterfaceDescriptor.bInterfaceNumber;
				test->Intf = test->InterfaceDescriptor.bAlternateSetting;
				test->SelectedDeviceProfile = list;
				return TRUE;
			}
			altSetting++;
			memset(&test->InterfaceDescriptor, 0, sizeof(test->InterfaceDescriptor));
		}
		if (K.GetAssociatedInterface(test->InterfaceHandle, ++interfaceIndex, &associatedHandle))
		{
			// this device has more interfaces to look at.
			//
			K.Free(test->InterfaceHandle);
			test->InterfaceHandle = associatedHandle;
			goto NextInterface;
		}

		// This one didn't match the test specifics; continue on to the next potential match.
		K.Free(test->InterfaceHandle);
		test->InterfaceHandle = NULL;
	}

	CONERR("device interface/alt interface not found (%02Xh/%02Xh).\n", test->Intf, test->Altf);
	return FALSE;
}

#define MAKE_REQUEST_TYPE(Direction, Type, Recipient)	\
	(((Direction & 0x1)<<7) | ((Type & 0x3)<< 5) | (Recipient & 0x1F))

BOOL Bench_SetTestType(WINUSB_INTERFACE_HANDLE handle, BENCHMARK_DEVICE_TEST_TYPE testType, int intf)
{
	UCHAR buffer[1];
	DWORD transferred = 0;
	WINUSB_SETUP_PACKET setupPacket =
	{
		(BMREQUEST_VENDOR << 5) | USB_ENDPOINT_DIRECTION_MASK,	// RequestType
		SET_TEST,			// Request	(BmCommand)
		0,					// Value	(TestType)
		0,					// Index	(Interface#)
		1					// Length	(1)
	};

	if (!handle || handle == INVALID_HANDLE_VALUE)
		return WinError(ERROR_INVALID_HANDLE);

	setupPacket.Request = (UCHAR)SET_TEST;
	setupPacket.Value = (USHORT) testType;
	setupPacket.Index = (USHORT)intf;

	if (K.ControlTransfer(handle, setupPacket, buffer, 1, &transferred, NULL))
	{
		if (transferred)
			return TRUE;
	}

	return FALSE;
}

BOOL Bench_GetTestType(WINUSB_INTERFACE_HANDLE handle, BENCHMARK_DEVICE_TEST_TYPE* testType, int intf)
{
	UCHAR buffer[1];
	DWORD transferred = 0;
	WINUSB_SETUP_PACKET setup;
	setup.RequestType = MAKE_REQUEST_TYPE(BMREQUEST_DEVICE_TO_HOST, BMREQUEST_VENDOR, BMREQUEST_TO_DEVICE);
	setup.Request = SET_TEST;
	setup.Index = (USHORT)intf;
	setup.Value = 0;
	setup.Length = 0;

	if (K.ControlTransfer(handle, setup, buffer, 1, &transferred, NULL))
	{
		if (transferred)
		{
			*testType = (BENCHMARK_DEVICE_TEST_TYPE)buffer[0];
			return TRUE;
		}
	}

	return FALSE;
}

INT VerifyData(PBENCHMARK_TRANSFER_PARAM transferParam, BYTE* data, INT dataLength)
{

	WORD verifyDataSize = transferParam->Test->VerifyBufferSize;
	BYTE* verifyData = transferParam->Test->VerifyBuffer;
	BYTE keyC = 0;
	BOOL seedKey = TRUE;
	INT dataLeft = dataLength;
	INT dataIndex = 0;
	INT packetIndex = 0;
	INT verifyIndex = 0;

	while(dataLeft > 1)
	{
		verifyDataSize = dataLeft > transferParam->Test->VerifyBufferSize ? transferParam->Test->VerifyBufferSize : (WORD)dataLeft;

		if (seedKey)
			keyC = data[dataIndex + 1];
		else
		{
			if (data[dataIndex + 1] == 0)
			{
				keyC = 0;
			}
			else
			{
				keyC++;
			}
		}
		seedKey = FALSE;
		// Index 0 is always 0.
		// The key is always at index 1
		verifyData[1] = keyC;
		if (memcmp(&data[dataIndex], verifyData, verifyDataSize) != 0)
		{
			// Packet verification failed.

			// Reset the key byte on the next packet.
			seedKey = TRUE;

			CONVDAT("data mismatch packet-index=%d data-index=%d\n", packetIndex, dataIndex);

			if (transferParam->Test->VerifyDetails)
			{
				for (verifyIndex = 0; verifyIndex < verifyDataSize; verifyIndex++)
				{
					if (verifyData[verifyIndex] == data[dataIndex + verifyIndex])
						continue;

					CONVDAT("packet-offset=%d expected %02Xh got %02Xh\n",
					        verifyIndex,
					        verifyData[verifyIndex],
					        data[dataIndex + verifyIndex]);

				}
			}

		}

		// Move to the next packet.
		packetIndex++;
		dataLeft -= verifyDataSize;
		dataIndex += verifyDataSize;

	}

	return 0;
}

int TransferSync(PBENCHMARK_TRANSFER_PARAM transferParam)
{
	ULONG transferred;
	BOOL success;
	if (transferParam->Ep.PipeId & USB_ENDPOINT_DIRECTION_MASK)
	{
		success = K.ReadPipe(transferParam->Test->InterfaceHandle,
		                     transferParam->Ep.PipeId,
		                     transferParam->Buffer,
		                     transferParam->Test->BufferSize,
		                     &transferred,
		                     NULL);
	}
	else
	{
		success = K.WritePipe(transferParam->Test->InterfaceHandle,
		                      transferParam->Ep.PipeId,
		                      transferParam->Buffer,
		                      transferParam->Test->BufferSize,
		                      &transferred,
		                      NULL);
	}

	return success ? (int)transferred : -labs(GetLastError());
}

int TransferAsync(PBENCHMARK_TRANSFER_PARAM transferParam, PBENCHMARK_TRANSFER_HANDLE* handleRef)
{
	int ret = 0;
	BOOL success;
	PBENCHMARK_TRANSFER_HANDLE handle;

	*handleRef = NULL;

	// Submit transfers until the maximum number of outstanding transfer(s) is reached.
	while (transferParam->OutstandingTransferCount < transferParam->Test->BufferCount)
	{
		// Get the next available benchmark transfer handle.
		*handleRef = handle = &transferParam->TransferHandles[transferParam->TransferHandleNextIndex];

		// If a libusb-win32 transfer context hasn't been setup for this benchmark transfer
		// handle, do it now.
		//
		if (!handle->Overlapped.hEvent)
		{
			handle->Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			// Data buffer(s) are located at the end of the transfer param.
			handle->Data = transferParam->Buffer + (transferParam->TransferHandleNextIndex * transferParam->Test->BufferSize);
		}
		else
		{
			// re-initialize and re-use the overlapped
			HANDLE h = handle->Overlapped.hEvent;
			ResetEvent(h);
			memset(&handle->Overlapped, 0, sizeof(handle->Overlapped));
			handle->Overlapped.hEvent = h;
		}
		handle->DataMaxLength = transferParam->Test->BufferSize;

		if (transferParam->Ep.PipeId & USB_ENDPOINT_DIRECTION_MASK)
		{
			success = K.ReadPipe(transferParam->Test->InterfaceHandle,
			                     transferParam->Ep.PipeId,
			                     handle->Data,
			                     handle->DataMaxLength,
			                     NULL,
			                     &handle->Overlapped);
		}
		else
		{
			success = K.WritePipe(transferParam->Test->InterfaceHandle,
			                      transferParam->Ep.PipeId,
			                      handle->Data,
			                      handle->DataMaxLength,
			                      NULL,
			                      &handle->Overlapped);
		}
		if (!success && GetLastError() == ERROR_IO_PENDING)
			success = TRUE;

		// Submit this transfer now.
		handle->ReturnCode = ret = ((success) ? ERROR_SUCCESS : -labs(GetLastError()));
		if (ret < 0) goto Done;

		// Mark this handle has InUse.
		handle->InUse = TRUE;

		// When transfers ir successfully submitted, OutstandingTransferCount goes up; when
		// they are completed it goes down.
		//
		transferParam->OutstandingTransferCount++;

		// Move TransferHandleNextIndex to the next available transfer.
		INC_ROLL(transferParam->TransferHandleNextIndex, transferParam->Test->BufferCount);

	}

	// If the number of outstanding transfers has reached the limit, wait for the
	// oldest outstanding transfer to complete.
	//
	if (transferParam->OutstandingTransferCount == transferParam->Test->BufferCount)
	{
		DWORD transferred;
		// TransferHandleWaitIndex is the index of the oldest outstanding transfer.
		*handleRef = handle = &transferParam->TransferHandles[transferParam->TransferHandleWaitIndex];

		// Only wait, cancelling & freeing is handled by the caller.
		if (WaitForSingleObject(handle->Overlapped.hEvent,  transferParam->Test->Timeout) != WAIT_OBJECT_0)
		{
			ret = WinError(0);
			handle->ReturnCode = ret;
			goto Done;
		}
		if (!K.GetOverlappedResult(transferParam->Test->InterfaceHandle, &handle->Overlapped, &transferred, FALSE))
		{
			ret = WinError(0);
			handle->ReturnCode = ret;
			goto Done;
		}
		handle->ReturnCode = ret = (DWORD)transferred;

		if (ret < 0) goto Done;

		// Mark this handle has no longer InUse.
		handle->InUse = FALSE;

		// When transfers ir successfully submitted, OutstandingTransferCount goes up; when
		// they are completed it goes down.
		//
		transferParam->OutstandingTransferCount--;

		// Move TransferHandleWaitIndex to the oldest outstanding transfer.
		INC_ROLL(transferParam->TransferHandleWaitIndex, transferParam->Test->BufferCount);
	}

Done:
	return ret;
}

DWORD TransferThreadProc(PBENCHMARK_TRANSFER_PARAM transferParam)
{
	int ret, i;
	PBENCHMARK_TRANSFER_HANDLE handle;
	PUCHAR data;
	transferParam->IsRunning = TRUE;

	while (!transferParam->Test->IsCancelled)
	{
		data = NULL;
		handle = NULL;

		if (transferParam->Test->TransferMode == TRANSFER_MODE_SYNC)
		{
			ret = TransferSync(transferParam);
			if (ret >= 0) data = transferParam->Buffer;
		}
		else if (transferParam->Test->TransferMode == TRANSFER_MODE_ASYNC)
		{
			ret = TransferAsync(transferParam, &handle);
			if ((handle) && ret >= 0) data = handle->Data;
		}
		else
		{
			CONERR("invalid transfer mode %d\n", transferParam->Test->TransferMode);
			goto Done;
		}
		if (ret < 0)
		{
			// The user pressed 'Q'.
			if (transferParam->Test->IsUserAborted) break;

			// Transfer timed out
			if (ret == ERROR_SEM_TIMEOUT ||
			        ret == ERROR_OPERATION_ABORTED)
			{
				transferParam->TotalTimeoutCount++;
				transferParam->RunningTimeoutCount++;
				CONWRN("Timeout #%d %s on Ep%02Xh..\n",
				       transferParam->RunningTimeoutCount,
				       TRANSFER_DISPLAY(transferParam, "reading", "writing"),
				       transferParam->Ep.PipeId);

				if (transferParam->RunningTimeoutCount > transferParam->Test->Retry)
					break;
			}
			else
			{
				// An error (other than a timeout) occured.
				transferParam->TotalErrorCount++;
				transferParam->RunningErrorCount++;
				CONERR("failed %s! %d of %d ret=%dn",
				       TRANSFER_DISPLAY(transferParam, "reading", "writing"),
				       transferParam->RunningErrorCount,
				       transferParam->Test->Retry + 1,
				       ret);

				K.ResetPipe(transferParam->Test->InterfaceHandle, transferParam->Ep.PipeId);

				if (transferParam->RunningErrorCount > transferParam->Test->Retry)
					break;

			}
			ret = 0;
		}
		else
		{
			if (ret < transferParam->Test->BufferSize && !transferParam->Test->IsCancelled)
			{
				if (ret > 0)
				{
					transferParam->ShortTransferCount++;
					CONWRN("Short transfer on Ep%02Xh expected %d got %d.\n",
					       transferParam->Ep.PipeId,
					       transferParam->Test->BufferSize,
					       ret);
				}
				else
				{
					CONWRN("Zero-length transfer on Ep%02Xh expected %d.\n",
					       transferParam->Ep.PipeId,
					       transferParam->Test->BufferSize);

					transferParam->TotalErrorCount++;
					transferParam->RunningErrorCount++;
					if (transferParam->RunningErrorCount > transferParam->Test->Retry)
						break;

					K.ResetPipe(transferParam->Test->InterfaceHandle, transferParam->Ep.PipeId);
				}
			}
			else
			{
				transferParam->RunningErrorCount = 0;
				transferParam->RunningTimeoutCount = 0;
			}

			if ((transferParam->Test->Verify) &&
			        (transferParam->Ep.PipeId & USB_ENDPOINT_DIRECTION_MASK))
			{
				VerifyData(transferParam, data, ret);
			}
		}

		EnterCriticalSection(&DisplayCriticalSection);

		if (!transferParam->StartTick && transferParam->Packets >= 0)
		{
			transferParam->StartTick = GetTickCount();
			transferParam->LastStartTick	= transferParam->StartTick;
			transferParam->LastTick			= transferParam->StartTick;

			transferParam->LastTransferred = 0;
			transferParam->TotalTransferred = 0;
			transferParam->Packets = 0;
		}
		else
		{
			if (!transferParam->LastStartTick)
			{
				transferParam->LastStartTick	= transferParam->LastTick;
				transferParam->LastTransferred = 0;
			}
			transferParam->LastTick			= GetTickCount();

			transferParam->LastTransferred  += ret;
			transferParam->TotalTransferred += ret;
			transferParam->Packets++;
		}

		LeaveCriticalSection(&DisplayCriticalSection);
	}

Done:

	for (i = 0; i < transferParam->Test->BufferCount; i++)
	{
		if (transferParam->TransferHandles[i].Overlapped.hEvent)
		{
			if (transferParam->TransferHandles[i].InUse)
			{
				if (!K.AbortPipe(
				            transferParam->Test->InterfaceHandle, transferParam->Ep.PipeId) &&
				        !transferParam->Test->IsUserAborted)
				{
					ret = WinError(0);
					CONERR("failed cancelling transfer! ret=%d\n", ret);
				}
			}
			Sleep(0);
		}
	}

	for (i = 0; i < transferParam->Test->BufferCount; i++)
	{
		if (transferParam->TransferHandles[i].Overlapped.hEvent)
		{
			WaitForSingleObject(transferParam->TransferHandles[i].Overlapped.hEvent, INFINITE);
			CloseHandle(transferParam->TransferHandles[i].Overlapped.hEvent);
			transferParam->TransferHandles[i].Overlapped.hEvent = NULL;
			transferParam->TransferHandles[i].InUse = FALSE;
		}
	}

	transferParam->IsRunning = FALSE;
	return 0;
}

char* GetParamStrValue(const char* src, const char* paramName)
{
	return (strstr(src, paramName) == src) ? (char*)(src + strlen(paramName)) : NULL;
}

BOOL GetParamIntValue(const char* src, const char* paramName, INT* returnValue)
{
	char* value = GetParamStrValue(src, paramName);
	if (value)
	{
		*returnValue = strtol(value, NULL, 0);
		return TRUE;
	}
	return FALSE;
}

int ValidateBenchmarkArgs(PBENCHMARK_TEST_PARAM test)
{
	if (test->BufferCount < 1 || test->BufferCount > MAX_OUTSTANDING_TRANSFERS)
	{
		CONERR("Invalid BufferCount argument %d. BufferCount must be greater than 0 and less than or equal to %d.\n",
		       test->BufferCount, MAX_OUTSTANDING_TRANSFERS);
		return -1;
	}

	return 0;
}

int ParseBenchmarkArgs(PBENCHMARK_TEST_PARAM testParams, int argc, char** argv)
{
#define GET_INT_VAL
	char arg[MAX_PATH];
	char* value;
	int iarg;

	for (iarg = 1; iarg < argc; iarg++)
	{
		if (strcpy_s(arg, _countof(arg), argv[iarg]) != ERROR_SUCCESS)
			return -1;

		_strlwr_s(arg, MAX_PATH);

		if      (GetParamIntValue(arg, "vid=", &testParams->Vid)) {}
		else if (GetParamIntValue(arg, "pid=", &testParams->Pid)) {}
		else if (GetParamIntValue(arg, "retry=", &testParams->Retry)) {}
		else if (GetParamIntValue(arg, "buffercount=", &testParams->BufferCount))
		{
			if (testParams->BufferCount > 1)
				testParams->TransferMode = TRANSFER_MODE_ASYNC;
		}
		else if (GetParamIntValue(arg, "buffersize=", &testParams->BufferSize)) {}
		else if (GetParamIntValue(arg, "size=", &testParams->BufferSize)) {}
		else if (GetParamIntValue(arg, "timeout=", &testParams->Timeout)) {}
		else if (GetParamIntValue(arg, "intf=", &testParams->Intf)) {}
		else if (GetParamIntValue(arg, "altf=", &testParams->Altf)) {}
		else if (GetParamIntValue(arg, "ep=", &testParams->Ep))
		{
			testParams->Ep &= 0xf;
		}
		else if (GetParamIntValue(arg, "refresh=", &testParams->Refresh)) {}
		else if (GetParamIntValue(arg, "isopacketsize=", &testParams->IsoPacketSize)) {}
		else if ((value = GetParamStrValue(arg, "mode=")) != NULL)
		{
			if (GetParamStrValue(value, "sync"))
			{
				testParams->TransferMode = TRANSFER_MODE_SYNC;
			}
			else if (GetParamStrValue(value, "async"))
			{
				testParams->TransferMode = TRANSFER_MODE_ASYNC;
			}
			else
			{
				// Invalid EndpointType argument.
				CONERR("invalid transfer mode argument! %s\n", argv[iarg]);
				return -1;

			}
		}
		else if ((value = GetParamStrValue(arg, "priority=")) != NULL)
		{
			if (GetParamStrValue(value, "lowest"))
			{
				testParams->Priority = THREAD_PRIORITY_LOWEST;
			}
			else if (GetParamStrValue(value, "belownormal"))
			{
				testParams->Priority = THREAD_PRIORITY_BELOW_NORMAL;
			}
			else if (GetParamStrValue(value, "normal"))
			{
				testParams->Priority = THREAD_PRIORITY_NORMAL;
			}
			else if (GetParamStrValue(value, "abovenormal"))
			{
				testParams->Priority = THREAD_PRIORITY_ABOVE_NORMAL;
			}
			else if (GetParamStrValue(value, "highest"))
			{
				testParams->Priority = THREAD_PRIORITY_HIGHEST;
			}
			else
			{
				CONERR("invalid priority argument! %s\n", argv[iarg]);
				return -1;
			}
		}
		else if (!_stricmp(arg, "notestselect"))
		{
			testParams->NoTestSelect = TRUE;
		}
		else if (!_stricmp(arg, "read"))
		{
			testParams->TestType = TestTypeRead;
		}
		else if (!_stricmp(arg, "write"))
		{
			testParams->TestType = TestTypeWrite;
		}
		else if (!_stricmp(arg, "loop"))
		{
			testParams->TestType = TestTypeLoop;
		}
		else if (!_stricmp(arg, "list"))
		{
			testParams->UseList = TRUE;
		}
		else if (!_stricmp(arg, "verifydetails"))
		{
			testParams->VerifyDetails = TRUE;
			testParams->Verify = TRUE;
		}
		else if (!_stricmp(arg, "verify"))
		{
			testParams->Verify = TRUE;
		}
		else
		{
			CONERR("invalid argument! %s\n", argv[iarg]);
			return -1;
		}
	}
	return ValidateBenchmarkArgs(testParams);
}

INT CreateVerifyBuffer(PBENCHMARK_TEST_PARAM test, WORD endpointMaxPacketSize)
{
	int i;
	BYTE indexC = 0;
	test->VerifyBuffer = malloc(endpointMaxPacketSize);
	if (!test->VerifyBuffer)
	{
		CONERR("memory allocation failure at line %d!\n", __LINE__);
		return -1;
	}

	test->VerifyBufferSize = endpointMaxPacketSize;

	for(i = 0; i < endpointMaxPacketSize; i++)
	{
		test->VerifyBuffer[i] = indexC++;
		if (indexC == 0) indexC = 1;
	}

	return 0;
}

void FreeTransferParam(PBENCHMARK_TRANSFER_PARAM* testTransferRef)
{
	PBENCHMARK_TRANSFER_PARAM pTransferParam;

	if ((!testTransferRef) || !*testTransferRef) return;
	pTransferParam = *testTransferRef;

	if (pTransferParam->ThreadHandle)
	{
		CloseHandle(pTransferParam->ThreadHandle);
		pTransferParam->ThreadHandle = NULL;
	}

	free(pTransferParam);

	*testTransferRef = NULL;
}

PBENCHMARK_TRANSFER_PARAM CreateTransferParam(PBENCHMARK_TEST_PARAM test, int endpointID)
{
	PBENCHMARK_TRANSFER_PARAM transferParam;
	int i;
	int allocSize = sizeof(BENCHMARK_TRANSFER_PARAM) + (test->BufferSize * test->BufferCount);

	transferParam = (PBENCHMARK_TRANSFER_PARAM) malloc(allocSize);

	if (transferParam)
	{
		memset(transferParam, 0, allocSize);
		transferParam->Test = test;

		for(i = 0; i < test->InterfaceDescriptor.bNumEndpoints; i++)
		{
			if (!(endpointID & USB_ENDPOINT_ADDRESS_MASK))
			{
				// Use first endpoint that matches the direction
				if ((test->PipeInformation[i].PipeId & USB_ENDPOINT_DIRECTION_MASK) == endpointID)
				{
					memcpy(&transferParam->Ep, &test->PipeInformation[i], sizeof(transferParam->Ep));
					break;
				}
			}
			else
			{
				if ((int)test->PipeInformation[i].PipeId == endpointID)
				{
					memcpy(&transferParam->Ep, &test->PipeInformation[i], sizeof(transferParam->Ep));
					break;
				}
			}
		}
		if (!transferParam->Ep.PipeId)
		{
			CONERR("failed locating EP%02Xh!\n", endpointID);
			FreeTransferParam(&transferParam);
			goto Done;
		}

		if (!transferParam->Ep.MaximumPacketSize)
		{
			CONERR("MaximumPacketSize=0 for EP%02Xh. check alternate settings.\n", transferParam->Ep.PipeId);
			FreeTransferParam(&transferParam);
			goto Done;
		}
		if (transferParam->Test->BufferSize % transferParam->Ep.MaximumPacketSize)
		{
			CONERR("buffer size %d is not an interval of EP%02Xh maximum packet size of %d!\n",
			       transferParam->Test->BufferSize,
			       transferParam->Ep.PipeId,
			       transferParam->Ep.MaximumPacketSize);

			FreeTransferParam(&transferParam);
			goto Done;
		}

		if (test->IsoPacketSize)
			transferParam->IsoPacketSize = test->IsoPacketSize;
		else
			transferParam->IsoPacketSize = transferParam->Ep.MaximumPacketSize;

		if (ENDPOINT_TYPE(transferParam) == USB_ENDPOINT_TYPE_ISOCHRONOUS)
			transferParam->Test->TransferMode = TRANSFER_MODE_ASYNC;

		ResetRunningStatus(transferParam);

		transferParam->ThreadHandle = CreateThread(
		                                  NULL,
		                                  0,
		                                  (LPTHREAD_START_ROUTINE)TransferThreadProc,
		                                  transferParam,
		                                  CREATE_SUSPENDED,
		                                  &transferParam->ThreadID);

		if (!transferParam->ThreadHandle)
		{
			CONERR0("failed creating thread!\n");
			FreeTransferParam(&transferParam);
			goto Done;
		}

		// If verify mode is on, this is a loop test, and this is a write endpoint, fill
		// the buffers with the same test data sent by a benchmark device when running
		// a read only test.
		if (transferParam->Test->Verify &&
		        transferParam->Test->TestType == TestTypeLoop &&
		        !(transferParam->Ep.PipeId & USB_ENDPOINT_DIRECTION_MASK))
		{
			// Data Format:
			// [0][KeyByte] 2 3 4 5 ..to.. wMaxPacketSize (if data byte rolls it is incremented to 1)
			// Increment KeyByte and repeat
			//
			BYTE indexC = 0;
			INT bufferIndex = 0;
			WORD dataIndex;
			INT packetIndex;
			INT packetCount = ((transferParam->Test->BufferCount * transferParam->Test->BufferSize) / transferParam->Ep.MaximumPacketSize);
			for(packetIndex = 0; packetIndex < packetCount; packetIndex++)
			{
				indexC = 2;
				for (dataIndex = 0; dataIndex < transferParam->Ep.MaximumPacketSize; dataIndex++)
				{
					if (dataIndex == 0)			// Start
						transferParam->Buffer[bufferIndex] = 0;
					else if (dataIndex == 1)	// Key
						transferParam->Buffer[bufferIndex] = packetIndex & 0xFF;
					else						// Data
						transferParam->Buffer[bufferIndex] = indexC++;

					// if wMaxPacketSize is > 255, indexC resets to 1.
					if (indexC == 0) indexC = 1;

					bufferIndex++;
				}
			}
		}
	}

Done:
	if (!transferParam)
		CONERR0("failed creating transfer param!\n");

	return transferParam;
}

void GetAverageBytesSec(PBENCHMARK_TRANSFER_PARAM transferParam, DOUBLE* bps)
{
	DOUBLE ticksSec;
	if ((!transferParam->StartTick) ||
	        (transferParam->StartTick >= transferParam->LastTick) ||
	        transferParam->TotalTransferred == 0)
	{
		*bps = 0;
	}
	else
	{
		ticksSec = (transferParam->LastTick - transferParam->StartTick) / 1000.0;
		*bps = (transferParam->TotalTransferred / ticksSec);
	}
}

void GetCurrentBytesSec(PBENCHMARK_TRANSFER_PARAM transferParam, DOUBLE* bps)
{
	DOUBLE ticksSec;
	if ((!transferParam->StartTick) ||
	        (!transferParam->LastStartTick) ||
	        (transferParam->LastTick <= transferParam->LastStartTick) ||
	        transferParam->LastTransferred == 0)
	{
		*bps = 0;
	}
	else
	{
		ticksSec = (transferParam->LastTick - transferParam->LastStartTick) / 1000.0;
		*bps = transferParam->LastTransferred / ticksSec;
	}
}

void ShowRunningStatus(PBENCHMARK_TRANSFER_PARAM transferParam)
{
	BENCHMARK_TRANSFER_PARAM temp;
	DOUBLE bpsOverall;
	DOUBLE bpsLastTransfer;

	// LOCK the display critical section
	EnterCriticalSection(&DisplayCriticalSection);

	memcpy(&temp, transferParam, sizeof(BENCHMARK_TRANSFER_PARAM));

	// UNLOCK the display critical section
	LeaveCriticalSection(&DisplayCriticalSection);

	if ((!temp.StartTick) || (temp.StartTick >= temp.LastTick))
	{
		CONMSG("Synchronizing %d..\n", abs(transferParam->Packets));
	}
	else
	{
		GetAverageBytesSec(&temp, &bpsOverall);
		GetCurrentBytesSec(&temp, &bpsLastTransfer);
		transferParam->LastStartTick = 0;
		CONMSG("Avg. Bytes/s: %.2f Transfers: %d Bytes/s: %.2f\n",
		       bpsOverall, temp.Packets, bpsLastTransfer);
	}

}
void ShowTransferInfo(PBENCHMARK_TRANSFER_PARAM transferParam)
{
	DOUBLE bpsAverage;
	DOUBLE bpsCurrent;
	DOUBLE elapsedSeconds;

	if (!transferParam) return;

	CONMSG("%s %s (Ep%02Xh) max packet size: %d\n",
	       EndpointTypeDisplayString[ENDPOINT_TYPE(transferParam)],
	       TRANSFER_DISPLAY(transferParam, "Read", "Write"),
	       transferParam->Ep.PipeId,
	       transferParam->Ep.MaximumPacketSize);

	if (transferParam->StartTick)
	{
		GetAverageBytesSec(transferParam, &bpsAverage);
		GetCurrentBytesSec(transferParam, &bpsCurrent);
		CONMSG("\tTotal Bytes     : %I64d\n", transferParam->TotalTransferred);
		CONMSG("\tTotal Transfers : %d\n", transferParam->Packets);

		if (transferParam->ShortTransferCount)
		{
			CONMSG("\tShort Transfers : %d\n", transferParam->ShortTransferCount);
		}
		if (transferParam->TotalTimeoutCount)
		{
			CONMSG("\tTimeout Errors  : %d\n", transferParam->TotalTimeoutCount);
		}
		if (transferParam->TotalErrorCount)
		{
			CONMSG("\tOther Errors    : %d\n", transferParam->TotalErrorCount);
		}

		CONMSG("\tAvg. Bytes/sec  : %.2f\n", bpsAverage);

		if (transferParam->StartTick && transferParam->StartTick < transferParam->LastTick)
		{
			elapsedSeconds = (transferParam->LastTick - transferParam->StartTick) / 1000.0;

			CONMSG("\tElapsed Time    : %.2f seconds\n", elapsedSeconds);
		}

		CONMSG0("\n");
	}

}

void ShowTestInfo(PBENCHMARK_TEST_PARAM test)
{
	if (!test) return;

	CONMSG("%s Test Information\n", TestDisplayString[test->TestType & 3]);
	CONMSG("\tDriver          : %s\n", GetDrvIdString(test->SelectedDeviceProfile->DrvId));
	CONMSG("\tVid / Pid       : %04Xh / %04Xh\n", test->DeviceDescriptor.idVendor,  test->DeviceDescriptor.idProduct);
	CONMSG("\tDevicePath      : %s\n", test->SelectedDeviceProfile->DevicePath);
	CONMSG("\tInterface #     : %02Xh\n", test->InterfaceDescriptor.bInterfaceNumber);
	CONMSG("\tAlt Interface # : %02Xh\n", test->InterfaceDescriptor.bAlternateSetting);
	CONMSG("\tNum Endpoints   : %u\n", test->InterfaceDescriptor.bNumEndpoints);
	CONMSG("\tPriority        : %d\n", test->Priority);
	CONMSG("\tBuffer Size     : %d\n", test->BufferSize);
	CONMSG("\tBuffer Count    : %d\n", test->BufferCount);
	CONMSG("\tDisplay Refresh : %d (ms)\n", test->Refresh);
	CONMSG("\tTransfer Timeout: %d (ms)\n", test->Timeout);
	CONMSG("\tRetry Count     : %d\n", test->Retry);
	CONMSG("\tVerify Data     : %s%s\n",
	       test->Verify ? "On" : "Off",
	       (test->Verify && test->VerifyDetails) ? " (Detailed)" : "");

	CONMSG0("\n");
}

void WaitForTestTransfer(PBENCHMARK_TRANSFER_PARAM transferParam)
{
	DWORD exitCode;
	while (transferParam)
	{
		if (!transferParam->IsRunning)
		{
			if (GetExitCodeThread(transferParam->ThreadHandle, &exitCode))
			{
				if (exitCode == 0)
				{
					CONMSG("stopped Ep%02Xh thread.\tExitCode=%d\n",
					       transferParam->Ep.PipeId, exitCode);
					break;
				}
			}
			else
			{
				CONERR("failed getting Ep%02Xh thread exit code!\n", transferParam->Ep.PipeId);
				break;
			}
		}
		Sleep(100);
		CONMSG("waiting for Ep%02Xh thread..\n", transferParam->Ep.PipeId);
	}
}
void ResetRunningStatus(PBENCHMARK_TRANSFER_PARAM transferParam)
{
	if (!transferParam) return;

	transferParam->StartTick = 0;
	transferParam->TotalTransferred = 0;
	transferParam->Packets = -2;
	transferParam->LastTick = 0;
	transferParam->RunningTimeoutCount = 0;
}

int GetTestDeviceFromArgs(PBENCHMARK_TEST_PARAM test)
{
	CHAR id[MAX_PATH];
	PKUSB_DEV_LIST list = test->DeviceList;
	while (list)
	{
		int vid = -1;
		int pid = -1;
		int mi = -1;
		memset(id, 0, sizeof(id));
		strcpy_s(id, MAX_PATH - 1, list->DeviceInstance);
		_strlwr_s(id, MAX_PATH);

		sscanf_s(id, "vid_%04x", vid);
		sscanf_s(id, "pid_%04x", vid);
		sscanf_s(id, "mi_%02x", mi);

		if (test->Vid == vid && test->Pid == pid)
			list->My.Byte[0] = TRUE;
		else
			list->My.Byte[0] = FALSE;

		list = list->next;
	}

	return ERROR_SUCCESS;
}

int GetTestDeviceFromList(PBENCHMARK_TEST_PARAM test)
{
	UCHAR selection;
	UCHAR count = 0;
	PKUSB_DEV_LIST list = test->DeviceList;
	while (list && count < 9)
	{
		CONMSG("%u. %s (%s) [%s]\n", count + 1, list->DeviceDesc, list->DeviceInstance, GetDrvIdString(list->DrvId));
		list->My.Byte[0] = FALSE;
		count++;
		list = list->next;
	}
	if (!count)
	{
		CONERR("%04Xh:%04Xh device not found\n", test->Vid, test->Pid);
		return -1;
	}

	CONMSG("Select device (1-%u) :", count);
	while(_kbhit()) _getch();

	selection = (CHAR)_getche();
	selection -= (UCHAR)'0';
	CONMSG0("\n\n");

	if (selection > 0 && selection <= count)
	{
		count = 0;
		list = test->DeviceList;

		while (list && ++count != selection)
			list = list->next;

		if (!list)
		{
			CONERR("unknown selection\n");
			return -1;
		}
		list->My.Byte[0] = TRUE;

		return ERROR_SUCCESS;
	}

	return -1;
}

int __cdecl main(int argc, char** argv)
{
	BENCHMARK_TEST_PARAM Test;
	PBENCHMARK_TRANSFER_PARAM ReadTest	= NULL;
	PBENCHMARK_TRANSFER_PARAM WriteTest	= NULL;
	int key;
	LONG ec;


	if (argc == 1)
	{
		ShowHelp();
		return -1;
	}

	ShowCopyright();

	SetTestDefaults(&Test);

	// Load the command line arguments.
	if (ParseBenchmarkArgs(&Test, argc, argv) < 0)
		return -1;

	// Initialize the critical section used for locking
	// the volatile members of the transfer params in order
	// to update/modify the running statistics.
	//
	InitializeCriticalSection(&DisplayCriticalSection);
	ec = LUsbK_GetDeviceList(&Test.DeviceList, NULL);
	if (ec < 0)
	{
		CONERR("failed getting device list ec=\n", ec);
		goto Done;
	}
	if (ec == 0)
	{
		CONERR("device list empty.\n", ec);
		goto Done;
	}


	if (Test.UseList)
	{
		if (GetTestDeviceFromList(&Test) < 0)
			goto Done;
	}
	else
	{
		if (GetTestDeviceFromArgs(&Test) < 0)
			goto Done;
	}

	if (!Bench_Open(&Test))
	{
		goto Done;
	}

	// If "NoTestSelect" appears in the command line then don't send the control
	// messages for selecting the test type.
	//
	if (!Test.NoTestSelect)
	{
		if (Bench_SetTestType(Test.InterfaceHandle, Test.TestType, Test.Intf) != 1)
		{
			CONERR("setting bechmark test type #%d!\n\n", Test.TestType);
			goto Done;
		}
	}

	CONMSG("opened %s (%s)..\n", Test.DeviceList->DeviceDesc, Test.DeviceList->DeviceInstance);

	// If reading from the device create the read transfer param. This will also create
	// a thread in a suspended state.
	//
	if (Test.TestType & TestTypeRead)
	{
		ReadTest = CreateTransferParam(&Test, Test.Ep | USB_ENDPOINT_DIRECTION_MASK);
		if (!ReadTest) goto Done;
	}

	// If writing to the device create the write transfer param. This will also create
	// a thread in a suspended state.
	//
	if (Test.TestType & TestTypeWrite)
	{
		WriteTest = CreateTransferParam(&Test, Test.Ep);
		if (!WriteTest) goto Done;
	}

	if (Test.Verify)
	{
		if (ReadTest && WriteTest)
		{
			if (CreateVerifyBuffer(&Test, WriteTest->Ep.MaximumPacketSize) < 0)
				goto Done;
		}
		else if (ReadTest)
		{
			if (CreateVerifyBuffer(&Test, ReadTest->Ep.MaximumPacketSize) < 0)
				goto Done;
		}
	}

	ShowTestInfo(&Test);
	ShowTransferInfo(ReadTest);
	ShowTransferInfo(WriteTest);

	CONMSG0("\nWhile the test is running:\n");
	CONMSG0("Press 'Q' to quit\n");
	CONMSG0("Press 'T' for test details\n");
	CONMSG0("Press 'I' for status information\n");
	CONMSG0("Press 'R' to reset averages\n");
	CONMSG0("\nPress 'Q' to exit, any other key to begin..");
	key = _getch();
	CONMSG0("\n");

	if (key == 'Q' || key == 'q') goto Done;

	// Set the thread priority and start it.
	if (ReadTest)
	{
		SetThreadPriority(ReadTest->ThreadHandle, Test.Priority);
		ResumeThread(ReadTest->ThreadHandle);
	}

	// Set the thread priority and start it.
	if (WriteTest)
	{
		SetThreadPriority(WriteTest->ThreadHandle, Test.Priority);
		ResumeThread(WriteTest->ThreadHandle);
	}

	while (!Test.IsCancelled)
	{
		Sleep(Test.Refresh);

		if (_kbhit())
		{
			// A key was pressed.
			key = _getch();
			switch (key)
			{
			case 'Q':
			case 'q':
				Test.IsUserAborted = TRUE;
				Test.IsCancelled = TRUE;
				break;
			case 'T':
			case 't':
				ShowTestInfo(&Test);
				break;
			case 'I':
			case 'i':
				// LOCK the display critical section
				EnterCriticalSection(&DisplayCriticalSection);

				// Print benchmark test details.
				ShowTransferInfo(ReadTest);
				ShowTransferInfo(WriteTest);


				// UNLOCK the display critical section
				LeaveCriticalSection(&DisplayCriticalSection);
				break;

			case 'R':
			case 'r':
				// LOCK the display critical section
				EnterCriticalSection(&DisplayCriticalSection);

				// Reset the running status.
				ResetRunningStatus(ReadTest);
				ResetRunningStatus(WriteTest);

				// UNLOCK the display critical section
				LeaveCriticalSection(&DisplayCriticalSection);
				break;
			}

			// Only one key at a time.
			while (_kbhit()) _getch();
		}

		// If the read test should be running and it isn't, cancel the test.
		if ((ReadTest) && !ReadTest->IsRunning)
		{
			Test.IsCancelled = TRUE;
			break;
		}

		// If the write test should be running and it isn't, cancel the test.
		if ((WriteTest) && !WriteTest->IsRunning)
		{
			Test.IsCancelled = TRUE;
			break;
		}

		// Print benchmark stats
		if (ReadTest)
			ShowRunningStatus(ReadTest);
		else
			ShowRunningStatus(WriteTest);

	}

	// Wait for the transfer threads to complete gracefully if it
	// can be done in 10ms. All of the code from this point to
	// WaitForTestTransfer() is not required.  It is here only to
	// improve response time when the test is cancelled.
	//
	Sleep(10);

	// If the thread is still running, abort and reset the endpoint.
	if ((ReadTest) && ReadTest->IsRunning)
		K.AbortPipe(Test.InterfaceHandle, ReadTest->Ep.PipeId);

	// If the thread is still running, abort and reset the endpoint.
	if ((WriteTest) && WriteTest->IsRunning)
		K.AbortPipe(Test.InterfaceHandle, WriteTest->Ep.PipeId);

	// Small delay incase usb_resetep() was called.
	Sleep(10);

	// WaitForTestTransfer will not return until the thread
	// has exited.
	WaitForTestTransfer(ReadTest);
	WaitForTestTransfer(WriteTest);

	// Print benchmark detailed stats
	ShowTestInfo(&Test);
	if (ReadTest) ShowTransferInfo(ReadTest);
	if (WriteTest) ShowTransferInfo(WriteTest);


Done:
	if (Test.InterfaceHandle)
	{
		K.Free(Test.InterfaceHandle);
		Test.InterfaceHandle = NULL;
	}
	if (Test.DeviceHandle)
	{
		CloseHandle(Test.DeviceHandle);
		Test.DeviceHandle = NULL;
	}
	if (Test.VerifyBuffer)
	{
		free(Test.VerifyBuffer);
		Test.VerifyBuffer = NULL;

	}

	LUsbK_FreeDeviceList(&Test.DeviceList);
	FreeTransferParam(&ReadTest);
	FreeTransferParam(&WriteTest);

	DeleteCriticalSection(&DisplayCriticalSection);

	CONMSG0("Press any key to exit..");
	_getch();
	CONMSG0("\n");

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
/* END OF PROGRAM                                                           */
//////////////////////////////////////////////////////////////////////////////
void ShowHelp(void)
{
#define ID_HELP_TEXT  10020
#define ID_DOS_TEXT   300

	CONST CHAR* src;
	DWORD src_count, charsWritten;
	HGLOBAL res_data;
	HANDLE handle;
	HRSRC hSrc;

	ShowCopyright();

	hSrc = FindResourceA(NULL, MAKEINTRESOURCEA(ID_HELP_TEXT), MAKEINTRESOURCEA(ID_DOS_TEXT));
	if (!hSrc)	return;

	src_count = SizeofResource(NULL, hSrc);

	res_data = LoadResource(NULL, hSrc);
	if (!res_data)	return;

	src = (char*) LockResource(res_data);
	if (!src) return;

	if ((handle = GetStdHandle(STD_ERROR_HANDLE)) != INVALID_HANDLE_VALUE)
		WriteConsoleA(handle, src, src_count, &charsWritten, NULL);
}

void ShowCopyright(void)
{
	CONMSG("%s v%s (%s)\n",
	       RC_FILENAME_STR,
	       RC_VERSION_STR,
	       DEFINE_TO_STR(VERSION_DATE));
	CONMSG0("Copyright (c) 2011 Travis Robinson. <libusbdotnet@gmail.com>\n");
}
