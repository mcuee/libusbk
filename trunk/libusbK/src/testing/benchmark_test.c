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
static USB_INTERFACE_DESCRIPTOR interfaceDescriptor;
static WINUSB_PIPE_INFORMATION pipeInformation;

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
TEST_FN TestType;
TEST_FN SyncRead;
TEST_FN SyncWrite;
TEST_FN SyncLoop;
TEST_FN Zlp;
TEST_FN VendorBuffer;
TEST_FN AltInterface;
TEST_FN ResetDevice;

#define IsTestMatch(text,TestFnItem) (_strnicmp(TestFnItem->MatchString,text,strlen(text))==0)
#define MAKE_FN(FnName) {FnName,#FnName}
#define TEST_FN_COUNT (sizeof(TestList)/sizeof(TEST_FN_ITEM))
TEST_FN_ITEM TestList[] =
{
	MAKE_FN(Query),
	MAKE_FN(PipeTimeout),
	MAKE_FN(FlushPipe),
	MAKE_FN(TestType),
	MAKE_FN(SyncRead),
	MAKE_FN(SyncWrite),
	MAKE_FN(SyncLoop),
	MAKE_FN(Zlp),
	MAKE_FN(VendorBuffer),
	MAKE_FN(AltInterface),

	{NULL, NULL}
};

static INT Xfer(LPCSTR catString, BM_TEST_TYPE testType, ULONG transferTestSize);

static LPCSTR tabs[] = {"", "  ", "    ", "      ", "        ", "          ", "            "};

#define TEST_FUNC_BEGIN() BOOL isDeviceHandleOwner=FALSE; ret = ERROR_SUCCESS; timeout=1000; timeoutValueLength=4; success=TRUE; tickStart=GetTickCount()

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

#define OpenDevice(DeviceIndex)																						\
	if (!deviceHandle)																								\
	{																												\
		isDeviceHandleOwner=TRUE;																					\
		if ((ret = Bm_Open(DeviceIndex, NULL, &deviceHandle)) != ERROR_SUCCESS) { success=FALSE; goto Done; }		\
		if ((success=LUsbK_Initialize(deviceHandle, &interfaceHandle)) != TRUE)									\
		{																											\
			ret = WinError(0);																						\
			goto Done;																								\
		}																											\
	}

#define CloseDevice() if (isDeviceHandleOwner) { LUsbK_Free(interfaceHandle); Bm_Close(&deviceHandle); }

#define SetPipeTimeout(PipeID,TimeoutPtr)	\
	if (!(success = LUsbK_SetPipePolicy(interfaceHandle,PipeID,PIPE_TRANSFER_TIMEOUT,4,TimeoutPtr)))	\
	{ ret = WinError(0); goto Done; }

#define GetPipeTimeout(PipeID,TimeoutPtr)	\
	timeoutValueLength=4;																								\
	if (!(success = LUsbK_GetPipePolicy(interfaceHandle,PipeID,PIPE_TRANSFER_TIMEOUT,&timeoutValueLength,TimeoutPtr)))	\
	{ ret = WinError(0); goto Done; }

#define Flush(PipeID)											\
	if (!(success = LUsbK_FlushPipe(interfaceHandle, PipeID)))	\
	{ ret = WinError(0); goto Done; }

int main(int argCount, char** argv)
{
	int argIndex;
	LONG ret = -1;
	PTEST_FN_ITEM fnList = TestList;

	TESTMSG("built-on: %s %s\n", __DATE__, __TIME__);

	if (ResetDevice() >= 0)
	{
		return Query();
	}
	return;

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

INT TestType()
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

INT VendorBuffer()
{
	BYTE vendorBuffer[8 + 1] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', '\0'};
	ULONG transferred = 0;
	WINUSB_SETUP_PACKET setupPacketSetBuffer =
	{
		(BMREQUEST_VENDOR << 5),	// RequestType
		BmSetVBuf,	// Request	(BmCommand)
		0,			// Value	(BmSetTest)
		0,			// Index	(Interface#)
		sizeof(vendorBuffer) - 1 // Length (Max=8)
	};

	WINUSB_SETUP_PACKET setupPacketGetBuffer =
	{
		(BMREQUEST_VENDOR << 5) | USB_ENDPOINT_DIRECTION_MASK,	// RequestType
		BmGetVBuf,	// Request	(BmCommand)
		0,			// Value	(BmSetTest)
		0,			// Index	(Interface#)
		sizeof(vendorBuffer) - 1 // Length (Max=8)
	};

	TEST_FUNC_BEGIN();

	OpenDevice(0);

	TESTMSG("setting vendor buffer to %s..", vendorBuffer);
	success = LUsbK_ControlTransfer(interfaceHandle, setupPacketSetBuffer, vendorBuffer, sizeof(vendorBuffer) - 1, &transferred, NULL);
	if (!success) goto Fail1;
	TESTPASS("vendor buffer set.");

	memset(vendorBuffer, 0, sizeof(vendorBuffer));

	TESTMSG("getting vendor buffer..");
	success = LUsbK_ControlTransfer(interfaceHandle, setupPacketGetBuffer, vendorBuffer, sizeof(vendorBuffer) - 1, &transferred, NULL);
	if (!success) goto Fail2;
	TESTPASS("vendor buffer=%s.", vendorBuffer);

	TEST_FUNC_END(TRUE, "\n");
	goto Done;

Fail1:
	ret = WinError(0);
	TEST_FUNC_END(FALSE, "setting vendor buffer failed.\n");
	goto Done;
Fail2:
	ret = WinError(0);
	TEST_FUNC_END(FALSE, "getting vendor buffer failed.\n");
	goto Done;

Done:
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

	while(LUsbK_ReadPipe(interfaceHandle, pipeID, transferBuffer, sizeof(transferBuffer), &transferTestSize, NULL));

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
	TEST_FUNC_END(FALSE, "unknown error code return from LUsbK_ReadPipe() function.\n");
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

INT AltInterface()
{
	UCHAR settingsNumber;
	UCHAR pipeIndex = 0;

	TEST_FUNC_BEGIN();
	OpenDevice(0);

	success = LUsbK_GetCurrentAlternateSetting(interfaceHandle, &settingsNumber);
	if (!success) goto Fail1;

	settingsNumber = (settingsNumber > 0) ? 0 : 1;

	if (!LUsbK_QueryInterfaceSettings(interfaceHandle, settingsNumber, &interfaceDescriptor))
	{
		TESTPASS("skipping test. reason: incompatible device configuration.\n");
		goto Done;
	}

	tab++;
	printf("%s[Selecting Interface #%03u:%03u]\n",
	       tabs[tab], interfaceDescriptor.bInterfaceNumber, interfaceDescriptor.bAlternateSetting);
	while (LUsbK_QueryPipe(interfaceHandle, settingsNumber, pipeIndex++, &pipeInformation))
	{
		tab++;
		printf("%s-%2u [PipeId %02Xh] Type=%s MaximumPacketSize=%u Interval=%u\n",
		       tabs[tab], pipeIndex - 1, pipeInformation.PipeId, GetPipeTypeString(pipeInformation.PipeType), pipeInformation.MaximumPacketSize, pipeInformation.Interval);
		tab--;
	}
	tab--;

	success = LUsbK_SetCurrentAlternateSetting(interfaceHandle, interfaceDescriptor.bAlternateSetting);
	if (!success) goto Fail2;

	TEST_FUNC_END(success, "\n");
	goto Done;

Fail1:
	ret = WinError(0);
	TEST_FUNC_END(FALSE, "GetCurrentAlternateSetting failed.\n");
	goto Done;

Fail2:
	ret = WinError(0);
	TEST_FUNC_END(FALSE, "SetCurrentAlternateSetting failed.\n");
	goto Done;

Done:
	CloseDevice();
	return ret;
}

INT ResetDevice()
{
	TEST_FUNC_BEGIN();

	OpenDevice(0);

	success=LUsbK_ResetDevice(interfaceHandle);
	if (!success)
	{
		ret = WinError(0);
		TEST_FUNC_END(success, "reset device failed.\n");
		goto Done;
	}

	TEST_FUNC_END(success, "\n");

Done:
	CloseDevice();
	return ret;
}

INT Query()
{
	WINUSB_INTERFACE_HANDLE assocInterfaceHandle = NULL;
	UCHAR altIndex;
	TEST_FUNC_BEGIN();

	OpenDevice(0);

AssociatedInterface:
	altIndex = 0;

	TESTMSG("Interface and pipe information..");
	while (LUsbK_QueryInterfaceSettings(interfaceHandle, altIndex++, &interfaceDescriptor))
	{
		UCHAR pipeIndex = 0;

		tab++;
		printf("%s[Interface #%03u:%03u] Type=%02Xh NumEndpoints=%02u Class=%02Xh Protocol=%02Xh SubClass=%02Xh Length=%u String=%03u\n",
		       tabs[tab], interfaceDescriptor.bInterfaceNumber, interfaceDescriptor.bAlternateSetting,
		       interfaceDescriptor.bDescriptorType, interfaceDescriptor.bNumEndpoints, interfaceDescriptor.bInterfaceClass,
		       interfaceDescriptor.bInterfaceProtocol, interfaceDescriptor.bInterfaceSubClass, interfaceDescriptor.bLength, interfaceDescriptor.iInterface);

		while (LUsbK_QueryPipe(interfaceHandle, altIndex - 1, pipeIndex++, &pipeInformation))
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

	if (LUsbK_GetAssociatedInterface(interfaceHandle, 0, &assocInterfaceHandle))
	{
		LUsbK_Free(interfaceHandle);
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
		success = LUsbK_ReadPipe(interfaceHandle, 0x81, transferBuffer, transferTestSize, &transferTestSize, NULL);
	else if(testType == BmTestTypeWrite)
		success = LUsbK_WritePipe(interfaceHandle, 0x01, transferBuffer, transferTestSize, &transferTestSize, NULL);
	else if(testType == BmTestTypeLoop)
	{
		if ((success = LUsbK_WritePipe(interfaceHandle, 0x01, transferBuffer, transferTestSize, &transferTestSize, NULL)))
			success = LUsbK_ReadPipe(interfaceHandle, 0x81, transferBuffer, transferTestSize, &transferTestSize, NULL);
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
