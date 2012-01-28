#Region "Copyright (c) Travis Robinson"
' Copyright (c) 2011 Travis Robinson <libusbdotnet@gmail.com>
' All rights reserved.
'
' C# libusbK Bindings
' Auto-generated on: 01.28.2012
'
' Redistribution and use in source and binary forms, with or without
' modification, are permitted provided that the following conditions are met:
'
'     * Redistributions of source code must retain the above copyright
'       notice, this list of conditions and the following disclaimer.
'
' THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
' IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
' TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
' PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS LEE ROBINSON
' BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
' CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
' SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
' INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
' CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
' ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
' THE POSSIBILITY OF SUCH DAMAGE.
#End Region


Imports System
Imports System.Runtime.CompilerServices
Imports System.Runtime.InteropServices
Imports Microsoft.Win32.SafeHandles


' ReSharper disable InconsistentNaming
' ReSharper disable CheckNamespace
' ReSharper disable UnassignedReadonlyField

Namespace libusbK
	Public NotInheritable Class Constants
		Private Sub New()
		End Sub
		''' <summary>
		'''   Allocated length for all strings in a \ref KLST_DEVINFO structure.
		''' </summary>
		Public Const KLST_STRING_MAX_LEN As Integer = 256

		''' <summary>
		'''   libusbK library
		''' </summary>
		Public Const LIBUSBK_DLL As String = "libusbK.dll"

		''' <summary>
		'''   Config power mask for the \c bmAttributes field of a \ref USB_CONFIGURATION_DESCRIPTOR
		''' </summary>
		Public Const USB_CONFIG_POWERED_MASK As Byte = &Hc0

		''' <summary>
		'''   Endpoint direction mask for the \c bEndpointAddress field of a \ref USB_ENDPOINT_DESCRIPTOR
		''' </summary>
		Public Const USB_ENDPOINT_DIRECTION_MASK As Byte = &H80

		''' <summary>
		'''   Endpoint address mask for the \c bEndpointAddress field of a \ref USB_ENDPOINT_DESCRIPTOR
		''' </summary>
		Public Const USB_ENDPOINT_ADDRESS_MASK As Byte = &Hf
	End Class

	Public Enum PipePolicyType
		SHORT_PACKET_TERMINATE = &H1
		AUTO_CLEAR_STALL = &H2
		PIPE_TRANSFER_TIMEOUT = &H3
		IGNORE_SHORT_PACKETS = &H4
		ALLOW_PARTIAL_READS = &H5
		AUTO_FLUSH = &H6
		RAW_IO = &H7
		MAXIMUM_TRANSFER_SIZE = &H8
		RESET_PIPE_ON_RESUME = &H9

		ISO_START_LATENCY = &H20
		ISO_ALWAYS_START_ASAP = &H21
		ISO_NUM_FIXED_PACKETS = &H22

		SIMUL_PARALLEL_REQUESTS = &H30
	End Enum

	Public Enum PowerPolicyType
		AUTO_SUSPEND = &H81
		SUSPEND_DELAY = &H83
	End Enum

	Public Enum DeviceInformationType
		DEVICE_SPEED = &H1
	End Enum

	Public Enum EndpointType
		''' <summary>
		'''   Indicates a control endpoint
		''' </summary>
		CONTROL = &H0

		''' <summary>
		'''   Indicates an isochronous endpoint
		''' </summary>
		ISOCHRONOUS = &H1

		''' <summary>
		'''   Indicates a bulk endpoint
		''' </summary>
		BULK = &H2

		''' <summary>
		'''   Indicates an interrupt endpoint
		''' </summary>
		INTERRUPT = &H3

		''' <summary>
		'''   Endpoint type mask for the \c bmAttributes field of a \ref USB_ENDPOINT_DESCRIPTOR
		''' </summary>
		MASK = &H3
	End Enum

	Public NotInheritable Class ErrorCodes
		Private Sub New()
		End Sub
		''' <summary>
		'''   The operation completed successfully.
		''' </summary>
		Public Const Success As Integer = 0

		''' <summary>
		'''   Access is denied.
		''' </summary>
		Public Const AccessDenied As Integer = 5

		''' <summary>
		'''   The handle is invalid.
		''' </summary>
		Public Const InvalidHandle As Integer = 6

		''' <summary>
		'''   Not enough storage is available to process this command.
		''' </summary>
		Public Const NotEnoughMemory As Integer = 8

		''' <summary>
		'''   The request is not supported.
		''' </summary>
		Public Const NotSupported As Integer = 50

		''' <summary>
		'''   The parameter is incorrect.
		''' </summary>
		Public Const InvalidParameter As Integer = 87

		''' <summary>
		'''   The semaphore timeout period has expired.
		''' </summary>
		Public Const SemTimeout As Integer = 121

		''' <summary>
		'''   The requested resource is in use.
		''' </summary>
		Public Const Busy As Integer = 170

		''' <summary>
		'''   Too many dynamic-link modules are attached to this program or dynamic-link module.
		''' </summary>
		Public Const TooManyModules As Integer = 214

		''' <summary>
		'''   More data is available.
		''' </summary>
		Public Const MoreData As Integer = 234

		''' <summary>
		'''   No more data is available.
		''' </summary>
		Public Const NoMoreItems As Integer = 259

		''' <summary>
		'''   An attempt was made to operate on a thread within a specific process, but the thread specified is not in the process specified.
		''' </summary>
		Public Const ThreadNotInProcess As Integer = 566

		''' <summary>
		'''   A thread termination occurred while the thread was suspended. The thread was resumed, and termination proceeded.
		''' </summary>
		Public Const ThreadWasSuspended As Integer = 699

		''' <summary>
		'''   The I/O operation has been aborted because of either a thread exit or an application request.
		''' </summary>
		Public Const OperationAborted As Integer = 995

		''' <summary>
		'''   Overlapped I/O event is not in a signaled state.
		''' </summary>
		Public Const IoIncomplete As Integer = 996

		''' <summary>
		'''   Overlapped I/O operation is in progress.
		''' </summary>
		Public Const IoPending As Integer = 997

		''' <summary>
		'''   Element not found.
		''' </summary>
		Public Const NotFound As Integer = 1168

		''' <summary>
		'''   The operation was canceled by the user.
		''' </summary>
		Public Const Cancelled As Integer = 1223

		''' <summary>
		'''   The library, drive, or media pool is empty.
		''' </summary>
		Public Const Empty As Integer = 4306

		''' <summary>
		'''   The cluster resource is not available.
		''' </summary>
		Public Const ResourceNotAvailable As Integer = 5006

		''' <summary>
		'''   The cluster resource could not be found.
		''' </summary>
		Public Const ResourceNotFound As Integer = 5007
	End Class


	#Region "Base KLIB Handle"
	Public MustInherit Class KLIB_HANDLE
		Inherits SafeHandleZeroOrMinusOneIsInvalid
		Implements IKLIB_HANDLE
		Protected Sub New(ownsHandle As Boolean)
			MyBase.New(ownsHandle)
		End Sub

		Protected Sub New()
			Me.New(True)
		End Sub

		Protected Sub New(handlePtrToWrap As IntPtr)
			MyBase.New(False)
			SetHandle(handlePtrToWrap)
		End Sub


		#Region "IKLIB_HANDLE Members"
		Public MustOverride ReadOnly Property HandleType() As KLIB_HANDLE_TYPE Implements IKLIB_HANDLE.HandleType

		Public Function GetContext() As IntPtr Implements IKLIB_HANDLE.GetContext
			Return Functions.LibK_GetContext(handle, HandleType)
		End Function

		Public Function SetContext(ContextValue As IntPtr) As Boolean Implements IKLIB_HANDLE.SetContext
			Return Functions.LibK_SetContext(handle, HandleType, ContextValue)
		End Function

		Public Function SetCleanupCallback(CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean Implements IKLIB_HANDLE.SetCleanupCallback
			Return Functions.LibK_SetCleanupCallback(handle, HandleType, CleanupCallback)
		End Function
		#End Region


		Protected MustOverride Overrides Function ReleaseHandle() As Boolean
	End Class

	Public Interface IKLIB_HANDLE
		ReadOnly Property HandleType() As KLIB_HANDLE_TYPE
		Function DangerousGetHandle() As IntPtr
		Function GetContext() As IntPtr
		Function SetContext(UserContext As IntPtr) As Boolean
		Function SetCleanupCallback(CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean
	End Interface
	#End Region


	#Region "Class Handles"
	Public Class KHOT_HANDLE
		Inherits KLIB_HANDLE
		Public Shared ReadOnly Property Empty() As KHOT_HANDLE
			Get
				Return New KHOT_HANDLE()
			End Get
		End Property

		Public Overrides ReadOnly Property HandleType() As KLIB_HANDLE_TYPE
			Get
				Return KLIB_HANDLE_TYPE.HOTK
			End Get
		End Property

		Protected Overrides Function ReleaseHandle() As Boolean
			Return Functions.HotK_Free(handle)
		End Function
	End Class

	Public Class KUSB_HANDLE
		Inherits KLIB_HANDLE
		Public Shared ReadOnly Property Empty() As KUSB_HANDLE
			Get
				Return New KUSB_HANDLE()
			End Get
		End Property

		Public Overrides ReadOnly Property HandleType() As KLIB_HANDLE_TYPE
			Get
				Return KLIB_HANDLE_TYPE.USBK
			End Get
		End Property

		Protected Overrides Function ReleaseHandle() As Boolean
			If True Then
				Return Functions.UsbK_Free(handle)
			End If
		End Function


		#Region "USB Shared Device Context"
		Public Function GetSharedContext() As IntPtr
			Return Functions.LibK_GetContext(handle, KLIB_HANDLE_TYPE.USBSHAREDK)
		End Function

		Public Function SetSharedContext(SharedUserContext As IntPtr) As Boolean
			Return Functions.LibK_SetContext(handle, KLIB_HANDLE_TYPE.USBSHAREDK, SharedUserContext)
		End Function

		Public Function SetSharedCleanupCallback(CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean
			Return Functions.LibK_SetCleanupCallback(handle, KLIB_HANDLE_TYPE.USBSHAREDK, CleanupCallback)
		End Function
		#End Region
	End Class

	Public Class KLST_HANDLE
		Inherits KLIB_HANDLE
		Public Shared ReadOnly Property Empty() As KLST_HANDLE
			Get
				Return New KLST_HANDLE()
			End Get
		End Property

		Public Overrides ReadOnly Property HandleType() As KLIB_HANDLE_TYPE
			Get
				Return KLIB_HANDLE_TYPE.LSTK
			End Get
		End Property

		Protected Overrides Function ReleaseHandle() As Boolean
			Return Functions.LstK_Free(handle)
		End Function
	End Class

	Public Class KOVL_POOL_HANDLE
		Inherits KLIB_HANDLE
		Public Shared ReadOnly Property Empty() As KOVL_POOL_HANDLE
			Get
				Return New KOVL_POOL_HANDLE()
			End Get
		End Property

		Public Overrides ReadOnly Property HandleType() As KLIB_HANDLE_TYPE
			Get
				Return KLIB_HANDLE_TYPE.OVLPOOLK
			End Get
		End Property

		Protected Overrides Function ReleaseHandle() As Boolean
			Return Functions.OvlK_Free(handle)
		End Function
	End Class

	Public Class KSTM_HANDLE
		Inherits KLIB_HANDLE
		Public Shared ReadOnly Property Empty() As KSTM_HANDLE
			Get
				Return New KSTM_HANDLE()
			End Get
		End Property

		Public Overrides ReadOnly Property HandleType() As KLIB_HANDLE_TYPE
			Get
				Return KLIB_HANDLE_TYPE.STMK
			End Get
		End Property

		Protected Overrides Function ReleaseHandle() As Boolean
			Return Functions.StmK_Free(handle)
		End Function
	End Class

	Public Class KISO_CONTEXT
		Inherits SafeHandleZeroOrMinusOneIsInvalid
		Protected Sub New(ownsHandle As Boolean)
			MyBase.New(ownsHandle)
		End Sub

		Protected Sub New()
			Me.New(True)
		End Sub

		Protected Sub New(handlePtrToWrap As IntPtr)
			MyBase.New(False)
			SetHandle(handlePtrToWrap)
		End Sub

		Protected Overrides Function ReleaseHandle() As Boolean
			Functions.IsoK_Free(handle)
			handle = IntPtr.Zero
			Return True
		End Function
	End Class
	#End Region


	#Region "Struct Handles"
	Public Structure KOVL_HANDLE
		Implements IKLIB_HANDLE
		Private ReadOnly handle As IntPtr

		Public Shared ReadOnly Property Empty() As KOVL_HANDLE
			Get
				Return New KOVL_HANDLE()
			End Get
		End Property


		#Region "IKLIB_HANDLE Members"
		Public ReadOnly Property HandleType() As KLIB_HANDLE_TYPE Implements IKLIB_HANDLE.HandleType
			Get
				Return KLIB_HANDLE_TYPE.OVLK
			End Get
		End Property

		Public Function GetContext() As IntPtr Implements IKLIB_HANDLE.GetContext
			Return Functions.LibK_GetContext(handle, HandleType)
		End Function

		Public Function SetContext(ContextValue As IntPtr) As Boolean Implements IKLIB_HANDLE.SetContext
			Return Functions.LibK_SetContext(handle, HandleType, ContextValue)
		End Function

		Public Function SetCleanupCallback(CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean Implements IKLIB_HANDLE.SetCleanupCallback
			Return Functions.LibK_SetCleanupCallback(handle, HandleType, CleanupCallback)
		End Function

		Public Function DangerousGetHandle() As IntPtr Implements IKLIB_HANDLE.DangerousGetHandle
			Return handle
		End Function
		#End Region
	End Structure

	Public Structure KLST_DEVINFO_HANDLE
		Implements IKLIB_HANDLE
		Private Shared ReadOnly ofsClassGUID As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "ClassGUID").ToInt32()
		Private Shared ReadOnly ofsCommon As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "Common").ToInt32()
		Private Shared ReadOnly ofsConnected As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "Connected").ToInt32()
		Private Shared ReadOnly ofsDeviceDesc As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "DeviceDesc").ToInt32()
		Private Shared ReadOnly ofsDeviceInterfaceGUID As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "DeviceInterfaceGUID").ToInt32()
		Private Shared ReadOnly ofsDevicePath As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "DevicePath").ToInt32()
		Private Shared ReadOnly ofsDriverID As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "DriverID").ToInt32()
		Private Shared ReadOnly ofsInstanceID As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "InstanceID").ToInt32()
		Private Shared ReadOnly ofsLUsb0FilterIndex As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "LUsb0FilterIndex").ToInt32()
		Private Shared ReadOnly ofsMfg As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "Mfg").ToInt32()
		Private Shared ReadOnly ofsService As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "Service").ToInt32()
		Private Shared ReadOnly ofsSymbolicLink As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "SymbolicLink").ToInt32()
		Private Shared ReadOnly ofsSyncFlags As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO), "SyncFlags").ToInt32()

		Private ReadOnly handle As IntPtr

		Public Sub New(HandleToWrap As IntPtr)
			handle = HandleToWrap
		End Sub

		Public Shared ReadOnly Property Empty() As KLST_DEVINFO_HANDLE
			Get
				Return New KLST_DEVINFO_HANDLE()
			End Get
		End Property

		Public ReadOnly Property SyncFlags() As KLST_SYNC_FLAG
			Get
				Return CType(Marshal.ReadInt32(handle, ofsSyncFlags), KLST_SYNC_FLAG)
			End Get
		End Property

		Public ReadOnly Property Connected() As Boolean
			Get
				Return Marshal.ReadInt32(handle, ofsConnected) <> 0
			End Get
		End Property

		Public ReadOnly Property LUsb0FilterIndex() As Integer
			Get
				Return Marshal.ReadInt32(handle, ofsLUsb0FilterIndex)
			End Get
		End Property

		Public ReadOnly Property DevicePath() As String
			Get
				Return Marshal.PtrToStringAnsi(New IntPtr(handle.ToInt64() + ofsDevicePath))
			End Get
		End Property

		Public ReadOnly Property SymbolicLink() As String
			Get
				Return Marshal.PtrToStringAnsi(New IntPtr(handle.ToInt64() + ofsSymbolicLink))
			End Get
		End Property

		Public ReadOnly Property Service() As String
			Get
				Return Marshal.PtrToStringAnsi(New IntPtr(handle.ToInt64() + ofsService))
			End Get
		End Property

		Public ReadOnly Property DeviceDesc() As String
			Get
				Return Marshal.PtrToStringAnsi(New IntPtr(handle.ToInt64() + ofsDeviceDesc))
			End Get
		End Property

		Public ReadOnly Property Mfg() As String
			Get
				Return Marshal.PtrToStringAnsi(New IntPtr(handle.ToInt64() + ofsMfg))
			End Get
		End Property

		Public ReadOnly Property ClassGUID() As String
			Get
				Return Marshal.PtrToStringAnsi(New IntPtr(handle.ToInt64() + ofsClassGUID))
			End Get
		End Property

		Public ReadOnly Property InstanceID() As String
			Get
				Return Marshal.PtrToStringAnsi(New IntPtr(handle.ToInt64() + ofsInstanceID))
			End Get
		End Property

		Public ReadOnly Property DeviceInterfaceGUID() As String
			Get
				Return Marshal.PtrToStringAnsi(New IntPtr(handle.ToInt64() + ofsDeviceInterfaceGUID))
			End Get
		End Property

		Public ReadOnly Property DriverID() As Integer
			Get
				Return Marshal.ReadInt32(handle, ofsDriverID)
			End Get
		End Property

		Public ReadOnly Property Common() As KLST_DEV_COMMON_INFO
			Get
				Return CType(Marshal.PtrToStructure(New IntPtr(handle.ToInt64() + ofsCommon), GetType(KLST_DEV_COMMON_INFO)), KLST_DEV_COMMON_INFO)
			End Get
		End Property


		#Region "IKLIB_HANDLE Members"
		Public ReadOnly Property HandleType() As KLIB_HANDLE_TYPE Implements IKLIB_HANDLE.HandleType
			Get
				Return KLIB_HANDLE_TYPE.LSTINFOK
			End Get
		End Property

		Public Function GetContext() As IntPtr Implements IKLIB_HANDLE.GetContext
			Return Functions.LibK_GetContext(handle, HandleType)
		End Function

		Public Function SetContext(ContextValue As IntPtr) As Boolean Implements IKLIB_HANDLE.SetContext
			Return Functions.LibK_SetContext(handle, HandleType, ContextValue)
		End Function

		Public Function DangerousGetHandle() As IntPtr Implements IKLIB_HANDLE.DangerousGetHandle
			Return handle
		End Function

		Public Function SetCleanupCallback(CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean Implements IKLIB_HANDLE.SetCleanupCallback
			Return Functions.LibK_SetCleanupCallback(handle, HandleType, CleanupCallback)
		End Function
		#End Region


		Public Overrides Function ToString() As String
			Return String.Format("{{Common: {0}, DriverID: {1}, DeviceInterfaceGuid: {2}, InstanceId: {3}, ClassGuid: {4}, Mfg: {5}, DeviceDesc: {6}, Service: {7}, SymbolicLink: {8}, DevicePath: {9}, LUsb0FilterIndex: {10}, Connected: {11}, SyncFlags: {12}}}", Common, DriverID, DeviceInterfaceGUID, InstanceID, ClassGUID, _
				Mfg, DeviceDesc, Service, SymbolicLink, DevicePath, LUsb0FilterIndex, _
				Connected, SyncFlags)
		End Function
	End Structure
	#End Region


	#Region "Internal Function Imports"
	Friend NotInheritable Class Functions
		Private Sub New()
		End Sub
		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LibK_GetVersion", SetLastError := True)> _
		Public Shared Sub LibK_GetVersion(<Out> ByRef Version As KLIB_VERSION)
		End Sub

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LibK_GetContext", SetLastError := True)> _
		Public Shared Function LibK_GetContext(<[In]> Handle As IntPtr, HandleType As KLIB_HANDLE_TYPE) As IntPtr
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LibK_SetContext", SetLastError := True)> _
		Public Shared Function LibK_SetContext(<[In]> Handle As IntPtr, HandleType As KLIB_HANDLE_TYPE, ContextValue As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LibK_SetCleanupCallback", SetLastError := True)> _
		Public Shared Function LibK_SetCleanupCallback(<[In]> Handle As IntPtr, HandleType As KLIB_HANDLE_TYPE, CleanupCB As KLIB_HANDLE_CLEANUP_CB) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LibK_LoadDriverAPI", SetLastError := True)> _
		Public Shared Function LibK_LoadDriverAPI(<Out> ByRef DriverAPI As KUSB_DRIVER_API, DriverID As Integer) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LibK_CopyDriverAPI", SetLastError := True)> _
		Public Shared Function LibK_CopyDriverAPI(<Out> ByRef DriverAPI As KUSB_DRIVER_API, <[In]> UsbHandle As KUSB_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LibK_GetProcAddress", SetLastError := True)> _
		Public Shared Function LibK_GetProcAddress(ProcAddress As IntPtr, DriverID As Integer, FunctionID As Integer) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_Init", SetLastError := True)> _
		Public Shared Function UsbK_Init(<Out> ByRef InterfaceHandle As KUSB_HANDLE, <[In]> DevInfo As KLST_DEVINFO_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_Free", SetLastError := True)> _
		Public Shared Function UsbK_Free(<[In]> InterfaceHandle As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_ClaimInterface", SetLastError := True)> _
		Public Shared Function UsbK_ClaimInterface(<[In]> InterfaceHandle As KUSB_HANDLE, NumberOrIndex As Byte, IsIndex As Boolean) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_ReleaseInterface", SetLastError := True)> _
		Public Shared Function UsbK_ReleaseInterface(<[In]> InterfaceHandle As KUSB_HANDLE, NumberOrIndex As Byte, IsIndex As Boolean) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_SetAltInterface", SetLastError := True)> _
		Public Shared Function UsbK_SetAltInterface(<[In]> InterfaceHandle As KUSB_HANDLE, NumberOrIndex As Byte, IsIndex As Boolean, AltSettingNumber As Byte) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_GetAltInterface", SetLastError := True)> _
		Public Shared Function UsbK_GetAltInterface(<[In]> InterfaceHandle As KUSB_HANDLE, NumberOrIndex As Byte, IsIndex As Boolean, ByRef AltSettingNumber As Byte) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_GetDescriptor", SetLastError := True)> _
		Public Shared Function UsbK_GetDescriptor(<[In]> InterfaceHandle As KUSB_HANDLE, DescriptorType As Byte, Index As Byte, LanguageID As UShort, Buffer As IntPtr, BufferLength As UInteger, _
			ByRef LengthTransferred As UInteger) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_ControlTransfer", SetLastError := True)> _
		Public Shared Function UsbK_ControlTransfer(<[In]> InterfaceHandle As KUSB_HANDLE, SetupPacket As WINUSB_SETUP_PACKET, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_SetPowerPolicy", SetLastError := True)> _
		Public Shared Function UsbK_SetPowerPolicy(<[In]> InterfaceHandle As KUSB_HANDLE, PolicyType As UInteger, ValueLength As UInteger, Value As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_GetPowerPolicy", SetLastError := True)> _
		Public Shared Function UsbK_GetPowerPolicy(<[In]> InterfaceHandle As KUSB_HANDLE, PolicyType As UInteger, ByRef ValueLength As UInteger, Value As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_SetConfiguration", SetLastError := True)> _
		Public Shared Function UsbK_SetConfiguration(<[In]> InterfaceHandle As KUSB_HANDLE, ConfigurationNumber As Byte) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_GetConfiguration", SetLastError := True)> _
		Public Shared Function UsbK_GetConfiguration(<[In]> InterfaceHandle As KUSB_HANDLE, ByRef ConfigurationNumber As Byte) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_ResetDevice", SetLastError := True)> _
		Public Shared Function UsbK_ResetDevice(<[In]> InterfaceHandle As KUSB_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_Initialize", SetLastError := True)> _
		Public Shared Function UsbK_Initialize(DeviceHandle As IntPtr, <Out> ByRef InterfaceHandle As KUSB_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_SelectInterface", SetLastError := True)> _
		Public Shared Function UsbK_SelectInterface(<[In]> InterfaceHandle As KUSB_HANDLE, NumberOrIndex As Byte, IsIndex As Boolean) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_GetAssociatedInterface", SetLastError := True)> _
		Public Shared Function UsbK_GetAssociatedInterface(<[In]> InterfaceHandle As KUSB_HANDLE, AssociatedInterfaceIndex As Byte, <Out> ByRef AssociatedInterfaceHandle As KUSB_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_Clone", SetLastError := True)> _
		Public Shared Function UsbK_Clone(<[In]> InterfaceHandle As KUSB_HANDLE, <Out> ByRef DstInterfaceHandle As KUSB_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_QueryInterfaceSettings", SetLastError := True)> _
		Public Shared Function UsbK_QueryInterfaceSettings(<[In]> InterfaceHandle As KUSB_HANDLE, AltSettingNumber As Byte, <Out> ByRef UsbAltInterfaceDescriptor As USB_INTERFACE_DESCRIPTOR) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_QueryDeviceInformation", SetLastError := True)> _
		Public Shared Function UsbK_QueryDeviceInformation(<[In]> InterfaceHandle As KUSB_HANDLE, InformationType As UInteger, ByRef BufferLength As UInteger, Buffer As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_SetCurrentAlternateSetting", SetLastError := True)> _
		Public Shared Function UsbK_SetCurrentAlternateSetting(<[In]> InterfaceHandle As KUSB_HANDLE, AltSettingNumber As Byte) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_GetCurrentAlternateSetting", SetLastError := True)> _
		Public Shared Function UsbK_GetCurrentAlternateSetting(<[In]> InterfaceHandle As KUSB_HANDLE, ByRef AltSettingNumber As Byte) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_QueryPipe", SetLastError := True)> _
		Public Shared Function UsbK_QueryPipe(<[In]> InterfaceHandle As KUSB_HANDLE, AltSettingNumber As Byte, PipeIndex As Byte, <Out> ByRef PipeInformation As WINUSB_PIPE_INFORMATION) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_SetPipePolicy", SetLastError := True)> _
		Public Shared Function UsbK_SetPipePolicy(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte, PolicyType As UInteger, ValueLength As UInteger, Value As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_GetPipePolicy", SetLastError := True)> _
		Public Shared Function UsbK_GetPipePolicy(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte, PolicyType As UInteger, ByRef ValueLength As UInteger, Value As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_ReadPipe", SetLastError := True)> _
		Public Shared Function UsbK_ReadPipe(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_WritePipe", SetLastError := True)> _
		Public Shared Function UsbK_WritePipe(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_ResetPipe", SetLastError := True)> _
		Public Shared Function UsbK_ResetPipe(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_AbortPipe", SetLastError := True)> _
		Public Shared Function UsbK_AbortPipe(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_FlushPipe", SetLastError := True)> _
		Public Shared Function UsbK_FlushPipe(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_IsoReadPipe", SetLastError := True)> _
		Public Shared Function UsbK_IsoReadPipe(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, Overlapped As IntPtr, <[In]> IsoContext As KISO_CONTEXT) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_IsoWritePipe", SetLastError := True)> _
		Public Shared Function UsbK_IsoWritePipe(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, Overlapped As IntPtr, <[In]> IsoContext As KISO_CONTEXT) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_GetCurrentFrameNumber", SetLastError := True)> _
		Public Shared Function UsbK_GetCurrentFrameNumber(<[In]> InterfaceHandle As KUSB_HANDLE, ByRef FrameNumber As UInteger) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_GetOverlappedResult", SetLastError := True)> _
		Public Shared Function UsbK_GetOverlappedResult(<[In]> InterfaceHandle As KUSB_HANDLE, Overlapped As IntPtr, ByRef lpNumberOfBytesTransferred As UInteger, bWait As Boolean) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "UsbK_GetProperty", SetLastError := True)> _
		Public Shared Function UsbK_GetProperty(<[In]> InterfaceHandle As KUSB_HANDLE, PropertyType As KUSB_PROPERTY, ByRef PropertySize As UInteger, Value As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LstK_Init", SetLastError := True)> _
		Public Shared Function LstK_Init(<Out> ByRef DeviceList As KLST_HANDLE, Flags As KLST_FLAG) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LstK_InitEx", SetLastError := True)> _
		Public Shared Function LstK_InitEx(<Out> ByRef DeviceList As KLST_HANDLE, <[In]> ByRef InitParams As KLST_INITEX_PARAMS) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LstK_Free", SetLastError := True)> _
		Public Shared Function LstK_Free(<[In]> DeviceList As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LstK_Enumerate", SetLastError := True)> _
		Public Shared Function LstK_Enumerate(<[In]> DeviceList As KLST_HANDLE, EnumDevListCB As KLST_ENUM_DEVINFO_CB, Context As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LstK_Current", SetLastError := True)> _
		Public Shared Function LstK_Current(<[In]> DeviceList As KLST_HANDLE, <Out> ByRef DeviceInfo As KLST_DEVINFO_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LstK_MoveNext", SetLastError := True)> _
		Public Shared Function LstK_MoveNext(<[In]> DeviceList As KLST_HANDLE, <Out> ByRef DeviceInfo As KLST_DEVINFO_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LstK_MoveReset", SetLastError := True)> _
		Public Shared Sub LstK_MoveReset(<[In]> DeviceList As KLST_HANDLE)
		End Sub

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LstK_FindByVidPid", SetLastError := True)> _
		Public Shared Function LstK_FindByVidPid(<[In]> DeviceList As KLST_HANDLE, Vid As Integer, Pid As Integer, <Out> ByRef DeviceInfo As KLST_DEVINFO_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "LstK_Count", SetLastError := True)> _
		Public Shared Function LstK_Count(<[In]> DeviceList As KLST_HANDLE, ByRef Count As UInteger) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "HotK_Init", SetLastError := True)> _
		Public Shared Function HotK_Init(<Out> ByRef Handle As KHOT_HANDLE, <[In], Out> ByRef InitParams As KHOT_PARAMS) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "HotK_Free", SetLastError := True)> _
		Public Shared Function HotK_Free(<[In]> Handle As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "HotK_FreeAll", SetLastError := True)> _
		Public Shared Sub HotK_FreeAll()
		End Sub

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "OvlK_Acquire", SetLastError := True)> _
		Public Shared Function OvlK_Acquire(<Out> ByRef OverlappedK As KOVL_HANDLE, <[In]> PoolHandle As KOVL_POOL_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "OvlK_Release", SetLastError := True)> _
		Public Shared Function OvlK_Release(<[In]> OverlappedK As KOVL_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "OvlK_Init", SetLastError := True)> _
		Public Shared Function OvlK_Init(<Out> ByRef PoolHandle As KOVL_POOL_HANDLE, <[In]> UsbHandle As KUSB_HANDLE, MaxOverlappedCount As Integer, Flags As KOVL_POOL_FLAG) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "OvlK_Free", SetLastError := True)> _
		Public Shared Function OvlK_Free(<[In]> PoolHandle As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "OvlK_GetEventHandle", SetLastError := True)> _
		Public Shared Function OvlK_GetEventHandle(<[In]> OverlappedK As KOVL_HANDLE) As IntPtr
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "OvlK_Wait", SetLastError := True)> _
		Public Shared Function OvlK_Wait(<[In]> OverlappedK As KOVL_HANDLE, TimeoutMS As Integer, WaitFlags As KOVL_WAIT_FLAG, ByRef TransferredLength As UInteger) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "OvlK_WaitOrCancel", SetLastError := True)> _
		Public Shared Function OvlK_WaitOrCancel(<[In]> OverlappedK As KOVL_HANDLE, TimeoutMS As Integer, ByRef TransferredLength As UInteger) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "OvlK_WaitAndRelease", SetLastError := True)> _
		Public Shared Function OvlK_WaitAndRelease(<[In]> OverlappedK As KOVL_HANDLE, TimeoutMS As Integer, ByRef TransferredLength As UInteger) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "OvlK_IsComplete", SetLastError := True)> _
		Public Shared Function OvlK_IsComplete(<[In]> OverlappedK As KOVL_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "OvlK_ReUse", SetLastError := True)> _
		Public Shared Function OvlK_ReUse(<[In]> OverlappedK As KOVL_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "StmK_Init", SetLastError := True)> _
		Public Shared Function StmK_Init(<Out> ByRef StreamHandle As KSTM_HANDLE, <[In]> UsbHandle As KUSB_HANDLE, PipeID As Byte, MaxTransferSize As Integer, MaxPendingTransfers As Integer, MaxPendingIO As Integer, _
			<[In]> ByRef Callbacks As KSTM_CALLBACK, Flags As KSTM_FLAG) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "StmK_Free", SetLastError := True)> _
		Public Shared Function StmK_Free(<[In]> StreamHandle As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "StmK_Start", SetLastError := True)> _
		Public Shared Function StmK_Start(<[In]> StreamHandle As KSTM_HANDLE) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "StmK_Stop", SetLastError := True)> _
		Public Shared Function StmK_Stop(<[In]> StreamHandle As KSTM_HANDLE, TimeoutCancelMS As Integer) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "StmK_Read", SetLastError := True)> _
		Public Shared Function StmK_Read(<[In]> StreamHandle As KSTM_HANDLE, Buffer As IntPtr, Offset As Integer, Length As Integer, ByRef TransferredLength As UInteger) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "StmK_Write", SetLastError := True)> _
		Public Shared Function StmK_Write(<[In]> StreamHandle As KSTM_HANDLE, Buffer As IntPtr, Offset As Integer, Length As Integer, ByRef TransferredLength As UInteger) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "IsoK_Init", SetLastError := True)> _
		Public Shared Function IsoK_Init(<Out> ByRef IsoContext As KISO_CONTEXT, NumberOfPackets As Integer, StartFrame As Integer) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "IsoK_Free", SetLastError := True)> _
		Public Shared Function IsoK_Free(<[In]> IsoContext As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "IsoK_SetPackets", SetLastError := True)> _
		Public Shared Function IsoK_SetPackets(<[In]> IsoContext As KISO_CONTEXT, PacketSize As Integer) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "IsoK_SetPacket", SetLastError := True)> _
		Public Shared Function IsoK_SetPacket(<[In]> IsoContext As KISO_CONTEXT, PacketIndex As Integer, <[In]> ByRef IsoPacket As KISO_PACKET) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "IsoK_GetPacket", SetLastError := True)> _
		Public Shared Function IsoK_GetPacket(<[In]> IsoContext As KISO_CONTEXT, PacketIndex As Integer, <Out> ByRef IsoPacket As KISO_PACKET) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "IsoK_EnumPackets", SetLastError := True)> _
		Public Shared Function IsoK_EnumPackets(<[In]> IsoContext As KISO_CONTEXT, EnumPackets As KISO_ENUM_PACKETS_CB, StartPacketIndex As Integer, UserState As IntPtr) As Boolean
		End Function

		<DllImport(Constants.LIBUSBK_DLL, CallingConvention := CallingConvention.Winapi, CharSet := CharSet.Ansi, EntryPoint := "IsoK_ReUse", SetLastError := True)> _
		Public Shared Function IsoK_ReUse(<[In]> IsoContext As KISO_CONTEXT) As Boolean
		End Function
	End Class
	#End Region


	#Region "Enumerations"
	''' <Summary>Values used in the \c bmAttributes field of a \ref USB_ENDPOINT_DESCRIPTOR</Summary>
	Public Enum USBD_PIPE_TYPE
		''' <Summary>Indicates a control endpoint</Summary>
		UsbdPipeTypeControl

		''' <Summary>Indicates an isochronous endpoint</Summary>
		UsbdPipeTypeIsochronous

		''' <Summary>Indicates a bulk endpoint</Summary>
		UsbdPipeTypeBulk

		''' <Summary>Indicates an interrupt endpoint</Summary>
		UsbdPipeTypeInterrupt
	End Enum

	''' <Summary>Additional ISO transfer flags.</Summary>
	<Flags> _
	Public Enum KISO_FLAG
		NONE = 0

		''' <Summary>Do not start the transfer immediately, instead use \ref KISO_CONTEXT::StartFrame.</Summary>
		SET_START_FRAME = &H1
	End Enum

	''' <Summary>Handle type enumeration.</Summary>
	Public Enum KLIB_HANDLE_TYPE
		''' <Summary>Hot plug handle. \ref KHOT_HANDLE</Summary>
		HOTK

		''' <Summary>USB handle. \ref KUSB_HANDLE</Summary>
		USBK

		''' <Summary>Shared USB handle. \ref KUSB_HANDLE</Summary>
		USBSHAREDK

		''' <Summary>Device list handle. \ref KLST_HANDLE</Summary>
		LSTK

		''' <Summary>Device info handle. \ref KLST_DEVINFO_HANDLE</Summary>
		LSTINFOK

		''' <Summary>Overlapped handle. \ref KOVL_HANDLE</Summary>
		OVLK

		''' <Summary>Overlapped pool handle. \ref KOVL_POOL_HANDLE</Summary>
		OVLPOOLK

		''' <Summary>Pipe stream handle. \ref KSTM_HANDLE</Summary>
		STMK

		''' <Summary>Max handle type count.</Summary>
		COUNT
	End Enum

	''' <Summary>Device list sync flags.</Summary>
	<Flags> _
	Public Enum KLST_SYNC_FLAG
		''' <Summary>Cleared/invalid state.</Summary>
		NONE = 0

		''' <Summary>Unchanged state,</Summary>
		UNCHANGED = &H1

		''' <Summary>Added (Arrival) state,</Summary>
		ADDED = &H2

		''' <Summary>Removed (Unplugged) state,</Summary>
		REMOVED = &H4

		''' <Summary>Connect changed state.</Summary>
		CONNECT_CHANGE = &H8

		''' <Summary>All states.</Summary>
		MASK = &Hf
	End Enum

	''' <Summary>Device list initialization flags.</Summary>
	<Flags> _
	Public Enum KLST_FLAG
		''' <Summary>No flags (or 0)</Summary>
		NONE = 0

		''' <Summary>Enable listings for the raw device interface GUID.{A5DCBF10-6530-11D2-901F-00C04FB951ED}</Summary>
		INCLUDE_RAWGUID = &H1

		''' <Summary>List libusbK devices that not currently connected.</Summary>
		INCLUDE_DISCONNECT = &H2
	End Enum

	''' <Summary>bmRequest.Dir</Summary>
	Public Enum BMREQUEST_DIR
		HOST_TO_DEVICE = 0
		DEVICE_TO_HOST = 1
	End Enum

	''' <Summary>bmRequest.Type</Summary>
	Public Enum BMREQUEST_TYPE
		''' <Summary>Standard request. See \ref USB_REQUEST_ENUM</Summary>
		STANDARD = 0

		''' <Summary>Class-specific request.</Summary>
		[CLASS] = 1

		''' <Summary>Vendor-specific request</Summary>
		VENDOR = 2
	End Enum

	''' <Summary>bmRequest.Recipient</Summary>
	Public Enum BMREQUEST_RECIPIENT
		''' <Summary>Request is for a device.</Summary>
		DEVICE = 0

		''' <Summary>Request is for an interface of a device.</Summary>
		[INTERFACE] = 1

		''' <Summary>Request is for an endpoint of a device.</Summary>
		ENDPOINT = 2

		''' <Summary>Request is for a vendor-specific purpose.</Summary>
		OTHER = 3
	End Enum

	''' <Summary>Values for the bits returned by the \ref USB_REQUEST_GET_STATUS request.</Summary>
	Public Enum USB_GETSTATUS
		''' <Summary>Device is self powered</Summary>
		SELF_POWERED = &H1

		''' <Summary>Device can wake the system from a low power/sleeping state.</Summary>
		REMOTE_WAKEUP_ENABLED = &H2
	End Enum

	''' <Summary>Standard USB descriptor types. For more information, see section 9-5 of the USB 3.0 specifications.</Summary>
	Public Enum USB_DESCRIPTOR_TYPE
		''' <Summary>Device descriptor type.</Summary>
		DEVICE = &H1

		''' <Summary>Configuration descriptor type.</Summary>
		CONFIGURATION = &H2

		''' <Summary>String descriptor type.</Summary>
		[STRING] = &H3

		''' <Summary>Interface descriptor type.</Summary>
		[INTERFACE] = &H4

		''' <Summary>Endpoint descriptor type.</Summary>
		ENDPOINT = &H5

		''' <Summary>Device qualifier descriptor type.</Summary>
		DEVICE_QUALIFIER = &H6

		''' <Summary>Config power descriptor type.</Summary>
		CONFIG_POWER = &H7

		''' <Summary>Interface power descriptor type.</Summary>
		INTERFACE_POWER = &H8

		''' <Summary>Interface association descriptor type.</Summary>
		INTERFACE_ASSOCIATION = &Hb
	End Enum

	''' <Summary>Usb handle specific properties that can be retrieved with \ref UsbK_GetProperty.</Summary>
	Public Enum KUSB_PROPERTY
		''' <Summary>Get the internal device file handle used for operations such as GetOverlappedResult or DeviceIoControl.</Summary>
		DEVICE_FILE_HANDLE

		COUNT
	End Enum

	''' <Summary>Supported driver id enumeration.</Summary>
	Public Enum KUSB_DRVID
		''' <Summary>libusbK.sys driver ID</Summary>
		LIBUSBK

		''' <Summary>libusb0.sys driver ID</Summary>
		LIBUSB0

		''' <Summary>WinUSB.sys driver ID</Summary>
		WINUSB

		''' <Summary>libusb0.sys filter driver ID</Summary>
		LIBUSB0_FILTER

		''' <Summary>Supported driver count</Summary>
		COUNT
	End Enum

	''' <Summary>Supported function id enumeration.</Summary>
	Public Enum KUSB_FNID
		''' <Summary>\ref UsbK_Init dynamic driver function id.</Summary>
		Init

		''' <Summary>\ref UsbK_Free dynamic driver function id.</Summary>
		Free

		''' <Summary>\ref UsbK_ClaimInterface dynamic driver function id.</Summary>
		ClaimInterface

		''' <Summary>\ref UsbK_ReleaseInterface dynamic driver function id.</Summary>
		ReleaseInterface

		''' <Summary>\ref UsbK_SetAltInterface dynamic driver function id.</Summary>
		SetAltInterface

		''' <Summary>\ref UsbK_GetAltInterface dynamic driver function id.</Summary>
		GetAltInterface

		''' <Summary>\ref UsbK_GetDescriptor dynamic driver function id.</Summary>
		GetDescriptor

		''' <Summary>\ref UsbK_ControlTransfer dynamic driver function id.</Summary>
		ControlTransfer

		''' <Summary>\ref UsbK_SetPowerPolicy dynamic driver function id.</Summary>
		SetPowerPolicy

		''' <Summary>\ref UsbK_GetPowerPolicy dynamic driver function id.</Summary>
		GetPowerPolicy

		''' <Summary>\ref UsbK_SetConfiguration dynamic driver function id.</Summary>
		SetConfiguration

		''' <Summary>\ref UsbK_GetConfiguration dynamic driver function id.</Summary>
		GetConfiguration

		''' <Summary>\ref UsbK_ResetDevice dynamic driver function id.</Summary>
		ResetDevice

		''' <Summary>\ref UsbK_Initialize dynamic driver function id.</Summary>
		Initialize

		''' <Summary>\ref UsbK_SelectInterface dynamic driver function id.</Summary>
		SelectInterface

		''' <Summary>\ref UsbK_GetAssociatedInterface dynamic driver function id.</Summary>
		GetAssociatedInterface

		''' <Summary>\ref UsbK_Clone dynamic driver function id.</Summary>
		Clone

		''' <Summary>\ref UsbK_QueryInterfaceSettings dynamic driver function id.</Summary>
		QueryInterfaceSettings

		''' <Summary>\ref UsbK_QueryDeviceInformation dynamic driver function id.</Summary>
		QueryDeviceInformation

		''' <Summary>\ref UsbK_SetCurrentAlternateSetting dynamic driver function id.</Summary>
		SetCurrentAlternateSetting

		''' <Summary>\ref UsbK_GetCurrentAlternateSetting dynamic driver function id.</Summary>
		GetCurrentAlternateSetting

		''' <Summary>\ref UsbK_QueryPipe dynamic driver function id.</Summary>
		QueryPipe

		''' <Summary>\ref UsbK_SetPipePolicy dynamic driver function id.</Summary>
		SetPipePolicy

		''' <Summary>\ref UsbK_GetPipePolicy dynamic driver function id.</Summary>
		GetPipePolicy

		''' <Summary>\ref UsbK_ReadPipe dynamic driver function id.</Summary>
		ReadPipe

		''' <Summary>\ref UsbK_WritePipe dynamic driver function id.</Summary>
		WritePipe

		''' <Summary>\ref UsbK_ResetPipe dynamic driver function id.</Summary>
		ResetPipe

		''' <Summary>\ref UsbK_AbortPipe dynamic driver function id.</Summary>
		AbortPipe

		''' <Summary>\ref UsbK_FlushPipe dynamic driver function id.</Summary>
		FlushPipe

		''' <Summary>\ref UsbK_IsoReadPipe dynamic driver function id.</Summary>
		IsoReadPipe

		''' <Summary>\ref UsbK_IsoWritePipe dynamic driver function id.</Summary>
		IsoWritePipe

		''' <Summary>\ref UsbK_GetCurrentFrameNumber dynamic driver function id.</Summary>
		GetCurrentFrameNumber

		''' <Summary>\ref UsbK_GetOverlappedResult dynamic driver function id.</Summary>
		GetOverlappedResult

		''' <Summary>\ref UsbK_GetProperty dynamic driver function id.</Summary>
		GetProperty

		''' <Summary>Supported function count</Summary>
		COUNT
	End Enum

	''' <Summary>Hot plug config flags.</Summary>
	<Flags> _
	Public Enum KHOT_FLAG
		''' <Summary>No flags (or 0)</Summary>
		NONE

		''' <Summary>Notify all devices which match upon a succuessful call to \ref HotK_Init.</Summary>
		PLUG_ALL_ON_INIT = &H1

		''' <Summary>Allow other \ref KHOT_HANDLE instances to consume this match.</Summary>
		PASS_DUPE_INSTANCE = &H2

		''' <Summary>If a \c UserHwnd is specified, use \c PostMessage instead of \c SendMessage.</Summary>
		POST_USER_MESSAGE = &H4
	End Enum

	''' <Summary>\c WaitFlags used by \ref OvlK_Wait.</Summary>
	<Flags> _
	Public Enum KOVL_WAIT_FLAG
		''' <Summary>Do not perform any additional actions upon exiting \ref OvlK_Wait.</Summary>
		NONE = 0

		''' <Summary>If the i/o operation completes successfully, release the OverlappedK back to it's pool.</Summary>
		RELEASE_ON_SUCCESS = &H1

		''' <Summary>If the i/o operation fails, release the OverlappedK back to it's pool.</Summary>
		RELEASE_ON_FAIL = &H2

		''' <Summary>If the i/o operation fails or completes successfully, release the OverlappedK back to its pool. Perform no actions if it times-out.</Summary>
		RELEASE_ON_SUCCESS_FAIL = &H3

		''' <Summary>If the i/o operation times-out cancel it, but do not release the OverlappedK back to its pool.</Summary>
		CANCEL_ON_TIMEOUT = &H4

		''' <Summary>If the i/o operation times-out, cancel it and release the OverlappedK back to its pool.</Summary>
		RELEASE_ON_TIMEOUT = &Hc

		''' <Summary>Always release the OverlappedK back to its pool.  If the operation timed-out, cancel it before releasing back to its pool.</Summary>
		RELEASE_ALWAYS = &Hf

		''' <Summary>Uses alterable wait functions.  See http://msdn.microsoft.com/en-us/library/windows/desktop/ms687036%28v=vs.85%29.aspx</Summary>
		ALERTABLE = &H10
	End Enum

	''' <Summary>\c Overlapped pool config flags.</Summary>
	<Flags> _
	Public Enum KOVL_POOL_FLAG
		NONE = 0
	End Enum

	''' <Summary>Stream config flags.</Summary>
	<Flags> _
	Public Enum KSTM_FLAG
		NONE = 0
	End Enum

	''' <Summary>Stream config flags.</Summary>
	Public Enum KSTM_COMPLETE_RESULT
		VALID = 0
		INVALID
	End Enum
	#End Region


	#Region "Structs"
	''' <Summary>The \c WINUSB_PIPE_INFORMATION structure contains pipe information that the \ref UsbK_QueryPipe routine retrieves.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi)> _
	Public Structure WINUSB_PIPE_INFORMATION
		''' <Summary>A \c USBD_PIPE_TYPE enumeration value that specifies the pipe type</Summary>
		Public PipeType As USBD_PIPE_TYPE

		''' <Summary>The pipe identifier (ID)</Summary>
		Public PipeId As Byte

		''' <Summary>The maximum size, in bytes, of the packets that are transmitted on the pipe</Summary>
		Public MaximumPacketSize As UShort

		''' <Summary>The pipe interval</Summary>
		Public Interval As Byte
	End Structure

	''' <Summary>The \c WINUSB_SETUP_PACKET structure describes a USB setup packet.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Pack := 1)> _
	Public Structure WINUSB_SETUP_PACKET
		''' <Summary>The request type. The values that are assigned to this member are defined in Table 9.2 of section 9.3 of the Universal Serial Bus (USB) specification (www.usb.org).</Summary>
		Public RequestType As Byte

		''' <Summary>The device request. The values that are assigned to this member are defined in Table 9.3 of section 9.4 of the Universal Serial Bus (USB) specification.</Summary>
		Public Request As Byte

		''' <Summary>The meaning of this member varies according to the request. For an explanation of this member, see the Universal Serial Bus (USB) specification.</Summary>
		Public Value As UShort

		''' <Summary>The meaning of this member varies according to the request. For an explanation of this member, see the Universal Serial Bus (USB) specification.</Summary>
		Public Index As UShort

		''' <Summary>The number of bytes to transfer. (not including the \c WINUSB_SETUP_PACKET itself)</Summary>
		Public Length As UShort
	End Structure

	''' <Summary>Structure describing an isochronous transfer packet.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Pack := 1)> _
	Public Structure KISO_PACKET
		''' <Summary>Specifies the offset, in bytes, of the buffer for this packet from the beginning of the entire isochronous transfer data buffer.</Summary>
		Public Offset As UInteger

		''' <Summary>Set by the host controller to indicate the actual number of bytes received by the device for isochronous IN transfers. Length not used for isochronous OUT transfers.</Summary>
		Public Length As UShort

		''' <Summary>Contains the 16 least significant USBD status bits, on return from the host controller driver, of this transfer packet.</Summary>
		Public Status As UShort
	End Structure

	''' <summary>
	'''   KISO_CONTEXT_MAP is used for calculating field offsets only as it lacks an IsoPackets field.
	''' </summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Pack := 1, Size := 16)> _
	Friend Structure KISO_CONTEXT_MAP
		''' <Summary>Additional ISO transfer flags. See \ref KISO_FLAG.</Summary>
		Public Flags As KISO_FLAG

		''' <Summary>Specifies the frame number that the transfer should begin on (0 for ASAP).</Summary>
		Public StartFrame As UInteger

		''' <Summary>Contains the number of packets that completed with an error condition on return from the host controller driver.</Summary>
		Public ErrorCount As Short

		''' <Summary>Specifies the number of packets that are described by the variable-length array member \c IsoPacket.</Summary>
		Public NumberOfPackets As Short
	End Structure

	''' <Summary>libusbK verson information structure.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi)> _
	Public Structure KLIB_VERSION
		''' <Summary>Major version number.</Summary>
		Public Major As Integer

		''' <Summary>Minor version number.</Summary>
		Public Minor As Integer

		''' <Summary>Micro version number.</Summary>
		Public Micro As Integer

		''' <Summary>Nano version number.</Summary>
		Public Nano As Integer
	End Structure

	''' <Summary>Common usb device information structure</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi)> _
	Public Structure KLST_DEV_COMMON_INFO
		''' <Summary>VendorID parsed from \ref KLST_DEVINFO::InstanceID</Summary>
		Public Vid As Integer

		''' <Summary>ProductID parsed from \ref KLST_DEVINFO::InstanceID</Summary>
		Public Pid As Integer

		''' <Summary>Interface number (valid for composite devices only) parsed from \ref KLST_DEVINFO::InstanceID</Summary>
		Public MI As Integer

		' An ID that uniquely identifies a USB device.
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public InstanceID As String
	End Structure

	''' <Summary>Semi-opaque device information structure of a device list.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi)> _
	Public Structure KLST_DEVINFO
		''' <Summary>Common usb device information</Summary>
		Public Common As KLST_DEV_COMMON_INFO

		''' <Summary>Driver id this device element is using</Summary>
		Public DriverID As Integer

		''' <Summary>Device interface GUID</Summary>
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public DeviceInterfaceGUID As String

		''' <Summary>Device instance ID.</Summary>
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public InstanceID As String

		''' <Summary>Class GUID.</Summary>
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public ClassGUID As String

		''' <Summary>Manufacturer name as specified in the INF file.</Summary>
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public Mfg As String

		''' <Summary>Device description as specified in the INF file.</Summary>
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public DeviceDesc As String

		''' <Summary>Driver service name.</Summary>
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public Service As String

		''' <Summary>Unique identifier.</Summary>
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public SymbolicLink As String

		''' <Summary>physical device filename used with the Windows \c CreateFile()</Summary>
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public DevicePath As String

		''' <Summary>libusb-win32 filter index id.</Summary>
		Public LUsb0FilterIndex As Integer

		''' <Summary>Indicates the devices connection state.</Summary>
		Public Connected As Boolean

		''' <Summary>Synchronization flags. (internal use only)</Summary>
		Public SyncFlags As KLST_SYNC_FLAG
	End Structure

	''' <Summary>Hot plug parameter structure.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Size := 1024)> _
	Public Structure KLST_PATTERN_MATCH
		''' <Summary>Pattern match a device instance id.</Summary>
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public InstanceID As String

		''' <Summary>Pattern match a device interface guid.</Summary>
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public DeviceInterfaceGUID As String

		''' <Summary>Pattern match a symbolic link.</Summary>
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public SymbolicLink As String
	End Structure

	''' <Summary>Device list parameter structure.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Size := 64)> _
	Public Structure KLST_INITEX_PARAMS
		''' <Summary>Device list initialization flags.</Summary>
		Public Flags As KLST_FLAG

		''' <Summary>File pattern match includes.</Summary>
		Public PatternMatch As KLST_PATTERN_MATCH()
	End Structure

	''' <Summary>A structure representing the standard USB device descriptor.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Pack := 1)> _
	Public Structure USB_DEVICE_DESCRIPTOR
		''' <Summary>Size of this descriptor (in bytes)</Summary>
		Public bLength As Byte

		''' <Summary>Descriptor type</Summary>
		Public bDescriptorType As Byte

		''' <Summary>USB specification release number in binary-coded decimal.</Summary>
		Public bcdUSB As UShort

		''' <Summary>USB-IF class code for the device</Summary>
		Public bDeviceClass As Byte

		''' <Summary>USB-IF subclass code for the device</Summary>
		Public bDeviceSubClass As Byte

		''' <Summary>USB-IF protocol code for the device</Summary>
		Public bDeviceProtocol As Byte

		''' <Summary>Maximum packet size for control endpoint 0</Summary>
		Public bMaxPacketSize0 As Byte

		''' <Summary>USB-IF vendor ID</Summary>
		Public idVendor As UShort

		''' <Summary>USB-IF product ID</Summary>
		Public idProduct As UShort

		''' <Summary>Device release number in binary-coded decimal</Summary>
		Public bcdDevice As UShort

		''' <Summary>Index of string descriptor describing manufacturer</Summary>
		Public iManufacturer As Byte

		''' <Summary>Index of string descriptor describing product</Summary>
		Public iProduct As Byte

		''' <Summary>Index of string descriptor containing device serial number</Summary>
		Public iSerialNumber As Byte

		''' <Summary>Number of possible configurations</Summary>
		Public bNumConfigurations As Byte
	End Structure

	''' <Summary>A structure representing the standard USB endpoint descriptor.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Pack := 1)> _
	Public Structure USB_ENDPOINT_DESCRIPTOR
		''' <Summary>Size of this descriptor (in bytes)</Summary>
		Public bLength As Byte

		''' <Summary>Descriptor type</Summary>
		Public bDescriptorType As Byte

		''' <Summary>The address of the endpoint described by this descriptor.</Summary>
		Public bEndpointAddress As Byte

		''' <Summary>Attributes which apply to the endpoint when it is configured using the bConfigurationValue.</Summary>
		Public bmAttributes As Byte

		''' <Summary>Maximum packet size this endpoint is capable of sending/receiving.</Summary>
		Public wMaxPacketSize As UShort

		''' <Summary>Interval for polling endpoint for data transfers.</Summary>
		Public bInterval As Byte
	End Structure

	''' <Summary>A structure representing the standard USB configuration descriptor.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Pack := 1)> _
	Public Structure USB_CONFIGURATION_DESCRIPTOR
		''' <Summary>Size of this descriptor (in bytes)</Summary>
		Public bLength As Byte

		''' <Summary>Descriptor type</Summary>
		Public bDescriptorType As Byte

		''' <Summary>Total length of data returned for this configuration</Summary>
		Public wTotalLength As UShort

		''' <Summary>Number of interfaces supported by this configuration</Summary>
		Public bNumInterfaces As Byte

		''' <Summary>Identifier value for this configuration</Summary>
		Public bConfigurationValue As Byte

		''' <Summary>Index of string descriptor describing this configuration</Summary>
		Public iConfiguration As Byte

		''' <Summary>Configuration characteristics</Summary>
		Public bmAttributes As Byte

		''' <Summary>Maximum power consumption of the USB device from this bus in this configuration when the device is fully operation.</Summary>
		Public MaxPower As Byte
	End Structure

	''' <Summary>A structure representing the standard USB interface descriptor.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Pack := 1)> _
	Public Structure USB_INTERFACE_DESCRIPTOR
		''' <Summary>Size of this descriptor (in bytes)</Summary>
		Public bLength As Byte

		''' <Summary>Descriptor type</Summary>
		Public bDescriptorType As Byte

		''' <Summary>Number of this interface</Summary>
		Public bInterfaceNumber As Byte

		''' <Summary>Value used to select this alternate setting for this interface</Summary>
		Public bAlternateSetting As Byte

		''' <Summary>Number of endpoints used by this interface (excluding the control endpoint)</Summary>
		Public bNumEndpoints As Byte

		''' <Summary>USB-IF class code for this interface</Summary>
		Public bInterfaceClass As Byte

		''' <Summary>USB-IF subclass code for this interface</Summary>
		Public bInterfaceSubClass As Byte

		''' <Summary>USB-IF protocol code for this interface</Summary>
		Public bInterfaceProtocol As Byte

		''' <Summary>Index of string descriptor describing this interface</Summary>
		Public iInterface As Byte
	End Structure

	''' <Summary>A structure representing the standard USB string descriptor.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Unicode, Pack := 1)> _
	Public Structure USB_STRING_DESCRIPTOR
		''' <Summary>Size of this descriptor (in bytes)</Summary>
		Public bLength As Byte

		''' <Summary>Descriptor type</Summary>
		Public bDescriptorType As Byte

		''' <Summary>Content of the string</Summary>
		<MarshalAs(UnmanagedType.ByValTStr, SizeConst := Constants.KLST_STRING_MAX_LEN)> _
		Public bString As String
	End Structure

	''' <Summary>A structure representing the common USB descriptor.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Pack := 1)> _
	Public Structure USB_COMMON_DESCRIPTOR
		''' <Summary>Size of this descriptor (in bytes)</Summary>
		Public bLength As Byte

		''' <Summary>Descriptor type</Summary>
		Public bDescriptorType As Byte
	End Structure

	''' <Summary>Allows hardware manufacturers to define groupings of interfaces.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Pack := 1)> _
	Public Structure USB_INTERFACE_ASSOCIATION_DESCRIPTOR
		''' <Summary>Size of this descriptor (in bytes)</Summary>
		Public bLength As Byte

		''' <Summary>Descriptor type</Summary>
		Public bDescriptorType As Byte

		''' <Summary>First interface number of the set of interfaces that follow this descriptor</Summary>
		Public bFirstInterface As Byte

		''' <Summary>The Number of interfaces follow this descriptor that are considered "associated"</Summary>
		Public bInterfaceCount As Byte

		''' <Summary>\c bInterfaceClass used for this associated interfaces</Summary>
		Public bFunctionClass As Byte

		''' <Summary>\c bInterfaceSubClass used for the associated interfaces</Summary>
		Public bFunctionSubClass As Byte

		''' <Summary>\c bInterfaceProtocol used for the associated interfaces</Summary>
		Public bFunctionProtocol As Byte

		''' <Summary>Index of string descriptor describing the associated interfaces</Summary>
		Public iFunction As Byte
	End Structure

	''' <Summary>USB core driver API information structure.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi)> _
	Public Structure KUSB_DRIVER_API_INFO
		''' <Summary>\readonly Driver id of the driver api.</Summary>
		Public DriverID As Integer

		''' <Summary>\readonly Number of valid functions contained in the driver API.</Summary>
		Public FunctionCount As Integer
	End Structure

	''' <Summary>Driver API function set structure.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Size := 512)> _
	Public Structure KUSB_DRIVER_API
		''' <Summary>Driver API information.</Summary>
		Public Info As KUSB_DRIVER_API_INFO

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public Init As KUSB_InitDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public Free As KUSB_FreeDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public ClaimInterface As KUSB_ClaimInterfaceDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public ReleaseInterface As KUSB_ReleaseInterfaceDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public SetAltInterface As KUSB_SetAltInterfaceDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public GetAltInterface As KUSB_GetAltInterfaceDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public GetDescriptor As KUSB_GetDescriptorDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public ControlTransfer As KUSB_ControlTransferDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public SetPowerPolicy As KUSB_SetPowerPolicyDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public GetPowerPolicy As KUSB_GetPowerPolicyDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public SetConfiguration As KUSB_SetConfigurationDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public GetConfiguration As KUSB_GetConfigurationDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public ResetDevice As KUSB_ResetDeviceDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public Initialize As KUSB_InitializeDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public SelectInterface As KUSB_SelectInterfaceDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public GetAssociatedInterface As KUSB_GetAssociatedInterfaceDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public Clone As KUSB_CloneDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public QueryInterfaceSettings As KUSB_QueryInterfaceSettingsDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public QueryDeviceInformation As KUSB_QueryDeviceInformationDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public SetCurrentAlternateSetting As KUSB_SetCurrentAlternateSettingDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public GetCurrentAlternateSetting As KUSB_GetCurrentAlternateSettingDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public QueryPipe As KUSB_QueryPipeDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public SetPipePolicy As KUSB_SetPipePolicyDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public GetPipePolicy As KUSB_GetPipePolicyDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public ReadPipe As KUSB_ReadPipeDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public WritePipe As KUSB_WritePipeDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public ResetPipe As KUSB_ResetPipeDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public AbortPipe As KUSB_AbortPipeDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public FlushPipe As KUSB_FlushPipeDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public IsoReadPipe As KUSB_IsoReadPipeDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public IsoWritePipe As KUSB_IsoWritePipeDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public GetCurrentFrameNumber As KUSB_GetCurrentFrameNumberDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public GetOverlappedResult As KUSB_GetOverlappedResultDelegate

		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public GetProperty As KUSB_GetPropertyDelegate
	End Structure

	''' <Summary>Hot plug parameter structure.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Size := 2048)> _
	Public Structure KHOT_PARAMS
		''' <Summary>Hot plug event window handle to send/post messages when notifications occur.</Summary>
		Public UserHwnd As IntPtr

		''' <Summary>WM_USER message start offset used when sending/posting messages, See details.</Summary>
		Public UserMessage As UInteger

		''' <Summary>Additional init/config parameters</Summary>
		Public Flags As KHOT_FLAG

		''' <Summary>File pattern matches for restricting notifcations to a single/group or all supported usb devices.</Summary>
		Public PatternMatch As KLST_PATTERN_MATCH

		''' <Summary>Hot plug event callback function invoked when notifications occur.</Summary>
		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public OnHotPlug As KHOT_PLUG_CB
	End Structure

	''' <Summary>Stream transfer context structure.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi)> _
	Public Structure KSTM_XFER_CONTEXT
		''' <Summary>Internal stream buffer.</Summary>
		Public Buffer As IntPtr

		''' <Summary>Size of internal stream buffer.</Summary>
		Public BufferSize As Integer

		''' <Summary>Number of bytes to write or number of bytes read.</Summary>
		Public TransferLength As Integer

		''' <Summary>User defined state.</Summary>
		Public UserState As IntPtr
	End Structure

	''' <Summary>Stream information structure.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi)> _
	Public Structure KSTM_INFO
		''' <Summary>\ref KUSB_HANDLE this stream uses.</Summary>
		Public UsbHandle As IntPtr

		''' <Summary>This parameter corresponds to the bEndpointAddress field in the endpoint descriptor.</Summary>
		Public PipeID As Byte

		''' <Summary>Maximum transfer read/write request allowed pending.</Summary>
		Public MaxPendingTransfers As Integer

		''' <Summary>Maximum transfer sage size.</Summary>
		Public MaxTransferSize As Integer

		''' <Summary>Maximum number of I/O request allowed pending.</Summary>
		Public MaxPendingIO As Integer

		''' <Summary>Populated with the endpoint descriptor for the specified \c PipeID.</Summary>
		Public EndpointDescriptor As USB_ENDPOINT_DESCRIPTOR

		''' <Summary>Populated with the driver api for the specified \c UsbHandle.</Summary>
		Public DriverAPI As KUSB_DRIVER_API

		''' <Summary>Populated with the device file handle for the specified \c UsbHandle.</Summary>
		Public DeviceHandle As IntPtr
	End Structure

	''' <Summary>Stream callback structure.</Summary>
	<StructLayout(LayoutKind.Sequential, CharSet := CharSet.Ansi, Size := 64)> _
	Public Structure KSTM_CALLBACK
		''' <Summary>Executed when a transfer error occurs.</Summary>
		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public [Error] As KSTM_ERROR_CB

		''' <Summary>Executed to submit a transfer.</Summary>
		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public Submit As KSTM_SUBMIT_CB

		''' <Summary>Executed when a valid transfer completes.</Summary>
		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public Complete As KSTM_COMPLETE_CB

		''' <Summary>Executed for every transfer context when the stream is started with \ref StmK_Start.</Summary>
		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public Started As KSTM_STARTED_CB

		''' <Summary>Executed for every transfer context when the stream is stopped with \ref StmK_Stop.</Summary>
		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public Stopped As KSTM_STOPPED_CB

		''' <Summary>Executed immediately after a transfer completes.</Summary>
		<MarshalAs(UnmanagedType.FunctionPtr)> _
		Public BeforeComplete As KSTM_BEFORE_COMPLETE_CB
	End Structure
	#End Region


	#Region "Delegates"
	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KLIB_HANDLE_CLEANUP_CB(<[In]> Handle As IntPtr, HandleType As KLIB_HANDLE_TYPE, UserContext As IntPtr) As Integer

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KISO_ENUM_PACKETS_CB(PacketIndex As UInteger, <[In]> ByRef IsoPacket As KISO_PACKET, UserState As IntPtr) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KLST_ENUM_DEVINFO_CB(<[In]> DeviceList As IntPtr, <[In]> DeviceInfo As KLST_DEVINFO_HANDLE, Context As IntPtr) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_InitDelegate(<Out> ByRef InterfaceHandle As KUSB_HANDLE, <[In]> DevInfo As KLST_DEVINFO_HANDLE) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_FreeDelegate(<[In]> InterfaceHandle As KUSB_HANDLE) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_ClaimInterfaceDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, NumberOrIndex As Byte, IsIndex As Boolean) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_ReleaseInterfaceDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, NumberOrIndex As Byte, IsIndex As Boolean) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_SetAltInterfaceDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, NumberOrIndex As Byte, IsIndex As Boolean, AltSettingNumber As Byte) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_GetAltInterfaceDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, NumberOrIndex As Byte, IsIndex As Boolean, ByRef AltSettingNumber As Byte) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_GetDescriptorDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, DescriptorType As Byte, Index As Byte, LanguageID As UShort, Buffer As IntPtr, BufferLength As UInteger, _
		ByRef LengthTransferred As UInteger) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_ControlTransferDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, SetupPacket As WINUSB_SETUP_PACKET, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As IntPtr) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_SetPowerPolicyDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, PolicyType As UInteger, ValueLength As UInteger, Value As IntPtr) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_GetPowerPolicyDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, PolicyType As UInteger, ByRef ValueLength As UInteger, Value As IntPtr) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_SetConfigurationDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, ConfigurationNumber As Byte) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_GetConfigurationDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, ByRef ConfigurationNumber As Byte) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_ResetDeviceDelegate(<[In]> InterfaceHandle As KUSB_HANDLE) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_InitializeDelegate(DeviceHandle As IntPtr, <Out> ByRef InterfaceHandle As KUSB_HANDLE) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_SelectInterfaceDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, NumberOrIndex As Byte, IsIndex As Boolean) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_GetAssociatedInterfaceDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, AssociatedInterfaceIndex As Byte, <Out> ByRef AssociatedInterfaceHandle As KUSB_HANDLE) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_CloneDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, <Out> ByRef DstInterfaceHandle As KUSB_HANDLE) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_QueryInterfaceSettingsDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, AltSettingNumber As Byte, <Out> ByRef UsbAltInterfaceDescriptor As USB_INTERFACE_DESCRIPTOR) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_QueryDeviceInformationDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, InformationType As UInteger, ByRef BufferLength As UInteger, Buffer As IntPtr) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_SetCurrentAlternateSettingDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, AltSettingNumber As Byte) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_GetCurrentAlternateSettingDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, ByRef AltSettingNumber As Byte) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_QueryPipeDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, AltSettingNumber As Byte, PipeIndex As Byte, <Out> ByRef PipeInformation As WINUSB_PIPE_INFORMATION) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_SetPipePolicyDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte, PolicyType As UInteger, ValueLength As UInteger, Value As IntPtr) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_GetPipePolicyDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte, PolicyType As UInteger, ByRef ValueLength As UInteger, Value As IntPtr) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_ReadPipeDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As IntPtr) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_WritePipeDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As IntPtr) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_ResetPipeDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_AbortPipeDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_FlushPipeDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_IsoReadPipeDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, Overlapped As IntPtr, <[In]> IsoContext As KISO_CONTEXT) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_IsoWritePipeDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, Overlapped As IntPtr, <[In]> IsoContext As KISO_CONTEXT) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_GetCurrentFrameNumberDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, ByRef FrameNumber As UInteger) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_GetOverlappedResultDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, Overlapped As IntPtr, ByRef lpNumberOfBytesTransferred As UInteger, bWait As Boolean) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KUSB_GetPropertyDelegate(<[In]> InterfaceHandle As KUSB_HANDLE, PropertyType As KUSB_PROPERTY, ByRef PropertySize As UInteger, Value As IntPtr) As Boolean

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Sub KHOT_PLUG_CB(<[In]> HotHandle As IntPtr, <[In]> DeviceInfo As KLST_DEVINFO_HANDLE, PlugType As KLST_SYNC_FLAG)

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KSTM_ERROR_CB(<[In]> ByRef StreamInfo As KSTM_INFO, <[In]> ByRef XferContext As KSTM_XFER_CONTEXT, ErrorCode As Integer) As Integer

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KSTM_SUBMIT_CB(<[In]> ByRef StreamInfo As KSTM_INFO, <[In]> ByRef XferContext As KSTM_XFER_CONTEXT, Overlapped As IntPtr) As Integer

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KSTM_STARTED_CB(<[In]> ByRef StreamInfo As KSTM_INFO, <[In]> ByRef XferContext As KSTM_XFER_CONTEXT, XferContextIndex As Integer) As Integer

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KSTM_STOPPED_CB(<[In]> ByRef StreamInfo As KSTM_INFO, <[In]> ByRef XferContext As KSTM_XFER_CONTEXT, XferContextIndex As Integer) As Integer

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KSTM_COMPLETE_CB(<[In]> ByRef StreamInfo As KSTM_INFO, <[In]> ByRef XferContext As KSTM_XFER_CONTEXT, ErrorCode As Integer) As Integer

	<UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet := CharSet.Ansi, SetLastError := True)> _
	Public Delegate Function KSTM_BEFORE_COMPLETE_CB(<[In]> ByRef StreamInfo As KSTM_INFO, <[In]> ByRef XferContext As KSTM_XFER_CONTEXT, ByRef ErrorCode As Integer) As KSTM_COMPLETE_RESULT
	#End Region


	Public Class LstK
		Protected ReadOnly m_handle As KLST_HANDLE

		''' <Summary>Initializes a new usb device list containing all supported devices.</Summary>
		Public Sub New(Flags As KLST_FLAG)
			RuntimeHelpers.PrepareConstrainedRegions()

			Try
			Finally
				Dim success As Boolean = Functions.LstK_Init(m_handle, Flags)

				If Not success OrElse m_handle.IsInvalid OrElse m_handle.IsClosed Then
					m_handle.SetHandleAsInvalid()
					Dim errorCode As Integer = Marshal.GetLastWin32Error()
					Throw New Exception([GetType]().Name & " failed. ErrorCode=" & errorCode.ToString("X"))
				End If
			End Try
		End Sub

		''' <Summary>Initializes a new usb device list containing only devices matching a specific class GUID.</Summary>
		Public Sub New(ByRef InitParams As KLST_INITEX_PARAMS)
			RuntimeHelpers.PrepareConstrainedRegions()

			Try
			Finally
				Dim success As Boolean = Functions.LstK_InitEx(m_handle, InitParams)

				If Not success OrElse m_handle.IsInvalid OrElse m_handle.IsClosed Then
					m_handle.SetHandleAsInvalid()
					Dim errorCode As Integer = Marshal.GetLastWin32Error()
					Throw New Exception([GetType]().Name & " failed. ErrorCode=" & errorCode.ToString("X"))
				End If
			End Try
		End Sub

		Public ReadOnly Property Handle() As KLST_HANDLE
			Get
				Return m_handle
			End Get
		End Property

		Public Overridable Function Free() As Boolean
			If m_handle.IsInvalid OrElse m_handle.IsClosed Then
				Return False
			End If

			m_handle.Close()
			Return True
		End Function

		''' <Summary>Enumerates \ref KLST_DEVINFO elements of a \ref KLST_HANDLE.</Summary>
		Public Function Enumerate(EnumDevListCB As KLST_ENUM_DEVINFO_CB, Context As IntPtr) As Boolean
			Return Functions.LstK_Enumerate(m_handle, EnumDevListCB, Context)
		End Function

		''' <Summary>Gets the \ref KLST_DEVINFO element for the current position.</Summary>
		Public Function Current(ByRef DeviceInfo As KLST_DEVINFO_HANDLE) As Boolean
			Return Functions.LstK_Current(m_handle, DeviceInfo)
		End Function

		''' <Summary>Advances the device list current \ref KLST_DEVINFO position.</Summary>
		Public Function MoveNext(ByRef DeviceInfo As KLST_DEVINFO_HANDLE) As Boolean
			Return Functions.LstK_MoveNext(m_handle, DeviceInfo)
		End Function

		''' <Summary>Sets the device list to its initial position, which is before the first element in the list.</Summary>
		Public Sub MoveReset()
			Functions.LstK_MoveReset(m_handle)
		End Sub

		''' <Summary>Find a device by vendor and product id</Summary>
		Public Function FindByVidPid(Vid As Integer, Pid As Integer, ByRef DeviceInfo As KLST_DEVINFO_HANDLE) As Boolean
			Return Functions.LstK_FindByVidPid(m_handle, Vid, Pid, DeviceInfo)
		End Function

		''' <Summary>Counts the number of device info elements in a device list.</Summary>
		Public Function Count(ByRef Count As UInteger) As Boolean
			Return Functions.LstK_Count(m_handle, Count)
		End Function
	End Class

	Public Class HotK
		Protected ReadOnly m_handle As KHOT_HANDLE

		''' <Summary>Creates a new hot-plug handle for USB device arrival/removal event monitoring.</Summary>
		Public Sub New(ByRef InitParams As KHOT_PARAMS)
			RuntimeHelpers.PrepareConstrainedRegions()

			Try
			Finally
				Dim success As Boolean = Functions.HotK_Init(m_handle, InitParams)

				If Not success OrElse m_handle.IsInvalid OrElse m_handle.IsClosed Then
					m_handle.SetHandleAsInvalid()
					Dim errorCode As Integer = Marshal.GetLastWin32Error()
					Throw New Exception([GetType]().Name & " failed. ErrorCode=" & errorCode.ToString("X"))
				End If
			End Try
		End Sub

		Public ReadOnly Property Handle() As KHOT_HANDLE
			Get
				Return m_handle
			End Get
		End Property

		Public Overridable Function Free() As Boolean
			If m_handle.IsInvalid OrElse m_handle.IsClosed Then
				Return False
			End If

			m_handle.Close()
			Return True
		End Function

		''' <Summary>Frees all hot-plug handles initialized with \ref HotK_Init.</Summary>
		Public Sub FreeAll()
			Functions.HotK_FreeAll()
		End Sub
	End Class

	Public Class UsbK
		Protected ReadOnly driverAPI As KUSB_DRIVER_API
		Protected ReadOnly m_handle As KUSB_HANDLE

		''' <Summary>Creates/opens a libusbK interface handle from the device list. This is a preferred method.</Summary>
		Public Sub New(DevInfo As KLST_DEVINFO_HANDLE)
			If Not Functions.LibK_LoadDriverAPI(driverAPI, DevInfo.DriverID) Then
				Throw New Exception([GetType]().Name & " failed loading Driver API. ErrorCode=" & Marshal.GetLastWin32Error().ToString("X"))
			End If

			RuntimeHelpers.PrepareConstrainedRegions()

			Try
			Finally
				Dim success As Boolean = driverAPI.Init(m_handle, DevInfo)

				If Not success OrElse m_handle.IsInvalid OrElse m_handle.IsClosed Then
					m_handle.SetHandleAsInvalid()
					Dim errorCode As Integer = Marshal.GetLastWin32Error()
					Throw New Exception([GetType]().Name & " failed. ErrorCode=" & errorCode.ToString("X"))
				End If
			End Try
		End Sub

		''' <Summary>Creates a libusbK handle for the device specified by a file handle.</Summary>
		Public Sub New(DeviceHandle As IntPtr, driverID As KUSB_DRVID)
			If Not Functions.LibK_LoadDriverAPI(driverAPI, CInt(driverID)) Then
				Throw New Exception([GetType]().Name & " failed loading Driver API. ErrorCode=" & Marshal.GetLastWin32Error().ToString("X"))
			End If

			RuntimeHelpers.PrepareConstrainedRegions()

			Try
			Finally
				Dim success As Boolean = driverAPI.Initialize(DeviceHandle, m_handle)

				If Not success OrElse m_handle.IsInvalid OrElse m_handle.IsClosed Then
					m_handle.SetHandleAsInvalid()
					Dim errorCode As Integer = Marshal.GetLastWin32Error()
					Throw New Exception([GetType]().Name & " failed. ErrorCode=" & errorCode.ToString("X"))
				End If
			End Try
		End Sub

		Public ReadOnly Property Handle() As KUSB_HANDLE
			Get
				Return m_handle
			End Get
		End Property

		Public Overridable Function Free() As Boolean
			If m_handle.IsInvalid OrElse m_handle.IsClosed Then
				Return False
			End If

			m_handle.Close()
			Return True
		End Function

		''' <Summary>Claims the specified interface by number or index.</Summary>
		Public Function ClaimInterface(NumberOrIndex As Byte, IsIndex As Boolean) As Boolean
			Return driverAPI.ClaimInterface(m_handle, NumberOrIndex, IsIndex)
		End Function

		''' <Summary>Releases the specified interface by number or index.</Summary>
		Public Function ReleaseInterface(NumberOrIndex As Byte, IsIndex As Boolean) As Boolean
			Return driverAPI.ReleaseInterface(m_handle, NumberOrIndex, IsIndex)
		End Function

		''' <Summary>Sets the alternate setting of the specified interface.</Summary>
		Public Function SetAltInterface(NumberOrIndex As Byte, IsIndex As Boolean, AltSettingNumber As Byte) As Boolean
			Return driverAPI.SetAltInterface(m_handle, NumberOrIndex, IsIndex, AltSettingNumber)
		End Function

		''' <Summary>Gets the alternate setting for the specified interface.</Summary>
		Public Function GetAltInterface(NumberOrIndex As Byte, IsIndex As Boolean, ByRef AltSettingNumber As Byte) As Boolean
			Return driverAPI.GetAltInterface(m_handle, NumberOrIndex, IsIndex, AltSettingNumber)
		End Function

		''' <Summary>Gets the requested descriptor. This is a synchronous operation.</Summary>
		Public Function GetDescriptor(DescriptorType As Byte, Index As Byte, LanguageID As UShort, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger) As Boolean
			Return driverAPI.GetDescriptor(m_handle, DescriptorType, Index, LanguageID, Buffer, BufferLength, _
				LengthTransferred)
		End Function

		''' <Summary>Gets the requested descriptor. This is a synchronous operation.</Summary>
		Public Function GetDescriptor(DescriptorType As Byte, Index As Byte, LanguageID As UShort, Buffer As Array, BufferLength As UInteger, ByRef LengthTransferred As UInteger) As Boolean
			Return driverAPI.GetDescriptor(m_handle, DescriptorType, Index, LanguageID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, _
				LengthTransferred)
		End Function

		''' <Summary>Transmits control data over a default control endpoint.</Summary>
		Public Function ControlTransfer(SetupPacket As WINUSB_SETUP_PACKET, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As IntPtr) As Boolean
			Return driverAPI.ControlTransfer(m_handle, SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped)
		End Function

		''' <Summary>Transmits control data over a default control endpoint.</Summary>
		Public Function ControlTransfer(SetupPacket As WINUSB_SETUP_PACKET, Buffer As Array, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As IntPtr) As Boolean
			Return driverAPI.ControlTransfer(m_handle, SetupPacket, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped)
		End Function

		''' <Summary>Transmits control data over a default control endpoint.</Summary>
		Public Function ControlTransfer(SetupPacket As WINUSB_SETUP_PACKET, Buffer As Array, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As KOVL_HANDLE) As Boolean
			Return driverAPI.ControlTransfer(m_handle, SetupPacket, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped.DangerousGetHandle())
		End Function

		''' <Summary>Transmits control data over a default control endpoint.</Summary>
		Public Function ControlTransfer(SetupPacket As WINUSB_SETUP_PACKET, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As KOVL_HANDLE) As Boolean
			Return driverAPI.ControlTransfer(m_handle, SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped.DangerousGetHandle())
		End Function

		''' <Summary>Sets the power policy for a device.</Summary>
		Public Function SetPowerPolicy(PolicyType As UInteger, ValueLength As UInteger, Value As IntPtr) As Boolean
			Return driverAPI.SetPowerPolicy(m_handle, PolicyType, ValueLength, Value)
		End Function

		''' <Summary>Sets the power policy for a device.</Summary>
		Public Function SetPowerPolicy(PolicyType As UInteger, ValueLength As UInteger, Value As Array) As Boolean
			Return driverAPI.SetPowerPolicy(m_handle, PolicyType, ValueLength, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))
		End Function

		''' <Summary>Gets the power policy for a device.</Summary>
		Public Function GetPowerPolicy(PolicyType As UInteger, ByRef ValueLength As UInteger, Value As IntPtr) As Boolean
			Return driverAPI.GetPowerPolicy(m_handle, PolicyType, ValueLength, Value)
		End Function

		''' <Summary>Gets the power policy for a device.</Summary>
		Public Function GetPowerPolicy(PolicyType As UInteger, ByRef ValueLength As UInteger, Value As Array) As Boolean
			Return driverAPI.GetPowerPolicy(m_handle, PolicyType, ValueLength, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))
		End Function

		''' <Summary>Sets the device configuration number.</Summary>
		Public Function SetConfiguration(ConfigurationNumber As Byte) As Boolean
			Return driverAPI.SetConfiguration(m_handle, ConfigurationNumber)
		End Function

		''' <Summary>Gets the device current configuration number.</Summary>
		Public Function GetConfiguration(ByRef ConfigurationNumber As Byte) As Boolean
			Return driverAPI.GetConfiguration(m_handle, ConfigurationNumber)
		End Function

		''' <Summary>Resets the usb device of the specified interface handle. (port cycle).</Summary>
		Public Function ResetDevice() As Boolean
			Return driverAPI.ResetDevice(m_handle)
		End Function

		''' <Summary>Selects the specified interface by number or index as the current interface.</Summary>
		Public Function SelectInterface(NumberOrIndex As Byte, IsIndex As Boolean) As Boolean
			Return driverAPI.SelectInterface(m_handle, NumberOrIndex, IsIndex)
		End Function

		''' <Summary>Retrieves a handle for an associated interface.</Summary>
		Public Function GetAssociatedInterface(AssociatedInterfaceIndex As Byte, ByRef AssociatedInterfaceHandle As KUSB_HANDLE) As Boolean
			Return driverAPI.GetAssociatedInterface(m_handle, AssociatedInterfaceIndex, AssociatedInterfaceHandle)
		End Function

		''' <Summary>Clones the specified interface handle.</Summary>
		Public Function Clone(ByRef DstInterfaceHandle As KUSB_HANDLE) As Boolean
			Return driverAPI.Clone(m_handle, DstInterfaceHandle)
		End Function

		''' <Summary>Retrieves the interface descriptor for the specified alternate interface settings for a particular interface handle.</Summary>
		Public Function QueryInterfaceSettings(AltSettingNumber As Byte, ByRef UsbAltInterfaceDescriptor As USB_INTERFACE_DESCRIPTOR) As Boolean
			Return driverAPI.QueryInterfaceSettings(m_handle, AltSettingNumber, UsbAltInterfaceDescriptor)
		End Function

		''' <Summary>Retrieves information about the physical device that is associated with a libusbK handle.</Summary>
		Public Function QueryDeviceInformation(InformationType As UInteger, ByRef BufferLength As UInteger, Buffer As IntPtr) As Boolean
			Return driverAPI.QueryDeviceInformation(m_handle, InformationType, BufferLength, Buffer)
		End Function

		''' <Summary>Sets the alternate setting of an interface.</Summary>
		Public Function SetCurrentAlternateSetting(AltSettingNumber As Byte) As Boolean
			Return driverAPI.SetCurrentAlternateSetting(m_handle, AltSettingNumber)
		End Function

		''' <Summary>Gets the current alternate interface setting for an interface.</Summary>
		Public Function GetCurrentAlternateSetting(ByRef AltSettingNumber As Byte) As Boolean
			Return driverAPI.GetCurrentAlternateSetting(m_handle, AltSettingNumber)
		End Function

		''' <Summary>Retrieves information about a pipe that is associated with an interface.</Summary>
		Public Function QueryPipe(AltSettingNumber As Byte, PipeIndex As Byte, ByRef PipeInformation As WINUSB_PIPE_INFORMATION) As Boolean
			Return driverAPI.QueryPipe(m_handle, AltSettingNumber, PipeIndex, PipeInformation)
		End Function

		''' <Summary>Sets the policy for a specific pipe associated with an endpoint on the device. This is a synchronous operation.</Summary>
		Public Function SetPipePolicy(PipeID As Byte, PolicyType As UInteger, ValueLength As UInteger, Value As IntPtr) As Boolean
			Return driverAPI.SetPipePolicy(m_handle, PipeID, PolicyType, ValueLength, Value)
		End Function

		''' <Summary>Sets the policy for a specific pipe associated with an endpoint on the device. This is a synchronous operation.</Summary>
		Public Function SetPipePolicy(PipeID As Byte, PolicyType As UInteger, ValueLength As UInteger, Value As Array) As Boolean
			Return driverAPI.SetPipePolicy(m_handle, PipeID, PolicyType, ValueLength, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))
		End Function

		''' <Summary>Gets the policy for a specific pipe (endpoint).</Summary>
		Public Function GetPipePolicy(PipeID As Byte, PolicyType As UInteger, ByRef ValueLength As UInteger, Value As IntPtr) As Boolean
			Return driverAPI.GetPipePolicy(m_handle, PipeID, PolicyType, ValueLength, Value)
		End Function

		''' <Summary>Gets the policy for a specific pipe (endpoint).</Summary>
		Public Function GetPipePolicy(PipeID As Byte, PolicyType As UInteger, ByRef ValueLength As UInteger, Value As Array) As Boolean
			Return driverAPI.GetPipePolicy(m_handle, PipeID, PolicyType, ValueLength, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))
		End Function

		''' <Summary>Reads data from the specified pipe.</Summary>
		Public Function ReadPipe(PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As IntPtr) As Boolean
			Return driverAPI.ReadPipe(m_handle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped)
		End Function

		''' <Summary>Reads data from the specified pipe.</Summary>
		Public Function ReadPipe(PipeID As Byte, Buffer As Array, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As IntPtr) As Boolean
			Return driverAPI.ReadPipe(m_handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped)
		End Function

		''' <Summary>Reads data from the specified pipe.</Summary>
		Public Function ReadPipe(PipeID As Byte, Buffer As Array, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As KOVL_HANDLE) As Boolean
			Return driverAPI.ReadPipe(m_handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped.DangerousGetHandle())
		End Function

		''' <Summary>Reads data from the specified pipe.</Summary>
		Public Function ReadPipe(PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As KOVL_HANDLE) As Boolean
			Return driverAPI.ReadPipe(m_handle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped.DangerousGetHandle())
		End Function

		''' <Summary>Writes data to a pipe.</Summary>
		Public Function WritePipe(PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As IntPtr) As Boolean
			Return driverAPI.WritePipe(m_handle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped)
		End Function

		''' <Summary>Writes data to a pipe.</Summary>
		Public Function WritePipe(PipeID As Byte, Buffer As Array, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As IntPtr) As Boolean
			Return driverAPI.WritePipe(m_handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped)
		End Function

		''' <Summary>Writes data to a pipe.</Summary>
		Public Function WritePipe(PipeID As Byte, Buffer As Array, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As KOVL_HANDLE) As Boolean
			Return driverAPI.WritePipe(m_handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped.DangerousGetHandle())
		End Function

		''' <Summary>Writes data to a pipe.</Summary>
		Public Function WritePipe(PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, ByRef LengthTransferred As UInteger, Overlapped As KOVL_HANDLE) As Boolean
			Return driverAPI.WritePipe(m_handle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped.DangerousGetHandle())
		End Function

		''' <Summary>Resets the data toggle and clears the stall condition on a pipe.</Summary>
		Public Function ResetPipe(PipeID As Byte) As Boolean
			Return driverAPI.ResetPipe(m_handle, PipeID)
		End Function

		''' <Summary>Aborts all of the pending transfers for a pipe.</Summary>
		Public Function AbortPipe(PipeID As Byte) As Boolean
			Return driverAPI.AbortPipe(m_handle, PipeID)
		End Function

		''' <Summary>Discards any data that is cached in a pipe.</Summary>
		Public Function FlushPipe(PipeID As Byte) As Boolean
			Return driverAPI.FlushPipe(m_handle, PipeID)
		End Function

		''' <Summary>Reads from an isochronous pipe.</Summary>
		Public Function IsoReadPipe(PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, Overlapped As IntPtr, IsoContext As KISO_CONTEXT) As Boolean
			Return driverAPI.IsoReadPipe(m_handle, PipeID, Buffer, BufferLength, Overlapped, IsoContext)
		End Function

		''' <Summary>Reads from an isochronous pipe.</Summary>
		Public Function IsoReadPipe(PipeID As Byte, Buffer As Array, BufferLength As UInteger, Overlapped As IntPtr, IsoContext As KISO_CONTEXT) As Boolean
			Return driverAPI.IsoReadPipe(m_handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, Overlapped, IsoContext)
		End Function

		''' <Summary>Reads from an isochronous pipe.</Summary>
		Public Function IsoReadPipe(PipeID As Byte, Buffer As Array, BufferLength As UInteger, Overlapped As KOVL_HANDLE, IsoContext As KISO_CONTEXT) As Boolean
			Return driverAPI.IsoReadPipe(m_handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, Overlapped.DangerousGetHandle(), IsoContext)
		End Function

		''' <Summary>Reads from an isochronous pipe.</Summary>
		Public Function IsoReadPipe(PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, Overlapped As KOVL_HANDLE, IsoContext As KISO_CONTEXT) As Boolean
			Return driverAPI.IsoReadPipe(m_handle, PipeID, Buffer, BufferLength, Overlapped.DangerousGetHandle(), IsoContext)
		End Function

		''' <Summary>Writes to an isochronous pipe.</Summary>
		Public Function IsoWritePipe(PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, Overlapped As IntPtr, IsoContext As KISO_CONTEXT) As Boolean
			Return driverAPI.IsoWritePipe(m_handle, PipeID, Buffer, BufferLength, Overlapped, IsoContext)
		End Function

		''' <Summary>Writes to an isochronous pipe.</Summary>
		Public Function IsoWritePipe(PipeID As Byte, Buffer As Array, BufferLength As UInteger, Overlapped As IntPtr, IsoContext As KISO_CONTEXT) As Boolean
			Return driverAPI.IsoWritePipe(m_handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, Overlapped, IsoContext)
		End Function

		''' <Summary>Writes to an isochronous pipe.</Summary>
		Public Function IsoWritePipe(PipeID As Byte, Buffer As Array, BufferLength As UInteger, Overlapped As KOVL_HANDLE, IsoContext As KISO_CONTEXT) As Boolean
			Return driverAPI.IsoWritePipe(m_handle, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, Overlapped.DangerousGetHandle(), IsoContext)
		End Function

		''' <Summary>Writes to an isochronous pipe.</Summary>
		Public Function IsoWritePipe(PipeID As Byte, Buffer As IntPtr, BufferLength As UInteger, Overlapped As KOVL_HANDLE, IsoContext As KISO_CONTEXT) As Boolean
			Return driverAPI.IsoWritePipe(m_handle, PipeID, Buffer, BufferLength, Overlapped.DangerousGetHandle(), IsoContext)
		End Function

		''' <Summary>Retrieves the current USB frame number.</Summary>
		Public Function GetCurrentFrameNumber(ByRef FrameNumber As UInteger) As Boolean
			Return driverAPI.GetCurrentFrameNumber(m_handle, FrameNumber)
		End Function

		''' <Summary>Retrieves the results of an overlapped operation on the specified libusbK handle.</Summary>
		Public Function GetOverlappedResult(Overlapped As IntPtr, ByRef lpNumberOfBytesTransferred As UInteger, bWait As Boolean) As Boolean
			Return driverAPI.GetOverlappedResult(m_handle, Overlapped, lpNumberOfBytesTransferred, bWait)
		End Function

		''' <Summary>Retrieves the results of an overlapped operation on the specified libusbK handle.</Summary>
		Public Function GetOverlappedResult(Overlapped As KOVL_HANDLE, ByRef lpNumberOfBytesTransferred As UInteger, bWait As Boolean) As Boolean
			Return driverAPI.GetOverlappedResult(m_handle, Overlapped.DangerousGetHandle(), lpNumberOfBytesTransferred, bWait)
		End Function

		''' <Summary>Gets a USB device (driver specific) property from usb handle.</Summary>
		Public Function GetProperty(PropertyType As KUSB_PROPERTY, ByRef PropertySize As UInteger, Value As IntPtr) As Boolean
			Return driverAPI.GetProperty(m_handle, PropertyType, PropertySize, Value)
		End Function

		''' <Summary>Gets a USB device (driver specific) property from usb handle.</Summary>
		Public Function GetProperty(PropertyType As KUSB_PROPERTY, ByRef PropertySize As UInteger, Value As Array) As Boolean
			Return driverAPI.GetProperty(m_handle, PropertyType, PropertySize, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))
		End Function
	End Class

	Public Class OvlK
		Protected ReadOnly m_handle As KOVL_POOL_HANDLE

		''' <Summary>Creates a new overlapped pool.</Summary>
		Public Sub New(UsbHandle As KUSB_HANDLE, MaxOverlappedCount As Integer, Flags As KOVL_POOL_FLAG)
			RuntimeHelpers.PrepareConstrainedRegions()

			Try
			Finally
				Dim success As Boolean = Functions.OvlK_Init(m_handle, UsbHandle, MaxOverlappedCount, Flags)

				If Not success OrElse m_handle.IsInvalid OrElse m_handle.IsClosed Then
					m_handle.SetHandleAsInvalid()
					Dim errorCode As Integer = Marshal.GetLastWin32Error()
					Throw New Exception([GetType]().Name & " failed. ErrorCode=" & errorCode.ToString("X"))
				End If
			End Try
		End Sub

		Public ReadOnly Property Handle() As KOVL_POOL_HANDLE
			Get
				Return m_handle
			End Get
		End Property

		Public Overridable Function Free() As Boolean
			If m_handle.IsInvalid OrElse m_handle.IsClosed Then
				Return False
			End If

			m_handle.Close()
			Return True
		End Function

		''' <Summary>Gets a preallocated \c OverlappedK structure from the specified/default pool.</Summary>
		Public Function Acquire(ByRef OverlappedK As KOVL_HANDLE) As Boolean
			Return Functions.OvlK_Acquire(OverlappedK, m_handle)
		End Function

		''' <Summary>Returns an \c OverlappedK structure to it's pool.</Summary>
		Public Function Release(OverlappedK As KOVL_HANDLE) As Boolean
			Return Functions.OvlK_Release(OverlappedK)
		End Function

		''' <Summary>Returns the internal event handle used to signal IO operations.</Summary>
		Public Function GetEventHandle(OverlappedK As KOVL_HANDLE) As IntPtr
			Return Functions.OvlK_GetEventHandle(OverlappedK)
		End Function

		''' <Summary>Waits for overlapped I/O completion, and performs actions specified in \c WaitFlags.</Summary>
		Public Function Wait(OverlappedK As KOVL_HANDLE, TimeoutMS As Integer, WaitFlags As KOVL_WAIT_FLAG, ByRef TransferredLength As UInteger) As Boolean
			Return Functions.OvlK_Wait(OverlappedK, TimeoutMS, WaitFlags, TransferredLength)
		End Function

		''' <Summary>Waits for overlapped I/O completion, cancels on a timeout error.</Summary>
		Public Function WaitOrCancel(OverlappedK As KOVL_HANDLE, TimeoutMS As Integer, ByRef TransferredLength As UInteger) As Boolean
			Return Functions.OvlK_WaitOrCancel(OverlappedK, TimeoutMS, TransferredLength)
		End Function

		''' <Summary>Waits for overlapped I/O completion, cancels on a timeout error and always releases the OvlK handle back to its pool.</Summary>
		Public Function WaitAndRelease(OverlappedK As KOVL_HANDLE, TimeoutMS As Integer, ByRef TransferredLength As UInteger) As Boolean
			Return Functions.OvlK_WaitAndRelease(OverlappedK, TimeoutMS, TransferredLength)
		End Function

		''' <Summary>Checks for i/o completion; returns immediately. (polling)</Summary>
		Public Function IsComplete(OverlappedK As KOVL_HANDLE) As Boolean
			Return Functions.OvlK_IsComplete(OverlappedK)
		End Function

		''' <Summary>Initializes an overlappedK for re-use. The overlappedK is not return to its pool.</Summary>
		Public Function ReUse(OverlappedK As KOVL_HANDLE) As Boolean
			Return Functions.OvlK_ReUse(OverlappedK)
		End Function
	End Class

	Public Class StmK
		Protected ReadOnly m_handle As KSTM_HANDLE

		''' <Summary>Initializes a new uni-directional pipe stream.</Summary>
		Public Sub New(UsbHandle As KUSB_HANDLE, PipeID As Byte, MaxTransferSize As Integer, MaxPendingTransfers As Integer, MaxPendingIO As Integer, ByRef Callbacks As KSTM_CALLBACK, _
			Flags As KSTM_FLAG)
			RuntimeHelpers.PrepareConstrainedRegions()

			Try
			Finally
				Dim success As Boolean = Functions.StmK_Init(m_handle, UsbHandle, PipeID, MaxTransferSize, MaxPendingTransfers, MaxPendingIO, _
					Callbacks, Flags)

				If Not success OrElse m_handle.IsInvalid OrElse m_handle.IsClosed Then
					m_handle.SetHandleAsInvalid()
					Dim errorCode As Integer = Marshal.GetLastWin32Error()
					Throw New Exception([GetType]().Name & " failed. ErrorCode=" & errorCode.ToString("X"))
				End If
			End Try
		End Sub

		Public ReadOnly Property Handle() As KSTM_HANDLE
			Get
				Return m_handle
			End Get
		End Property

		Public Overridable Function Free() As Boolean
			If m_handle.IsInvalid OrElse m_handle.IsClosed Then
				Return False
			End If

			m_handle.Close()
			Return True
		End Function

		''' <Summary>Starts the internal stream thread.</Summary>
		Public Function Start() As Boolean
			Return Functions.StmK_Start(m_handle)
		End Function

		''' <Summary>Stops the internal stream thread.</Summary>
		Public Function [Stop](TimeoutCancelMS As Integer) As Boolean
			Return Functions.StmK_Stop(m_handle, TimeoutCancelMS)
		End Function

		''' <Summary>Reads data from the stream buffer.</Summary>
		Public Function Read(Buffer As IntPtr, Offset As Integer, Length As Integer, ByRef TransferredLength As UInteger) As Boolean
			Return Functions.StmK_Read(m_handle, Buffer, Offset, Length, TransferredLength)
		End Function

		''' <Summary>Reads data from the stream buffer.</Summary>
		Public Function Read(Buffer As Array, Offset As Integer, Length As Integer, ByRef TransferredLength As UInteger) As Boolean
			Return Functions.StmK_Read(m_handle, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), Offset, Length, TransferredLength)
		End Function

		''' <Summary>Writes data to the stream buffer.</Summary>
		Public Function Write(Buffer As IntPtr, Offset As Integer, Length As Integer, ByRef TransferredLength As UInteger) As Boolean
			Return Functions.StmK_Write(m_handle, Buffer, Offset, Length, TransferredLength)
		End Function

		''' <Summary>Writes data to the stream buffer.</Summary>
		Public Function Write(Buffer As Array, Offset As Integer, Length As Integer, ByRef TransferredLength As UInteger) As Boolean
			Return Functions.StmK_Write(m_handle, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), Offset, Length, TransferredLength)
		End Function
	End Class

	Public Class IsoK
		Private Shared ReadOnly ofsFlags As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "Flags").ToInt32()
		Private Shared ReadOnly ofsStartFrame As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "StartFrame").ToInt32()
		Private Shared ReadOnly ofsErrorCount As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "ErrorCount").ToInt32()
		Private Shared ReadOnly ofsNumberOfPackets As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "NumberOfPackets").ToInt32()
		Protected ReadOnly m_handle As KISO_CONTEXT

		''' <Summary>Creates a new isochronous transfer context.</Summary>
		Public Sub New(NumberOfPackets As Integer, StartFrame As Integer)
			RuntimeHelpers.PrepareConstrainedRegions()

			Try
			Finally
				Dim success As Boolean = Functions.IsoK_Init(m_handle, NumberOfPackets, StartFrame)

				If Not success OrElse m_handle.IsInvalid OrElse m_handle.IsClosed Then
					m_handle.SetHandleAsInvalid()
					Dim errorCode As Integer = Marshal.GetLastWin32Error()
					Throw New Exception([GetType]().Name & " failed. ErrorCode=" & errorCode.ToString("X"))
				End If
			End Try
		End Sub

		Public ReadOnly Property Handle() As KISO_CONTEXT
			Get
				Return m_handle
			End Get
		End Property

		''' <Summary>Additional ISO transfer flags. See \ref KISO_FLAG.</Summary>
		Public Property Flags() As KISO_FLAG
			Get
				Return CType(Marshal.ReadInt32(m_handle.DangerousGetHandle(), ofsFlags), KISO_FLAG)
			End Get
			Set
				Marshal.WriteInt32(m_handle.DangerousGetHandle(), ofsFlags, CInt(value))
			End Set
		End Property

		''' <Summary>Specifies the frame number that the transfer should begin on (0 for ASAP).</Summary>
		Public Property StartFrame() As UInteger
			Get
				Return CUInt(Marshal.ReadInt32(m_handle.DangerousGetHandle(), ofsStartFrame))
			End Get
			Set
				Marshal.WriteInt32(m_handle.DangerousGetHandle(), ofsStartFrame, CType(value, Int32))
			End Set
		End Property

		''' <Summary>Contains the number of packets that completed with an error condition on return from the host controller driver.</Summary>
		Public Property ErrorCount() As Short
			Get
				Return Marshal.ReadInt16(m_handle.DangerousGetHandle(), ofsErrorCount)
			End Get
			Set
				Marshal.WriteInt16(m_handle.DangerousGetHandle(), ofsErrorCount, value)
			End Set
		End Property

		''' <Summary>Specifies the number of packets that are described by the variable-length array member \c IsoPacket.</Summary>
		Public Property NumberOfPackets() As Short
			Get
				Return Marshal.ReadInt16(m_handle.DangerousGetHandle(), ofsNumberOfPackets)
			End Get
			Set
				Marshal.WriteInt16(m_handle.DangerousGetHandle(), ofsNumberOfPackets, value)
			End Set
		End Property

		Public Overridable Function Free() As Boolean
			If m_handle.IsInvalid OrElse m_handle.IsClosed Then
				Return False
			End If

			m_handle.Close()
			Return True
		End Function

		''' <Summary>Convenience function for setting the offset of all ISO packets of an isochronous transfer context.</Summary>
		Public Function SetPackets(PacketSize As Integer) As Boolean
			Return Functions.IsoK_SetPackets(m_handle, PacketSize)
		End Function

		''' <Summary>Convenience function for setting all fields of a \ref KISO_PACKET.</Summary>
		Public Function SetPacket(PacketIndex As Integer, ByRef IsoPacket As KISO_PACKET) As Boolean
			Return Functions.IsoK_SetPacket(m_handle, PacketIndex, IsoPacket)
		End Function

		''' <Summary>Convenience function for getting all fields of a \ref KISO_PACKET.</Summary>
		Public Function GetPacket(PacketIndex As Integer, ByRef IsoPacket As KISO_PACKET) As Boolean
			Return Functions.IsoK_GetPacket(m_handle, PacketIndex, IsoPacket)
		End Function

		''' <Summary>Convenience function for enumerating ISO packets of an isochronous transfer context.</Summary>
		Public Function EnumPackets(EnumPackets As KISO_ENUM_PACKETS_CB, StartPacketIndex As Integer, UserState As IntPtr) As Boolean
			Return Functions.IsoK_EnumPackets(m_handle, EnumPackets, StartPacketIndex, UserState)
		End Function

		''' <Summary>Convenience function for re-using an isochronous transfer context in a subsequent request.</Summary>
		Public Function ReUse() As Boolean
			Return Functions.IsoK_ReUse(m_handle)
		End Function
	End Class
End Namespace
