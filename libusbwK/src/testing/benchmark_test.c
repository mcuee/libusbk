#include "benchmark_api.h"

static ULONG DebugLevel = 3;

static UCHAR tab = 0;
static HANDLE deviceHandle = NULL;
static WINUSB_INTERFACE_HANDLE interfaceHandle = NULL;
static LONG ret = ERROR_SUCCESS;
static ULONG timeout = 1000;
static ULONG timeoutValueLength = 4;
static BOOL success = TRUE;
static ULONG tickEnd;
static ULONG tickStart;

typedef INT TEST_FN();
typedef INT (*TEST_FN_T)();

typedef struct _TEST_FN_ITEM
{
	TEST_FN_T Function;
	LPCSTR MatchString;
} TEST_FN_ITEM, *PTEST_FN_ITEM;

#define GetPipeTypeString(PipeType) PipeTypeStrings[(PipeType) & 0x7]
CONST PCHAR PipeTypeStrings[8] = {"Control", "Isochronous", "Bulk", "Interrupt", "na", "na", "na", "na"};

extern LONG WinError(__in_opt DWORD errorCode);

TEST_FN Query;
TEST_FN PipeTimeout;
TEST_FN FlushPipe;
TEST_FN Vendor;
TEST_FN SyncRead;
TEST_FN SyncWrite;
TEST_FN SyncLoop;
TEST_FN Zlp;

#define LUsbWF(functionName) LUsbW_##functionName

#define IsTestMatch(text,TestFnItem) (_strnicmp(TestFnItem->MatchString,text,strlen(text))==0)
#define MAKE_FN(FnName) {FnName,#FnName}
#define TEST_FN_COUNT (sizeof(TestList)/sizeof(TEST_FN_ITEM))
TEST_FN_ITEM TestList[] =
{
	MAKE_FN(Query),
	MAKE_FN(PipeTimeout),
	MAKE_FN(FlushPipe),
	MAKE_FN(Vendor),
	MAKE_FN(SyncRead),
	MAKE_FN(SyncWrite),
	MAKE_FN(SyncLoop),
	MAKE_FN(Zlp),

	{NULL, NULL}
};

static INT Xfer(LPCSTR catString, BM_TEST_TYPE testType, ULONG transferTestSize);

static LPCSTR tabs[] = {"", "  ", "    ", "      ", "        ", "          ", "            "};

#define TEST_FUNC_BEGIN() ret = ERROR_SUCCESS; timeout=1000; timeoutValueLength=4; success=TRUE; tickStart=GetTickCount()

#define TESTMSG(format,...) USBLOG(2,LOG_APPNAME,"","["__FUNCTION__"]",format"\n",__VA_ARGS__)
#define TESTPASS(format,...) USBLOG(2,LOG_APPNAME,"","["__FUNCTION__"][Ok!]",format"\n",__VA_ARGS__)
#define TESTFAIL(format,...) USBLOG(2,LOG_APPNAME,"","["__FUNCTION__"][Fail!]",format"\n",__VA_ARGS__)

#define TEST_FUNC_END_EX(success,category,format,...) {											\
	tickEnd=GetTickCount();																		\
	if (success)																				\
		TESTPASS("[%s][ret=%d elapsed-ms=%u] "format,category,ret,tickEnd-tickStart,__VA_ARGS__);	\
	else																					\
		TESTFAIL("[%s][ret=%d elapsed-ms=%u] "format,category,ret,tickEnd-tickStart,__VA_ARGS__);	\
}

#define TEST_FUNC_END(success,format,...) TEST_FUNC_END_EX(success,__FUNCTION__,format,__VA_ARGS__)

#define OpenDevice(DeviceIndex)	\
	if ((ret = Bm_Open(DeviceIndex, NULL, &deviceHandle)) != ERROR_SUCCESS) { success=FALSE; goto Done; }		\
	if ((success=LUsbWF(Initialize)(deviceHandle, &interfaceHandle)) != TRUE)										\
	{																											\
		ret = WinError(0);																						\
		goto Done;																								\
	}

#define CloseDevice() { LUsbWF(Free)(interfaceHandle); Bm_Close(&deviceHandle); }

#define SetPipeTimeout(PipeID,TimeoutPtr)	\
	if (!(success = LUsbWF(SetPipePolicy)(interfaceHandle,PipeID,PIPE_TRANSFER_TIMEOUT,4,TimeoutPtr)))	\
	{ ret = WinError(0); goto Done; }

#define GetPipeTimeout(PipeID,TimeoutPtr)	\
	timeoutValueLength=4;																								\
	if (!(success = LUsbWF(GetPipePolicy)(interfaceHandle,PipeID,PIPE_TRANSFER_TIMEOUT,&timeoutValueLength,TimeoutPtr)))	\
	{ ret = WinError(0); goto Done; }

#define Flush(PipeID)											\
	if (!(success = LUsbWF(FlushPipe)(interfaceHandle, PipeID)))	\
	{ ret = WinError(0); goto Done; }

int main(int argCount, char** argv)
{
	int argIndex;
	LONG ret = -1;
	PTEST_FN_ITEM fnList = TestList;

	TESTMSG("built-on: %s %s\n", __DATE__, __TIME__);

	if (argCount == 1)
	{
		fnList = TestList;
		do
		{
			if ((ret = fnList->Function()) < 0)
				goto Failed;

			fnList++;
		}
		while (fnList->Function);
	}
	else
	{
		// some simple arguments for test automation..
		for (argIndex = 1; argIndex < argCount; argIndex++)
		{
			fnList = TestList;
			do
			{
				if (IsTestMatch(argv[argIndex], fnList))
				{
					if ((ret = fnList->Function()) < 0)
						goto Failed;
				}
				fnList++;
			}
			while (fnList->Function);
		}
	}

	return ret;

Failed:
	if (fnList->Function && fnList->MatchString)
		TESTMSG("Stopped by %s fail.", fnList->MatchString);
	return ret;
}

INT Vendor()
{
	BM_TEST_TYPE testType = BmTestTypeNone;
	TEST_FUNC_BEGIN();

	OpenDevice(0);

	TESTMSG("Getting benchmark test type..");
	ret = Bm_TestSelect(interfaceHandle, BmGetTest, &testType);

Done:
	TEST_FUNC_END(ret == 1, "TestType=%s\n", BmGetTestTypeString(testType));
	CloseDevice();
	return ret;
}

INT PipeTimeout()
{
	UCHAR pipeID = 0x81;
	ULONG timeoutCheck = 0;
	BM_TEST_TYPE testType = BmTestTypeLoop;
	UCHAR transferBuffer[64];
	ULONG transferTestSize;
	TEST_FUNC_BEGIN();

	OpenDevice(0);

	TESTMSG("pipeID=%02Xh setting pipe timeout=%ums..", pipeID, timeout);
	SetPipeTimeout(pipeID, &timeout);
	GetPipeTimeout(pipeID, &timeoutCheck);
	if (timeoutCheck != timeout) goto Fail1;

	TESTPASS("pipeID=%02Xh timeout set.  selecting %s test..", pipeID, BmGetTestTypeString(testType));

	// select the loop test
	if ((ret = Bm_TestSelect(interfaceHandle, BmSetTest, &testType)) != 1)
		goto Fail2;

	TESTPASS("pipeID=%02Xh loop test selected.  Reading until timeout..", pipeID);

	while(LUsbWF(ReadPipe)(interfaceHandle, pipeID, transferBuffer, sizeof(transferBuffer), &transferTestSize, NULL));

	ret = GetLastError();
	if (ret == ERROR_SEM_TIMEOUT)
		goto Success;

	goto Fail3;

Fail1:
	TEST_FUNC_END(FALSE, "timeout=%u timeoutCheck=%u\n", timeout, timeoutCheck);
	goto Done;

Fail2:
	ret = WinError(0);
	TEST_FUNC_END(FALSE, "test select failed.\n");
	goto Done;

Fail3:
	ret = WinError(ret);
	TEST_FUNC_END(FALSE, "unknown error code return from LUsbW_ReadPipe() function.\n");
	goto Done;

Success:
	TEST_FUNC_END(TRUE, "timeout=%u pipeID=%02Xh transfer timed out successful.\n", timeout, pipeID);
	ret = 0;

Done:
	CloseDevice();
	return ret;
}

INT FlushPipe()
{
	TEST_FUNC_BEGIN();

	OpenDevice(0);

	Flush(0x01);
	Flush(0x81);

Done:
	TEST_FUNC_END(success, "\n");

	CloseDevice();
	return ret;
}

INT Query()
{
	WINUSB_INTERFACE_HANDLE assocInterfaceHandle = NULL;
	UCHAR altIndex;
	USB_INTERFACE_DESCRIPTOR interfaceDescriptor;
	TEST_FUNC_BEGIN();

	OpenDevice(0);

AssociatedInterface:
	altIndex = 0;

	TESTMSG("Interface and pipe information..");
	while (LUsbWF(QueryInterfaceSettings)(interfaceHandle, altIndex++, &interfaceDescriptor))
	{
		UCHAR pipeIndex = 0;
		WINUSB_PIPE_INFORMATION pipeInformation;


		tab++;
		printf("%s[Interface #%03u:%03u] Type=%02Xh NumEndpoints=%02u Class=%02Xh Protocol=%02Xh SubClass=%02Xh Length=%u String=%03u\n",
		       tabs[tab], interfaceDescriptor.bInterfaceNumber, interfaceDescriptor.bAlternateSetting,
		       interfaceDescriptor.bDescriptorType, interfaceDescriptor.bNumEndpoints, interfaceDescriptor.bInterfaceClass,
		       interfaceDescriptor.bInterfaceProtocol, interfaceDescriptor.bInterfaceSubClass, interfaceDescriptor.bLength, interfaceDescriptor.iInterface);

		while (LUsbWF(QueryPipe)(interfaceHandle, altIndex - 1, pipeIndex++, &pipeInformation))
		{
			tab++;
			printf("%s[PipeId %02Xh] Type=%s MaximumPacketSize=%u Interval=%u\n",
			       tabs[tab], pipeInformation.PipeId, GetPipeTypeString(pipeInformation.PipeType), pipeInformation.MaximumPacketSize, pipeInformation.Interval);
			tab--;
		}
		tab--;
		if ((success = (GetLastError() == ERROR_NO_MORE_ITEMS)) == FALSE) ret = WinError(0);
	}
	if ((success = (GetLastError() == ERROR_NO_MORE_ITEMS)) == FALSE) ret = WinError(0);

	if (LUsbWF(GetAssociatedInterface)(interfaceHandle, 0, &assocInterfaceHandle))
	{
		LUsbWF(Free)(interfaceHandle);
		assocInterfaceHandle = interfaceHandle;
		goto AssociatedInterface;
	}
	if ((success = (GetLastError() == ERROR_NO_MORE_ITEMS)) == FALSE) ret = WinError(0);

Done:
	CloseDevice();
	TEST_FUNC_END(success, "\n");
	return ret;
}

INT SyncRead()
{
	return Xfer(__FUNCTION__, BmTestTypeRead, 64);
}
INT SyncWrite()
{
	return Xfer(__FUNCTION__, BmTestTypeWrite, 64);
}
INT SyncLoop()
{
	return Xfer(__FUNCTION__, BmTestTypeLoop, 64);
}

INT Zlp()
{
	return Xfer(__FUNCTION__, BmTestTypeWrite, 0);
}

static INT Xfer(LPCSTR catString, BM_TEST_TYPE testType, ULONG transferTestSize)
{
	PUCHAR transferBuffer = NULL;
	TEST_FUNC_BEGIN();

	OpenDevice(0);
	SetPipeTimeout(0x81, &timeout);
	SetPipeTimeout(0x01, &timeout);

	if ((ret = Bm_TestSelect(interfaceHandle, BmSetTest, &testType)) != 1)
		goto Done;

	USBLOG(2, LOG_APPNAME, "", catString, "- %s %d bytes..\n", BmGetTestTypeString(testType), transferTestSize);

	if (transferTestSize)
		transferBuffer = malloc(transferTestSize);

	if(testType == BmTestTypeRead)
		success = LUsbWF(ReadPipe)(interfaceHandle, 0x81, transferBuffer, transferTestSize, &transferTestSize, NULL);
	else if(testType == BmTestTypeWrite)
		success = LUsbWF(WritePipe)(interfaceHandle, 0x01, transferBuffer, transferTestSize, &transferTestSize, NULL);
	else if(testType == BmTestTypeLoop)
	{
		if ((success = LUsbWF(WritePipe)(interfaceHandle, 0x01, transferBuffer, transferTestSize, &transferTestSize, NULL)))
			success = LUsbWF(ReadPipe)(interfaceHandle, 0x81, transferBuffer, transferTestSize, &transferTestSize, NULL);
	}

	if (!success)
		ret = WinError(0);

Done:

	TEST_FUNC_END_EX(success, catString, "- %s %u bytes %s.\n", BmGetTestTypeString(testType), transferTestSize, (success ? "completed." : "failed"));

	CloseDevice();

	if (transferBuffer)
		free(transferBuffer);

	return ret;
}
