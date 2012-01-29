#region Copyright (c) Travis Robinson
// Copyright (c) 2011 Travis Robinson <libusbdotnet@gmail.com>
// All rights reserved.
//
// C# libusbK Bindings
// Auto-generated on: 01.28.2012
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS LEE ROBINSON
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
#endregion




// ReSharper disable InconsistentNaming
// ReSharper disable CheckNamespace
// ReSharper disable UnassignedReadonlyField

namespace libusbK

import System
import System.Runtime.CompilerServices
import System.Runtime.InteropServices
import Microsoft.Win32.SafeHandles

public static class Constants:

	public static final KLST_STRING_MAX_LEN = 256

	
	public static final LIBUSBK_DLL = 'libusbK.dll'

	
	public static final USB_CONFIG_POWERED_MASK as byte = 192

	
	public static final USB_ENDPOINT_DIRECTION_MASK as byte = 128

	
	public static final USB_ENDPOINT_ADDRESS_MASK as byte = 15


public enum PipePolicyType:

	SHORT_PACKET_TERMINATE = 1

	AUTO_CLEAR_STALL = 2

	PIPE_TRANSFER_TIMEOUT = 3

	IGNORE_SHORT_PACKETS = 4

	ALLOW_PARTIAL_READS = 5

	AUTO_FLUSH = 6

	RAW_IO = 7

	MAXIMUM_TRANSFER_SIZE = 8

	RESET_PIPE_ON_RESUME = 9

	
	ISO_START_LATENCY = 32

	ISO_ALWAYS_START_ASAP = 33

	ISO_NUM_FIXED_PACKETS = 34

	
	SIMUL_PARALLEL_REQUESTS = 48


public enum PowerPolicyType:

	AUTO_SUSPEND = 129

	SUSPEND_DELAY = 131


public enum DeviceInformationType:

	DEVICE_SPEED = 1


public enum EndpointType:

	CONTROL = 0

	
	ISOCHRONOUS = 1

	
	BULK = 2

	
	INTERRUPT = 3

	
	MASK = 3


public static class ErrorCodes:

	public static final Success = 0

	
	public static final AccessDenied = 5

	
	public static final InvalidHandle = 6

	
	public static final NotEnoughMemory = 8

	
	public static final NotSupported = 50

	
	public static final InvalidParameter = 87

	
	public static final SemTimeout = 121

	
	public static final Busy = 170

	
	public static final TooManyModules = 214

	
	public static final MoreData = 234

	
	public static final NoMoreItems = 259

	
	public static final ThreadNotInProcess = 566

	
	public static final ThreadWasSuspended = 699

	
	public static final OperationAborted = 995

	
	public static final IoIncomplete = 996

	
	public static final IoPending = 997

	
	public static final NotFound = 1168

	
	public static final Cancelled = 1223

	
	public static final Empty = 4306

	
	public static final ResourceNotAvailable = 5006

	
	public static final ResourceNotFound = 5007



#region Base KLIB Handle
public abstract class KLIB_HANDLE(SafeHandleZeroOrMinusOneIsInvalid, IKLIB_HANDLE):

	protected def constructor(ownsHandle as bool):
		super(ownsHandle)

	
	protected def constructor():
		self(true)

	
	protected def constructor(handlePtrToWrap as IntPtr):
		super(false)
		SetHandle(handlePtrToWrap)

	
	
	#region IKLIB_HANDLE Members
	public abstract HandleType as KLIB_HANDLE_TYPE:
		get:
			pass

	
	public def GetContext() as IntPtr:
		return Functions.LibK_GetContext(handle, HandleType)

	
	public def SetContext(ContextValue as IntPtr) as bool:
		return Functions.LibK_SetContext(handle, HandleType, ContextValue)

	
	public def SetCleanupCallback(CleanupCallback as KLIB_HANDLE_CLEANUP_CB) as bool:
		return Functions.LibK_SetCleanupCallback(handle, HandleType, CleanupCallback)

	#endregion
	
	
	protected override def ReleaseHandle() as bool:
		pass


public interface IKLIB_HANDLE:

	HandleType as KLIB_HANDLE_TYPE:
		get

	def DangerousGetHandle() as IntPtr

	def GetContext() as IntPtr

	def SetContext(UserContext as IntPtr) as bool

	def SetCleanupCallback(CleanupCallback as KLIB_HANDLE_CLEANUP_CB) as bool

#endregion


#region Class Handles
public class KHOT_HANDLE(KLIB_HANDLE):

	public static Empty as KHOT_HANDLE:
		get:
			return KHOT_HANDLE()

	
	public override HandleType as KLIB_HANDLE_TYPE:
		get:
			return KLIB_HANDLE_TYPE.HOTK

	
	protected override def ReleaseHandle() as bool:
		return Functions.HotK_Free(handle)


public class KUSB_HANDLE(KLIB_HANDLE):

	public static Empty as KUSB_HANDLE:
		get:
			return KUSB_HANDLE()

	
	public override HandleType as KLIB_HANDLE_TYPE:
		get:
			return KLIB_HANDLE_TYPE.USBK

	
	protected override def ReleaseHandle() as bool:
		return Functions.UsbK_Free(handle)

	
	
	#region USB Shared Device Context
	public def GetSharedContext() as IntPtr:
		return Functions.LibK_GetContext(handle, KLIB_HANDLE_TYPE.USBSHAREDK)

	
	public def SetSharedContext(SharedUserContext as IntPtr) as bool:
		return Functions.LibK_SetContext(handle, KLIB_HANDLE_TYPE.USBSHAREDK, SharedUserContext)

	
	public def SetSharedCleanupCallback(CleanupCallback as KLIB_HANDLE_CLEANUP_CB) as bool:
		return Functions.LibK_SetCleanupCallback(handle, KLIB_HANDLE_TYPE.USBSHAREDK, CleanupCallback)
	#endregion


public class KLST_HANDLE(KLIB_HANDLE):

	public static Empty as KLST_HANDLE:
		get:
			return KLST_HANDLE()

	
	public override HandleType as KLIB_HANDLE_TYPE:
		get:
			return KLIB_HANDLE_TYPE.LSTK

	
	protected override def ReleaseHandle() as bool:
		return Functions.LstK_Free(handle)


public class KOVL_POOL_HANDLE(KLIB_HANDLE):

	public static Empty as KOVL_POOL_HANDLE:
		get:
			return KOVL_POOL_HANDLE()

	
	public override HandleType as KLIB_HANDLE_TYPE:
		get:
			return KLIB_HANDLE_TYPE.OVLPOOLK

	
	protected override def ReleaseHandle() as bool:
		return Functions.OvlK_Free(handle)


public class KSTM_HANDLE(KLIB_HANDLE):

	public static Empty as KSTM_HANDLE:
		get:
			return KSTM_HANDLE()

	
	public override HandleType as KLIB_HANDLE_TYPE:
		get:
			return KLIB_HANDLE_TYPE.STMK

	
	protected override def ReleaseHandle() as bool:
		return Functions.StmK_Free(handle)


public class KISO_CONTEXT(SafeHandleZeroOrMinusOneIsInvalid):

	protected def constructor(ownsHandle as bool):
		super(ownsHandle)

	
	protected def constructor():
		self(true)

	
	protected def constructor(handlePtrToWrap as IntPtr):
		super(false)
		SetHandle(handlePtrToWrap)

	
	protected override def ReleaseHandle() as bool:
		Functions.IsoK_Free(handle)
		handle = IntPtr.Zero
		return true

#endregion


#region Struct Handles
public struct KOVL_HANDLE(IKLIB_HANDLE):

	private final handle as IntPtr

	
	public static Empty as KOVL_HANDLE:
		get:
			return KOVL_HANDLE()

	
	
	#region IKLIB_HANDLE Members
	public HandleType as KLIB_HANDLE_TYPE:
		get:
			return KLIB_HANDLE_TYPE.OVLK

	
	public def GetContext() as IntPtr:
		return Functions.LibK_GetContext(handle, HandleType)

	
	public def SetContext(ContextValue as IntPtr) as bool:
		return Functions.LibK_SetContext(handle, HandleType, ContextValue)

	
	public def SetCleanupCallback(CleanupCallback as KLIB_HANDLE_CLEANUP_CB) as bool:
		return Functions.LibK_SetCleanupCallback(handle, HandleType, CleanupCallback)

	
	public def DangerousGetHandle() as IntPtr:
		return handle
	#endregion


public struct KLST_DEVINFO_HANDLE(IKLIB_HANDLE):

	private static final ofsClassGUID as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'ClassGUID').ToInt32()

	private static final ofsCommon as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'Common').ToInt32()

	private static final ofsConnected as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'Connected').ToInt32()

	private static final ofsDeviceDesc as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'DeviceDesc').ToInt32()

	private static final ofsDeviceInterfaceGUID as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'DeviceInterfaceGUID').ToInt32()

	private static final ofsDevicePath as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'DevicePath').ToInt32()

	private static final ofsDriverID as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'DriverID').ToInt32()

	private static final ofsInstanceID as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'InstanceID').ToInt32()

	private static final ofsLUsb0FilterIndex as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'LUsb0FilterIndex').ToInt32()

	private static final ofsMfg as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'Mfg').ToInt32()

	private static final ofsService as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'Service').ToInt32()

	private static final ofsSymbolicLink as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'SymbolicLink').ToInt32()

	private static final ofsSyncFlags as int = Marshal.OffsetOf(typeof(KLST_DEVINFO), 'SyncFlags').ToInt32()

	
	private final handle as IntPtr

	
	public def constructor(HandleToWrap as IntPtr):
		handle = HandleToWrap

	
	public static Empty as KLST_DEVINFO_HANDLE:
		get:
			return KLST_DEVINFO_HANDLE()

	
	public SyncFlags as KLST_SYNC_FLAG:
		get:
			return (Marshal.ReadInt32(handle, ofsSyncFlags) cast KLST_SYNC_FLAG)

	
	public Connected as bool:
		get:
			return (Marshal.ReadInt32(handle, ofsConnected) != 0)

	
	public LUsb0FilterIndex as int:
		get:
			return Marshal.ReadInt32(handle, ofsLUsb0FilterIndex)

	
	public DevicePath as string:
		get:
			return Marshal.PtrToStringAnsi(IntPtr((handle.ToInt64() + ofsDevicePath)))

	
	public SymbolicLink as string:
		get:
			return Marshal.PtrToStringAnsi(IntPtr((handle.ToInt64() + ofsSymbolicLink)))

	
	public Service as string:
		get:
			return Marshal.PtrToStringAnsi(IntPtr((handle.ToInt64() + ofsService)))

	
	public DeviceDesc as string:
		get:
			return Marshal.PtrToStringAnsi(IntPtr((handle.ToInt64() + ofsDeviceDesc)))

	
	public Mfg as string:
		get:
			return Marshal.PtrToStringAnsi(IntPtr((handle.ToInt64() + ofsMfg)))

	
	public ClassGUID as string:
		get:
			return Marshal.PtrToStringAnsi(IntPtr((handle.ToInt64() + ofsClassGUID)))

	
	public InstanceID as string:
		get:
			return Marshal.PtrToStringAnsi(IntPtr((handle.ToInt64() + ofsInstanceID)))

	
	public DeviceInterfaceGUID as string:
		get:
			return Marshal.PtrToStringAnsi(IntPtr((handle.ToInt64() + ofsDeviceInterfaceGUID)))

	
	public DriverID as int:
		get:
			return Marshal.ReadInt32(handle, ofsDriverID)

	
	public Common as KLST_DEV_COMMON_INFO:
		get:
			return (Marshal.PtrToStructure(IntPtr((handle.ToInt64() + ofsCommon)), typeof(KLST_DEV_COMMON_INFO)) cast KLST_DEV_COMMON_INFO)

	
	
	#region IKLIB_HANDLE Members
	public HandleType as KLIB_HANDLE_TYPE:
		get:
			return KLIB_HANDLE_TYPE.LSTINFOK

	
	public def GetContext() as IntPtr:
		return Functions.LibK_GetContext(handle, HandleType)

	
	public def SetContext(ContextValue as IntPtr) as bool:
		return Functions.LibK_SetContext(handle, HandleType, ContextValue)

	
	public def DangerousGetHandle() as IntPtr:
		return handle

	
	public def SetCleanupCallback(CleanupCallback as KLIB_HANDLE_CLEANUP_CB) as bool:
		return Functions.LibK_SetCleanupCallback(handle, HandleType, CleanupCallback)

	#endregion
	
	
	public override def ToString() as string:
		return string.Format('{{Common: {0}, DriverID: {1}, DeviceInterfaceGuid: {2}, InstanceId: {3}, ClassGuid: {4}, Mfg: {5}, DeviceDesc: {6}, Service: {7}, SymbolicLink: {8}, DevicePath: {9}, LUsb0FilterIndex: {10}, Connected: {11}, SyncFlags: {12}}}', Common, DriverID, DeviceInterfaceGUID, InstanceID, ClassGUID, Mfg, DeviceDesc, Service, SymbolicLink, DevicePath, LUsb0FilterIndex, Connected, SyncFlags)

#endregion


#region Internal Function Imports
internal static class Functions:

	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LibK_GetVersion', SetLastError: true)]
	public static def LibK_GetVersion([Out] ref Version as KLIB_VERSION):
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LibK_GetContext', SetLastError: true)]
	public static def LibK_GetContext([In] Handle as IntPtr, HandleType as KLIB_HANDLE_TYPE) as IntPtr:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LibK_SetContext', SetLastError: true)]
	public static def LibK_SetContext([In] Handle as IntPtr, HandleType as KLIB_HANDLE_TYPE, ContextValue as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LibK_SetCleanupCallback', SetLastError: true)]
	public static def LibK_SetCleanupCallback([In] Handle as IntPtr, HandleType as KLIB_HANDLE_TYPE, CleanupCB as KLIB_HANDLE_CLEANUP_CB) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LibK_LoadDriverAPI', SetLastError: true)]
	public static def LibK_LoadDriverAPI([Out] ref DriverAPI as KUSB_DRIVER_API, DriverID as int) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LibK_CopyDriverAPI', SetLastError: true)]
	public static def LibK_CopyDriverAPI([Out] ref DriverAPI as KUSB_DRIVER_API, [In] UsbHandle as KUSB_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LibK_GetProcAddress', SetLastError: true)]
	public static def LibK_GetProcAddress(ProcAddress as IntPtr, DriverID as int, FunctionID as int) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_Init', SetLastError: true)]
	public static def UsbK_Init([Out] ref InterfaceHandle as KUSB_HANDLE, [In] DevInfo as KLST_DEVINFO_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_Free', SetLastError: true)]
	public static def UsbK_Free([In] InterfaceHandle as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_ClaimInterface', SetLastError: true)]
	public static def UsbK_ClaimInterface([In] InterfaceHandle as KUSB_HANDLE, NumberOrIndex as byte, IsIndex as bool) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_ReleaseInterface', SetLastError: true)]
	public static def UsbK_ReleaseInterface([In] InterfaceHandle as KUSB_HANDLE, NumberOrIndex as byte, IsIndex as bool) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_SetAltInterface', SetLastError: true)]
	public static def UsbK_SetAltInterface([In] InterfaceHandle as KUSB_HANDLE, NumberOrIndex as byte, IsIndex as bool, AltSettingNumber as byte) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_GetAltInterface', SetLastError: true)]
	public static def UsbK_GetAltInterface([In] InterfaceHandle as KUSB_HANDLE, NumberOrIndex as byte, IsIndex as bool, ref AltSettingNumber as byte) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_GetDescriptor', SetLastError: true)]
	public static def UsbK_GetDescriptor([In] InterfaceHandle as KUSB_HANDLE, DescriptorType as byte, Index as byte, LanguageID as ushort, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_ControlTransfer', SetLastError: true)]
	public static def UsbK_ControlTransfer([In] InterfaceHandle as KUSB_HANDLE, SetupPacket as WINUSB_SETUP_PACKET, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint, Overlapped as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_SetPowerPolicy', SetLastError: true)]
	public static def UsbK_SetPowerPolicy([In] InterfaceHandle as KUSB_HANDLE, PolicyType as uint, ValueLength as uint, Value as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_GetPowerPolicy', SetLastError: true)]
	public static def UsbK_GetPowerPolicy([In] InterfaceHandle as KUSB_HANDLE, PolicyType as uint, ref ValueLength as uint, Value as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_SetConfiguration', SetLastError: true)]
	public static def UsbK_SetConfiguration([In] InterfaceHandle as KUSB_HANDLE, ConfigurationNumber as byte) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_GetConfiguration', SetLastError: true)]
	public static def UsbK_GetConfiguration([In] InterfaceHandle as KUSB_HANDLE, ref ConfigurationNumber as byte) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_ResetDevice', SetLastError: true)]
	public static def UsbK_ResetDevice([In] InterfaceHandle as KUSB_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_Initialize', SetLastError: true)]
	public static def UsbK_Initialize(DeviceHandle as IntPtr, [Out] ref InterfaceHandle as KUSB_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_SelectInterface', SetLastError: true)]
	public static def UsbK_SelectInterface([In] InterfaceHandle as KUSB_HANDLE, NumberOrIndex as byte, IsIndex as bool) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_GetAssociatedInterface', SetLastError: true)]
	public static def UsbK_GetAssociatedInterface([In] InterfaceHandle as KUSB_HANDLE, AssociatedInterfaceIndex as byte, [Out] ref AssociatedInterfaceHandle as KUSB_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_Clone', SetLastError: true)]
	public static def UsbK_Clone([In] InterfaceHandle as KUSB_HANDLE, [Out] ref DstInterfaceHandle as KUSB_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_QueryInterfaceSettings', SetLastError: true)]
	public static def UsbK_QueryInterfaceSettings([In] InterfaceHandle as KUSB_HANDLE, AltSettingNumber as byte, [Out] ref UsbAltInterfaceDescriptor as USB_INTERFACE_DESCRIPTOR) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_QueryDeviceInformation', SetLastError: true)]
	public static def UsbK_QueryDeviceInformation([In] InterfaceHandle as KUSB_HANDLE, InformationType as uint, ref BufferLength as uint, Buffer as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_SetCurrentAlternateSetting', SetLastError: true)]
	public static def UsbK_SetCurrentAlternateSetting([In] InterfaceHandle as KUSB_HANDLE, AltSettingNumber as byte) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_GetCurrentAlternateSetting', SetLastError: true)]
	public static def UsbK_GetCurrentAlternateSetting([In] InterfaceHandle as KUSB_HANDLE, ref AltSettingNumber as byte) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_QueryPipe', SetLastError: true)]
	public static def UsbK_QueryPipe([In] InterfaceHandle as KUSB_HANDLE, AltSettingNumber as byte, PipeIndex as byte, [Out] ref PipeInformation as WINUSB_PIPE_INFORMATION) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_SetPipePolicy', SetLastError: true)]
	public static def UsbK_SetPipePolicy([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte, PolicyType as uint, ValueLength as uint, Value as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_GetPipePolicy', SetLastError: true)]
	public static def UsbK_GetPipePolicy([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte, PolicyType as uint, ref ValueLength as uint, Value as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_ReadPipe', SetLastError: true)]
	public static def UsbK_ReadPipe([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint, Overlapped as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_WritePipe', SetLastError: true)]
	public static def UsbK_WritePipe([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint, Overlapped as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_ResetPipe', SetLastError: true)]
	public static def UsbK_ResetPipe([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_AbortPipe', SetLastError: true)]
	public static def UsbK_AbortPipe([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_FlushPipe', SetLastError: true)]
	public static def UsbK_FlushPipe([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_IsoReadPipe', SetLastError: true)]
	public static def UsbK_IsoReadPipe([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte, Buffer as IntPtr, BufferLength as uint, Overlapped as IntPtr, [In] IsoContext as KISO_CONTEXT) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_IsoWritePipe', SetLastError: true)]
	public static def UsbK_IsoWritePipe([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte, Buffer as IntPtr, BufferLength as uint, Overlapped as IntPtr, [In] IsoContext as KISO_CONTEXT) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_GetCurrentFrameNumber', SetLastError: true)]
	public static def UsbK_GetCurrentFrameNumber([In] InterfaceHandle as KUSB_HANDLE, ref FrameNumber as uint) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_GetOverlappedResult', SetLastError: true)]
	public static def UsbK_GetOverlappedResult([In] InterfaceHandle as KUSB_HANDLE, Overlapped as IntPtr, ref lpNumberOfBytesTransferred as uint, bWait as bool) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'UsbK_GetProperty', SetLastError: true)]
	public static def UsbK_GetProperty([In] InterfaceHandle as KUSB_HANDLE, PropertyType as KUSB_PROPERTY, ref PropertySize as uint, Value as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LstK_Init', SetLastError: true)]
	public static def LstK_Init([Out] ref DeviceList as KLST_HANDLE, Flags as KLST_FLAG) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LstK_InitEx', SetLastError: true)]
	public static def LstK_InitEx([Out] ref DeviceList as KLST_HANDLE, Flags as KLST_FLAG, [In] ref PatternMatch as KLST_PATTERN_MATCH) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LstK_Free', SetLastError: true)]
	public static def LstK_Free([In] DeviceList as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LstK_Enumerate', SetLastError: true)]
	public static def LstK_Enumerate([In] DeviceList as KLST_HANDLE, EnumDevListCB as KLST_ENUM_DEVINFO_CB, Context as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LstK_Current', SetLastError: true)]
	public static def LstK_Current([In] DeviceList as KLST_HANDLE, [Out] ref DeviceInfo as KLST_DEVINFO_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LstK_MoveNext', SetLastError: true)]
	public static def LstK_MoveNext([In] DeviceList as KLST_HANDLE, [Out] ref DeviceInfo as KLST_DEVINFO_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LstK_MoveReset', SetLastError: true)]
	public static def LstK_MoveReset([In] DeviceList as KLST_HANDLE):
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LstK_FindByVidPid', SetLastError: true)]
	public static def LstK_FindByVidPid([In] DeviceList as KLST_HANDLE, Vid as int, Pid as int, [Out] ref DeviceInfo as KLST_DEVINFO_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'LstK_Count', SetLastError: true)]
	public static def LstK_Count([In] DeviceList as KLST_HANDLE, ref Count as uint) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'HotK_Init', SetLastError: true)]
	public static def HotK_Init([Out] ref Handle as KHOT_HANDLE, [In] [Out] ref InitParams as KHOT_PARAMS) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'HotK_Free', SetLastError: true)]
	public static def HotK_Free([In] Handle as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'HotK_FreeAll', SetLastError: true)]
	public static def HotK_FreeAll():
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'OvlK_Acquire', SetLastError: true)]
	public static def OvlK_Acquire([Out] ref OverlappedK as KOVL_HANDLE, [In] PoolHandle as KOVL_POOL_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'OvlK_Release', SetLastError: true)]
	public static def OvlK_Release([In] OverlappedK as KOVL_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'OvlK_Init', SetLastError: true)]
	public static def OvlK_Init([Out] ref PoolHandle as KOVL_POOL_HANDLE, [In] UsbHandle as KUSB_HANDLE, MaxOverlappedCount as int, Flags as KOVL_POOL_FLAG) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'OvlK_Free', SetLastError: true)]
	public static def OvlK_Free([In] PoolHandle as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'OvlK_GetEventHandle', SetLastError: true)]
	public static def OvlK_GetEventHandle([In] OverlappedK as KOVL_HANDLE) as IntPtr:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'OvlK_Wait', SetLastError: true)]
	public static def OvlK_Wait([In] OverlappedK as KOVL_HANDLE, TimeoutMS as int, WaitFlags as KOVL_WAIT_FLAG, ref TransferredLength as uint) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'OvlK_WaitOrCancel', SetLastError: true)]
	public static def OvlK_WaitOrCancel([In] OverlappedK as KOVL_HANDLE, TimeoutMS as int, ref TransferredLength as uint) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'OvlK_WaitAndRelease', SetLastError: true)]
	public static def OvlK_WaitAndRelease([In] OverlappedK as KOVL_HANDLE, TimeoutMS as int, ref TransferredLength as uint) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'OvlK_IsComplete', SetLastError: true)]
	public static def OvlK_IsComplete([In] OverlappedK as KOVL_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'OvlK_ReUse', SetLastError: true)]
	public static def OvlK_ReUse([In] OverlappedK as KOVL_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'StmK_Init', SetLastError: true)]
	public static def StmK_Init([Out] ref StreamHandle as KSTM_HANDLE, [In] UsbHandle as KUSB_HANDLE, PipeID as byte, MaxTransferSize as int, MaxPendingTransfers as int, MaxPendingIO as int, [In] ref Callbacks as KSTM_CALLBACK, Flags as KSTM_FLAG) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'StmK_Free', SetLastError: true)]
	public static def StmK_Free([In] StreamHandle as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'StmK_Start', SetLastError: true)]
	public static def StmK_Start([In] StreamHandle as KSTM_HANDLE) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'StmK_Stop', SetLastError: true)]
	public static def StmK_Stop([In] StreamHandle as KSTM_HANDLE, TimeoutCancelMS as int) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'StmK_Read', SetLastError: true)]
	public static def StmK_Read([In] StreamHandle as KSTM_HANDLE, Buffer as IntPtr, Offset as int, Length as int, ref TransferredLength as uint) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'StmK_Write', SetLastError: true)]
	public static def StmK_Write([In] StreamHandle as KSTM_HANDLE, Buffer as IntPtr, Offset as int, Length as int, ref TransferredLength as uint) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'IsoK_Init', SetLastError: true)]
	public static def IsoK_Init([Out] ref IsoContext as KISO_CONTEXT, NumberOfPackets as int, StartFrame as int) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'IsoK_Free', SetLastError: true)]
	public static def IsoK_Free([In] IsoContext as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'IsoK_SetPackets', SetLastError: true)]
	public static def IsoK_SetPackets([In] IsoContext as KISO_CONTEXT, PacketSize as int) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'IsoK_SetPacket', SetLastError: true)]
	public static def IsoK_SetPacket([In] IsoContext as KISO_CONTEXT, PacketIndex as int, [In] ref IsoPacket as KISO_PACKET) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'IsoK_GetPacket', SetLastError: true)]
	public static def IsoK_GetPacket([In] IsoContext as KISO_CONTEXT, PacketIndex as int, [Out] ref IsoPacket as KISO_PACKET) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'IsoK_EnumPackets', SetLastError: true)]
	public static def IsoK_EnumPackets([In] IsoContext as KISO_CONTEXT, EnumPackets as KISO_ENUM_PACKETS_CB, StartPacketIndex as int, UserState as IntPtr) as bool:
		pass

	
	[DllImport(Constants.LIBUSBK_DLL, CallingConvention: CallingConvention.Winapi, CharSet: CharSet.Ansi, EntryPoint: 'IsoK_ReUse', SetLastError: true)]
	public static def IsoK_ReUse([In] IsoContext as KISO_CONTEXT) as bool:
		pass

#endregion


#region Enumerations
public enum USBD_PIPE_TYPE:

	UsbdPipeTypeControl

	
	UsbdPipeTypeIsochronous

	
	UsbdPipeTypeBulk

	
	UsbdPipeTypeInterrupt


[Flags]
public enum KISO_FLAG:

	NONE = 0

	
	SET_START_FRAME = 1


public enum KLIB_HANDLE_TYPE:

	HOTK

	
	USBK

	
	USBSHAREDK

	
	LSTK

	
	LSTINFOK

	
	OVLK

	
	OVLPOOLK

	
	STMK

	
	COUNT


[Flags]
public enum KLST_SYNC_FLAG:

	NONE = 0

	
	UNCHANGED = 1

	
	ADDED = 2

	
	REMOVED = 4

	
	CONNECT_CHANGE = 8

	
	MASK = 15


[Flags]
public enum KLST_FLAG:

	NONE = 0

	
	INCLUDE_RAWGUID = 1

	
	INCLUDE_DISCONNECT = 2


public enum BMREQUEST_DIR:

	HOST_TO_DEVICE = 0

	DEVICE_TO_HOST = 1


public enum BMREQUEST_TYPE:

	STANDARD = 0

	
	CLASS = 1

	
	VENDOR = 2


public enum BMREQUEST_RECIPIENT:

	DEVICE = 0

	
	INTERFACE = 1

	
	ENDPOINT = 2

	
	OTHER = 3


public enum USB_GETSTATUS:

	SELF_POWERED = 1

	
	REMOTE_WAKEUP_ENABLED = 2


public enum USB_DESCRIPTOR_TYPE:

	DEVICE = 1

	
	CONFIGURATION = 2

	
	STRING = 3

	
	INTERFACE = 4

	
	ENDPOINT = 5

	
	DEVICE_QUALIFIER = 6

	
	CONFIG_POWER = 7

	
	INTERFACE_POWER = 8

	
	INTERFACE_ASSOCIATION = 11


public enum KUSB_PROPERTY:

	DEVICE_FILE_HANDLE

	
	COUNT


public enum KUSB_DRVID:

	LIBUSBK

	
	LIBUSB0

	
	WINUSB

	
	LIBUSB0_FILTER

	
	COUNT


public enum KUSB_FNID:

	Init

	
	Free

	
	ClaimInterface

	
	ReleaseInterface

	
	SetAltInterface

	
	GetAltInterface

	
	GetDescriptor

	
	ControlTransfer

	
	SetPowerPolicy

	
	GetPowerPolicy

	
	SetConfiguration

	
	GetConfiguration

	
	ResetDevice

	
	Initialize

	
	SelectInterface

	
	GetAssociatedInterface

	
	Clone

	
	QueryInterfaceSettings

	
	QueryDeviceInformation

	
	SetCurrentAlternateSetting

	
	GetCurrentAlternateSetting

	
	QueryPipe

	
	SetPipePolicy

	
	GetPipePolicy

	
	ReadPipe

	
	WritePipe

	
	ResetPipe

	
	AbortPipe

	
	FlushPipe

	
	IsoReadPipe

	
	IsoWritePipe

	
	GetCurrentFrameNumber

	
	GetOverlappedResult

	
	GetProperty

	
	COUNT


[Flags]
public enum KHOT_FLAG:

	NONE

	
	PLUG_ALL_ON_INIT = 1

	
	PASS_DUPE_INSTANCE = 2

	
	POST_USER_MESSAGE = 4


[Flags]
public enum KOVL_WAIT_FLAG:

	NONE = 0

	
	RELEASE_ON_SUCCESS = 1

	
	RELEASE_ON_FAIL = 2

	
	RELEASE_ON_SUCCESS_FAIL = 3

	
	CANCEL_ON_TIMEOUT = 4

	
	RELEASE_ON_TIMEOUT = 12

	
	RELEASE_ALWAYS = 15

	
	ALERTABLE = 16


[Flags]
public enum KOVL_POOL_FLAG:

	NONE = 0


[Flags]
public enum KSTM_FLAG:

	NONE = 0


public enum KSTM_COMPLETE_RESULT:

	VALID = 0

	INVALID

#endregion


#region Structs
[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi)]
public struct WINUSB_PIPE_INFORMATION:

	public PipeType as USBD_PIPE_TYPE

	
	public PipeId as byte

	
	public MaximumPacketSize as ushort

	
	public Interval as byte


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Pack: 1)]
public struct WINUSB_SETUP_PACKET:

	public RequestType as byte

	
	public Request as byte

	
	public Value as ushort

	
	public Index as ushort

	
	public Length as ushort


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Pack: 1)]
public struct KISO_PACKET:

	public Offset as uint

	
	public Length as ushort

	
	public Status as ushort


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Pack: 1, Size: 16)]
internal struct KISO_CONTEXT_MAP:

	public Flags as KISO_FLAG

	
	public StartFrame as uint

	
	public ErrorCount as short

	
	public NumberOfPackets as short


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi)]
public struct KLIB_VERSION:

	public Major as int

	
	public Minor as int

	
	public Micro as int

	
	public Nano as int


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi)]
public struct KLST_DEV_COMMON_INFO:

	public Vid as int

	
	public Pid as int

	
	public MI as int

	
	// An ID that uniquely identifies a USB device.
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public InstanceID as string


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi)]
public struct KLST_DEVINFO:

	public Common as KLST_DEV_COMMON_INFO

	
	public DriverID as int

	
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public DeviceInterfaceGUID as string

	
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public InstanceID as string

	
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public ClassGUID as string

	
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public Mfg as string

	
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public DeviceDesc as string

	
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public Service as string

	
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public SymbolicLink as string

	
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public DevicePath as string

	
	public LUsb0FilterIndex as int

	
	public Connected as bool

	
	public SyncFlags as KLST_SYNC_FLAG


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Size: 1024)]
public struct KLST_PATTERN_MATCH:

	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public InstanceID as string

	
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public DeviceInterfaceGUID as string

	
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public SymbolicLink as string


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Pack: 1)]
public struct USB_DEVICE_DESCRIPTOR:

	public bLength as byte

	
	public bDescriptorType as byte

	
	public bcdUSB as ushort

	
	public bDeviceClass as byte

	
	public bDeviceSubClass as byte

	
	public bDeviceProtocol as byte

	
	public bMaxPacketSize0 as byte

	
	public idVendor as ushort

	
	public idProduct as ushort

	
	public bcdDevice as ushort

	
	public iManufacturer as byte

	
	public iProduct as byte

	
	public iSerialNumber as byte

	
	public bNumConfigurations as byte


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Pack: 1)]
public struct USB_ENDPOINT_DESCRIPTOR:

	public bLength as byte

	
	public bDescriptorType as byte

	
	public bEndpointAddress as byte

	
	public bmAttributes as byte

	
	public wMaxPacketSize as ushort

	
	public bInterval as byte


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Pack: 1)]
public struct USB_CONFIGURATION_DESCRIPTOR:

	public bLength as byte

	
	public bDescriptorType as byte

	
	public wTotalLength as ushort

	
	public bNumInterfaces as byte

	
	public bConfigurationValue as byte

	
	public iConfiguration as byte

	
	public bmAttributes as byte

	
	public MaxPower as byte


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Pack: 1)]
public struct USB_INTERFACE_DESCRIPTOR:

	public bLength as byte

	
	public bDescriptorType as byte

	
	public bInterfaceNumber as byte

	
	public bAlternateSetting as byte

	
	public bNumEndpoints as byte

	
	public bInterfaceClass as byte

	
	public bInterfaceSubClass as byte

	
	public bInterfaceProtocol as byte

	
	public iInterface as byte


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Unicode, Pack: 1)]
public struct USB_STRING_DESCRIPTOR:

	public bLength as byte

	
	public bDescriptorType as byte

	
	[MarshalAs(UnmanagedType.ByValTStr, SizeConst: Constants.KLST_STRING_MAX_LEN)]
	public bString as string


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Pack: 1)]
public struct USB_COMMON_DESCRIPTOR:

	public bLength as byte

	
	public bDescriptorType as byte


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Pack: 1)]
public struct USB_INTERFACE_ASSOCIATION_DESCRIPTOR:

	public bLength as byte

	
	public bDescriptorType as byte

	
	public bFirstInterface as byte

	
	public bInterfaceCount as byte

	
	public bFunctionClass as byte

	
	public bFunctionSubClass as byte

	
	public bFunctionProtocol as byte

	
	public iFunction as byte


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi)]
public struct KUSB_DRIVER_API_INFO:

	public DriverID as int

	
	public FunctionCount as int


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Size: 512)]
public struct KUSB_DRIVER_API:

	public Info as KUSB_DRIVER_API_INFO

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public Init as KUSB_InitDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public Free as KUSB_FreeDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public ClaimInterface as KUSB_ClaimInterfaceDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public ReleaseInterface as KUSB_ReleaseInterfaceDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public SetAltInterface as KUSB_SetAltInterfaceDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public GetAltInterface as KUSB_GetAltInterfaceDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public GetDescriptor as KUSB_GetDescriptorDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public ControlTransfer as KUSB_ControlTransferDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public SetPowerPolicy as KUSB_SetPowerPolicyDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public GetPowerPolicy as KUSB_GetPowerPolicyDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public SetConfiguration as KUSB_SetConfigurationDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public GetConfiguration as KUSB_GetConfigurationDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public ResetDevice as KUSB_ResetDeviceDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public Initialize as KUSB_InitializeDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public SelectInterface as KUSB_SelectInterfaceDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public GetAssociatedInterface as KUSB_GetAssociatedInterfaceDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public Clone as KUSB_CloneDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public QueryInterfaceSettings as KUSB_QueryInterfaceSettingsDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public QueryDeviceInformation as KUSB_QueryDeviceInformationDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public SetCurrentAlternateSetting as KUSB_SetCurrentAlternateSettingDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public GetCurrentAlternateSetting as KUSB_GetCurrentAlternateSettingDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public QueryPipe as KUSB_QueryPipeDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public SetPipePolicy as KUSB_SetPipePolicyDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public GetPipePolicy as KUSB_GetPipePolicyDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public ReadPipe as KUSB_ReadPipeDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public WritePipe as KUSB_WritePipeDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public ResetPipe as KUSB_ResetPipeDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public AbortPipe as KUSB_AbortPipeDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public FlushPipe as KUSB_FlushPipeDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public IsoReadPipe as KUSB_IsoReadPipeDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public IsoWritePipe as KUSB_IsoWritePipeDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public GetCurrentFrameNumber as KUSB_GetCurrentFrameNumberDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public GetOverlappedResult as KUSB_GetOverlappedResultDelegate

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public GetProperty as KUSB_GetPropertyDelegate


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Size: 2048)]
public struct KHOT_PARAMS:

	public UserHwnd as IntPtr

	
	public UserMessage as uint

	
	public Flags as KHOT_FLAG

	
	public PatternMatch as KLST_PATTERN_MATCH

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public OnHotPlug as KHOT_PLUG_CB


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi)]
public struct KSTM_XFER_CONTEXT:

	public Buffer as IntPtr

	
	public BufferSize as int

	
	public TransferLength as int

	
	public UserState as IntPtr


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi)]
public struct KSTM_INFO:

	public UsbHandle as IntPtr

	
	public PipeID as byte

	
	public MaxPendingTransfers as int

	
	public MaxTransferSize as int

	
	public MaxPendingIO as int

	
	public EndpointDescriptor as USB_ENDPOINT_DESCRIPTOR

	
	public DriverAPI as KUSB_DRIVER_API

	
	public DeviceHandle as IntPtr


[StructLayout(LayoutKind.Sequential, CharSet: CharSet.Ansi, Size: 64)]
public struct KSTM_CALLBACK:

	[MarshalAs(UnmanagedType.FunctionPtr)]
	public Error as KSTM_ERROR_CB

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public Submit as KSTM_SUBMIT_CB

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public Complete as KSTM_COMPLETE_CB

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public Started as KSTM_STARTED_CB

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public Stopped as KSTM_STOPPED_CB

	
	[MarshalAs(UnmanagedType.FunctionPtr)]
	public BeforeComplete as KSTM_BEFORE_COMPLETE_CB

#endregion


#region Delegates
[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KLIB_HANDLE_CLEANUP_CB([In] Handle as IntPtr, HandleType as KLIB_HANDLE_TYPE, UserContext as IntPtr) as int

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KISO_ENUM_PACKETS_CB(PacketIndex as uint, [In] ref IsoPacket as KISO_PACKET, UserState as IntPtr) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KLST_ENUM_DEVINFO_CB([In] DeviceList as IntPtr, [In] DeviceInfo as KLST_DEVINFO_HANDLE, Context as IntPtr) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_InitDelegate([Out] ref InterfaceHandle as KUSB_HANDLE, [In] DevInfo as KLST_DEVINFO_HANDLE) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_FreeDelegate([In] InterfaceHandle as KUSB_HANDLE) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_ClaimInterfaceDelegate([In] InterfaceHandle as KUSB_HANDLE, NumberOrIndex as byte, IsIndex as bool) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_ReleaseInterfaceDelegate([In] InterfaceHandle as KUSB_HANDLE, NumberOrIndex as byte, IsIndex as bool) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_SetAltInterfaceDelegate([In] InterfaceHandle as KUSB_HANDLE, NumberOrIndex as byte, IsIndex as bool, AltSettingNumber as byte) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_GetAltInterfaceDelegate([In] InterfaceHandle as KUSB_HANDLE, NumberOrIndex as byte, IsIndex as bool, ref AltSettingNumber as byte) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_GetDescriptorDelegate([In] InterfaceHandle as KUSB_HANDLE, DescriptorType as byte, Index as byte, LanguageID as ushort, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_ControlTransferDelegate([In] InterfaceHandle as KUSB_HANDLE, SetupPacket as WINUSB_SETUP_PACKET, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint, Overlapped as IntPtr) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_SetPowerPolicyDelegate([In] InterfaceHandle as KUSB_HANDLE, PolicyType as uint, ValueLength as uint, Value as IntPtr) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_GetPowerPolicyDelegate([In] InterfaceHandle as KUSB_HANDLE, PolicyType as uint, ref ValueLength as uint, Value as IntPtr) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_SetConfigurationDelegate([In] InterfaceHandle as KUSB_HANDLE, ConfigurationNumber as byte) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_GetConfigurationDelegate([In] InterfaceHandle as KUSB_HANDLE, ref ConfigurationNumber as byte) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_ResetDeviceDelegate([In] InterfaceHandle as KUSB_HANDLE) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_InitializeDelegate(DeviceHandle as IntPtr, [Out] ref InterfaceHandle as KUSB_HANDLE) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_SelectInterfaceDelegate([In] InterfaceHandle as KUSB_HANDLE, NumberOrIndex as byte, IsIndex as bool) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_GetAssociatedInterfaceDelegate([In] InterfaceHandle as KUSB_HANDLE, AssociatedInterfaceIndex as byte, [Out] ref AssociatedInterfaceHandle as KUSB_HANDLE) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_CloneDelegate([In] InterfaceHandle as KUSB_HANDLE, [Out] ref DstInterfaceHandle as KUSB_HANDLE) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_QueryInterfaceSettingsDelegate([In] InterfaceHandle as KUSB_HANDLE, AltSettingNumber as byte, [Out] ref UsbAltInterfaceDescriptor as USB_INTERFACE_DESCRIPTOR) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_QueryDeviceInformationDelegate([In] InterfaceHandle as KUSB_HANDLE, InformationType as uint, ref BufferLength as uint, Buffer as IntPtr) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_SetCurrentAlternateSettingDelegate([In] InterfaceHandle as KUSB_HANDLE, AltSettingNumber as byte) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_GetCurrentAlternateSettingDelegate([In] InterfaceHandle as KUSB_HANDLE, ref AltSettingNumber as byte) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_QueryPipeDelegate([In] InterfaceHandle as KUSB_HANDLE, AltSettingNumber as byte, PipeIndex as byte, [Out] ref PipeInformation as WINUSB_PIPE_INFORMATION) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_SetPipePolicyDelegate([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte, PolicyType as uint, ValueLength as uint, Value as IntPtr) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_GetPipePolicyDelegate([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte, PolicyType as uint, ref ValueLength as uint, Value as IntPtr) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_ReadPipeDelegate([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint, Overlapped as IntPtr) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_WritePipeDelegate([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint, Overlapped as IntPtr) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_ResetPipeDelegate([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_AbortPipeDelegate([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_FlushPipeDelegate([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_IsoReadPipeDelegate([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte, Buffer as IntPtr, BufferLength as uint, Overlapped as IntPtr, [In] IsoContext as KISO_CONTEXT) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_IsoWritePipeDelegate([In] InterfaceHandle as KUSB_HANDLE, PipeID as byte, Buffer as IntPtr, BufferLength as uint, Overlapped as IntPtr, [In] IsoContext as KISO_CONTEXT) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_GetCurrentFrameNumberDelegate([In] InterfaceHandle as KUSB_HANDLE, ref FrameNumber as uint) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_GetOverlappedResultDelegate([In] InterfaceHandle as KUSB_HANDLE, Overlapped as IntPtr, ref lpNumberOfBytesTransferred as uint, bWait as bool) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KUSB_GetPropertyDelegate([In] InterfaceHandle as KUSB_HANDLE, PropertyType as KUSB_PROPERTY, ref PropertySize as uint, Value as IntPtr) as bool

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KHOT_PLUG_CB([In] HotHandle as IntPtr, [In] DeviceInfo as KLST_DEVINFO_HANDLE, PlugType as KLST_SYNC_FLAG) as void

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KSTM_ERROR_CB([In] ref StreamInfo as KSTM_INFO, [In] ref XferContext as KSTM_XFER_CONTEXT, ErrorCode as int) as int

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KSTM_SUBMIT_CB([In] ref StreamInfo as KSTM_INFO, [In] ref XferContext as KSTM_XFER_CONTEXT, Overlapped as IntPtr) as int

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KSTM_STARTED_CB([In] ref StreamInfo as KSTM_INFO, [In] ref XferContext as KSTM_XFER_CONTEXT, XferContextIndex as int) as int

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KSTM_STOPPED_CB([In] ref StreamInfo as KSTM_INFO, [In] ref XferContext as KSTM_XFER_CONTEXT, XferContextIndex as int) as int

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KSTM_COMPLETE_CB([In] ref StreamInfo as KSTM_INFO, [In] ref XferContext as KSTM_XFER_CONTEXT, ErrorCode as int) as int

[UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet: CharSet.Ansi, SetLastError: true)]
public callable KSTM_BEFORE_COMPLETE_CB([In] ref StreamInfo as KSTM_INFO, [In] ref XferContext as KSTM_XFER_CONTEXT, ref ErrorCode as int) as KSTM_COMPLETE_RESULT
#endregion


public class LstK:

	protected final handle as KLST_HANDLE

	
	public def constructor(Flags as KLST_FLAG):
		RuntimeHelpers.PrepareConstrainedRegions()
		
		try:
			pass
		ensure:
			success as bool = Functions.LstK_Init(handle, Flags)
			
			if ((not success) or handle.IsInvalid) or handle.IsClosed:
				handle.SetHandleAsInvalid()
				errorCode as int = Marshal.GetLastWin32Error()
				raise Exception(((GetType().Name + ' failed. ErrorCode=') + errorCode.ToString('X')))

	
	public def constructor(Flags as KLST_FLAG, ref PatternMatch as KLST_PATTERN_MATCH):
		RuntimeHelpers.PrepareConstrainedRegions()
		
		try:
			pass
		ensure:
			success as bool = Functions.LstK_InitEx(handle, Flags, PatternMatch)
			
			if ((not success) or handle.IsInvalid) or handle.IsClosed:
				handle.SetHandleAsInvalid()
				errorCode as int = Marshal.GetLastWin32Error()
				raise Exception(((GetType().Name + ' failed. ErrorCode=') + errorCode.ToString('X')))

	
	public Handle as KLST_HANDLE:
		get:
			return handle

	
	public virtual def Free() as bool:
		if handle.IsInvalid or handle.IsClosed:
			return false
		
		handle.Close()
		return true

	
	public def Enumerate(EnumDevListCB as KLST_ENUM_DEVINFO_CB, Context as IntPtr) as bool:
		return Functions.LstK_Enumerate(handle, EnumDevListCB, Context)

	
	public def Current(ref DeviceInfo as KLST_DEVINFO_HANDLE) as bool:
		return Functions.LstK_Current(handle, DeviceInfo)

	
	public def MoveNext(ref DeviceInfo as KLST_DEVINFO_HANDLE) as bool:
		return Functions.LstK_MoveNext(handle, DeviceInfo)

	
	public def MoveReset():
		Functions.LstK_MoveReset(handle)

	
	public def FindByVidPid(Vid as int, Pid as int, ref DeviceInfo as KLST_DEVINFO_HANDLE) as bool:
		return Functions.LstK_FindByVidPid(handle, Vid, Pid, DeviceInfo)

	
	public def Count(ref Count as uint) as bool:
		return Functions.LstK_Count(handle, Count)


public class HotK:

	protected final handle as KHOT_HANDLE

	
	public def constructor(ref InitParams as KHOT_PARAMS):
		RuntimeHelpers.PrepareConstrainedRegions()
		
		try:
			pass
		ensure:
			success as bool = Functions.HotK_Init(handle, InitParams)
			
			if ((not success) or handle.IsInvalid) or handle.IsClosed:
				handle.SetHandleAsInvalid()
				errorCode as int = Marshal.GetLastWin32Error()
				raise Exception(((GetType().Name + ' failed. ErrorCode=') + errorCode.ToString('X')))

	
	public Handle as KHOT_HANDLE:
		get:
			return handle

	
	public virtual def Free() as bool:
		if handle.IsInvalid or handle.IsClosed:
			return false
		
		handle.Close()
		return true

	
	public def FreeAll():
		Functions.HotK_FreeAll()


public class UsbK:

	protected final driverAPI as KUSB_DRIVER_API

	protected final handle as KUSB_HANDLE

	
	public def constructor(DevInfo as KLST_DEVINFO_HANDLE):
		if not Functions.LibK_LoadDriverAPI(driverAPI, DevInfo.DriverID):
			raise Exception(((GetType().Name + ' failed loading Driver API. ErrorCode=') + Marshal.GetLastWin32Error().ToString('X')))
		
		RuntimeHelpers.PrepareConstrainedRegions()
		
		try:
			pass
		ensure:
			success as bool = driverAPI.Init(handle, DevInfo)
			
			if ((not success) or handle.IsInvalid) or handle.IsClosed:
				handle.SetHandleAsInvalid()
				errorCode as int = Marshal.GetLastWin32Error()
				raise Exception(((GetType().Name + ' failed. ErrorCode=') + errorCode.ToString('X')))

	
	public def constructor(DeviceHandle as IntPtr, driverID as KUSB_DRVID):
		if not Functions.LibK_LoadDriverAPI(driverAPI, (driverID cast int)):
			raise Exception(((GetType().Name + ' failed loading Driver API. ErrorCode=') + Marshal.GetLastWin32Error().ToString('X')))
		
		RuntimeHelpers.PrepareConstrainedRegions()
		
		try:
			pass
		ensure:
			success as bool = driverAPI.Initialize(DeviceHandle, handle)
			
			if ((not success) or handle.IsInvalid) or handle.IsClosed:
				handle.SetHandleAsInvalid()
				errorCode as int = Marshal.GetLastWin32Error()
				raise Exception(((GetType().Name + ' failed. ErrorCode=') + errorCode.ToString('X')))

	
	public Handle as KUSB_HANDLE:
		get:
			return handle

	
	public virtual def Free() as bool:
		if handle.IsInvalid or handle.IsClosed:
			return false
		
		handle.Close()
		return true

	
	public def ClaimInterface(NumberOrIndex as byte, IsIndex as bool) as bool:
		return driverAPI.ClaimInterface(handle, NumberOrIndex, IsIndex)

	
	public def ReleaseInterface(NumberOrIndex as byte, IsIndex as bool) as bool:
		return driverAPI.ReleaseInterface(handle, NumberOrIndex, IsIndex)

	
	public def SetAltInterface(NumberOrIndex as byte, IsIndex as bool, AltSettingNumber as byte) as bool:
		return driverAPI.SetAltInterface(handle, NumberOrIndex, IsIndex, AltSettingNumber)

	
	public def GetAltInterface(NumberOrIndex as byte, IsIndex as bool, ref AltSettingNumber as byte) as bool:
		return driverAPI.GetAltInterface(handle, NumberOrIndex, IsIndex, AltSettingNumber)

	
	public def GetDescriptor(DescriptorType as byte, Index as byte, LanguageID as ushort, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint) as bool:
		return driverAPI.GetDescriptor(handle, DescriptorType, Index, LanguageID, Buffer, BufferLength, LengthTransferred)

	
	public def GetDescriptor(DescriptorType as byte, Index as byte, LanguageID as ushort, Buffer as Array, BufferLength as uint, ref LengthTransferred as uint) as bool:
		return driverAPI.GetDescriptor(handle, DescriptorType, Index, LanguageID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred)

	
	public def ControlTransfer(SetupPacket as WINUSB_SETUP_PACKET, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint, Overlapped as IntPtr) as bool:
		return driverAPI.ControlTransfer(handle, SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped)

	
	public def ControlTransfer(SetupPacket as WINUSB_SETUP_PACKET, Buffer as Array, BufferLength as uint, ref LengthTransferred as uint, Overlapped as IntPtr) as bool:
		return driverAPI.ControlTransfer(handle, SetupPacket, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped)

	
	public def ControlTransfer(SetupPacket as WINUSB_SETUP_PACKET, Buffer as Array, BufferLength as uint, ref LengthTransferred as uint, Overlapped as KOVL_HANDLE) as bool:
		return driverAPI.ControlTransfer(handle, SetupPacket, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped.DangerousGetHandle())

	
	public def ControlTransfer(SetupPacket as WINUSB_SETUP_PACKET, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint, Overlapped as KOVL_HANDLE) as bool:
		return driverAPI.ControlTransfer(handle, SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped.DangerousGetHandle())

	
	public def SetPowerPolicy(PolicyType as uint, ValueLength as uint, Value as IntPtr) as bool:
		return driverAPI.SetPowerPolicy(handle, PolicyType, ValueLength, Value)

	
	public def SetPowerPolicy(PolicyType as uint, ValueLength as uint, Value as Array) as bool:
		return driverAPI.SetPowerPolicy(handle, PolicyType, ValueLength, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))

	
	public def GetPowerPolicy(PolicyType as uint, ref ValueLength as uint, Value as IntPtr) as bool:
		return driverAPI.GetPowerPolicy(handle, PolicyType, ValueLength, Value)

	
	public def GetPowerPolicy(PolicyType as uint, ref ValueLength as uint, Value as Array) as bool:
		return driverAPI.GetPowerPolicy(handle, PolicyType, ValueLength, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))

	
	public def SetConfiguration(ConfigurationNumber as byte) as bool:
		return driverAPI.SetConfiguration(handle, ConfigurationNumber)

	
	public def GetConfiguration(ref ConfigurationNumber as byte) as bool:
		return driverAPI.GetConfiguration(handle, ConfigurationNumber)

	
	public def ResetDevice() as bool:
		return driverAPI.ResetDevice(handle)

	
	public def SelectInterface(NumberOrIndex as byte, IsIndex as bool) as bool:
		return driverAPI.SelectInterface(handle, NumberOrIndex, IsIndex)

	
	public def GetAssociatedInterface(AssociatedInterfaceIndex as byte, ref AssociatedInterfaceHandle as KUSB_HANDLE) as bool:
		return driverAPI.GetAssociatedInterface(handle, AssociatedInterfaceIndex, AssociatedInterfaceHandle)

	
	public def Clone(ref DstInterfaceHandle as KUSB_HANDLE) as bool:
		return driverAPI.Clone(handle, DstInterfaceHandle)

	
	public def QueryInterfaceSettings(AltSettingNumber as byte, ref UsbAltInterfaceDescriptor as USB_INTERFACE_DESCRIPTOR) as bool:
		return driverAPI.QueryInterfaceSettings(handle, AltSettingNumber, UsbAltInterfaceDescriptor)

	
	public def QueryDeviceInformation(InformationType as uint, ref BufferLength as uint, Buffer as IntPtr) as bool:
		return driverAPI.QueryDeviceInformation(handle, InformationType, BufferLength, Buffer)

	
	public def SetCurrentAlternateSetting(AltSettingNumber as byte) as bool:
		return driverAPI.SetCurrentAlternateSetting(handle, AltSettingNumber)

	
	public def GetCurrentAlternateSetting(ref AltSettingNumber as byte) as bool:
		return driverAPI.GetCurrentAlternateSetting(handle, AltSettingNumber)

	
	public def QueryPipe(AltSettingNumber as byte, PipeIndex as byte, ref PipeInformation as WINUSB_PIPE_INFORMATION) as bool:
		return driverAPI.QueryPipe(handle, AltSettingNumber, PipeIndex, PipeInformation)

	
	public def SetPipePolicy(PipeID as byte, PolicyType as uint, ValueLength as uint, Value as IntPtr) as bool:
		return driverAPI.SetPipePolicy(handle, PipeID, PolicyType, ValueLength, Value)

	
	public def SetPipePolicy(PipeID as byte, PolicyType as uint, ValueLength as uint, Value as Array) as bool:
		return driverAPI.SetPipePolicy(handle, PipeID, PolicyType, ValueLength, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))

	
	public def GetPipePolicy(PipeID as byte, PolicyType as uint, ref ValueLength as uint, Value as IntPtr) as bool:
		return driverAPI.GetPipePolicy(handle, PipeID, PolicyType, ValueLength, Value)

	
	public def GetPipePolicy(PipeID as byte, PolicyType as uint, ref ValueLength as uint, Value as Array) as bool:
		return driverAPI.GetPipePolicy(handle, PipeID, PolicyType, ValueLength, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))

	
	public def ReadPipe(PipeID as byte, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint, Overlapped as IntPtr) as bool:
		return driverAPI.ReadPipe(handle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped)

	
	public def ReadPipe(PipeID as byte, Buffer as Array, BufferLength as uint, ref LengthTransferred as uint, Overlapped as IntPtr) as bool:
		return driverAPI.ReadPipe(handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped)

	
	public def ReadPipe(PipeID as byte, Buffer as Array, BufferLength as uint, ref LengthTransferred as uint, Overlapped as KOVL_HANDLE) as bool:
		return driverAPI.ReadPipe(handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped.DangerousGetHandle())

	
	public def ReadPipe(PipeID as byte, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint, Overlapped as KOVL_HANDLE) as bool:
		return driverAPI.ReadPipe(handle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped.DangerousGetHandle())

	
	public def WritePipe(PipeID as byte, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint, Overlapped as IntPtr) as bool:
		return driverAPI.WritePipe(handle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped)

	
	public def WritePipe(PipeID as byte, Buffer as Array, BufferLength as uint, ref LengthTransferred as uint, Overlapped as IntPtr) as bool:
		return driverAPI.WritePipe(handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped)

	
	public def WritePipe(PipeID as byte, Buffer as Array, BufferLength as uint, ref LengthTransferred as uint, Overlapped as KOVL_HANDLE) as bool:
		return driverAPI.WritePipe(handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped.DangerousGetHandle())

	
	public def WritePipe(PipeID as byte, Buffer as IntPtr, BufferLength as uint, ref LengthTransferred as uint, Overlapped as KOVL_HANDLE) as bool:
		return driverAPI.WritePipe(handle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped.DangerousGetHandle())

	
	public def ResetPipe(PipeID as byte) as bool:
		return driverAPI.ResetPipe(handle, PipeID)

	
	public def AbortPipe(PipeID as byte) as bool:
		return driverAPI.AbortPipe(handle, PipeID)

	
	public def FlushPipe(PipeID as byte) as bool:
		return driverAPI.FlushPipe(handle, PipeID)

	
	public def IsoReadPipe(PipeID as byte, Buffer as IntPtr, BufferLength as uint, Overlapped as IntPtr, IsoContext as KISO_CONTEXT) as bool:
		return driverAPI.IsoReadPipe(handle, PipeID, Buffer, BufferLength, Overlapped, IsoContext)

	
	public def IsoReadPipe(PipeID as byte, Buffer as Array, BufferLength as uint, Overlapped as IntPtr, IsoContext as KISO_CONTEXT) as bool:
		return driverAPI.IsoReadPipe(handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, Overlapped, IsoContext)

	
	public def IsoReadPipe(PipeID as byte, Buffer as Array, BufferLength as uint, Overlapped as KOVL_HANDLE, IsoContext as KISO_CONTEXT) as bool:
		return driverAPI.IsoReadPipe(handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, Overlapped.DangerousGetHandle(), IsoContext)

	
	public def IsoReadPipe(PipeID as byte, Buffer as IntPtr, BufferLength as uint, Overlapped as KOVL_HANDLE, IsoContext as KISO_CONTEXT) as bool:
		return driverAPI.IsoReadPipe(handle, PipeID, Buffer, BufferLength, Overlapped.DangerousGetHandle(), IsoContext)

	
	public def IsoWritePipe(PipeID as byte, Buffer as IntPtr, BufferLength as uint, Overlapped as IntPtr, IsoContext as KISO_CONTEXT) as bool:
		return driverAPI.IsoWritePipe(handle, PipeID, Buffer, BufferLength, Overlapped, IsoContext)

	
	public def IsoWritePipe(PipeID as byte, Buffer as Array, BufferLength as uint, Overlapped as IntPtr, IsoContext as KISO_CONTEXT) as bool:
		return driverAPI.IsoWritePipe(handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, Overlapped, IsoContext)

	
	public def IsoWritePipe(PipeID as byte, Buffer as Array, BufferLength as uint, Overlapped as KOVL_HANDLE, IsoContext as KISO_CONTEXT) as bool:
		return driverAPI.IsoWritePipe(handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, Overlapped.DangerousGetHandle(), IsoContext)

	
	public def IsoWritePipe(PipeID as byte, Buffer as IntPtr, BufferLength as uint, Overlapped as KOVL_HANDLE, IsoContext as KISO_CONTEXT) as bool:
		return driverAPI.IsoWritePipe(handle, PipeID, Buffer, BufferLength, Overlapped.DangerousGetHandle(), IsoContext)

	
	public def GetCurrentFrameNumber(ref FrameNumber as uint) as bool:
		return driverAPI.GetCurrentFrameNumber(handle, FrameNumber)

	
	public def GetOverlappedResult(Overlapped as IntPtr, ref lpNumberOfBytesTransferred as uint, bWait as bool) as bool:
		return driverAPI.GetOverlappedResult(handle, Overlapped, lpNumberOfBytesTransferred, bWait)

	
	public def GetOverlappedResult(Overlapped as KOVL_HANDLE, ref lpNumberOfBytesTransferred as uint, bWait as bool) as bool:
		return driverAPI.GetOverlappedResult(handle, Overlapped.DangerousGetHandle(), lpNumberOfBytesTransferred, bWait)

	
	public def GetProperty(PropertyType as KUSB_PROPERTY, ref PropertySize as uint, Value as IntPtr) as bool:
		return driverAPI.GetProperty(handle, PropertyType, PropertySize, Value)

	
	public def GetProperty(PropertyType as KUSB_PROPERTY, ref PropertySize as uint, Value as Array) as bool:
		return driverAPI.GetProperty(handle, PropertyType, PropertySize, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))


public class OvlK:

	protected final handle as KOVL_POOL_HANDLE

	
	public def constructor(UsbHandle as KUSB_HANDLE, MaxOverlappedCount as int, Flags as KOVL_POOL_FLAG):
		RuntimeHelpers.PrepareConstrainedRegions()
		
		try:
			pass
		ensure:
			success as bool = Functions.OvlK_Init(handle, UsbHandle, MaxOverlappedCount, Flags)
			
			if ((not success) or handle.IsInvalid) or handle.IsClosed:
				handle.SetHandleAsInvalid()
				errorCode as int = Marshal.GetLastWin32Error()
				raise Exception(((GetType().Name + ' failed. ErrorCode=') + errorCode.ToString('X')))

	
	public Handle as KOVL_POOL_HANDLE:
		get:
			return handle

	
	public virtual def Free() as bool:
		if handle.IsInvalid or handle.IsClosed:
			return false
		
		handle.Close()
		return true

	
	public def Acquire(ref OverlappedK as KOVL_HANDLE) as bool:
		return Functions.OvlK_Acquire(OverlappedK, handle)

	
	public def Release(OverlappedK as KOVL_HANDLE) as bool:
		return Functions.OvlK_Release(OverlappedK)

	
	public def GetEventHandle(OverlappedK as KOVL_HANDLE) as IntPtr:
		return Functions.OvlK_GetEventHandle(OverlappedK)

	
	public def Wait(OverlappedK as KOVL_HANDLE, TimeoutMS as int, WaitFlags as KOVL_WAIT_FLAG, ref TransferredLength as uint) as bool:
		return Functions.OvlK_Wait(OverlappedK, TimeoutMS, WaitFlags, TransferredLength)

	
	public def WaitOrCancel(OverlappedK as KOVL_HANDLE, TimeoutMS as int, ref TransferredLength as uint) as bool:
		return Functions.OvlK_WaitOrCancel(OverlappedK, TimeoutMS, TransferredLength)

	
	public def WaitAndRelease(OverlappedK as KOVL_HANDLE, TimeoutMS as int, ref TransferredLength as uint) as bool:
		return Functions.OvlK_WaitAndRelease(OverlappedK, TimeoutMS, TransferredLength)

	
	public def IsComplete(OverlappedK as KOVL_HANDLE) as bool:
		return Functions.OvlK_IsComplete(OverlappedK)

	
	public def ReUse(OverlappedK as KOVL_HANDLE) as bool:
		return Functions.OvlK_ReUse(OverlappedK)


public class StmK:

	protected final handle as KSTM_HANDLE

	
	public def constructor(UsbHandle as KUSB_HANDLE, PipeID as byte, MaxTransferSize as int, MaxPendingTransfers as int, MaxPendingIO as int, ref Callbacks as KSTM_CALLBACK, Flags as KSTM_FLAG):
		RuntimeHelpers.PrepareConstrainedRegions()
		
		try:
			pass
		ensure:
			success as bool = Functions.StmK_Init(handle, UsbHandle, PipeID, MaxTransferSize, MaxPendingTransfers, MaxPendingIO, Callbacks, Flags)
			
			if ((not success) or handle.IsInvalid) or handle.IsClosed:
				handle.SetHandleAsInvalid()
				errorCode as int = Marshal.GetLastWin32Error()
				raise Exception(((GetType().Name + ' failed. ErrorCode=') + errorCode.ToString('X')))

	
	public Handle as KSTM_HANDLE:
		get:
			return handle

	
	public virtual def Free() as bool:
		if handle.IsInvalid or handle.IsClosed:
			return false
		
		handle.Close()
		return true

	
	public def Start() as bool:
		return Functions.StmK_Start(handle)

	
	public def Stop(TimeoutCancelMS as int) as bool:
		return Functions.StmK_Stop(handle, TimeoutCancelMS)

	
	public def Read(Buffer as IntPtr, Offset as int, Length as int, ref TransferredLength as uint) as bool:
		return Functions.StmK_Read(handle, Buffer, Offset, Length, TransferredLength)

	
	public def Read(Buffer as Array, Offset as int, Length as int, ref TransferredLength as uint) as bool:
		return Functions.StmK_Read(handle, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), Offset, Length, TransferredLength)

	
	public def Write(Buffer as IntPtr, Offset as int, Length as int, ref TransferredLength as uint) as bool:
		return Functions.StmK_Write(handle, Buffer, Offset, Length, TransferredLength)

	
	public def Write(Buffer as Array, Offset as int, Length as int, ref TransferredLength as uint) as bool:
		return Functions.StmK_Write(handle, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), Offset, Length, TransferredLength)


public class IsoK:

	private static final ofsFlags as int = Marshal.OffsetOf(typeof(KISO_CONTEXT_MAP), 'Flags').ToInt32()

	private static final ofsStartFrame as int = Marshal.OffsetOf(typeof(KISO_CONTEXT_MAP), 'StartFrame').ToInt32()

	private static final ofsErrorCount as int = Marshal.OffsetOf(typeof(KISO_CONTEXT_MAP), 'ErrorCount').ToInt32()

	private static final ofsNumberOfPackets as int = Marshal.OffsetOf(typeof(KISO_CONTEXT_MAP), 'NumberOfPackets').ToInt32()

	protected final handle as KISO_CONTEXT

	
	public def constructor(NumberOfPackets as int, StartFrame as int):
		RuntimeHelpers.PrepareConstrainedRegions()
		
		try:
			pass
		ensure:
			success as bool = Functions.IsoK_Init(handle, NumberOfPackets, StartFrame)
			
			if ((not success) or handle.IsInvalid) or handle.IsClosed:
				handle.SetHandleAsInvalid()
				errorCode as int = Marshal.GetLastWin32Error()
				raise Exception(((GetType().Name + ' failed. ErrorCode=') + errorCode.ToString('X')))

	
	public Handle as KISO_CONTEXT:
		get:
			return handle

	
	public Flags as KISO_FLAG:
		get:
			return (Marshal.ReadInt32(handle.DangerousGetHandle(), ofsFlags) cast KISO_FLAG)
		set:
			Marshal.WriteInt32(handle.DangerousGetHandle(), ofsFlags, (value cast int))

	
	public StartFrame as uint:
		get:
			return (Marshal.ReadInt32(handle.DangerousGetHandle(), ofsStartFrame) cast uint)
		set:
			Marshal.WriteInt32(handle.DangerousGetHandle(), ofsStartFrame, (value cast Int32))

	
	public ErrorCount as short:
		get:
			return Marshal.ReadInt16(handle.DangerousGetHandle(), ofsErrorCount)
		set:
			Marshal.WriteInt16(handle.DangerousGetHandle(), ofsErrorCount, value)

	
	public NumberOfPackets as short:
		get:
			return Marshal.ReadInt16(handle.DangerousGetHandle(), ofsNumberOfPackets)
		set:
			Marshal.WriteInt16(handle.DangerousGetHandle(), ofsNumberOfPackets, value)

	
	public virtual def Free() as bool:
		if handle.IsInvalid or handle.IsClosed:
			return false
		
		handle.Close()
		return true

	
	public def SetPackets(PacketSize as int) as bool:
		return Functions.IsoK_SetPackets(handle, PacketSize)

	
	public def SetPacket(PacketIndex as int, ref IsoPacket as KISO_PACKET) as bool:
		return Functions.IsoK_SetPacket(handle, PacketIndex, IsoPacket)

	
	public def GetPacket(PacketIndex as int, ref IsoPacket as KISO_PACKET) as bool:
		return Functions.IsoK_GetPacket(handle, PacketIndex, IsoPacket)

	
	public def EnumPackets(EnumPackets as KISO_ENUM_PACKETS_CB, StartPacketIndex as int, UserState as IntPtr) as bool:
		return Functions.IsoK_EnumPackets(handle, EnumPackets, StartPacketIndex, UserState)

	
	public def ReUse() as bool:
		return Functions.IsoK_ReUse(handle)

