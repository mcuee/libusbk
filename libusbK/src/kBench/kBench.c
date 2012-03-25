/*!********************************************************************
libusbK - kBench USB benchmark/diagnostic tool.
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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <wtypes.h>

#include "libusbk.h"
#include "lusbk_version.h"
#include "lusbk_linked_list.h"

// warning C4127: conditional expression is constant.
#pragma warning(disable: 4127)

#define COMPOSITE_MERGE_MODE 1

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

static LPCSTR DrvIdNames[8] = {"libusbK", "libusb0", "WinUSB", "libusb0 filter", "Unknown", "Unknown", "Unknown"};
#define GetDrvIdString(DriverID)	(DrvIdNames[((((LONG)(DriverID))<0) || ((LONG)(DriverID)) >= KUSB_DRVID_COUNT)?KUSB_DRVID_COUNT:(DriverID)])

static LPCSTR DevSpeedStrings[4] = {"Unknown", "Low/Full", "Unknown", "High"};
#define GetDevSpeedString(DevSpeed)	(DevSpeedStrings[(DevSpeed) & 0x3])

#define VerifyListLock(mTest) while(InterlockedExchange(&((mTest)->VerifyLock),1) != 0) Sleep(0)
#define VerifyListUnlock(mTest) InterlockedExchange(&((mTest)->VerifyLock),0)

KUSB_DRIVER_API K;

typedef struct _BENCHMARK_BUFFER
{
	PUCHAR	Data;
	LONG	DataLength;
	LONG	SyncFailed;

	struct _BENCHMARK_BUFFER* prev;
	struct _BENCHMARK_BUFFER* next;
} BENCHMARK_BUFFER, *PBENCHMARK_BUFFER;
// Custom vendor requests that must be implemented in the benchmark firmware.
// Test selection can be bypassed with the "notestselect" argument.
//
typedef enum _BENCHMARK_DEVICE_COMMAND
{
    SET_TEST = 0x0E,
    GET_TEST = 0x0F,
} BENCHMARK_DEVICE_COMMAND, *PBENCHMARK_DEVICE_COMMAND;

// Tests supported by the official benchmark firmware.
//
typedef enum _BENCHMARK_DEVICE_TEST_TYPE
{
    TestTypeNone	= 0x00,
    TestTypeRead	= 0x01,
    TestTypeWrite	= 0x02,
    TestTypeLoop	= TestTypeRead | TestTypeWrite,
} BENCHMARK_DEVICE_TEST_TYPE, *PBENCHMARK_DEVICE_TEST_TYPE;

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
	INT AllocBufferSize;		// Number of bytes to transfer
	INT ReadLength;
	INT WriteLength;
	INT BufferCount;	// Number of outstanding asynchronous transfers
	BOOL NoTestSelect;	// If true, don't send control message to select the test type.
	BOOL UseList;		// Show the user a device list and let them choose a benchmark device.
	INT FixedIsoPackets; // Number of fixed packets to use for ISO writes. Ignored for read eps.
	INT Priority;		// Priority to run this thread at.
	BOOL Verify;		// Only for loop and read test. If true, verifies data integrity.
	BOOL VerifyDetails;	// If true, prints detailed information for each invalid byte.
	enum BENCHMARK_DEVICE_TEST_TYPE TestType;	// The benchmark test type.
	enum BENCHMARK_TRANSFER_MODE TransferMode;	// Sync or Async

	// Internal value use during the test.
	//
	KLST_HANDLE DeviceList;
	KLST_DEVINFO_HANDLE SelectedDeviceProfile;
	HANDLE DeviceHandle;
	KUSB_HANDLE InterfaceHandle;
	USB_DEVICE_DESCRIPTOR DeviceDescriptor;
	USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
	WINUSB_PIPE_INFORMATION PipeInformation[32];
	BOOL IsCancelled;
	BOOL IsUserAborted;

	BYTE* VerifyBuffer;		// Stores the verify test pattern for 1 packet.
	WORD VerifyBufferSize;	// Size of VerifyBuffer
	BOOL Use_UsbK_Init;
	BOOL ListDevicesOnly;
	ULONG DeviceSpeed;

	BOOL ReadLogEnabled;
	FILE* ReadLogFile;

	BOOL WriteLogEnabled;
	FILE* WriteLogFile;

	volatile long VerifyLock;
	BENCHMARK_BUFFER* VerifyList;
	BOOL IsLoopSynced;

	UCHAR UseRawIO;

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

#include <pshpack1.h>
typedef struct _KBENCH_CONTEXT_LSTK
{
	BYTE Selected;
} KBENCH_CONTEXT_LSTK, *PKBENCH_CONTEXT_LSTK;
#include <poppack.h>

#pragma warning(default:4200)

// Benchmark device api.
BOOL Bench_Open(__in PBENCHMARK_TEST_PARAM test);

BOOL Bench_Configure(__in KUSB_HANDLE handle,
                     __in BENCHMARK_DEVICE_COMMAND command,
                     __in UCHAR intf,
                     __deref_inout PBENCHMARK_DEVICE_TEST_TYPE testType);

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
		SetLastError(0);
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

	test->Ep				= 0x00;
	test->Vid				= 0x04D8;
	test->Pid				= 0xFA2E;
	test->Refresh			= 1000;
	test->Timeout			= 5000;
	test->TestType			= TestTypeLoop;
	test->AllocBufferSize	= 4096;
	test->ReadLength		= test->AllocBufferSize;
	test->WriteLength		= test->AllocBufferSize;
	test->BufferCount		= 1;
	test->Priority			= THREAD_PRIORITY_NORMAL;
	test->Intf				= -1;
	test->Altf				= -1;
	test->UseRawIO			= 0xFF;
}

VOID AppendLoopBuffer(PBENCHMARK_TEST_PARAM Test, PUCHAR data, LONG dataLength)
{
	if (Test->Verify && Test->TestType == TestTypeLoop)
	{
		BENCHMARK_BUFFER* newVerifyBuf = malloc(sizeof(BENCHMARK_BUFFER) + dataLength);

		memset(newVerifyBuf, 0, sizeof(BENCHMARK_BUFFER));

		newVerifyBuf->Data = &(((PUCHAR)newVerifyBuf)[sizeof(BENCHMARK_BUFFER)]);
		newVerifyBuf->DataLength = dataLength;
		memcpy(newVerifyBuf->Data, data, dataLength);

		VerifyListLock(Test);
		DL_APPEND(Test->VerifyList, newVerifyBuf);
		VerifyListUnlock(Test);
	}
}

BOOL Bench_Open(__in PBENCHMARK_TEST_PARAM test)
{
	UCHAR altSetting;
	KUSB_HANDLE associatedHandle;
	UINT transferred;
	KLST_DEVINFO_HANDLE deviceInfo;

	test->SelectedDeviceProfile = NULL;

	LstK_MoveReset(test->DeviceList);

	while (LstK_MoveNext(test->DeviceList, &deviceInfo))
	{
		// enabled
		UINT userContext = (UINT)LibK_GetContext(deviceInfo, KLIB_HANDLE_TYPE_LSTINFOK);
		if (userContext != TRUE) continue;

		if (!LibK_LoadDriverAPI(&K, deviceInfo->DriverID))
		{
			WinError(0);
			CONWRN("could not load driver api %s.\n", GetDrvIdString(deviceInfo->DriverID));
			continue;
		}
		if (!test->Use_UsbK_Init)
		{
			test->DeviceHandle = CreateFileA(deviceInfo->DevicePath,
			                                 GENERIC_READ | GENERIC_WRITE,
			                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
			                                 NULL,
			                                 OPEN_EXISTING,
			                                 FILE_FLAG_OVERLAPPED,
			                                 NULL);

			if (!test->DeviceHandle || test->DeviceHandle == INVALID_HANDLE_VALUE)
			{
				WinError(0);
				test->DeviceHandle = NULL;
				CONWRN("could not create device handle.\n%s\n", deviceInfo->DevicePath);
				continue;
			}

			if (!K.Initialize(test->DeviceHandle, &test->InterfaceHandle))
			{
				WinError(0);
				CloseHandle(test->DeviceHandle);
				test->DeviceHandle = NULL;
				test->InterfaceHandle = NULL;
				CONWRN("could not initialize device.\n%s\n", deviceInfo->DevicePath);
				continue;
			}
		}
		else
		{
			if (!K.Init(&test->InterfaceHandle, deviceInfo))
			{
				WinError(0);
				test->DeviceHandle = NULL;
				test->InterfaceHandle = NULL;
				CONWRN("could not open device.\n%s\n", deviceInfo->DevicePath);
				continue;
			}

		}

		if (!K.GetDescriptor(test->InterfaceHandle,
		                     USB_DESCRIPTOR_TYPE_DEVICE,
		                     0, 0,
		                     (PUCHAR)&test->DeviceDescriptor,
		                     sizeof(test->DeviceDescriptor),
		                     &transferred))
		{
			WinError(0);

			K.Free(test->InterfaceHandle);
			test->InterfaceHandle = NULL;

			if (!test->Use_UsbK_Init)
			{
				CloseHandle(test->DeviceHandle);
				test->DeviceHandle = NULL;
			}

			CONWRN("could not get device descriptor.\n%s\n", deviceInfo->DevicePath);
			continue;
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
			int hasIsoEndpoints = 0;
			int hasZeroMaxPacketEndpoints = 0;

			memset(&test->PipeInformation, 0, sizeof(test->PipeInformation));
			while(K.QueryPipe(test->InterfaceHandle, altSetting, pipeIndex, &test->PipeInformation[pipeIndex]))
			{
				// found a pipe
				if (test->PipeInformation[pipeIndex].PipeType == UsbdPipeTypeIsochronous)
					hasIsoEndpoints++;

				if (!test->PipeInformation[pipeIndex].MaximumPacketSize)
					hasZeroMaxPacketEndpoints++;

				pipeIndex++;
			}

			// -1 means the user din't specifiy so we find the most suitable device.
			//
			if ( (( test->Intf == -1) || (test->Intf == test->InterfaceDescriptor.bInterfaceNumber)) &&
			        (( test->Altf == -1) || (test->Altf == test->InterfaceDescriptor.bAlternateSetting)) )
			{
				// if the user actually specifies an alt iso setting with zero MaxPacketEndpoints
				// we let let them.
				if (test->Altf == -1 && hasIsoEndpoints && hasZeroMaxPacketEndpoints)
				{
					// user didn't specfiy and we know we can't tranfer with this alt setting so skip it.
					CONMSG("skipping interface %02X:%02X. zero-length iso endpoints exist.\n",
					       test->InterfaceDescriptor.bInterfaceNumber,
					       test->InterfaceDescriptor.bAlternateSetting);
				}
				else
				{
					// this is the one we are looking for.
					test->Intf = test->InterfaceDescriptor.bInterfaceNumber;
					test->Altf = test->InterfaceDescriptor.bAlternateSetting;
					test->SelectedDeviceProfile = deviceInfo;

					// some buffering is required for iso.
					if (hasIsoEndpoints && test->BufferCount == 1)
						test->BufferCount++;

					if (!K.SetCurrentAlternateSetting(test->InterfaceHandle, test->InterfaceDescriptor.bAlternateSetting))
					{
						CONERR("failed selecting alternate setting %02Xh\n", test->Altf);
						return FALSE;
					}
					return TRUE;
				}
			}
			altSetting++;
			memset(&test->InterfaceDescriptor, 0, sizeof(test->InterfaceDescriptor));
		}
		if (K.GetAssociatedInterface(test->InterfaceHandle, 0, &associatedHandle))
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

BOOL Bench_Configure(__in KUSB_HANDLE handle,
                     __in BENCHMARK_DEVICE_COMMAND command,
                     __in UCHAR intf,
                     __deref_inout PBENCHMARK_DEVICE_TEST_TYPE testType)
{
	UCHAR buffer[1];
	UINT transferred = 0;
	WINUSB_SETUP_PACKET Pkt;
	KUSB_SETUP_PACKET* defPkt = (KUSB_SETUP_PACKET*)&Pkt;

	memset(&Pkt, 0, sizeof(Pkt));
	defPkt->BmRequest.Dir = BMREQUEST_DIR_DEVICE_TO_HOST;
	defPkt->BmRequest.Type = BMREQUEST_TYPE_VENDOR;
	defPkt->Request = (UCHAR)command;
	defPkt->Value = (UCHAR) * testType;
	defPkt->Index = intf;
	defPkt->Length = 1;

	if (!handle || handle == INVALID_HANDLE_VALUE)
		return WinError(ERROR_INVALID_HANDLE);

	if (K.ControlTransfer(handle, Pkt, buffer, 1, &transferred, NULL))
	{
		if (transferred)
			return TRUE;
	}

	return WinError(0);
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

			CONVDAT("Packet=#%d Data=#%d\n", packetIndex, dataIndex);

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
	UINT transferred;
	BOOL success;
	if (transferParam->Ep.PipeId & USB_ENDPOINT_DIRECTION_MASK)
	{
		success = K.ReadPipe(transferParam->Test->InterfaceHandle,
		                     transferParam->Ep.PipeId,
		                     transferParam->Buffer,
		                     transferParam->Test->ReadLength,
		                     &transferred,
		                     NULL);
	}
	else
	{
		AppendLoopBuffer(transferParam->Test, transferParam->Buffer, transferParam->Test->WriteLength);
		success = K.WritePipe(transferParam->Test->InterfaceHandle,
		                      transferParam->Ep.PipeId,
		                      transferParam->Buffer,
		                      transferParam->Test->WriteLength,
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
	DWORD transferErrorCode;

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
			handle->Data = transferParam->Buffer + (transferParam->TransferHandleNextIndex * transferParam->Test->AllocBufferSize);
		}
		else
		{
			// re-initialize and re-use the overlapped
			HANDLE h = handle->Overlapped.hEvent;
			ResetEvent(h);
			memset(&handle->Overlapped, 0, sizeof(handle->Overlapped));
			handle->Overlapped.hEvent = h;
		}

		if (transferParam->Ep.PipeId & USB_ENDPOINT_DIRECTION_MASK)
		{
			handle->DataMaxLength = transferParam->Test->ReadLength;
			success = K.ReadPipe(transferParam->Test->InterfaceHandle,
			                     transferParam->Ep.PipeId,
			                     handle->Data,
			                     handle->DataMaxLength,
			                     NULL,
			                     &handle->Overlapped);
		}
		else
		{
			AppendLoopBuffer(transferParam->Test, handle->Data, transferParam->Test->WriteLength);
			handle->DataMaxLength = transferParam->Test->WriteLength;
			success = K.WritePipe(transferParam->Test->InterfaceHandle,
			                      transferParam->Ep.PipeId,
			                      handle->Data,
			                      handle->DataMaxLength,
			                      NULL,
			                      &handle->Overlapped);
		}
		transferErrorCode = GetLastError();

		if (!success && transferErrorCode == ERROR_IO_PENDING)
		{
			transferErrorCode = ERROR_SUCCESS;
			success = TRUE;
		}

		// Submit this transfer now.
		handle->ReturnCode = ret = -labs(transferErrorCode);
		if (ret < 0)
		{
			handle->InUse = FALSE;
			goto Done;
		}

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
		UINT transferred;
		// TransferHandleWaitIndex is the index of the oldest outstanding transfer.
		*handleRef = handle = &transferParam->TransferHandles[transferParam->TransferHandleWaitIndex];

		// Only wait, cancelling & freeing is handled by the caller.
		if (WaitForSingleObject(handle->Overlapped.hEvent,  transferParam->Test->Timeout) != WAIT_OBJECT_0)
		{
			if (!transferParam->Test->IsUserAborted)
				ret = WinError(0);
			else
				ret = -labs(GetLastError());

			handle->ReturnCode = ret;
			goto Done;
		}
		if (!K.GetOverlappedResult(transferParam->Test->InterfaceHandle, &handle->Overlapped, &transferred, FALSE))
		{
			if (!transferParam->Test->IsUserAborted)
				ret = WinError(0);
			else
				ret = -labs(GetLastError());

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

VOID VerifyLoopData(PBENCHMARK_TEST_PARAM Test, PUCHAR chData, int dataRemaining)
{
	LONG syncOffset		= 0;
	PUCHAR dataBuffer	= chData;
	int dataLength		= dataRemaining;

	VerifyListLock(Test);

	if (dataRemaining)
	{
		int verifyStageSize;
		PBENCHMARK_BUFFER writeVerifyBuffer, tmpWriteVerifyBuffer;

		DL_FOREACH_SAFE(Test->VerifyList, writeVerifyBuffer, tmpWriteVerifyBuffer)
		{
			if (!Test->IsLoopSynced)
			{
				verifyStageSize = min(writeVerifyBuffer->DataLength, dataRemaining);
				while(verifyStageSize > 0)
				{
					Test->IsLoopSynced = memcmp(writeVerifyBuffer->Data, chData, verifyStageSize) == 0 ? TRUE : FALSE;
					if (Test->IsLoopSynced) break;

					syncOffset++;
					dataRemaining--;
					chData++;
					verifyStageSize = min(writeVerifyBuffer->DataLength, dataRemaining);
				}

				if (!Test->IsLoopSynced)
				{
					chData = dataBuffer;
					dataRemaining = dataLength;

					if (writeVerifyBuffer->SyncFailed++ == 0)
					{
						writeVerifyBuffer = NULL;
					}
				}
				else
				{
					float reliability = ((float)verifyStageSize / (float)dataLength) * 100.0F;
					CONMSG("Loop data synchronized. Offset=%u Byte=%02Xh Reliability=%.1f%%.\n",
					       syncOffset, chData[0], reliability);
				}
			}

			if (Test->IsLoopSynced)
			{
				verifyStageSize = min(writeVerifyBuffer->DataLength, dataRemaining);

				if (memcmp(writeVerifyBuffer->Data, chData, verifyStageSize) == 0)
				{
					writeVerifyBuffer->DataLength -= verifyStageSize;
					dataRemaining -= verifyStageSize;
					writeVerifyBuffer->Data += verifyStageSize;
					chData = &chData[verifyStageSize];
				}
				else
				{
					CONVDAT("DataLength=%u\n", writeVerifyBuffer->DataLength);
					Test->IsLoopSynced = FALSE;

					if (writeVerifyBuffer->SyncFailed++ == 0)
					{
						// Do not delete the verify buffer until it fails to verify once before.
						writeVerifyBuffer = NULL;
						dataRemaining = 0;
					}
				}
			}

			if (dataRemaining == 0)
				break;
		}

		if (!Test->IsLoopSynced)
		{
			CONVDAT("Loop data not in sync.\n");
		}

		// If writeVerifyBuffer is not null at this point, all buffers before it are deleted.
		// if writeVerifyBuffer.DataLength == 0, it is also deleted.
		if (writeVerifyBuffer && Test->VerifyList)
		{
			PBENCHMARK_BUFFER writeVerifyBufferDel, tmpWriteVerifyBuffer;
			DL_FOREACH_SAFE(Test->VerifyList, writeVerifyBufferDel, tmpWriteVerifyBuffer)
			{
				if (writeVerifyBuffer != writeVerifyBufferDel)
				{
					DL_DELETE(Test->VerifyList, writeVerifyBufferDel);
					free(writeVerifyBufferDel);
				}
				else
				{
					if (writeVerifyBufferDel->DataLength <= 0)
					{
						DL_DELETE(Test->VerifyList, writeVerifyBufferDel);
						free(writeVerifyBufferDel);
					}
					break;
				}
			}
		}
	}

	VerifyListUnlock(Test);
}

DWORD TransferThreadProc(PBENCHMARK_TRANSFER_PARAM transferParam)
{
	int ret, i;
	PBENCHMARK_TRANSFER_HANDLE handle;
	PUCHAR data;

	transferParam->IsRunning = TRUE;

	K.ResetPipe(transferParam->Test->InterfaceHandle, transferParam->Ep.PipeId);

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

		if (transferParam->Test->Verify &&
		        transferParam->Test->VerifyList &&
		        transferParam->Test->TestType == TestTypeLoop &&
		        USB_ENDPOINT_DIRECTION_IN(transferParam->Ep.PipeId) && ret > 0)
		{
			VerifyLoopData(transferParam->Test, data, ret);
		}

		if (ret < 0)
		{
			// The user pressed 'Q'.
			if (transferParam->Test->IsUserAborted) break;

			// Transfer timed out
			if (ret == ERROR_SEM_TIMEOUT || ret == ERROR_OPERATION_ABORTED || ret == ERROR_CANCELLED)
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
				CONERR("failed %s! %d of %d ret=%d\n",
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
			transferParam->RunningErrorCount = 0;
			transferParam->RunningTimeoutCount = 0;
			if (USB_ENDPOINT_DIRECTION_IN(transferParam->Ep.PipeId))
			{
				if (transferParam->Test->ReadLogFile)
				{
					fwrite(data, ret, 1, transferParam->Test->ReadLogFile);
				}

				if (transferParam->Test->Verify && transferParam->Test->TestType != TestTypeLoop)
				{
					VerifyData(transferParam, data, ret);
				}
			}
			else
			{
				if (transferParam->Test->WriteLogFile)
				{
					fwrite(data, ret, 1, transferParam->Test->WriteLogFile);
				}
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
				else
				{
					CloseHandle(transferParam->TransferHandles[i].Overlapped.hEvent);
					transferParam->TransferHandles[i].Overlapped.hEvent = NULL;
					transferParam->TransferHandles[i].InUse = FALSE;
				}
			}
			Sleep(0);
		}
	}

	for (i = 0; i < transferParam->Test->BufferCount; i++)
	{
		if (transferParam->TransferHandles[i].Overlapped.hEvent)
		{
			if (transferParam->TransferHandles[i].InUse)
			{
				WaitForSingleObject(transferParam->TransferHandles[i].Overlapped.hEvent, transferParam->Test->Timeout);
			}
			else
			{
				WaitForSingleObject(transferParam->TransferHandles[i].Overlapped.hEvent, 0);
			}
			CloseHandle(transferParam->TransferHandles[i].Overlapped.hEvent);
			transferParam->TransferHandles[i].Overlapped.hEvent = NULL;
		}
		transferParam->TransferHandles[i].InUse = FALSE;
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
		else if (GetParamIntValue(arg, "buffersize=", &testParams->AllocBufferSize) ||
		         GetParamIntValue(arg, "size=", &testParams->AllocBufferSize))
		{
			if (testParams->ReadLength == 4096)
				testParams->ReadLength = testParams->AllocBufferSize;
			if (testParams->WriteLength == 4096)
				testParams->WriteLength = testParams->AllocBufferSize;

		}
		else if (GetParamIntValue(arg, "readsize=", &testParams->ReadLength)) {}
		else if (GetParamIntValue(arg, "writesize=", &testParams->WriteLength)) {}
		else if (GetParamIntValue(arg, "timeout=", &testParams->Timeout)) {}
		else if (GetParamIntValue(arg, "intf=", &testParams->Intf)) {}
		else if (GetParamIntValue(arg, "altf=", &testParams->Altf)) {}
		else if (GetParamIntValue(arg, "ep=", &testParams->Ep))
		{
			testParams->Ep &= 0xf;
		}
		else if (GetParamIntValue(arg, "refresh=", &testParams->Refresh)) {}
		else if (GetParamIntValue(arg, "fixedisopackets=", &testParams->FixedIsoPackets)) {}
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
		else if ((value = GetParamStrValue(arg, "rawio=")) != NULL)
		{
			if (GetParamStrValue(value, "t") ||
			        GetParamStrValue(value, "y") ||
			        GetParamStrValue(value, "1"))
			{
				testParams->UseRawIO = 1;
			}
			else
			{
				testParams->UseRawIO = 0;
			}
		}
		else if (!_stricmp(arg, "notestselect"))
		{
			testParams->NoTestSelect = TRUE;
		}
		else if (!_stricmp(arg, "logread"))
		{
			testParams->ReadLogEnabled = TRUE;
		}
		else if (!_stricmp(arg, "logwrite"))
		{
			testParams->WriteLogEnabled = TRUE;
		}
		else if (!_stricmp(arg, "log"))
		{
			testParams->ReadLogEnabled = TRUE;
			testParams->WriteLogEnabled = TRUE;
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
		else if (!_stricmp(arg, "listonly"))
		{
			testParams->UseList = TRUE;
			testParams->ListDevicesOnly = TRUE;
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
		else if (!_stricmp(arg, "composite"))
		{
			testParams->Use_UsbK_Init = TRUE;
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
	int allocSize;

	PWINUSB_PIPE_INFORMATION pipeInfo = NULL;

	/// Get Pipe Information
	for(i = 0; i < test->InterfaceDescriptor.bNumEndpoints; i++)
	{
		if (!(endpointID & USB_ENDPOINT_ADDRESS_MASK))
		{
			// Use first endpoint that matches the direction
			if ((test->PipeInformation[i].PipeId & USB_ENDPOINT_DIRECTION_MASK) == endpointID)
			{
				pipeInfo =  &test->PipeInformation[i];
				break;
			}
		}
		else
		{
			if ((int)test->PipeInformation[i].PipeId == endpointID)
			{
				pipeInfo =  &test->PipeInformation[i];
				break;
			}
		}
	}

	if (!pipeInfo)
	{
		CONERR("failed locating EP%02Xh!\n", endpointID);
		FreeTransferParam(&transferParam);
		goto Done;
	}

	if (!pipeInfo->MaximumPacketSize)
	{
		CONWRN("MaximumPacketSize=0 for EP%02Xh. check alternate settings.\n", pipeInfo->PipeId);
	}

	test->AllocBufferSize = max(test->AllocBufferSize, test->ReadLength);
	test->AllocBufferSize = max(test->AllocBufferSize, test->WriteLength);

	allocSize = sizeof(BENCHMARK_TRANSFER_PARAM) + (test->AllocBufferSize * test->BufferCount);
	transferParam = (PBENCHMARK_TRANSFER_PARAM) malloc(allocSize);

	if (transferParam)
	{
		memset(transferParam, 0, allocSize);
		transferParam->Test = test;

		memcpy(&transferParam->Ep, pipeInfo, sizeof(transferParam->Ep));

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
		if (transferParam->Test->TestType == TestTypeLoop && USB_ENDPOINT_DIRECTION_OUT(pipeInfo->PipeId))
		{
			// Data Format:
			// [0][KeyByte] 2 3 4 5 ..to.. wMaxPacketSize (if data byte rolls it is incremented to 1)
			// Increment KeyByte and repeat
			//
			BYTE indexC = 0;
			INT bufferIndex = 0;
			WORD dataIndex;
			INT packetIndex;
			INT packetCount = ((transferParam->Test->BufferCount * test->ReadLength) / pipeInfo->MaximumPacketSize);
			for(packetIndex = 0; packetIndex < packetCount; packetIndex++)
			{
				indexC = 2;
				for (dataIndex = 0; dataIndex < pipeInfo->MaximumPacketSize; dataIndex++)
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
	static BENCHMARK_TRANSFER_PARAM gRunningStatusTransferParam;
	DOUBLE bpsOverall;
	DOUBLE bpsLastTransfer;

	// LOCK the display critical section
	EnterCriticalSection(&DisplayCriticalSection);

	memcpy(&gRunningStatusTransferParam, transferParam, sizeof(BENCHMARK_TRANSFER_PARAM));

	// UNLOCK the display critical section
	LeaveCriticalSection(&DisplayCriticalSection);

	if ((!gRunningStatusTransferParam.StartTick) || (gRunningStatusTransferParam.StartTick >= gRunningStatusTransferParam.LastTick))
	{
		CONMSG("Synchronizing %d..\n", abs(transferParam->Packets));
	}
	else
	{
		GetAverageBytesSec(&gRunningStatusTransferParam, &bpsOverall);
		GetCurrentBytesSec(&gRunningStatusTransferParam, &bpsLastTransfer);
		transferParam->LastStartTick = 0;

		if (transferParam->LastTransferred == 0)
		{
			CONMSG("Avg. Bytes/s: %.2f Transfers: %d 0 Zero-length-transfer(s)\n",
			       bpsOverall, gRunningStatusTransferParam.Packets);
		}
		else
		{
			CONMSG("Avg. Bytes/s: %.2f Transfers: %d Bytes/s: %.2f\n",
			       bpsOverall, gRunningStatusTransferParam.Packets, bpsLastTransfer);
		}
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
	CONMSG("\tDriver          : %s\n", GetDrvIdString(test->SelectedDeviceProfile->DriverID));
	CONMSG("\tVid / Pid       : %04Xh / %04Xh\n", test->DeviceDescriptor.idVendor,  test->DeviceDescriptor.idProduct);
	CONMSG("\tDevicePath      : %s\n", test->SelectedDeviceProfile->DevicePath);
	CONMSG("\tDevice Speed    : %s\n", GetDevSpeedString(test->DeviceSpeed));
	CONMSG("\tInterface #     : %02Xh\n", test->InterfaceDescriptor.bInterfaceNumber);
	CONMSG("\tAlt Interface # : %02Xh\n", test->InterfaceDescriptor.bAlternateSetting);
	CONMSG("\tNum Endpoints   : %u\n", test->InterfaceDescriptor.bNumEndpoints);
	CONMSG("\tPriority        : %d\n", test->Priority);
	CONMSG("\tRead Size       : %d\n", test->ReadLength);
	CONMSG("\tWrite Size      : %d\n", test->WriteLength);
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
	KLST_DEVINFO_HANDLE deviceInfo = NULL;

	LstK_MoveReset(test->DeviceList);

	while (LstK_MoveNext(test->DeviceList, &deviceInfo))
	{
		int vid = -1;
		int pid = -1;
		int mi = -1;
		PCHAR chID;

		// disabled
		LibK_SetContext(deviceInfo, KLIB_HANDLE_TYPE_LSTINFOK, (KLIB_USER_CONTEXT)FALSE);

		memset(id, 0, sizeof(id));
		strcpy_s(id, MAX_PATH - 1, deviceInfo->DeviceID);
		_strlwr_s(id, MAX_PATH);

		if ( (chID = strstr(id, "vid_")) != NULL)
			sscanf_s(chID, "vid_%04x", &vid);
		if ( (chID = strstr(id, "pid_")) != NULL)
			sscanf_s(chID, "pid_%04x", &pid);
		if ( (chID = strstr(id, "mi_")) != NULL)
			sscanf_s(chID, "mi_%02x", &mi);

		if (test->Vid == vid && test->Pid == pid)
		{
			// enabled
			LibK_SetContext(deviceInfo, KLIB_HANDLE_TYPE_LSTINFOK, (KLIB_USER_CONTEXT)TRUE);
		}
	}

	return ERROR_SUCCESS;
}

int GetTestDeviceFromList(PBENCHMARK_TEST_PARAM test)
{
	UCHAR selection;
	UCHAR count = 0;
	KLST_DEVINFO_HANDLE deviceInfo = NULL;

	LstK_MoveReset(test->DeviceList);

	if (test->ListDevicesOnly)
	{
		while (LstK_MoveNext(test->DeviceList, &deviceInfo))
		{
			count++;
			CONMSG("%02u. %s (%s) [%s]\n", count, deviceInfo->DeviceDesc, deviceInfo->DeviceID, GetDrvIdString(deviceInfo->DriverID));
		}

		return ERROR_SUCCESS;
	}
	else
	{
		while (LstK_MoveNext(test->DeviceList, &deviceInfo) && count < 9)
		{
			CONMSG("%u. %s (%s) [%s]\n", count + 1, deviceInfo->DeviceDesc, deviceInfo->DeviceID, GetDrvIdString(deviceInfo->DriverID));
			count++;

			// enabled
			LibK_SetContext(deviceInfo, KLIB_HANDLE_TYPE_LSTINFOK, (KLIB_USER_CONTEXT)TRUE);

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
			while (LstK_MoveNext(test->DeviceList, &deviceInfo) && ++count != selection)
			{
				// disabled
				LibK_SetContext(deviceInfo, KLIB_HANDLE_TYPE_LSTINFOK, (KLIB_USER_CONTEXT)FALSE);

			}

			if (!deviceInfo)
			{
				CONERR("unknown selection\n");
				return -1;
			}

			return ERROR_SUCCESS;
		}
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
	UINT count, length;


	if (argc == 1)
	{
		ShowHelp();
		return -1;
	}

	SetTestDefaults(&Test);

	// Load the command line arguments.
	if (ParseBenchmarkArgs(&Test, argc, argv) < 0)
		return -1;

	// Initialize the critical section used for locking
	// the volatile members of the transfer params in order
	// to update/modify the running statistics.
	//
	InitializeCriticalSection(&DisplayCriticalSection);

	if (!LstK_Init(&Test.DeviceList, 0))
	{
		ec = GetLastError();
		CONERR("failed getting device list ec=%08Xh\n", ec);
		goto Done;
	}

	count = 0;
	LstK_Count(Test.DeviceList, &count);
	if (!count)
	{
		CONERR("device list empty.\n");
		goto Done;
	}

	if (Test.ListDevicesOnly)
	{
		CONMSG("CurrentProcessId=%u Count=%d\n", GetCurrentProcessId(), count);
	}
	else
	{
		CONMSG("device-count=%u\n", count);
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

	if (Test.ListDevicesOnly)
		goto Done;

	if (!Bench_Open(&Test))
		goto Done;

	CONMSG("opened %s (%s)..\n", Test.SelectedDeviceProfile->DeviceDesc, Test.SelectedDeviceProfile->DeviceID);

	// If "NoTestSelect" appears in the command line then don't send the control
	// messages for selecting the test type.
	//
	if (!Test.NoTestSelect)
	{
		if (Bench_Configure(Test.InterfaceHandle, SET_TEST, (UCHAR)Test.Intf, &Test.TestType) != TRUE)
		{
			CONERR("setting bechmark test type #%d!\n\n", Test.TestType);
			goto Done;
		}
	}

	// If reading from the device create the read transfer param. This will also create
	// a thread in a suspended state.
	//
	if (Test.TestType & TestTypeRead)
	{
		ReadTest = CreateTransferParam(&Test, Test.Ep | USB_ENDPOINT_DIRECTION_MASK);
		if (!ReadTest) goto Done;
		if (Test.UseRawIO != 0xFF)
		{
			if (!K.SetPipePolicy(Test.InterfaceHandle, ReadTest->Ep.PipeId, RAW_IO, 1, &Test.UseRawIO))
			{
				CONERR("SetPipePolicy:RAW_IO failed. ErrorCode=%08Xh\n", GetLastError());
				goto Done;
			}
		}
	}

	// If writing to the device create the write transfer param. This will also create
	// a thread in a suspended state.
	//
	if (Test.TestType & TestTypeWrite)
	{
		WriteTest = CreateTransferParam(&Test, Test.Ep);
		if (!WriteTest) goto Done;
		if (Test.FixedIsoPackets)
		{
			if (!K.SetPipePolicy(Test.InterfaceHandle, WriteTest->Ep.PipeId, ISO_NUM_FIXED_PACKETS, 2, &Test.FixedIsoPackets))
			{
				CONERR("SetPipePolicy:ISO_NUM_FIXED_PACKETS failed. ErrorCode=%08Xh\n", GetLastError());
				goto Done;
			}
		}
		if (Test.UseRawIO != 0xFF)
		{
			if (!K.SetPipePolicy(Test.InterfaceHandle, WriteTest->Ep.PipeId, RAW_IO, 1, &Test.UseRawIO))
			{
				CONERR("SetPipePolicy:RAW_IO failed. ErrorCode=%08Xh\n", GetLastError());
				goto Done;
			}
		}
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

	if (Test.ReadLogEnabled)
		Test.ReadLogFile = fopen("read.log", "wb");

	if (Test.WriteLogEnabled)
		Test.WriteLogFile = fopen("write.log", "wb");

	// Get the device speed.
	length = sizeof(Test.DeviceSpeed);
	K.QueryDeviceInformation(Test.InterfaceHandle, DEVICE_SPEED, &length, &Test.DeviceSpeed);

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
	if (!Test.Use_UsbK_Init)
	{
		if (Test.DeviceHandle)
		{
			CloseHandle(Test.DeviceHandle);
			Test.DeviceHandle = NULL;
		}
	}
	if (Test.VerifyBuffer)
	{
		PBENCHMARK_BUFFER verifyBuffer, verifyListTmp;

		free(Test.VerifyBuffer);
		Test.VerifyBuffer = NULL;

		DL_FOREACH_SAFE(Test.VerifyList, verifyBuffer, verifyListTmp)
		{
			DL_DELETE(Test.VerifyList, verifyBuffer);
			free(verifyBuffer);
		}
	}

	if (Test.ReadLogFile)
	{
		fflush(Test.ReadLogFile);
		fclose(Test.ReadLogFile);
		Test.ReadLogFile = NULL;
	}

	if (Test.WriteLogFile)
	{
		fflush(Test.WriteLogFile);
		fclose(Test.WriteLogFile);
		Test.WriteLogFile = NULL;
	}

	LstK_Free(Test.DeviceList);
	FreeTransferParam(&ReadTest);
	FreeTransferParam(&WriteTest);

	DeleteCriticalSection(&DisplayCriticalSection);

	if (!Test.ListDevicesOnly)
	{
		CONMSG0("Press any key to exit..");
		_getch();
		CONMSG0("\n");
	}
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
	CONMSG0("Copyright (c) 2012 Travis Lee Robinson. <libusbdotnet@gmail.com>\n");
}
