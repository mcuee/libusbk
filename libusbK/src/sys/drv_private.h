/*! \file drv_private.h
*/

#ifndef __KUSB_PRIVATE_H__
#define __KUSB_PRIVATE_H__

#pragma warning(disable:4200)  //
#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4214)  // bit field types other than int

#include <initguid.h>
#include <ntddk.h>
#include <ntintsafe.h>
#include "usbdi.h"
#include "usbdlib.h"

#include "drv_api.h"

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)

#include <wdf.h>
#include <wdfusb.h>
#include <wchar.h>

/////////////////////////////////////////////////////////////////////
// Global/shared includes
#include "lusbk_version.h"
#include "lusbk_debug.h"
#include "..\includes\lusbk_shared.h"
/////////////////////////////////////////////////////////////////////

#define POOL_TAG (ULONG) 'KBSU'

// declared in drv_driver.c; set the default there.
extern ULONG DebugLevel;

#undef ExAllocatePool
#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, POOL_TAG)
#define GetListHeadEntry(ListHead)  ((ListHead)->Flink)

// Max pipe/power/device policies.
#define MAX_POLICY			(16)

// These are pre-allocated to device context space.
#define LIBUSB_MAX_INTERFACE_COUNT 32
#define LIBUSB_MAX_ENDPOINT_COUNT  32

#define REMOTE_WAKEUP_MASK 0x20

// Max allowed transfer size for the default pipe.
#define MAX_CONTROL_TRANSFER_SIZE 4096

// Used as a timeout override for some control request; most will use either the pipe policy timeout or if not set, the request.timeout.
// If neither are set the timeout is infinite.
#define LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT 5000

// libusbK creates this same symbolic name for libusb0.dll compatibility.
#define LIBUSB_SYMBOLIC_LINK_NAME L"\\DosDevices\\libusb0-"
#define LIBUSB_SYMBOLIC_LINK_NAMEA "\\DosDevices\\libusb0-"

#define CONTEXT_SECTION_START

// Declared in lusbk_general.  These are used, primarily, for log messages.
extern CONST PCHAR PipeTypeStrings[8];
extern CONST PCHAR DeviceSpeedStrings[8];
extern CONST PCHAR EndpointDirStrings[2];
extern CONST PCHAR BmRequestDirStrings[2];
extern CONST PCHAR BmRequestTypeStrings[4];
extern CONST PCHAR BmRequestRecipientStrings[4];
extern CONST PCHAR BoolStrings[2];

// Use these safe defines to access the strings above.  They will alwas return a valid
// string pointer so there is no need to check the parameters before hand.
#define GetPipeTypeString(PipeType) PipeTypeStrings[(PipeType) & 0x7]
#define GetDeviceSpeedString(UsbDeviceSpeed) DeviceSpeedStrings[(UsbDeviceSpeed) & 0x7]
#define GetEndpointDirString(EndpointAddress) EndpointDirStrings[((EndpointAddress)>>7) & 0x1]
#define GetBmRequestDirString(BmRequestDir) BmRequestDirStrings[((BmRequestDir)?1:0)]
#define GetBmRequestTypeString(BmRequestType) BmRequestTypeStrings[((BmRequestType) & 0x3)]
#define GetBmRequestRecipientString(BmRequestRecipient) BmRequestRecipientStrings[((BmRequestRecipient) & 0x3)]
#define GetRequestTypeString(WdfRequestType) EndpointDirStrings[(((WdfRequestType)>>2) & 0x1)?0:1]
#define GetBoolString(TrueOrFalse) BoolStrings[(TrueOrFalse) ? 1:0]

// All Policies (and DeviceInformation) are stored in there own array of ULONGs.  Each ULONG array
// is MAX_POLICY in length.  This macro is used to safely get/set the values.
#define GetPolicyValue(PolicyType, PolicyULongArray) PolicyULongArray[(PolicyType) & (MAX_POLICY-1)]

// PAGED_CODE()
// This macro replacement does not assert and show a BSOD.  Instead it simply logs a messages and continues.
// Paged code doesn't make much sense in a driver that compiles to under 30k.
#ifdef PAGED_CODE
#undef PAGED_CODE
#endif

// All paging is disabled by default.
// When enabled, this macro overrides the default PAGED_CODE() macro for
// logging only; this macro does not assert and cause a BSOD.
//
#if defined(PAGING_ENABLED)
#define PAGED_CODE() \
	{																													\
		if (KeGetCurrentIrql() > APC_LEVEL) {																			\
			DbgPrint("ERROR:%s[%s] Pageable code called at IRQL %d\n",LOG_APPNAME,__FUNCTION__,KeGetCurrentIrql());		\
		}																												\
	}
#else
#define PAGED_CODE() NOP_FUNCTION
#endif

typedef struct _DEVICE_REGSETTINGS
{
	// This feature is controlled by the SystemWakeEnabled DWORD registry setting. This value value indicates whether or not the device
	// should be allowed to wake the system from a low power state.
	// Example: HKR,,SystemWakeEnabled,0x00010001,1
	ULONG SystemWakeEnabled;

	// DeviceIdleEnabled	This is a DWORD value. This registry value indicates whether or not the device is capable of being powered down when idle (Selective Suspend).
	//
	//    * A value of zero, or the absence of this value indicates that the device does not support being powered down when idle.
	//    * A non-zero value indicates that the device supports being powered down when idle.
	//    * If DeviceIdleEnabled is not set, the value of the AUTO_SUSPEND power policy setting is ignored.	{"DeviceIdleEnabled", WdfFalse},
	//	Example: HKR,,DeviceIdleEnabled,0x00010001,1
	ULONG DeviceIdleEnabled;

	// DeviceIdleIgnoreWakeEnable	When set to a nonzero value, it suspends the device even if it does not support RemoteWake.
	ULONG DeviceIdleIgnoreWakeEnable;

	// UserSetDeviceIdleEnabled	This value is a DWORD value. This registry value indicates whether or not a check box should be enabled in the device Properties page that allows a user to override the idle defaults. When UserSetDeviceIdleEnabled is set to a nonzero value the check box is enabled and the user can disable powering down the device when idle. A value of zero, or the absence of this value indicates that the check box is not enabled.
	//
	//    * If the user disables device power savings, the value of the AUTO_SUSPEND power policy setting is ignored.
	//    * If the user enables device power savings, then the value of AUTO_SUSPEND is used to determine whether or not to suspend the device when idle.
	//
	// The UserSetDeviceIdleEnabled is ignored if DeviceIdleEnabled is not set.
	//	Example: HKR,,UserSetDeviceIdleEnabled,0x00010001,1
	ULONG UserSetDeviceIdleEnabled;

	// DefaultIdleState	This is a DWORD value. This registry value sets the default value of the AUTO_SUSPEND power policy setting. This registry key is used to enable or disable selective suspend when a handle is not open to the device.
	//
	//    * A value of zero or the absence of this value indicates that by default, the device is not suspended when idle. The device be allowed to suspend when idle only when the AUTO_SUSPEND power policy is enabled.
	//    * A non-zero value indicates that by default the device is allowed to be suspended when idle.
	//
	// This value is ignored if DeviceIdleEnabled is not set.
	//	Example: HKR,,DefaultIdleState,0x00010001,1
	ULONG DefaultIdleState;

	// DefaultIdleTimeout	This is a DWORD value. This registry value sets the default state of the SUSPEND_DELAY power policy setting.
	//
	// The value indicates the amount of time in milliseconds to wait before determining that a device is idle.
	//	Example: HKR,,DefaultIdleTimeout,0x00010001,100
	//
	ULONG DefaultIdleTimeout;

	// DeviceInterfaceGUIDs	This is a string array of device insterface guid.DWORD value.
	//
	// When the device is created it registers with all of the GUIDs listed.
	//	Example: HKR,,DeviceInterfaceGUIDs, 0x10000,"{D696BFEB-1734-417d-8A04-86D01071C512}"
	//
	WDFCOLLECTION DeviceInterfaceGUIDs;

} DEVICE_REGSETTINGS, *PDEVICE_REGSETTINGS;

#pragma warning(push)
#pragma warning(disable:4201)	// nonstandard extension used : nameless struct/union
typedef union _PIPE_POLICIES
{
	volatile long values;
	struct
	{
		unsigned int IsoStartLatency : 10;
		unsigned int IsoAlwaysStartAsap: 1;
		unsigned int AutoClearStall: 1;
		unsigned int AllowPartialReads: 1;
		unsigned int AutoFlush: 1;
		unsigned int IgnoreShortPackets: 1;
		unsigned int RawIO: 1;

		unsigned int IsoNumFixedPackets: 11;
		unsigned int ResetPipeOnResume: 1;
		unsigned int ShortPacketTerminate: 1;
		unsigned: 3;
	};
} PIPE_POLICIES;
#pragma warning(pop)

// This define in only here so this section and be folded in VStudio. //
#ifdef CONTEXT_SECTION_START

//////////////////////////////////////////////////////////////////////////////
/////////////////////// START OF CONTEXT SECTION /////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct _PIPE_CONTEXT
{
	volatile WDFQUEUE Queue;					// Pipe queue.
	volatile ULONG TimeoutPolicy;

	BOOLEAN IsValid;							// True only when the pipe exists and is ready.
	BOOLEAN PipePoliciesInitialized;			// True only the first time a PipeID is initialized.
	WDF_USB_PIPE_INFORMATION PipeInformation;	// WDF pipe info.
	WDFUSBPIPE Pipe;							// Pipe handle.

	PIPE_POLICIES Policies;

	// Set when policies have changes that require queue settings to change.
	BOOLEAN IsQueueDirty;

	// http://msdn.microsoft.com/en-us/library/windows/hardware/ff552359%28v=vs.85%29.aspx
	// Settings.Parallel.NumberOfPresentedRequests
	// Maximum number of transfers that can be asynchronously delivered at a
	// time. Available in version 1.9 and later versions of KMDF.
	ULONG SimulParallelRequests;

} PIPE_CONTEXT, *PPIPE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PIPE_CONTEXT,
                                   GetPipeContext)

//
// This context is associated with every open handle.
//
typedef struct _INTERFACE_CONTEXT
{
	WDFUSBINTERFACE Interface;	// Interface handle
	UCHAR InterfaceIndex;		// Interface index
	UCHAR SettingIndex;			// Alt setting selected on UsbInterface
	USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;	// Interface descriptor of current alt setting
	UCHAR PipeCount;								// Number of configured pipes of current alt setting
	PPIPE_CONTEXT PipeContextByIndex[LIBUSB_MAX_ENDPOINT_COUNT];	// Pipe context maps of current intf setting
	PFILE_OBJECT ClaimedByFileObject;				// File object the owns this interface.
} INTERFACE_CONTEXT, *PINTERFACE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(INTERFACE_CONTEXT,
                                   GetInterfaceContext)

typedef struct _DEVICE_CONTEXT
{

	USB_DEVICE_DESCRIPTOR           UsbDeviceDescriptor;
	ULONG							InstanceNumber;			// Unique number; incremented when a device is created.
	PUSB_CONFIGURATION_DESCRIPTOR   UsbConfigurationDescriptor;
	ULONG							ConfigurationDescriptorSize;
	UCHAR							ConfigDescriptorIndex;
	WDFDEVICE                       WdfDevice;			// WDFDEVICE used to create the target device.
	WDFUSBDEVICE                    WdfUsbTargetDevice;	// Target device created from WdfDevice.
	USB_DEVICE_SPEED                DeviceSpeed;		// Low/Full/High
	BOOLEAN							RemoteWakeCapable;	// If the device can wake itself up.
	BOOLEAN							SelfPowered;
	UCHAR							InterfaceCount;
	PIPE_CONTEXT					PipeContextByID[LIBUSB_MAX_ENDPOINT_COUNT]; // all pipes contexts by id
	INTERFACE_CONTEXT				InterfaceContext[LIBUSB_MAX_INTERFACE_COUNT];
	ULONG							DevicePolicy[MAX_POLICY];	// DeviceInformation (actually).
	USBD_VERSION_INFORMATION		UsbVersionInfo;
	DEVICE_REGSETTINGS				DeviceRegSettings; // Device regisitry settings (from inf Dev_AddReq)
	BOOLEAN							IsIdleSettingsInitialized;
	volatile long					OpenedFileHandleCount;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT,
                                   GetDeviceContext)

typedef DEVICE_CONTEXT libusb_device_t;

//
// This context is associated with every open handle.
//
typedef struct _FILE_CONTEXT
{
	PDEVICE_CONTEXT		DeviceContext;
	UCHAR				PipeID;
	WDF_USB_PIPE_TYPE	PipeType;
} FILE_CONTEXT, *PFILE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILE_CONTEXT,
                                   GetFileContext)

typedef struct _QUEUE_CONTEXT
{
	WDFUSBPIPE					PipeHandle;	// Pipe handle.		[RO]
	WDF_USB_PIPE_INFORMATION	Info;		// WDF pipe info.	[RO]

	PPIPE_CONTEXT				PipeContext;

	BOOLEAN						IsFreshPipeReset;
	ULONG						NextFrameNumber;

	// Set in EvtResume and CancelRoutine (AUTO_CLEAR_STALL)
	BOOLEAN						ResetPipeForResume;
	BOOLEAN						ResetPipeForStall;

	// RunOverOfs is zeroed when the queue is created; a BufferLegnth > 0 indicates RunOverOfs
	//            is a valid range of bytes left over from a previous read when
	//            AllowPartialReads=1 and AutoFlush=0.
	// RunOverMem is pre-allocated when the first request is sent to a newly created queue; size always equals wMaxPacketSize.
	WDFMEMORY_OFFSET	OverOfs;
	WDFMEMORY			OverMem;
	PUCHAR				OverBuf;

	struct
	{
		// UserMem represents the users transfer buffer.
		// UserOfs represents the portion of the user buffer for a stage transfer.
		WDFMEMORY_OFFSET	UserOfs;
		WDFMEMORY			UserMem;

		struct
		{
			UCHAR Required;
			UCHAR Sent;

		} Zlps;

		ULONG				Length;
		ULONG				Transferred;
	} Xfer;

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, GetQueueContext)


typedef struct _XFER_AUTOISO_COLLECTION_CONTEXT
{
	WDFREQUEST					MainRequest;
	struct _REQUEST_CONTEXT*	MainRequestContext;
	PMDL				OriginalTransferMDL;
	PUCHAR				TransferBuffer;
	ULONG				AdjustedReadOffset;
	WDFCOLLECTION		SubRequestCollection;		// used for doing Isoch
	WDFSPINLOCK			SubRequestCollectionLock; // used to sync access to collection at DISPATCH_LEVEL

	PMDL              Mdl;
	ULONG             TotalTransferred;			// cumulate xfer
} XFER_AUTOISO_COLLECTION_CONTEXT, *PXFER_AUTOISO_COLLECTION_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XFER_AUTOISO_COLLECTION_CONTEXT, GetAutoIsoCollectionContext)

// C4201: nonstandard extension used : nameless struct/union
#pragma warning(push)
#pragma warning(disable:4201)
typedef struct _REQUEST_CONTEXT
{
	ULONG Length;

	// Read-Write-DeviceIoControl
	WDF_REQUEST_TYPE ActualRequestType;
	// Read-Write
	WDF_REQUEST_TYPE RequestType;

	ULONG			IoControlCode;
	libusb_request	IoControlRequest;

	PVOID	InputBuffer;
	ULONG	InputBufferLength;

	PQUEUE_CONTEXT		QueueContext;
	PIPE_POLICIES		Policies;
	ULONG				Timeout;

	union
	{
		struct
		{
			WDFMEMORY			UrbMemory;
			WDFMEMORY			ContextMemory;
		} IsoEx;
		struct
		{
			WDFMEMORY			UrbMemory;
			PXFER_AUTOISO_COLLECTION_CONTEXT Context;
		} AutoIso;
		struct
		{
			WDFMEMORY			UrbMemory;
			ULONG				RequestedLength;
			SHORT				NumPackets;

			// Fixed packets are assigned in the xfer functions and come frome the pipe context.
			struct
			{
				WDFMEMORY		Memory;
				PUCHAR			Buffer;
			} FixedIsoPackets;
		} AutoIsoEx;
	};

} REQUEST_CONTEXT, *PREQUEST_CONTEXT;
#pragma warning(pop)

// This is only used to track the request context size.  It represents the actual size for a 64bit driver.
// It can be safely removed without causing problems.
C_ASSERT(sizeof(REQUEST_CONTEXT) <= 256);

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT            ,
                                   GetRequestContext)

typedef struct _WORKITEM_CONTEXT
{
	WDFDEVICE       Device;
	WDFUSBPIPE      Pipe;
} WORKITEM_CONTEXT, *PWORKITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WORKITEM_CONTEXT,
                                   GetWorkItemContext)

//////////////////////////////////////////////////////////////////////////////
//////////////////////// END OF CONTEXT SECTION //////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#endif

//////////////////////////////////////////////////////////////////////////////
// lusbk_driver.c function prototypes.
//
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD DriverExit;

//////////////////////////////////////////////////////////////////////////////
// lusbk_registry.c function prototypes.
// Driver and device registry.
//
NTSTATUS Registry_ReadAllDeviceKeys(__in PDEVICE_CONTEXT deviceContext);

//////////////////////////////////////////////////////////////////////////////
// lusbk_general.c function prototypes.
// strings, guids, etc.
//
NTSTATUS GUIDFromWdfString(__in WDFSTRING guidString,
                           __out PGUID guid);

NTSTATUS GetTransferMemory(__in WDFREQUEST Request,
                           __in WDF_REQUEST_TYPE RequestType,
                           __out WDFMEMORY* wdfMemory);

NTSTATUS GetTransferMdl(__in WDFREQUEST Request,
                        __in WDF_REQUEST_TYPE RequestType,
                        __out PMDL* wdmMdl);

//////////////////////////////////////////////////////////////////////////////
// lusbk_registry.c function prototypes.
NTSTATUS AddDefaultDeviceInterfaceGUID(__in PDEVICE_CONTEXT deviceContext);

//
// GetPipeContextByID()	The pipe collections are indexed by endpoint address as well as the pipe index.
//						Use this macro to get (and set) the pipe context(s) by endpoint address.
//						This macro is safe. i.e: it will always return a valid pointer to a pipe context.
// in pipes  = 0x01-0x0F
// out pipes = 0x11-0x1F
// default pipe = 0x00
// usused = 0x10
//
FORCEINLINE PPIPE_CONTEXT GetPipeContextByID(__in PDEVICE_CONTEXT deviceContext,
        __in UCHAR pipeID)
{
	return (pipeID == 0x80)
	       ? &deviceContext->PipeContextByID[0]
	       : &deviceContext->PipeContextByID[((pipeID) & 0xF) | ((pipeID & 0x80) >> 3)];
}

#endif

