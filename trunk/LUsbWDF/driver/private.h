/* libusb-win32 WDF, Generic KMDF Windows USB Driver
 * Copyright (c) 2010-2011 Travis Robinson <libusbdotnet@gmail.com>
 * Copyright (c) 2002-2005 Stephan Meyer <ste_meyer@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma warning(disable:4200)  //
#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4214)  // bit field types other than int

#include <initguid.h>
#include <ntddk.h>
#include <ntintsafe.h>
#include "usbdi.h"
#include "usbdlib.h"
#include "driver_api.h"

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)

#include <wdf.h>
#include <wdfusb.h>
#include <wchar.h>

#include "public.h"
#include "libusb-win32_version.h"
#include "driver_debug.h"

#ifndef _H
#define _H

#define POOL_TAG (ULONG) 'BSUL'

#undef ExAllocatePool
#define ExAllocatePool(type, size) \
	ExAllocatePoolWithTag(type, size, POOL_TAG);

#define MAX_TRANSFER_SIZE   4096
#define REMOTE_WAKEUP_MASK 0x20

#define DEFAULT_REGISTRY_TRANSFER_SIZE 65536

//////////////////////////////////////////////////////////////////////////////
// libusb-win32 private driver externs
//////////////////////////////////////////////////////////////////////////////
extern CONST PCHAR PipeTypeStrings[8];
#define GetPipeTypeString(PipeType) PipeTypeStrings[(PipeType) & 0x7]

extern CONST PCHAR DeviceSpeedStrings[8];
#define GetDeviceSpeedString(UsbDeviceSpeed) DeviceSpeedStrings[(UsbDeviceSpeed) & 0x7]

extern CONST PCHAR BoolStrings[2];
#define GetBoolString(TrueOrFalse) BoolStrings[(TrueOrFalse) ? 1:0]

// This is the device registry key for DeviceInterfaceGUIDs.
static PCWSTR LUsbW_DeviceInterfaceGUIDs_ValueName = L"LUsbW_DeviceInterfaceGUIDs";

// This is the default GUID used if LUsbW_DeviceInterfaceGUIDs_ValueName is not found.
static PCWSTR LUsbW_DeviceInterfaceGUID_DefaultString = L"{6C696275-7362-2D77-696E33322D574446}";
DEFINE_GUID(LUsbW_DeviceInterfaceGUID_Default,0x6C696275,0x7362,0x2D77,0x69,0x6E,0x33,0x32,0x2D,0x57,0x44,0x46);

//////////////////////////////////////////////////////////////////////////////
// libusb-win32 private driver defines
//////////////////////////////////////////////////////////////////////////////
#define IsHighSpeedDevice(DeviceContextPtr) (DeviceContextPtr->DeviceSpeed >= UsbHighSpeed ? TRUE : FALSE)

#define SET_CONFIG_ACTIVE_CONFIG -258

/*
#define USB_RECIP_DEVICE    0x00
#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT  0x02
#define USB_RECIP_OTHER     0x03

#define USB_TYPE_STANDARD   0x00
#define USB_TYPE_CLASS      0x01
#define USB_TYPE_VENDOR	    0x02
*/

#define LIBUSB_DEFAULT_TIMEOUT 5000
#define LIBUSB_MAX_CONTROL_TRANSFER_TIMEOUT 5000
#define LIBUSB_MAX_INTERFACE_COUNT 32
#define LIBUSB_MAX_ENDPOINT_COUNT 32



#define GetListHeadEntry(ListHead)  ((ListHead)->Flink)

#define GetContextByPipeID(DeviceContext, EndpointID) \
	DeviceContext->PipeContextByID[((EndpointID) & 0xF) | ((((EndpointID) >> 3) & 0x10))]
//
// Either IdleCannotWakeFromS0 or IdleUsbSelectiveSuspend for usb devices
//  IdleCannotWakeFromS0    - The device cannot wake itself from a low-power state while the system is in its working (S0) state.
//  IdleUsbSelectiveSuspend - The device is connected to a USB bus and supports USB selective suspend. Use this value if your
//                            USB-connected device supports both idling and waking itself while the computer is in its working
//                            state. If your USB device supports only idling, use IdleCannotWakeFromS0.
#define IDLE_CAPS_TYPE IdleCannotWakeFromS0

#define LIBUSB_SYMBOLIC_LINK_NAME L"\\DosDevices\\libusb0-"


// Pipe policy types
typedef enum _PIPE_POLICY_TYPE
{
	// 0x01
	// TRUE (or a nonzero value) indicates that every write request is terminated with a zero-length packet.
	// The default value is FALSE.
	//
	SHORT_PACKET_TERMINATE=0x01,

	// 0x02
	// TRUE indicates that WinUSB automatically clears a stalled condition on the endpoint. This policy parameter does not affect control pipes.
	// FALSE indicates that all transfers (that arrive to the endpoint after the stalled transfer) fail until the caller manually reset's endpoints pipe.
	// The default value is FALSE.
	//
	AUTO_CLEAR_STALL=0x02,

	// 0x03
	// A value of zero (default) indicates that transfers do not time out because the host controller never cancels the transfer.
	// A nonzero value indicates the time-out interval, in milliseconds, that the host controller waits for the transfer to complete before cancelling the transfer.
	//
	PIPE_TRANSFER_TIMEOUT=0x03,

	// 0x04
	// TRUE indicates that the host controller does not complete a read operation for a short packet. Instead, the host completes the operation only after the specified number of bytes have been read.
	// FALSE indicates that the host controller completes a read operation when either the specified number of bytes have been read or the host controller has received a short packet.
	// The default value is FALSE.
	//
	IGNORE_SHORT_PACKETS=0x04,

	// 0x05
	// FALSE indicates if the device returns more data that the client requested, WinUSB fails the read request.
	// TRUE indicates if the device returns more data that the client requested, WinUSB driver can save or discard the extra data that remained from the request.
	// To configure WinUSB's action for the remaining data, set the AUTO_FLUSH policy.
	//
	ALLOW_PARTIAL_READS=0x05,

	// 0x06
	// AUTO_FLUSH must be used with ALLOW_PARTIAL_READS enabled. If ALLOW_PARTIAL_READS is TRUE, the value of AUTO_FLUSH determines the action taken by WinUSB when the device returns more data than the caller requested.
	// TRUE indicates that WinUSB saves the extra data that remained from the request and adds it to the beginning of the next read request.
	// FALSE indicates that WinUSB discards the remaining data from the read request.
	//
	AUTO_FLUSH=0x06,

	// 0x07
	// TRUE (nonzero) indicates that RAW_IO is enabled, in which case buffers passed to WinUsb_ReadPipe for the specified endpoint must meet the following requirements:
	//    * The buffer length must be a multiple of the maximum endpoint packet size.
	//    * The length must be less than or equal to the value of MAXIMUM_TRANSFER_SIZE retrieved by WinUsb_GetPipePolicy.
	// If the preceding conditions are satisfied, WinUSB sends data directly to the USB driver stack, bypassing the queuing and error handling mechanism of WinUSB.
	// FALSE indicates that RAW_IO is disabled.
	//
	RAW_IO=0x07,

	// 0x08
	// The retrieved value indicates the maximum size of a USB transfer supported by WinUSB.
	//
	MAXIMUM_TRANSFER_SIZE=0x08,

	// 0x09
	// TRUE (or a nonzero value) indicates that on resume from suspend, WinUSB resets the endpoint before it allows the caller to send new requests to the endpoint.
	//
	RESET_PIPE_ON_RESUME=0x09,

	PIPE_POLICY_TYPE_MAX,

} PIPE_POLICY_TYPE;

typedef enum _POWER_POLICY_TYPE
{
	// Specifies the auto-suspend policy type; the power policy parameter must be specified by the caller in the Value parameter.
	// For auto-suspend, the Value parameter must point to a UCHAR variable.
	// If Value is TRUE (nonzero), the USB stack suspends the device if the device is idle. A device is idle if there are no transfers pending, or if the only pending transfers are IN transfers to interrupt or bulk endpoints.
	// The default value is determined by the value set in the DefaultIdleState registry setting. By default, this value is TRUE.
	//
	AUTO_SUSPEND=0x81,

	// Specifies the suspend-delay policy type; the power policy parameter must be specified by the caller in the Value parameter.
	// For suspend-delay, Value must point to a ULONG variable.
	// Value specifies the minimum amount of time, in milliseconds, that the WinUSB driver must wait post transfer before it can suspend the device.
	// The default value is determined by the value set in the DefaultIdleTimeout registry setting. By default, this value is five seconds.
	//
	SUSPEND_DELAY=0x83

} POWER_POLICY_TYPE;

typedef enum _LUSBW_DEVREG_KEYID_TYPE
{
	LUsbW_DeviceIdleEnabled,
	LUsbW_DeviceIdleIgnoreWakeEnable,
	LUsbW_UserSetDeviceIdleEnabled,
	LUsbW_DefaultIdleState,
	LUsbW_DefaultIdleTimeout,

	LUSBW_DEVREG_KEYID_TYPE_MAX,
} LUSBW_DEVREG_KEYID_TYPE;

typedef struct _LUSBW_USER_DEVREG_KEY
{
	BOOLEAN			IsSet;
	ULONG			Value;
	ULONG			DefaultValue;
	ULONG			ValueByteCount;
} LUSBW_USER_DEVREG_KEY,*PLUSBW_USER_DEVREG_KEY;

typedef struct _POWER_POLICY
{
	LUSBW_USER_DEVREG_KEY UserKeys[LUSBW_DEVREG_KEYID_TYPE_MAX];

	//WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS IdleSettings;
	//WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS WakeSettings;

} POWER_POLICY,*PPOWER_POLICY;

typedef struct _PIPE_CONTEXT
{
	BOOLEAN IsValid;

	// WDF pipe info
	WDF_USB_PIPE_INFORMATION PipeInformation;

	// Pipe handle
	WDFUSBPIPE Pipe;

	// Pipe queue
	WDFQUEUE Queue;

	// Pipe policies
	ULONG PipePolicy[PIPE_POLICY_TYPE_MAX];

} PIPE_CONTEXT, *PPIPE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PIPE_CONTEXT,
                                   GetPipeContext)

//
// This context is associated with every open handle.
//
typedef struct _INTERFACE_CONTEXT
{

	// Interface handle
	WDFUSBINTERFACE Interface;

	// Interface index
	UCHAR InterfaceIndex;

	// Alt setting selected on UsbInterface
	UCHAR SettingIndex;

	// Interface descriptor of current alt setting
	USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;

	// Number of configured pipes of current alt setting
	UCHAR PipeCount;

	// Pipe context maps of current alt setting
	PPIPE_CONTEXT PipeContextByIndex[LIBUSB_MAX_ENDPOINT_COUNT];

} INTERFACE_CONTEXT, *PINTERFACE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(INTERFACE_CONTEXT,
                                   GetInterfaceContext)

typedef struct _DEVICE_CONTEXT
{

	USB_DEVICE_DESCRIPTOR           UsbDeviceDescriptor;

	PUSB_CONFIGURATION_DESCRIPTOR   UsbConfigurationDescriptor;

	// Physical WDFDEVICE object used to create fdoDevice.
	WDFDEVICE                       pdoDevice;

	// WDFDEVICE for the function driver.
	WDFDEVICE                       fdoDevice;

	// WDFDEVICE used to create the target device.
	WDFDEVICE                       WdfDevice;

	// Target device created from WdfDevice.
	WDFUSBDEVICE                    WdfUsbTargetDevice;

	USB_DEVICE_SPEED                DeviceSpeed;

	BOOLEAN							RemoteWakeCapable;

	BOOLEAN							SelfPowered;

	ULONG                           MaximumTransferSize;

	UCHAR							InterfaceCount;

	// configured pipes context
	PIPE_CONTEXT					PipeContextByID[LIBUSB_MAX_ENDPOINT_COUNT];

	INTERFACE_CONTEXT				InterfaceContext[LIBUSB_MAX_INTERFACE_COUNT];

	POWER_POLICY					PowerPolicy;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT,
                                   GetDeviceContext)

typedef DEVICE_CONTEXT libusb_device_t;

//
// This context is associated with every open handle.
//
typedef struct _FILE_CONTEXT
{
	PPIPE_CONTEXT PipeContext;
} FILE_CONTEXT, *PFILE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILE_CONTEXT,
                                   GetFileContext)

//
// This context is associated with every request recevied by the driver
// from the app.
//
typedef struct _REQUEST_CONTEXT
{

	WDFMEMORY         UrbMemory;
	PMDL			  OriginalTransferMDL;
	PMDL              Mdl;
	ULONG             Length;         // remaining to xfer
	ULONG             TotalTransferred;        // cumulate xfer
	ULONG_PTR         VirtualAddress; // va for next segment of xfer.
	WDFCOLLECTION     SubRequestCollection; // used for doing Isoch
	WDFSPINLOCK		  SubRequestCollectionLock; // used to sync access to collection at DISPATCH_LEVEL
	BOOLEAN           Read; // TRUE if Read
	PPIPE_CONTEXT     PipeContext;
} REQUEST_CONTEXT, * PREQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT            ,
                                   GetRequestContext)

typedef struct _WORKITEM_CONTEXT
{
	WDFDEVICE       Device;
	WDFUSBPIPE      Pipe;
} WORKITEM_CONTEXT, *PWORKITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WORKITEM_CONTEXT,
                                   GetWorkItemContext)

//
// Required for doing ISOCH transfer. This context is associated with every
// subrequest created by the driver to do ISOCH transfer.
//
typedef struct _SUB_REQUEST_CONTEXT
{

	WDFREQUEST  UserRequest;
	PURB        SubUrb;
	PMDL        SubMdl;
	LIST_ENTRY  ListEntry; // used in CancelRoutine

} SUB_REQUEST_CONTEXT, *PSUB_REQUEST_CONTEXT ;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SUB_REQUEST_CONTEXT, GetSubRequestContext)

extern ULONG DebugLevel;

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD LUsbW_EvtDeviceAdd;

EVT_WDF_DEVICE_PREPARE_HARDWARE LUsbW_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_D0_ENTRY LUsbW_EvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT LUsbW_EvtDeviceD0Exit;

EVT_WDF_DEVICE_FILE_CREATE LUsbW_EvtDeviceFileCreate;

// Default queue events
EVT_WDF_IO_QUEUE_IO_READ LUsbW_EvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE LUsbW_EvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL LUsbW_EvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP LUsbWEvtIoStop;
EVT_WDF_IO_QUEUE_IO_RESUME LUsbWEvtIoResume;

// pipe queue events
EVT_WDF_IO_QUEUE_IO_READ LUsbW_EvtReadFromPipeQueue;
EVT_WDF_IO_QUEUE_IO_WRITE LUsbW_EvtWriteFromPipeQueue;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL LUsbW_EvtIoDeviceControlFromPipeQueue;

// iso queue events
EVT_WDF_REQUEST_CANCEL LUsbW_EvtRequestCancel;
EVT_WDF_REQUEST_COMPLETION_ROUTINE SubRequestCompletionRoutine;
EVT_WDF_REQUEST_COMPLETION_ROUTINE ReadWriteCompletion;

PPIPE_CONTEXT
GetPipeContextFromName(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PUNICODE_STRING FileName
);

NTSTATUS GetTransferMdl(
    WDFREQUEST Request,
    WDF_REQUEST_TYPE RequestType,
    PMDL* requestMdl);

VOID ReadWriteIsochEndPoints(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN ULONG            Length,
    IN WDF_REQUEST_TYPE RequestType,
    IN PPIPE_CONTEXT	PipeContext
);

VOID AddRequestToPipeQueue(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN ULONG            Length,
    IN PPIPE_CONTEXT	PipeContext
);

VOID TransferBulk(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN ULONG            Length,
    IN WDF_REQUEST_TYPE RequestType,
    IN PMDL				TransferMDL,
    IN PPIPE_CONTEXT	PipeContext
);

NTSTATUS
ResetPipe(
    IN WDFUSBPIPE             Pipe
);

NTSTATUS
ResetDevice(
    IN WDFDEVICE Device
);

VOID
CancelSelectSuspend(
    IN PDEVICE_CONTEXT DeviceContext
);


NTSTATUS
ReadAndSelectDescriptors(
    IN WDFDEVICE Device
);

NTSTATUS
ConfigureDevice(
    IN WDFDEVICE Device
);

NTSTATUS
SelectInterfaces(
    IN WDFDEVICE Device
);

NTSTATUS LUsbW_SetPowerPolicy(IN  PDEVICE_CONTEXT deviceContext,
                              IN  WDFDEVICE Device);


NTSTATUS AbortPipes(IN PDEVICE_CONTEXT deviceContext);


NTSTATUS QueuePassiveLevelCallback(IN  WDFDEVICE Device,
                                   IN  WDFUSBPIPE Pipe);

VOID TransferIsoFS(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN ULONG            Length,
    IN WDF_REQUEST_TYPE RequestType,
    IN PMDL				TransferMDL,
    IN PPIPE_CONTEXT	PipeContext
);

VOID TransferIsoHS(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN ULONG            Length,
    IN WDF_REQUEST_TYPE RequestType,
    IN PMDL				TransferMDL,
    IN PPIPE_CONTEXT	PipeContext
);

VOID
DbgPrintRWContext(
    PREQUEST_CONTEXT                 rwContext
);

NTSTATUS
LUsbW_ReadFdoRegistryKeyValue(
    __in  WDFDRIVER   Driver,
    __in LPWSTR      Name,
    __out PULONG      Value
);

VOID StopAllPipes(IN PDEVICE_CONTEXT deviceContext);

VOID StartAllPipes(IN PDEVICE_CONTEXT deviceContext);

//////////////////////////////////////////////////////////////////////////////
// libusb-win32 private driver functions
//////////////////////////////////////////////////////////////////////////////

NTSTATUS StartInterface(IN  PDEVICE_CONTEXT deviceContext,
                        IN  PINTERFACE_CONTEXT interfaceContext);

NTSTATUS InitInterfaceContext(IN  PDEVICE_CONTEXT deviceContext,
                              IN  PINTERFACE_CONTEXT interfaceContext);

NTSTATUS SetAltInterface(IN  PDEVICE_CONTEXT deviceContext,
                         IN  UCHAR interfaceNumber,
                         IN  UCHAR altSetting);

NTSTATUS GetAltInterface(IN  PDEVICE_CONTEXT deviceContext,
                         IN  UCHAR interfaceNumber,
                         OUT PUCHAR altSetting);

NTSTATUS SetClearFeature(IN	 PDEVICE_CONTEXT deviceContext,
                         IN	 WDF_USB_BMREQUEST_RECIPIENT recipient,
                         IN  USHORT featureIndex,
                         IN  USHORT feature,
                         IN  BOOLEAN setFeature,
                         IN  INT timeout);

NTSTATUS VendorReadWrite(IN  PDEVICE_CONTEXT deviceContext,
                         IN  PWDF_MEMORY_DESCRIPTOR memoryDescriptor,
                         IN  WDF_USB_BMREQUEST_DIRECTION direction,
                         IN  WDF_USB_BMREQUEST_RECIPIENT recipient,
                         IN  BYTE request,
                         IN  USHORT value,
                         IN  USHORT index,
                         IN  INT timeout,
                         OUT PULONG transferred);

PINTERFACE_CONTEXT GetInterfaceContextByNumber(IN  PDEVICE_CONTEXT deviceContext,
        IN  UCHAR interfaceNumber);

NTSTATUS SetGetDescriptor(IN  PDEVICE_CONTEXT deviceContext,
                          IN  PWDF_MEMORY_DESCRIPTOR memoryDescriptor,
                          IN  BOOLEAN setDescriptor,
                          IN  WDF_USB_BMREQUEST_RECIPIENT recipient,
                          IN  UCHAR descriptorType,
                          IN  UCHAR descriptorIndex,
                          IN  USHORT langID,
                          OUT PULONG descriptorLength,
                          IN  INT timeout);

NTSTATUS GetStatus(IN  PDEVICE_CONTEXT deviceContext,
                   IN  PWDF_MEMORY_DESCRIPTOR memoryDescriptor,
                   IN  WDF_USB_BMREQUEST_RECIPIENT recipient,
                   IN  USHORT index,
                   IN  INT timeout,
                   OUT PULONG transferred);

NTSTATUS ResetOrAbortPipe(IN  PDEVICE_CONTEXT deviceContext,
                          IN  BOOLEAN resetPipe,
                          IN  UCHAR pipeID,
                          IN  INT timeout);

NTSTATUS LUsbW_ReadDeviceRegistryKeys(IN OUT  PDEVICE_CONTEXT deviceContext);

#endif

