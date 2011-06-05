#include "lusbk_bknd.h"

BOOL LibInitialized = FALSE;
volatile long LibInitializedLockCount = 0;

// 8k
volatile long HandleListPos = -1;
KUSB_INTERFACE_HANDLE_INTERNAL InternalHandlePool[KUSB_MAX_INTERFACE_HANDLES];


// 4k
volatile long SharedDevicePos = -1;
KUSB_SHARED_DEVICE SharedDevicePool[KUSB_MAX_INTERFACE_HANDLES];

PCANCEL_IO_EX Opt_CancelIoEx = NULL;

PKUSB_SHARED_DEVICE GetSharedDevicePoolHandle(LPCSTR DevicePath)
{
	int count;
	long lockCount;
	int maxHandles = sizeof(SharedDevicePool) / sizeof(SharedDevicePool[0]);
	PKUSB_SHARED_DEVICE next;

	if (!Str_IsNullOrEmpty(DevicePath))
	{
		// first, look for an in-use shared device with the same path.
		for(count = 0; count < maxHandles; count++)
		{
			next = &SharedDevicePool[count];
			if (next->DevicePath[0])
			{
				AquireDeviceActionPendingLock(next);
				if (InterlockedIncrement(&next->UsageCount) > 1)
				{
					if (Str_Cmp(DevicePath, next->DevicePath) == 0)
					{
						// found a match.
						// UsageCount=UsageCount+1
						// ActionPendingLock=1
						return next;
					}
				}

				ReleaseDeviceActionPendingLock(next);
				InterlockedDecrement(&next->UsageCount);
			}
		}
	}

	count = 0;
	while(count++ < maxHandles)
	{
		next = &SharedDevicePool[InterlockedIncrement(&SharedDevicePos) & (maxHandles - 1)];
		AquireDeviceActionPendingLock(next);

		// fetching a new handle
		if ((lockCount = InterlockedIncrement(&next->UsageCount)) == 1)
		{
			next->MasterDeviceHandle = NULL;
			next->MasterInterfaceHandle = NULL;
			next->ConfigDescriptor = NULL;
			Mem_Zero(&next->SharedInterfaces, sizeof(next->SharedInterfaces));

			if (!Str_IsNullOrEmpty(DevicePath))
				memcpy(next->DevicePath, DevicePath, strlen(DevicePath) + 1);
			else
				Mem_Zero(next->DevicePath, sizeof(next->DevicePath));

			// UsageCount=UsageCount+1 (1)
			// ActionPendingLock=1
			return next;
		}

		ReleaseDeviceActionPendingLock(next);
		if ((lockCount = InterlockedDecrement(&next->UsageCount)) > 0)
			Sleep(lockCount);
	}

	USBERR("no more shared device handles! (max=%d)\n", maxHandles);
	LusbwError(ERROR_OUT_OF_STRUCTURES);
	return NULL;
}

PKUSB_INTERFACE_HANDLE_INTERNAL GetInternalPoolHandle()
{
	int count = 0;
	int maxHandles = sizeof(InternalHandlePool) / sizeof(InternalHandlePool[0]);

	CheckLibInitialized();

	while(count++ < maxHandles)
	{
		PKUSB_INTERFACE_HANDLE_INTERNAL next = &InternalHandlePool[InterlockedIncrement(&HandleListPos) & (maxHandles - 1)];
		if (InterlockedIncrement(&next->Instance.UsageCount) == 1)
		{
			next->BackendContext = NULL;
			memset(&next->UserContext, 0, sizeof(next->UserContext));
			return next;
		}
		InterlockedDecrement(&next->Instance.UsageCount);
		Sleep(0);
	}
	USBERR("no more internal handles! (max=%d)\n", maxHandles);
	LusbwError(ERROR_OUT_OF_STRUCTURES);
	return NULL;
}

VOID CheckLibInitialized()
{
recheck:
	if (!LibInitialized)
	{
		if (InterlockedIncrement(&LibInitializedLockCount) == 1)
		{
			Mem_Zero(&InternalHandlePool, sizeof(InternalHandlePool));
			Mem_Zero(&SharedDevicePool, sizeof(SharedDevicePool));

			Opt_CancelIoEx = (PCANCEL_IO_EX)GetProcAddress(GetModuleHandleA("kernel32"), "CancelIoEx");
			OvlK_CreateDefaultPool();
			LibInitialized = TRUE;

			USBDBG(
			    "Memory Usage:\r\n"
			    "\tInternalHandlePool : %u bytes (%u each)\r\n"
			    "\tSharedDevicePool   : %u bytes (%u each)\r\n",
			    sizeof(InternalHandlePool), sizeof(InternalHandlePool[0]),
			    sizeof(SharedDevicePool), sizeof(SharedDevicePool[0])
			);
		}
		else
		{
			InterlockedDecrement(&LibInitializedLockCount);
			Sleep(0);
			goto recheck;
		}
	}
}

// NOTE: This function cannot be nested!  It will dead-lock.
long AquireDeviceActionPendingLock(__in PKUSB_SHARED_DEVICE SharedDevice)
{
	long lockCount = 0;
	while (lockCount != 1)
	{
		// associated interfaces share a common nextDeviceEL->Device structure.
		// these structures are statically allocated so there are only so many of them.
		if ((lockCount = InterlockedIncrement(&SharedDevice->ActionPendingLock)) > 1)
		{
			USBDBG("lock collision. ActionPendingLock=%d\n", lockCount);
			if ((lockCount = InterlockedDecrement(&SharedDevice->ActionPendingLock)) > 0)
				Sleep(lockCount);

			lockCount = 0;
			continue;
		}
	}
	return lockCount;
}

long ReleaseDeviceActionPendingLock(__in PKUSB_SHARED_DEVICE SharedDevice)
{
	return InterlockedDecrement(&SharedDevice->ActionPendingLock);
}
