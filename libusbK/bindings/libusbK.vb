#Region "Copyright (c) Travis Robinson"

' Copyright (c) 2011-2021 Travis Robinson <libusbdotnet@gmail.com>
' All rights reserved.
'
' C# libusbK Bindings
' Auto-generated on: 07.08.2021
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
Imports System.Diagnostics
Imports System.Runtime.InteropServices

' ReSharper disable InconsistentNaming
' ReSharper disable CheckNamespace
' ReSharper disable UnassignedReadonlyField

Namespace libusbK
    Public Module AllKOptions
        ''' <summary>
        '''     Alternate libusbK library to use.  This must be assigned before any libusbK functions are called and it must be the
        '''     full path and file name to a libusbK.dll.
        ''' </summary>
        Public LIBUSBK_FULLPATH_TO_ALTERNATE_DLL As String
    End Module

    Public Module AllKConstants
        ''' <summary>
        '''     Allocated length for all strings in a \ref KLST_DEVINFO structure.
        ''' </summary>
        Public Const KLST_STRING_MAX_LEN As Integer = 256

        ''' <summary>
        '''     libusbK library
        ''' </summary>
        Public Const LIBUSBK_DLL As String = "libusbK.dll"

        ''' <summary>
        '''     Config power mask for the \c bmAttributes field of a \ref USB_CONFIGURATION_DESCRIPTOR
        ''' </summary>
        Public Const USB_CONFIG_POWERED_MASK As Byte = &HC0

        ''' <summary>
        '''     Endpoint direction mask for the \c bEndpointAddress field of a \ref USB_ENDPOINT_DESCRIPTOR
        ''' </summary>
        Public Const USB_ENDPOINT_DIRECTION_MASK As Byte = &H80

        ''' <summary>
        '''     Endpoint address mask for the \c bEndpointAddress field of a \ref USB_ENDPOINT_DESCRIPTOR
        ''' </summary>
        Public Const USB_ENDPOINT_ADDRESS_MASK As Byte = &H0F
    End Module

    Public Enum PipePolicyType
        SHORT_PACKET_TERMINATE = &H01
        AUTO_CLEAR_STALL = &H02
        PIPE_TRANSFER_TIMEOUT = &H03
        IGNORE_SHORT_PACKETS = &H04
        ALLOW_PARTIAL_READS = &H05
        AUTO_FLUSH = &H06
        RAW_IO = &H07
        MAXIMUM_TRANSFER_SIZE = &H08
        RESET_PIPE_ON_RESUME = &H09
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
        DEVICE_SPEED = &H01
    End Enum

    Public Enum EndpointType
        ''' <summary>
        '''     Indicates a control endpoint
        ''' </summary>
        CONTROL = &H00

        ''' <summary>
        '''     Indicates an isochronous endpoint
        ''' </summary>
        ISOCHRONOUS = &H01

        ''' <summary>
        '''     Indicates a bulk endpoint
        ''' </summary>
        BULK = &H02

        ''' <summary>
        '''     Indicates an interrupt endpoint
        ''' </summary>
        INTERRUPT = &H03

        ''' <summary>
        '''     Endpoint type mask for the \c bmAttributes field of a \ref USB_ENDPOINT_DESCRIPTOR
        ''' </summary>
        MASK = &H03
    End Enum

    Public Module ErrorCodes
        ''' <summary>
        '''     The operation completed successfully.
        ''' </summary>
        Public Const Success As Integer = 0

        ''' <summary>
        '''     Access is denied.
        ''' </summary>
        Public Const AccessDenied As Integer = 5

        ''' <summary>
        '''     The handle is invalid.
        ''' </summary>
        Public Const InvalidHandle As Integer = 6

        ''' <summary>
        '''     Not enough storage is available to process this command.
        ''' </summary>
        Public Const NotEnoughMemory As Integer = 8

        ''' <summary>
        '''     The request is not supported.
        ''' </summary>
        Public Const NotSupported As Integer = 50

        ''' <summary>
        '''     The parameter is incorrect.
        ''' </summary>
        Public Const InvalidParameter As Integer = 87

        ''' <summary>
        '''     The semaphore timeout period has expired.
        ''' </summary>
        Public Const SemTimeout As Integer = 121

        ''' <summary>
        '''     The requested resource is in use.
        ''' </summary>
        Public Const Busy As Integer = 170

        ''' <summary>
        '''     Too many dynamic-link modules are attached to this program or dynamic-link module.
        ''' </summary>
        Public Const TooManyModules As Integer = 214

        ''' <summary>
        '''     More data is available.
        ''' </summary>
        Public Const MoreData As Integer = 234

        ''' <summary>
        '''     No more data is available.
        ''' </summary>
        Public Const NoMoreItems As Integer = 259

        ''' <summary>
        '''     An attempt was made to operate on a thread within a specific process, but the thread specified is not in the
        '''     process specified.
        ''' </summary>
        Public Const ThreadNotInProcess As Integer = 566

        ''' <summary>
        '''     A thread termination occurred while the thread was suspended. The thread was resumed, and termination proceeded.
        ''' </summary>
        Public Const ThreadWasSuspended As Integer = 699

        ''' <summary>
        '''     The I/O operation has been aborted because of either a thread exit or an application request.
        ''' </summary>
        Public Const OperationAborted As Integer = 995

        ''' <summary>
        '''     Overlapped I/O event is not in a signaled state.
        ''' </summary>
        Public Const IoIncomplete As Integer = 996

        ''' <summary>
        '''     Overlapped I/O operation is in progress.
        ''' </summary>
        Public Const IoPending As Integer = 997

        ''' <summary>
        '''     Element not found.
        ''' </summary>
        Public Const NotFound As Integer = 1168

        ''' <summary>
        '''     The operation was canceled by the user.
        ''' </summary>
        Public Const Cancelled As Integer = 1223

        ''' <summary>
        '''     The library, drive, or media pool is empty.
        ''' </summary>
        Public Const Empty As Integer = 4306

        ''' <summary>
        '''     The cluster resource is not available.
        ''' </summary>
        Public Const ResourceNotAvailable As Integer = 5006

        ''' <summary>
        '''     The cluster resource could not be found.
        ''' </summary>
        Public Const ResourceNotFound As Integer = 5007
    End Module

    Public Interface IKLIB_HANDLE
        ReadOnly Property HandleType As KLIB_HANDLE_TYPE
        ReadOnly Property Pointer As IntPtr
        Function GetContext() As IntPtr
        Function SetContext(ByVal UserContext As IntPtr) As Boolean
        Function SetCleanupCallback(ByVal CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean
    End Interface

    ''' <Summary>
    '''     A structure representing additional information about super speed (or higher) endpoints.
    ''' </Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi)>
    Public Structure USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR
        ''' <Summary>Size of this descriptor (in bytes)</Summary>
        Public bLength As Byte

        ''' <Summary>Descriptor type</Summary>
        Public bDescriptorType As Byte

        ''' <Summary>Specifies the maximum number of packets that the endpoint can send or receive as a part of a burst.</Summary>
        Public bMaxBurst As Byte
        Public AsUchar As Byte

        ''' <Summary>Number of bytes per interval</Summary>
        Public wBytesPerInterval As UShort

        ''' <Summary>Specifies the maximum number of streams supported by the bulk endpoint.</Summary>
        Public ReadOnly Property BulkMaxStreams As Byte
            Get
                Return AsUchar And &H1F
            End Get
        End Property

        ''' <Summary>
        '''     Specifies a zero-based number that determines the maximum number of packets (bMaxBurst * (Mult + 1)) that can
        '''     be sent to the endpoint within a service interval.
        ''' </Summary>
        Public ReadOnly Property IsoMult As Byte
            Get
                Return AsUchar And &H3
            End Get
        End Property

        Public ReadOnly Property SspCompanion As Byte
            Get
                Return AsUchar >> 7
            End Get
        End Property
    End Structure

#Region "Opaque library handles"

    Public Structure KLST_HANDLE
        Implements IKLIB_HANDLE

        Private ReadOnly mHandlePtr As IntPtr

        Public Sub New(ByVal Handle As IntPtr)
            mHandlePtr = Handle
        End Sub

        Public ReadOnly Property Pointer As IntPtr Implements IKLIB_HANDLE.Pointer
            Get
                Return mHandlePtr
            End Get
        End Property

        Public ReadOnly Property HandleType As KLIB_HANDLE_TYPE Implements IKLIB_HANDLE.HandleType
            Get
                Return KLIB_HANDLE_TYPE.LSTK
            End Get
        End Property

        Public Function GetContext() As IntPtr Implements IKLIB_HANDLE.GetContext
            Return LibK_GetContext(mHandlePtr, HandleType)
        End Function

        Public Function SetContext(ByVal UserContext As IntPtr) As Boolean Implements IKLIB_HANDLE.SetContext
            Return LibK_SetContext(mHandlePtr, HandleType, UserContext)
        End Function

        Public Function SetCleanupCallback(ByVal CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean Implements IKLIB_HANDLE.SetCleanupCallback
            Return LibK_SetCleanupCallback(mHandlePtr, HandleType, CleanupCallback)
        End Function
    End Structure

    Public Structure KHOT_HANDLE
        Implements IKLIB_HANDLE

        Private ReadOnly mHandlePtr As IntPtr

        Public Sub New(ByVal Handle As IntPtr)
            mHandlePtr = Handle
        End Sub

        Public ReadOnly Property Pointer As IntPtr Implements IKLIB_HANDLE.Pointer
            Get
                Return mHandlePtr
            End Get
        End Property

        Public ReadOnly Property HandleType As KLIB_HANDLE_TYPE Implements IKLIB_HANDLE.HandleType
            Get
                Return KLIB_HANDLE_TYPE.HOTK
            End Get
        End Property

        Public Function GetContext() As IntPtr Implements IKLIB_HANDLE.GetContext
            Return LibK_GetContext(mHandlePtr, HandleType)
        End Function

        Public Function SetContext(ByVal UserContext As IntPtr) As Boolean Implements IKLIB_HANDLE.SetContext
            Return LibK_SetContext(mHandlePtr, HandleType, UserContext)
        End Function

        Public Function SetCleanupCallback(ByVal CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean Implements IKLIB_HANDLE.SetCleanupCallback
            Return LibK_SetCleanupCallback(mHandlePtr, HandleType, CleanupCallback)
        End Function
    End Structure

    Public Structure KUSB_HANDLE
        Implements IKLIB_HANDLE

        Private ReadOnly mHandlePtr As IntPtr

        Public Sub New(ByVal Handle As IntPtr)
            mHandlePtr = Handle
        End Sub

        Public ReadOnly Property Pointer As IntPtr Implements IKLIB_HANDLE.Pointer
            Get
                Return mHandlePtr
            End Get
        End Property

        Public ReadOnly Property HandleType As KLIB_HANDLE_TYPE Implements IKLIB_HANDLE.HandleType
            Get
                Return KLIB_HANDLE_TYPE.USBK
            End Get
        End Property

        Public Function GetContext() As IntPtr Implements IKLIB_HANDLE.GetContext
            Return LibK_GetContext(mHandlePtr, HandleType)
        End Function

        Public Function SetContext(ByVal UserContext As IntPtr) As Boolean Implements IKLIB_HANDLE.SetContext
            Return LibK_SetContext(mHandlePtr, HandleType, UserContext)
        End Function

        Public Function SetCleanupCallback(ByVal CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean Implements IKLIB_HANDLE.SetCleanupCallback
            Return LibK_SetCleanupCallback(mHandlePtr, HandleType, CleanupCallback)
        End Function

#Region "USB Shared Device Context"

        Public Function GetSharedContext() As IntPtr
            Return LibK_GetContext(mHandlePtr, KLIB_HANDLE_TYPE.USBSHAREDK)
        End Function

        Public Function SetSharedContext(ByVal UserContext As IntPtr) As Boolean
            Return LibK_SetContext(mHandlePtr, KLIB_HANDLE_TYPE.USBSHAREDK, UserContext)
        End Function

        Public Function SetSharedCleanupCallback(ByVal CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean
            Return LibK_SetCleanupCallback(mHandlePtr, KLIB_HANDLE_TYPE.USBSHAREDK, CleanupCallback)
        End Function

#End Region
    End Structure

    Public Structure KOVL_POOL_HANDLE
        Implements IKLIB_HANDLE

        Private ReadOnly mHandlePtr As IntPtr

        Public Sub New(ByVal Handle As IntPtr)
            mHandlePtr = Handle
        End Sub

        Public ReadOnly Property Pointer As IntPtr Implements IKLIB_HANDLE.Pointer
            Get
                Return mHandlePtr
            End Get
        End Property

        Public ReadOnly Property HandleType As KLIB_HANDLE_TYPE Implements IKLIB_HANDLE.HandleType
            Get
                Return KLIB_HANDLE_TYPE.OVLPOOLK
            End Get
        End Property

        Public Function GetContext() As IntPtr Implements IKLIB_HANDLE.GetContext
            Return LibK_GetContext(mHandlePtr, HandleType)
        End Function

        Public Function SetContext(ByVal UserContext As IntPtr) As Boolean Implements IKLIB_HANDLE.SetContext
            Return LibK_SetContext(mHandlePtr, HandleType, UserContext)
        End Function

        Public Function SetCleanupCallback(ByVal CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean Implements IKLIB_HANDLE.SetCleanupCallback
            Return LibK_SetCleanupCallback(mHandlePtr, HandleType, CleanupCallback)
        End Function
    End Structure

    Public Structure KOVL_HANDLE
        Implements IKLIB_HANDLE

        Private ReadOnly mHandlePtr As IntPtr

        Public Sub New(ByVal Handle As IntPtr)
            mHandlePtr = Handle
        End Sub

        Public ReadOnly Property Pointer As IntPtr Implements IKLIB_HANDLE.Pointer
            Get
                Return mHandlePtr
            End Get
        End Property

        Public ReadOnly Property HandleType As KLIB_HANDLE_TYPE Implements IKLIB_HANDLE.HandleType
            Get
                Return KLIB_HANDLE_TYPE.OVLK
            End Get
        End Property

        Public Function GetContext() As IntPtr Implements IKLIB_HANDLE.GetContext
            Return LibK_GetContext(mHandlePtr, HandleType)
        End Function

        Public Function SetContext(ByVal UserContext As IntPtr) As Boolean Implements IKLIB_HANDLE.SetContext
            Return LibK_SetContext(mHandlePtr, HandleType, UserContext)
        End Function

        Public Function SetCleanupCallback(ByVal CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean Implements IKLIB_HANDLE.SetCleanupCallback
            Return LibK_SetCleanupCallback(mHandlePtr, HandleType, CleanupCallback)
        End Function
    End Structure

    Public Structure KSTM_HANDLE
        Implements IKLIB_HANDLE

        Private ReadOnly mHandlePtr As IntPtr

        Public Sub New(ByVal Handle As IntPtr)
            mHandlePtr = Handle
        End Sub

        Public ReadOnly Property Pointer As IntPtr Implements IKLIB_HANDLE.Pointer
            Get
                Return mHandlePtr
            End Get
        End Property

        Public ReadOnly Property HandleType As KLIB_HANDLE_TYPE Implements IKLIB_HANDLE.HandleType
            Get
                Return KLIB_HANDLE_TYPE.STMK
            End Get
        End Property

        Public Function GetContext() As IntPtr Implements IKLIB_HANDLE.GetContext
            Return LibK_GetContext(mHandlePtr, HandleType)
        End Function

        Public Function SetContext(ByVal UserContext As IntPtr) As Boolean Implements IKLIB_HANDLE.SetContext
            Return LibK_SetContext(mHandlePtr, HandleType, UserContext)
        End Function

        Public Function SetCleanupCallback(ByVal CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean Implements IKLIB_HANDLE.SetCleanupCallback
            Return LibK_SetCleanupCallback(mHandlePtr, HandleType, CleanupCallback)
        End Function
    End Structure

    Public Structure KISOCH_HANDLE
        Implements IKLIB_HANDLE

        Private ReadOnly mHandlePtr As IntPtr

        Public Sub New(ByVal Handle As IntPtr)
            mHandlePtr = Handle
        End Sub

        Public ReadOnly Property Pointer As IntPtr Implements IKLIB_HANDLE.Pointer
            Get
                Return mHandlePtr
            End Get
        End Property

        Public ReadOnly Property HandleType As KLIB_HANDLE_TYPE Implements IKLIB_HANDLE.HandleType
            Get
                Return KLIB_HANDLE_TYPE.ISOCHK
            End Get
        End Property

        Public Function GetContext() As IntPtr Implements IKLIB_HANDLE.GetContext
            Return LibK_GetContext(mHandlePtr, HandleType)
        End Function

        Public Function SetContext(ByVal UserContext As IntPtr) As Boolean Implements IKLIB_HANDLE.SetContext
            Return LibK_SetContext(mHandlePtr, HandleType, UserContext)
        End Function

        Public Function SetCleanupCallback(ByVal CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean Implements IKLIB_HANDLE.SetCleanupCallback
            Return LibK_SetCleanupCallback(mHandlePtr, HandleType, CleanupCallback)
        End Function
    End Structure

#End Region

#Region "Internal Function Imports"

    Friend Module AllKFunctions
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Sub HotK_FreeAllDelegate()
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function HotK_FreeDelegate(
        <[In]> ByVal Handle As KHOT_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function HotK_InitDelegate(<Out> ByRef Handle As KHOT_HANDLE,
        <[In]>
        <Out> ByRef InitParams As KHOT_PARAMS) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsochK_CalcPacketInformationDelegate(ByVal IsHighSpeed As Boolean,
        <[In]> ByRef PipeInformationEx As WINUSB_PIPE_INFORMATION_EX, <Out> ByRef PacketInformation As KISOCH_PACKET_INFORMATION) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsochK_EnumPacketsDelegate(
        <[In]> ByVal IsochHandle As KISOCH_HANDLE, ByVal EnumPackets As KISOCH_ENUM_PACKETS_CB, ByVal StartPacketIndex As UInteger, ByVal UserState As IntPtr) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsochK_FreeDelegate(
        <[In]> ByVal IsochHandle As KISOCH_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsochK_GetNumberOfPacketsDelegate(
        <[In]> ByVal IsochHandle As KISOCH_HANDLE, <Out> ByRef NumberOfPackets As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsochK_GetPacketDelegate(
        <[In]> ByVal IsochHandle As KISOCH_HANDLE, ByVal PacketIndex As UInteger, <Out> ByRef Offset As UInteger, <Out> ByRef Length As UInteger, <Out> ByRef Status As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsochK_InitDelegate(<Out> ByRef IsochHandle As KISOCH_HANDLE,
        <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PipeId As Byte, ByVal MaxNumberOfPackets As UInteger, ByVal TransferBuffer As IntPtr, ByVal TransferBufferSize As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsochK_SetNumberOfPacketsDelegate(
        <[In]> ByVal IsochHandle As KISOCH_HANDLE, ByVal NumberOfPackets As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsochK_SetPacketDelegate(
        <[In]> ByVal IsochHandle As KISOCH_HANDLE, ByVal PacketIndex As UInteger, ByVal Offset As UInteger, ByVal Length As UInteger, ByVal Status As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsochK_SetPacketOffsetsDelegate(
        <[In]> ByVal IsochHandle As KISOCH_HANDLE, ByVal PacketSize As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsoK_EnumPacketsDelegate(
        <[In]> ByVal IsoContext As KISO_CONTEXT, ByVal EnumPackets As KISO_ENUM_PACKETS_CB, ByVal StartPacketIndex As Integer, ByVal UserState As IntPtr) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsoK_FreeDelegate(
        <[In]> ByVal IsoContext As KISO_CONTEXT) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsoK_GetPacketDelegate(
        <[In]> ByVal IsoContext As KISO_CONTEXT, ByVal PacketIndex As Integer, <Out> ByRef IsoPacket As KISO_PACKET) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsoK_InitDelegate(<Out> ByRef IsoContext As KISO_CONTEXT, ByVal NumberOfPackets As Integer, ByVal StartFrame As Integer) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsoK_ReUseDelegate(
        <[In]> ByVal IsoContext As KISO_CONTEXT) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsoK_SetPacketDelegate(
        <[In]> ByVal IsoContext As KISO_CONTEXT, ByVal PacketIndex As Integer,
        <[In]> ByRef IsoPacket As KISO_PACKET) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function IsoK_SetPacketsDelegate(
        <[In]> ByVal IsoContext As KISO_CONTEXT, ByVal PacketSize As Integer) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Sub LibK_Context_FreeDelegate()
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LibK_Context_InitDelegate(ByVal Heap As IntPtr, ByVal Reserved As IntPtr) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LibK_CopyDriverAPIDelegate(<Out> ByRef DriverAPI As KUSB_DRIVER_API,
        <[In]> ByVal UsbHandle As KUSB_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LibK_GetContextDelegate(
        <[In]> ByVal Handle As IntPtr, ByVal HandleType As KLIB_HANDLE_TYPE) As IntPtr
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LibK_GetDefaultContextDelegate(ByVal HandleType As KLIB_HANDLE_TYPE) As IntPtr
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LibK_GetProcAddressDelegate(ByVal ProcAddress As IntPtr, ByVal DriverID As Integer, ByVal FunctionID As Integer) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Sub LibK_GetVersionDelegate(<Out> ByRef Version As KLIB_VERSION)
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LibK_IsFunctionSupportedDelegate(
        <[In]> ByRef DriverAPI As KUSB_DRIVER_API, ByVal FunctionID As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LibK_LoadDriverAPIDelegate(<Out> ByRef DriverAPI As KUSB_DRIVER_API, ByVal DriverID As Integer) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LibK_SetCleanupCallbackDelegate(
        <[In]> ByVal Handle As IntPtr, ByVal HandleType As KLIB_HANDLE_TYPE, ByVal CleanupCB As KLIB_HANDLE_CLEANUP_CB) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LibK_SetContextDelegate(
        <[In]> ByVal Handle As IntPtr, ByVal HandleType As KLIB_HANDLE_TYPE, ByVal ContextValue As IntPtr) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LibK_SetDefaultContextDelegate(ByVal HandleType As KLIB_HANDLE_TYPE, ByVal ContextValue As IntPtr) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LstK_CountDelegate(
        <[In]> ByVal DeviceList As KLST_HANDLE, ByRef Count As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LstK_CurrentDelegate(
        <[In]> ByVal DeviceList As KLST_HANDLE, <Out> ByRef DeviceInfo As KLST_DEVINFO_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LstK_EnumerateDelegate(
        <[In]> ByVal DeviceList As KLST_HANDLE, ByVal EnumDevListCB As KLST_ENUM_DEVINFO_CB, ByVal Context As IntPtr) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LstK_FindByVidPidDelegate(
        <[In]> ByVal DeviceList As KLST_HANDLE, ByVal Vid As Integer, ByVal Pid As Integer, <Out> ByRef DeviceInfo As KLST_DEVINFO_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LstK_FreeDelegate(
        <[In]> ByVal DeviceList As KLST_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LstK_InitDelegate(<Out> ByRef DeviceList As KLST_HANDLE, ByVal Flags As KLST_FLAG) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LstK_InitExDelegate(<Out> ByRef DeviceList As KLST_HANDLE, ByVal Flags As KLST_FLAG,
        <[In]> ByRef PatternMatch As KLST_PATTERN_MATCH) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LstK_MoveNextDelegate(
        <[In]> ByVal DeviceList As KLST_HANDLE, <Out> ByRef DeviceInfo As KLST_DEVINFO_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Sub LstK_MoveResetDelegate(
        <[In]> ByVal DeviceList As KLST_HANDLE)
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LUsb0_ControlTransferDelegate(
        <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal SetupPacket As WINUSB_SETUP_PACKET, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As IntPtr) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function LUsb0_SetConfigurationDelegate(
        <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal ConfigurationNumber As Byte) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function OvlK_AcquireDelegate(<Out> ByRef OverlappedK As KOVL_HANDLE,
        <[In]> ByVal PoolHandle As KOVL_POOL_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function OvlK_FreeDelegate(
        <[In]> ByVal PoolHandle As KOVL_POOL_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function OvlK_GetEventHandleDelegate(
        <[In]> ByVal OverlappedK As KOVL_HANDLE) As IntPtr
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function OvlK_InitDelegate(<Out> ByRef PoolHandle As KOVL_POOL_HANDLE,
        <[In]> ByVal UsbHandle As KUSB_HANDLE, ByVal MaxOverlappedCount As Integer, ByVal Flags As KOVL_POOL_FLAG) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function OvlK_IsCompleteDelegate(
        <[In]> ByVal OverlappedK As KOVL_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function OvlK_ReleaseDelegate(
        <[In]> ByVal OverlappedK As KOVL_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function OvlK_ReUseDelegate(
        <[In]> ByVal OverlappedK As KOVL_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function OvlK_WaitAndReleaseDelegate(
        <[In]> ByVal OverlappedK As KOVL_HANDLE, ByVal TimeoutMS As Integer, <Out> ByRef TransferredLength As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function OvlK_WaitDelegate(
        <[In]> ByVal OverlappedK As KOVL_HANDLE, ByVal TimeoutMS As Integer, ByVal WaitFlags As KOVL_WAIT_FLAG, <Out> ByRef TransferredLength As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function OvlK_WaitOldestDelegate(
        <[In]> ByVal PoolHandle As KOVL_POOL_HANDLE, <Out> ByRef OverlappedK As KOVL_HANDLE, ByVal TimeoutMS As Integer, ByVal WaitFlags As KOVL_WAIT_FLAG, <Out> ByRef TransferredLength As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function OvlK_WaitOrCancelDelegate(
        <[In]> ByVal OverlappedK As KOVL_HANDLE, ByVal TimeoutMS As Integer, <Out> ByRef TransferredLength As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function StmK_FreeDelegate(
        <[In]> ByVal StreamHandle As KSTM_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function StmK_InitDelegate(<Out> ByRef StreamHandle As KSTM_HANDLE,
        <[In]> ByVal UsbHandle As KUSB_HANDLE, ByVal PipeID As Byte, ByVal MaxTransferSize As Integer, ByVal MaxPendingTransfers As Integer, ByVal MaxPendingIO As Integer,
        <[In]> ByRef Callbacks As KSTM_CALLBACK, ByVal Flags As KSTM_FLAG) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function StmK_ReadDelegate(
        <[In]> ByVal StreamHandle As KSTM_HANDLE, ByVal Buffer As IntPtr, ByVal Offset As Integer, ByVal Length As Integer, <Out> ByRef TransferredLength As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function StmK_StartDelegate(
        <[In]> ByVal StreamHandle As KSTM_HANDLE) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function StmK_StopDelegate(
        <[In]> ByVal StreamHandle As KSTM_HANDLE, ByVal TimeoutCancelMS As Integer) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function StmK_WriteDelegate(
        <[In]> ByVal StreamHandle As KSTM_HANDLE, ByVal Buffer As IntPtr, ByVal Offset As Integer, ByVal Length As Integer, <Out> ByRef TransferredLength As UInteger) As Boolean
        <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
        Public Delegate Function UsbK_FreeDelegate(
        <[In]> ByVal InterfaceHandle As KUSB_HANDLE) As Boolean
        Private ReadOnly mModuleLibusbK As IntPtr = IntPtr.Zero
        Public LibK_GetVersion As LibK_GetVersionDelegate
        Public LibK_GetContext As LibK_GetContextDelegate
        Public LibK_SetContext As LibK_SetContextDelegate
        Public LibK_SetCleanupCallback As LibK_SetCleanupCallbackDelegate
        Public LibK_LoadDriverAPI As LibK_LoadDriverAPIDelegate
        Public LibK_IsFunctionSupported As LibK_IsFunctionSupportedDelegate
        Public LibK_CopyDriverAPI As LibK_CopyDriverAPIDelegate
        Public LibK_GetProcAddress As LibK_GetProcAddressDelegate
        Public LibK_SetDefaultContext As LibK_SetDefaultContextDelegate
        Public LibK_GetDefaultContext As LibK_GetDefaultContextDelegate
        Public LibK_Context_Init As LibK_Context_InitDelegate
        Public LibK_Context_Free As LibK_Context_FreeDelegate
        Public UsbK_Free As UsbK_FreeDelegate
        Public LstK_Init As LstK_InitDelegate
        Public LstK_InitEx As LstK_InitExDelegate
        Public LstK_Free As LstK_FreeDelegate
        Public LstK_Enumerate As LstK_EnumerateDelegate
        Public LstK_Current As LstK_CurrentDelegate
        Public LstK_MoveNext As LstK_MoveNextDelegate
        Public LstK_MoveReset As LstK_MoveResetDelegate
        Public LstK_FindByVidPid As LstK_FindByVidPidDelegate
        Public LstK_Count As LstK_CountDelegate
        Public HotK_Init As HotK_InitDelegate
        Public HotK_Free As HotK_FreeDelegate
        Public HotK_FreeAll As HotK_FreeAllDelegate
        Public OvlK_Acquire As OvlK_AcquireDelegate
        Public OvlK_Release As OvlK_ReleaseDelegate
        Public OvlK_Init As OvlK_InitDelegate
        Public OvlK_Free As OvlK_FreeDelegate
        Public OvlK_GetEventHandle As OvlK_GetEventHandleDelegate
        Public OvlK_Wait As OvlK_WaitDelegate
        Public OvlK_WaitOldest As OvlK_WaitOldestDelegate
        Public OvlK_WaitOrCancel As OvlK_WaitOrCancelDelegate
        Public OvlK_WaitAndRelease As OvlK_WaitAndReleaseDelegate
        Public OvlK_IsComplete As OvlK_IsCompleteDelegate
        Public OvlK_ReUse As OvlK_ReUseDelegate
        Public StmK_Init As StmK_InitDelegate
        Public StmK_Free As StmK_FreeDelegate
        Public StmK_Start As StmK_StartDelegate
        Public StmK_Stop As StmK_StopDelegate
        Public StmK_Read As StmK_ReadDelegate
        Public StmK_Write As StmK_WriteDelegate
        Public IsoK_Init As IsoK_InitDelegate
        Public IsoK_Free As IsoK_FreeDelegate
        Public IsoK_SetPackets As IsoK_SetPacketsDelegate
        Public IsoK_SetPacket As IsoK_SetPacketDelegate
        Public IsoK_GetPacket As IsoK_GetPacketDelegate
        Public IsoK_EnumPackets As IsoK_EnumPacketsDelegate
        Public IsoK_ReUse As IsoK_ReUseDelegate
        Public IsochK_Init As IsochK_InitDelegate
        Public IsochK_Free As IsochK_FreeDelegate
        Public IsochK_SetPacketOffsets As IsochK_SetPacketOffsetsDelegate
        Public IsochK_SetPacket As IsochK_SetPacketDelegate
        Public IsochK_GetPacket As IsochK_GetPacketDelegate
        Public IsochK_EnumPackets As IsochK_EnumPacketsDelegate
        Public IsochK_CalcPacketInformation As IsochK_CalcPacketInformationDelegate
        Public IsochK_GetNumberOfPackets As IsochK_GetNumberOfPacketsDelegate
        Public IsochK_SetNumberOfPackets As IsochK_SetNumberOfPacketsDelegate
        Public LUsb0_ControlTransfer As LUsb0_ControlTransferDelegate
        Public LUsb0_SetConfiguration As LUsb0_SetConfigurationDelegate

        Sub New()
            If String.IsNullOrEmpty(LIBUSBK_FULLPATH_TO_ALTERNATE_DLL) Then
                mModuleLibusbK = LoadLibraryEx(LIBUSBK_DLL, IntPtr.Zero, LoadLibraryFlags.NONE)
            Else
                mModuleLibusbK = LoadLibraryEx(LIBUSBK_FULLPATH_TO_ALTERNATE_DLL, IntPtr.Zero, LoadLibraryFlags.LOAD_WITH_ALTERED_SEARCH_PATH)
            End If

            If mModuleLibusbK = IntPtr.Zero Then Throw New DllNotFoundException("libusbK.dll not found.  Please install drivers/applications and retry.")
            Call LoadDynamicFunctions()
        End Sub

        <DllImport("kernel32.dll")>
        Private Function LoadLibraryEx(ByVal lpFileName As String, ByVal hReservedNull As IntPtr, ByVal dwFlags As LoadLibraryFlags) As IntPtr
        End Function

        <DllImport("kernel32.dll", CharSet:=CharSet.Ansi, ExactSpelling:=True, SetLastError:=True)>
        Private Function GetProcAddress(ByVal hModule As IntPtr, ByVal procName As String) As IntPtr
        End Function

        Private Sub LoadDynamicFunctions()
            LibK_GetVersion = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LibK_GetVersion"), GetType(LibK_GetVersionDelegate)), LibK_GetVersionDelegate)
            LibK_GetContext = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LibK_GetContext"), GetType(LibK_GetContextDelegate)), LibK_GetContextDelegate)
            LibK_SetContext = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LibK_SetContext"), GetType(LibK_SetContextDelegate)), LibK_SetContextDelegate)
            LibK_SetCleanupCallback = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LibK_SetCleanupCallback"), GetType(LibK_SetCleanupCallbackDelegate)), LibK_SetCleanupCallbackDelegate)
            LibK_LoadDriverAPI = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LibK_LoadDriverAPI"), GetType(LibK_LoadDriverAPIDelegate)), LibK_LoadDriverAPIDelegate)
            LibK_IsFunctionSupported = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LibK_IsFunctionSupported"), GetType(LibK_IsFunctionSupportedDelegate)), LibK_IsFunctionSupportedDelegate)
            LibK_CopyDriverAPI = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LibK_CopyDriverAPI"), GetType(LibK_CopyDriverAPIDelegate)), LibK_CopyDriverAPIDelegate)
            LibK_GetProcAddress = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LibK_GetProcAddress"), GetType(LibK_GetProcAddressDelegate)), LibK_GetProcAddressDelegate)
            LibK_SetDefaultContext = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LibK_SetDefaultContext"), GetType(LibK_SetDefaultContextDelegate)), LibK_SetDefaultContextDelegate)
            LibK_GetDefaultContext = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LibK_GetDefaultContext"), GetType(LibK_GetDefaultContextDelegate)), LibK_GetDefaultContextDelegate)
            LibK_Context_Init = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LibK_Context_Init"), GetType(LibK_Context_InitDelegate)), LibK_Context_InitDelegate)
            LibK_Context_Free = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LibK_Context_Free"), GetType(LibK_Context_FreeDelegate)), LibK_Context_FreeDelegate)
            UsbK_Free = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "UsbK_Free"), GetType(UsbK_FreeDelegate)), UsbK_FreeDelegate)
            LstK_Init = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LstK_Init"), GetType(LstK_InitDelegate)), LstK_InitDelegate)
            LstK_InitEx = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LstK_InitEx"), GetType(LstK_InitExDelegate)), LstK_InitExDelegate)
            LstK_Free = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LstK_Free"), GetType(LstK_FreeDelegate)), LstK_FreeDelegate)
            LstK_Enumerate = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LstK_Enumerate"), GetType(LstK_EnumerateDelegate)), LstK_EnumerateDelegate)
            LstK_Current = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LstK_Current"), GetType(LstK_CurrentDelegate)), LstK_CurrentDelegate)
            LstK_MoveNext = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LstK_MoveNext"), GetType(LstK_MoveNextDelegate)), LstK_MoveNextDelegate)
            LstK_MoveReset = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LstK_MoveReset"), GetType(LstK_MoveResetDelegate)), LstK_MoveResetDelegate)
            LstK_FindByVidPid = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LstK_FindByVidPid"), GetType(LstK_FindByVidPidDelegate)), LstK_FindByVidPidDelegate)
            LstK_Count = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LstK_Count"), GetType(LstK_CountDelegate)), LstK_CountDelegate)
            HotK_Init = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "HotK_Init"), GetType(HotK_InitDelegate)), HotK_InitDelegate)
            HotK_Free = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "HotK_Free"), GetType(HotK_FreeDelegate)), HotK_FreeDelegate)
            HotK_FreeAll = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "HotK_FreeAll"), GetType(HotK_FreeAllDelegate)), HotK_FreeAllDelegate)
            OvlK_Acquire = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "OvlK_Acquire"), GetType(OvlK_AcquireDelegate)), OvlK_AcquireDelegate)
            OvlK_Release = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "OvlK_Release"), GetType(OvlK_ReleaseDelegate)), OvlK_ReleaseDelegate)
            OvlK_Init = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "OvlK_Init"), GetType(OvlK_InitDelegate)), OvlK_InitDelegate)
            OvlK_Free = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "OvlK_Free"), GetType(OvlK_FreeDelegate)), OvlK_FreeDelegate)
            OvlK_GetEventHandle = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "OvlK_GetEventHandle"), GetType(OvlK_GetEventHandleDelegate)), OvlK_GetEventHandleDelegate)
            OvlK_Wait = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "OvlK_Wait"), GetType(OvlK_WaitDelegate)), OvlK_WaitDelegate)
            OvlK_WaitOldest = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "OvlK_WaitOldest"), GetType(OvlK_WaitOldestDelegate)), OvlK_WaitOldestDelegate)
            OvlK_WaitOrCancel = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "OvlK_WaitOrCancel"), GetType(OvlK_WaitOrCancelDelegate)), OvlK_WaitOrCancelDelegate)
            OvlK_WaitAndRelease = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "OvlK_WaitAndRelease"), GetType(OvlK_WaitAndReleaseDelegate)), OvlK_WaitAndReleaseDelegate)
            OvlK_IsComplete = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "OvlK_IsComplete"), GetType(OvlK_IsCompleteDelegate)), OvlK_IsCompleteDelegate)
            OvlK_ReUse = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "OvlK_ReUse"), GetType(OvlK_ReUseDelegate)), OvlK_ReUseDelegate)
            StmK_Init = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "StmK_Init"), GetType(StmK_InitDelegate)), StmK_InitDelegate)
            StmK_Free = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "StmK_Free"), GetType(StmK_FreeDelegate)), StmK_FreeDelegate)
            StmK_Start = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "StmK_Start"), GetType(StmK_StartDelegate)), StmK_StartDelegate)
            StmK_Stop = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "StmK_Stop"), GetType(StmK_StopDelegate)), StmK_StopDelegate)
            StmK_Read = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "StmK_Read"), GetType(StmK_ReadDelegate)), StmK_ReadDelegate)
            StmK_Write = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "StmK_Write"), GetType(StmK_WriteDelegate)), StmK_WriteDelegate)
            IsoK_Init = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsoK_Init"), GetType(IsoK_InitDelegate)), IsoK_InitDelegate)
            IsoK_Free = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsoK_Free"), GetType(IsoK_FreeDelegate)), IsoK_FreeDelegate)
            IsoK_SetPackets = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsoK_SetPackets"), GetType(IsoK_SetPacketsDelegate)), IsoK_SetPacketsDelegate)
            IsoK_SetPacket = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsoK_SetPacket"), GetType(IsoK_SetPacketDelegate)), IsoK_SetPacketDelegate)
            IsoK_GetPacket = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsoK_GetPacket"), GetType(IsoK_GetPacketDelegate)), IsoK_GetPacketDelegate)
            IsoK_EnumPackets = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsoK_EnumPackets"), GetType(IsoK_EnumPacketsDelegate)), IsoK_EnumPacketsDelegate)
            IsoK_ReUse = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsoK_ReUse"), GetType(IsoK_ReUseDelegate)), IsoK_ReUseDelegate)
            IsochK_Init = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsochK_Init"), GetType(IsochK_InitDelegate)), IsochK_InitDelegate)
            IsochK_Free = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsochK_Free"), GetType(IsochK_FreeDelegate)), IsochK_FreeDelegate)
            IsochK_SetPacketOffsets = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsochK_SetPacketOffsets"), GetType(IsochK_SetPacketOffsetsDelegate)), IsochK_SetPacketOffsetsDelegate)
            IsochK_SetPacket = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsochK_SetPacket"), GetType(IsochK_SetPacketDelegate)), IsochK_SetPacketDelegate)
            IsochK_GetPacket = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsochK_GetPacket"), GetType(IsochK_GetPacketDelegate)), IsochK_GetPacketDelegate)
            IsochK_EnumPackets = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsochK_EnumPackets"), GetType(IsochK_EnumPacketsDelegate)), IsochK_EnumPacketsDelegate)
            IsochK_CalcPacketInformation = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsochK_CalcPacketInformation"), GetType(IsochK_CalcPacketInformationDelegate)), IsochK_CalcPacketInformationDelegate)
            IsochK_GetNumberOfPackets = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsochK_GetNumberOfPackets"), GetType(IsochK_GetNumberOfPacketsDelegate)), IsochK_GetNumberOfPacketsDelegate)
            IsochK_SetNumberOfPackets = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "IsochK_SetNumberOfPackets"), GetType(IsochK_SetNumberOfPacketsDelegate)), IsochK_SetNumberOfPacketsDelegate)
            LUsb0_ControlTransfer = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LUsb0_ControlTransfer"), GetType(LUsb0_ControlTransferDelegate)), LUsb0_ControlTransferDelegate)
            LUsb0_SetConfiguration = CType(Marshal.GetDelegateForFunctionPointer(GetProcAddress(mModuleLibusbK, "LUsb0_SetConfiguration"), GetType(LUsb0_SetConfigurationDelegate)), LUsb0_SetConfigurationDelegate)
        End Sub

        <Flags>
        Friend Enum LoadLibraryFlags
            NONE = 0
            DONT_RESOLVE_DLL_REFERENCES = &H00000001
            LOAD_IGNORE_CODE_AUTHZ_LEVEL = &H00000010
            LOAD_LIBRARY_AS_DATAFILE = &H00000002
            LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE = &H00000040
            LOAD_LIBRARY_AS_IMAGE_RESOURCE = &H00000020
            LOAD_WITH_ALTERED_SEARCH_PATH = &H00000008
        End Enum
    End Module

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
    <Flags>
    Public Enum KISO_FLAG
        NONE = 0

        ''' <Summary>Do not start the transfer immediately, instead use \ref KISO_CONTEXT::StartFrame.</Summary>
        SET_START_FRAME = &H00000001
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

        ''' <Summary>Pipe stream handle. \ref KSTM_HANDLE</Summary>
        ISOCHK

        ''' <Summary>Max handle type count.</Summary>
        COUNT
    End Enum

    ''' <Summary>Device list sync flags.</Summary>
    <Flags>
    Public Enum KLST_SYNC_FLAG
        ''' <Summary>Cleared/invalid state.</Summary>
        NONE = 0

        ''' <Summary>Unchanged state,</Summary>
        UNCHANGED = &H0001

        ''' <Summary>Added (Arrival) state,</Summary>
        ADDED = &H0002

        ''' <Summary>Removed (Unplugged) state,</Summary>
        REMOVED = &H0004

        ''' <Summary>Connect changed state.</Summary>
        CONNECT_CHANGE = &H0008

        ''' <Summary>All states.</Summary>
        MASK = &H000F
    End Enum

    ''' <Summary>Device list initialization flags.</Summary>
    <Flags>
    Public Enum KLST_FLAG
        ''' <Summary>No flags (or 0)</Summary>
        NONE = 0

        ''' <Summary>Enable listings for the raw device interface GUID \b only. {A5DCBF10-6530-11D2-901F-00C04FB951ED}</Summary>
        INCLUDE_RAWGUID = &H0001

        ''' <Summary>List all libusbK devices including those not currently connected.</Summary>
        INCLUDE_DISCONNECT = &H0002
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
        SELF_POWERED = &H01

        ''' <Summary>Device can wake the system from a low power/sleeping state.</Summary>
        REMOTE_WAKEUP_ENABLED = &H02
    End Enum

    ''' <Summary>Standard USB descriptor types. For more information, see section 9-5 of the USB 3.0 specifications.</Summary>
    Public Enum USB_DESCRIPTOR_TYPE
        ''' <Summary>Device descriptor type.</Summary>
        DEVICE = &H01

        ''' <Summary>Configuration descriptor type.</Summary>
        CONFIGURATION = &H02

        ''' <Summary>String descriptor type.</Summary>
        [STRING] = &H03

        ''' <Summary>Interface descriptor type.</Summary>
        [INTERFACE] = &H04

        ''' <Summary>Endpoint descriptor type.</Summary>
        ENDPOINT = &H05

        ''' <Summary>Device qualifier descriptor type.</Summary>
        DEVICE_QUALIFIER = &H06

        ''' <Summary>Config power descriptor type.</Summary>
        CONFIG_POWER = &H07

        ''' <Summary>Interface power descriptor type.</Summary>
        INTERFACE_POWER = &H08

        ''' <Summary>Interface association descriptor type.</Summary>
        INTERFACE_ASSOCIATION = &H0B
        USB_SUPERSPEED_ENDPOINT_COMPANION = &H30
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

        ''' <Summary>\ref UsbK_IsochReadPipe dynamic driver function id.</Summary>
        IsochReadPipe

        ''' <Summary>\ref UsbK_IsochWritePipe dynamic driver function id.</Summary>
        IsochWritePipe

        ''' <Summary>\ref UsbK_QueryPipeEx dynamic driver function id.</Summary>
        QueryPipeEx

        ''' <Summary>\ref UsbK_GetSuperSpeedPipeCompanionDescriptor dynamic driver function id.</Summary>
        GetSuperSpeedPipeCompanionDescriptor

        ''' <Summary>Supported function count</Summary>
        COUNT
    End Enum

    ''' <Summary>Hot plug config flags.</Summary>
    <Flags>
    Public Enum KHOT_FLAG
        ''' <Summary>No flags (or 0)</Summary>
        NONE

        ''' <Summary>Notify all devices which match upon a succuessful call to \ref HotK_Init.</Summary>
        PLUG_ALL_ON_INIT = &H0001

        ''' <Summary>Allow other \ref KHOT_HANDLE instances to consume this match.</Summary>
        PASS_DUPE_INSTANCE = &H0002

        ''' <Summary>If a \c UserHwnd is specified, use \c PostMessage instead of \c SendMessage.</Summary>
        POST_USER_MESSAGE = &H0004
    End Enum

    ''' <Summary>\c WaitFlags used by \ref OvlK_Wait.</Summary>
    <Flags>
    Public Enum KOVL_WAIT_FLAG
        ''' <Summary>Do not perform any additional actions upon exiting \ref OvlK_Wait.</Summary>
        NONE = 0

        ''' <Summary>If the i/o operation completes successfully, release the OverlappedK back to it's pool.</Summary>
        RELEASE_ON_SUCCESS = &H0001

        ''' <Summary>If the i/o operation fails, release the OverlappedK back to it's pool.</Summary>
        RELEASE_ON_FAIL = &H0002

        ''' <Summary>
        '''     If the i/o operation fails or completes successfully, release the OverlappedK back to its pool. Perform no
        '''     actions if it times-out.
        ''' </Summary>
        RELEASE_ON_SUCCESS_FAIL = &H0003

        ''' <Summary>If the i/o operation times-out cancel it, but do not release the OverlappedK back to its pool.</Summary>
        CANCEL_ON_TIMEOUT = &H0004

        ''' <Summary>If the i/o operation times-out, cancel it and release the OverlappedK back to its pool.</Summary>
        RELEASE_ON_TIMEOUT = &H000C

        ''' <Summary>
        '''     Always release the OverlappedK back to its pool.  If the operation timed-out, cancel it before releasing back
        '''     to its pool.
        ''' </Summary>
        RELEASE_ALWAYS = &H000F

        ''' <Summary>
        '''     Uses alterable wait functions.  See
        '''     http://msdn.microsoft.com/en-us/library/windows/desktop/ms687036%28v=vs.85%29.aspx
        ''' </Summary>
        ALERTABLE = &H0010
    End Enum

    ''' <Summary>\c Overlapped pool config flags.</Summary>
    <Flags>
    Public Enum KOVL_POOL_FLAG
        NONE = 0
    End Enum

    ''' <Summary>Stream config flags.</Summary>
    <Flags>
    Public Enum KSTM_FLAG As UInteger
        ''' <Summary>None</Summary>
        NONE = 0
        NO_PARTIAL_XFERS = &H00100000
        USE_TIMEOUT = &H80000000UI
        TIMEOUT_MASK = &H0001FFFF
    End Enum

    ''' <Summary>Stream config flags.</Summary>
    Public Enum KSTM_COMPLETE_RESULT
        ''' <Summary>Valid</Summary>
        VALID = 0

        ''' <Summary>Invalid</Summary>
        INVALID
    End Enum

#End Region

#Region "Structs"

    ''' <Summary>
    '''     The \c WINUSB_PIPE_INFORMATION structure contains pipe information that the \ref UsbK_QueryPipe routine
    '''     retrieves.
    ''' </Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi)>
    Public Structure WINUSB_PIPE_INFORMATION
        ''' <Summary>A \c USBD_PIPE_TYPE enumeration value that specifies the pipe type</Summary>
        Public PipeType As USBD_PIPE_TYPE

        ''' <Summary>The pipe identifier (ID)</Summary>
        Public PipeId As Byte

        ''' <Summary>The maximum size, in bytes, of the packets that are transmitted on the pipe</Summary>
        Public MaximumPacketSize As UShort

        ''' <Summary>The pipe interval</Summary>
        Public Interval As Byte

        Public Overrides Function ToString() As String
            Return String.Format("PipeType: {0}" & Microsoft.VisualBasic.Constants.vbLf & "PipeId: {1}" & Microsoft.VisualBasic.Constants.vbLf & "MaximumPacketSize: {2}" & Microsoft.VisualBasic.Constants.vbLf & "Interval: {3}" & Microsoft.VisualBasic.Constants.vbLf, PipeType, PipeId.ToString("X2") & "h", MaximumPacketSize, Interval.ToString("X2") & "h")
        End Function
    End Structure

    ''' <Summary>
    '''     The \c WINUSB_PIPE_INFORMATION_EX structure contains pipe information that the \ref UsbK_QueryPipeEx routine
    '''     retrieves.
    ''' </Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi)>
    Public Structure WINUSB_PIPE_INFORMATION_EX
        ''' <Summary>A \c USBD_PIPE_TYPE enumeration value that specifies the pipe type</Summary>
        Public PipeType As USBD_PIPE_TYPE

        ''' <Summary>The pipe identifier (ID)</Summary>
        Public PipeId As Byte

        ''' <Summary>The maximum size, in bytes, of the packets that are transmitted on the pipe</Summary>
        Public MaximumPacketSize As UShort

        ''' <Summary>The pipe interval</Summary>
        Public Interval As Byte

        ''' <Summary>The maximum number of bytes that can be transmitted in single interval.</Summary>
        Public MaximumBytesPerInterval As UInteger

        Public Overrides Function ToString() As String
            Return String.Format("PipeType: {0}" & Microsoft.VisualBasic.Constants.vbLf & "PipeId: {1}" & Microsoft.VisualBasic.Constants.vbLf & "MaximumPacketSize: {2}" & Microsoft.VisualBasic.Constants.vbLf & "Interval: {3}" & Microsoft.VisualBasic.Constants.vbLf & "MaximumBytesPerInterval: {4}" & Microsoft.VisualBasic.Constants.vbLf, PipeType, PipeId.ToString("X2") & "h", MaximumPacketSize, Interval.ToString("X2") & "h", MaximumBytesPerInterval)
        End Function
    End Structure

    ''' <Summary>The \c WINUSB_SETUP_PACKET structure describes a USB setup packet.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Pack:=1)>
    Public Structure WINUSB_SETUP_PACKET
        ''' <Summary>
        '''     The request type. The values that are assigned to this member are defined in Table 9.2 of section 9.3 of the
        '''     Universal Serial Bus (USB) specification (www.usb.org).
        ''' </Summary>
        Public RequestType As Byte

        ''' <Summary>
        '''     The device request. The values that are assigned to this member are defined in Table 9.3 of section 9.4 of the
        '''     Universal Serial Bus (USB) specification.
        ''' </Summary>
        Public Request As Byte

        ''' <Summary>
        '''     The meaning of this member varies according to the request. For an explanation of this member, see the
        '''     Universal Serial Bus (USB) specification.
        ''' </Summary>
        Public Value As UShort

        ''' <Summary>
        '''     The meaning of this member varies according to the request. For an explanation of this member, see the
        '''     Universal Serial Bus (USB) specification.
        ''' </Summary>
        Public Index As UShort

        ''' <Summary>The number of bytes to transfer. (not including the \c WINUSB_SETUP_PACKET itself)</Summary>
        Public Length As UShort

        Public Overrides Function ToString() As String
            Return String.Format("RequestType: {0}" & Microsoft.VisualBasic.Constants.vbLf & "Request: {1}" & Microsoft.VisualBasic.Constants.vbLf & "Value: {2}" & Microsoft.VisualBasic.Constants.vbLf & "Index: {3}" & Microsoft.VisualBasic.Constants.vbLf & "Length: {4}" & Microsoft.VisualBasic.Constants.vbLf, RequestType.ToString("X2") & "h", Request.ToString("X2") & "h", Value.ToString("X4") & "h", Index.ToString("X4") & "h", Length)
        End Function
    End Structure

    ''' <Summary>Structure describing an isochronous transfer packet for libusbK.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Pack:=1)>
    Public Structure KISO_PACKET
        ''' <Summary>
        '''     Specifies the offset, in bytes, of the buffer for this packet from the beginning of the entire isochronous
        '''     transfer data buffer.
        ''' </Summary>
        Public Offset As UInteger

        ''' <Summary>
        '''     Set by the host controller to indicate the actual number of bytes received by the device for isochronous IN
        '''     transfers. Length not used for isochronous OUT transfers.
        ''' </Summary>
        Public Length As UShort

        ''' <Summary>
        '''     Contains the 16 least significant USBD status bits, on return from the host controller driver, of this
        '''     transfer packet.
        ''' </Summary>
        Public Status As UShort

        Public Overrides Function ToString() As String
            Return String.Format("Offset: {0}" & Microsoft.VisualBasic.Constants.vbLf & "Length: {1}" & Microsoft.VisualBasic.Constants.vbLf & "Status: {2}" & Microsoft.VisualBasic.Constants.vbLf, Offset, Length, Status.ToString("X4") & "h")
        End Function
    End Structure

    ''' <Summary>Structure describing an isochronous transfer packet for winusb.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Pack:=1)>
    Public Structure KISO_WUSB_PACKET
        ''' <Summary>
        '''     Specifies the offset, in bytes, of the buffer for this packet from the beginning of the entire isochronous
        '''     transfer data buffer.
        ''' </Summary>
        Public Offset As UInteger

        ''' <Summary>
        '''     Set by the host controller to indicate the actual number of bytes received by the device for isochronous IN
        '''     transfers. Length not used for isochronous OUT transfers.
        ''' </Summary>
        Public Length As UInteger

        ''' <Summary>
        '''     Contains the 16 least significant USBD status bits, on return from the host controller driver, of this
        '''     transfer packet.
        ''' </Summary>
        Public Status As UInteger

        Public Overrides Function ToString() As String
            Return String.Format("Offset: {0}" & Microsoft.VisualBasic.Constants.vbLf & "Length: {1}" & Microsoft.VisualBasic.Constants.vbLf & "Status: {2}" & Microsoft.VisualBasic.Constants.vbLf, Offset, Length, Status)
        End Function
    End Structure

    <StructLayout(LayoutKind.Sequential)>
    Public Structure KISO_CONTEXT
        Private ReadOnly mHandlePtr As IntPtr

        Public Sub New(ByVal Handle As IntPtr)
            mHandlePtr = Handle
        End Sub

        Public ReadOnly Property Pointer As IntPtr
            Get
                Return mHandlePtr
            End Get
        End Property

        ''' <summary>KISO_CONTEXT_MAP is used for calculating field offsets only</summary>
        <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Pack:=1)>
        Private Structure KISO_CONTEXT_MAP
            ''' <Summary>Additional ISO transfer flags. See \ref KISO_FLAG.</Summary>
            Private ReadOnly Flags As KISO_FLAG

            ''' <Summary>Specifies the frame number that the transfer should begin on (0 for ASAP).</Summary>
            Private ReadOnly StartFrame As UInteger

            ''' <Summary>
            '''     Contains the number of packets that completed with an error condition on return from the host controller
            '''     driver.
            ''' </Summary>
            Private ReadOnly ErrorCount As Short

            ''' <Summary>Specifies the number of packets that are described by the variable-length array member \c IsoPacket.</Summary>
            Private ReadOnly NumberOfPackets As Short

            ''' <Summary>Contains the URB Hdr.Status value on return from the host controller driver.</Summary>
            Private ReadOnly UrbHdrStatus As UInteger
        End Structure

        Private Shared ReadOnly ofsFlags As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "Flags").ToInt32()
        Private Shared ReadOnly ofsStartFrame As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "StartFrame").ToInt32()
        Private Shared ReadOnly ofsErrorCount As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "ErrorCount").ToInt32()
        Private Shared ReadOnly ofsNumberOfPackets As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "NumberOfPackets").ToInt32()
        Private Shared ReadOnly ofsUrbHdrStatus As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "UrbHdrStatus").ToInt32()


        ''' <Summary>Additional ISO transfer flags. See \ref KISO_FLAG.</Summary>
        Public ReadOnly Property Flags As KISO_FLAG
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsFlags)
            End Get
        End Property


        ''' <Summary>Specifies the frame number that the transfer should begin on (0 for ASAP).</Summary>

        Public ReadOnly Property StartFrame As UInteger
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsStartFrame)
            End Get
        End Property


        ''' <Summary>
        '''     Contains the number of packets that completed with an error condition on return from the host controller
        '''     driver.
        ''' </Summary>

        Public ReadOnly Property ErrorCount As Short
            Get
                Return Marshal.ReadInt16(mHandlePtr, ofsErrorCount)
            End Get
        End Property


        ''' <Summary>Specifies the number of packets that are described by the variable-length array member \c IsoPacket.</Summary>

        Public ReadOnly Property NumberOfPackets As Short
            Get
                Return Marshal.ReadInt16(mHandlePtr, ofsNumberOfPackets)
            End Get
        End Property


        ''' <Summary>Contains the URB Hdr.Status value on return from the host controller driver.</Summary>

        Public ReadOnly Property UrbHdrStatus As UInteger
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsUrbHdrStatus)
            End Get
        End Property

        Public Overrides Function ToString() As String
            Return String.Format("Flags: {0}" & Microsoft.VisualBasic.Constants.vbLf & "StartFrame: {1}" & Microsoft.VisualBasic.Constants.vbLf & "ErrorCount: {2}" & Microsoft.VisualBasic.Constants.vbLf & "NumberOfPackets: {3}" & Microsoft.VisualBasic.Constants.vbLf & "UrbHdrStatus: {4}" & Microsoft.VisualBasic.Constants.vbLf, Flags.ToString(), StartFrame, ErrorCount, NumberOfPackets, UrbHdrStatus.ToString("X8") & "h")
        End Function
    End Structure

    ''' <Summary>Structure describing additional information about how an isochronous pipe transfers data.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Pack:=1)>
    Public Structure KISOCH_PACKET_INFORMATION
        ''' <Summary>Number of ISO packets transferred per whole USB frame (1 millisecond).</Summary>
        Public PacketsPerFrame As UInteger

        ''' <Summary>How often a pipe transfers data.</Summary>
        Public PollingPeriodMicroseconds As UInteger


        ''' <Summary>Number of bytes transferred per millisecond (or whole frame).</Summary>
        Public BytesPerMillisecond As UInteger

        Public Overrides Function ToString() As String
            Return String.Format("PacketsPerFrame: {0}" & Microsoft.VisualBasic.Constants.vbLf & "PollingPeriodMicroseconds: {1}" & Microsoft.VisualBasic.Constants.vbLf & "BytesPerMillisecond: {2}" & Microsoft.VisualBasic.Constants.vbLf, PacketsPerFrame, PollingPeriodMicroseconds, BytesPerMillisecond)
        End Function
    End Structure

    ''' <Summary>libusbK verson information structure.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi)>
    Public Structure KLIB_VERSION
        ''' <Summary>Major version number.</Summary>
        Public Major As Integer

        ''' <Summary>Minor version number.</Summary>
        Public Minor As Integer

        ''' <Summary>Micro version number.</Summary>
        Public Micro As Integer

        ''' <Summary>Nano version number.</Summary>
        Public Nano As Integer

        Public Overrides Function ToString() As String
            Return String.Format("Major: {0}" & Microsoft.VisualBasic.Constants.vbLf & "Minor: {1}" & Microsoft.VisualBasic.Constants.vbLf & "Micro: {2}" & Microsoft.VisualBasic.Constants.vbLf & "Nano: {3}" & Microsoft.VisualBasic.Constants.vbLf, Major, Minor, Micro, Nano)
        End Function
    End Structure

    ''' <Summary>Common usb device information structure</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi)>
    Public Structure KLST_DEV_COMMON_INFO
        ''' <Summary>VendorID parsed from \ref KLST_DEVINFO::DeviceID</Summary>
        Public Vid As Integer

        ''' <Summary>ProductID parsed from \ref KLST_DEVINFO::DeviceID</Summary>
        Public Pid As Integer

        ''' <Summary>
        '''     Composite interface number parsed from \ref KLST_DEVINFO::DeviceID.  Set to \b -1 for devices that do not have
        '''     the composite parent driver.
        ''' </Summary>
        Public MI As Integer

        ' An ID that uniquely identifies a USB device.
        <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
        Public InstanceID As String

        Public Overrides Function ToString() As String
            Return String.Format("Vid: {0}" & Microsoft.VisualBasic.Constants.vbLf & "Pid: {1}" & Microsoft.VisualBasic.Constants.vbLf & "MI: {2}" & Microsoft.VisualBasic.Constants.vbLf & "InstanceID: {3}" & Microsoft.VisualBasic.Constants.vbLf, Vid.ToString("X4") & "h", Pid.ToString("X4") & "h", MI.ToString("X2") & "h", InstanceID)
        End Function
    End Structure

    <StructLayout(LayoutKind.Sequential)>
    Public Structure KLST_DEVINFO_HANDLE
        Implements IKLIB_HANDLE

        Private ReadOnly mHandlePtr As IntPtr

        Public Sub New(ByVal Handle As IntPtr)
            mHandlePtr = Handle
        End Sub

        Public ReadOnly Property Pointer As IntPtr Implements IKLIB_HANDLE.Pointer
            Get
                Return mHandlePtr
            End Get
        End Property

        Public ReadOnly Property HandleType As KLIB_HANDLE_TYPE Implements IKLIB_HANDLE.HandleType
            Get
                Return KLIB_HANDLE_TYPE.LSTINFOK
            End Get
        End Property

        Public Function GetContext() As IntPtr Implements IKLIB_HANDLE.GetContext
            Return LibK_GetContext(mHandlePtr, HandleType)
        End Function

        Public Function SetContext(ByVal UserContext As IntPtr) As Boolean Implements IKLIB_HANDLE.SetContext
            Return LibK_SetContext(mHandlePtr, HandleType, UserContext)
        End Function

        Public Function SetCleanupCallback(ByVal CleanupCallback As KLIB_HANDLE_CLEANUP_CB) As Boolean Implements IKLIB_HANDLE.SetCleanupCallback
            Return LibK_SetCleanupCallback(mHandlePtr, HandleType, CleanupCallback)
        End Function

        ''' <summary>KLST_DEVINFO_MAP is used for calculating field offsets only</summary>
        <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi)>
        Private Structure KLST_DEVINFO_MAP
            ''' <Summary>Common usb device information</Summary>
            Private ReadOnly Common As KLST_DEV_COMMON_INFO

            ''' <Summary>Driver id this device element is using</Summary>
            Private ReadOnly DriverID As Integer

            ''' <Summary>Device interface GUID</Summary>
            <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
            Private ReadOnly DeviceInterfaceGUID As String

            ''' <Summary>Device instance ID.</Summary>
            <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
            Private ReadOnly DeviceID As String

            ''' <Summary>Class GUID.</Summary>
            <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
            Private ReadOnly ClassGUID As String

            ''' <Summary>Manufacturer name as specified in the INF file.</Summary>
            <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
            Private ReadOnly Mfg As String

            ''' <Summary>Device description as specified in the INF file.</Summary>
            <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
            Private ReadOnly DeviceDesc As String

            ''' <Summary>Driver service name.</Summary>
            <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
            Private ReadOnly Service As String

            ''' <Summary>Unique identifier.</Summary>
            <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
            Private ReadOnly SymbolicLink As String

            ''' <Summary>physical device filename used with the Windows \c CreateFile()</Summary>
            <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
            Private ReadOnly DevicePath As String

            ''' <Summary>libusb-win32 filter index id.</Summary>
            Private ReadOnly LUsb0FilterIndex As Integer

            ''' <Summary>Indicates the devices connection state.</Summary>
            Private ReadOnly Connected As Boolean

            ''' <Summary>Synchronization flags. (internal use only)</Summary>
            Private ReadOnly SyncFlags As KLST_SYNC_FLAG
            Private ReadOnly BusNumber As Integer
            Private ReadOnly DeviceAddress As Integer

            ''' <Summary>
            '''     If the the device is serialized, represents the string value of \ref USB_DEVICE_DESCRIPTOR::iSerialNumber. For
            '''     Devices without a \b iSerialNumber, represents the unique \b InstanceID assigned by \b Windows.
            ''' </Summary>
            <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
            Private ReadOnly SerialNumber As String
        End Structure

        Private Shared ReadOnly ofsCommon As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "Common").ToInt32()
        Private Shared ReadOnly ofsDriverID As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "DriverID").ToInt32()
        Private Shared ReadOnly ofsDeviceInterfaceGUID As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "DeviceInterfaceGUID").ToInt32()
        Private Shared ReadOnly ofsDeviceID As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "DeviceID").ToInt32()
        Private Shared ReadOnly ofsClassGUID As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "ClassGUID").ToInt32()
        Private Shared ReadOnly ofsMfg As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "Mfg").ToInt32()
        Private Shared ReadOnly ofsDeviceDesc As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "DeviceDesc").ToInt32()
        Private Shared ReadOnly ofsService As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "Service").ToInt32()
        Private Shared ReadOnly ofsSymbolicLink As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "SymbolicLink").ToInt32()
        Private Shared ReadOnly ofsDevicePath As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "DevicePath").ToInt32()
        Private Shared ReadOnly ofsLUsb0FilterIndex As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "LUsb0FilterIndex").ToInt32()
        Private Shared ReadOnly ofsConnected As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "Connected").ToInt32()
        Private Shared ReadOnly ofsSyncFlags As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "SyncFlags").ToInt32()
        Private Shared ReadOnly ofsBusNumber As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "BusNumber").ToInt32()
        Private Shared ReadOnly ofsDeviceAddress As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "DeviceAddress").ToInt32()
        Private Shared ReadOnly ofsSerialNumber As Integer = Marshal.OffsetOf(GetType(KLST_DEVINFO_MAP), "SerialNumber").ToInt32()


        ''' <Summary>Common usb device information</Summary>
        Public ReadOnly Property Common As KLST_DEV_COMMON_INFO
            Get
                Return CType(Marshal.PtrToStructure(New IntPtr(mHandlePtr.ToInt64() + ofsCommon), GetType(KLST_DEV_COMMON_INFO)), KLST_DEV_COMMON_INFO)
            End Get
        End Property


        ''' <Summary>Driver id this device element is using</Summary>
        Public ReadOnly Property DriverID As Integer
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsDriverID)
            End Get
        End Property


        ''' <Summary>Device interface GUID</Summary>
        Public ReadOnly Property DeviceInterfaceGUID As String
            Get
                Return Marshal.PtrToStringAnsi(New IntPtr(mHandlePtr.ToInt64() + ofsDeviceInterfaceGUID))
            End Get
        End Property


        ''' <Summary>Device instance ID.</Summary>

        Public ReadOnly Property DeviceID As String
            Get
                Return Marshal.PtrToStringAnsi(New IntPtr(mHandlePtr.ToInt64() + ofsDeviceID))
            End Get
        End Property


        ''' <Summary>Class GUID.</Summary>
        Public ReadOnly Property ClassGUID As String
            Get
                Return Marshal.PtrToStringAnsi(New IntPtr(mHandlePtr.ToInt64() + ofsClassGUID))
            End Get
        End Property


        ''' <Summary>Manufacturer name as specified in the INF file.</Summary>
        Public ReadOnly Property Mfg As String
            Get
                Return Marshal.PtrToStringAnsi(New IntPtr(mHandlePtr.ToInt64() + ofsMfg))
            End Get
        End Property


        ''' <Summary>Device description as specified in the INF file.</Summary>
        Public ReadOnly Property DeviceDesc As String
            Get
                Return Marshal.PtrToStringAnsi(New IntPtr(mHandlePtr.ToInt64() + ofsDeviceDesc))
            End Get
        End Property


        ''' <Summary>Driver service name.</Summary>
        Public ReadOnly Property Service As String
            Get
                Return Marshal.PtrToStringAnsi(New IntPtr(mHandlePtr.ToInt64() + ofsService))
            End Get
        End Property


        ''' <Summary>Unique identifier.</Summary>
        Public ReadOnly Property SymbolicLink As String
            Get
                Return Marshal.PtrToStringAnsi(New IntPtr(mHandlePtr.ToInt64() + ofsSymbolicLink))
            End Get
        End Property


        ''' <Summary>physical device filename used with the Windows \c CreateFile()</Summary>
        Public ReadOnly Property DevicePath As String
            Get
                Return Marshal.PtrToStringAnsi(New IntPtr(mHandlePtr.ToInt64() + ofsDevicePath))
            End Get
        End Property


        ''' <Summary>libusb-win32 filter index id.</Summary>
        Public ReadOnly Property LUsb0FilterIndex As Integer
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsLUsb0FilterIndex)
            End Get
        End Property


        ''' <Summary>Indicates the devices connection state.</Summary>
        Public ReadOnly Property Connected As Boolean
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsConnected) <> 0
            End Get
        End Property


        ''' <Summary>Synchronization flags. (internal use only)</Summary>
        Public ReadOnly Property SyncFlags As KLST_SYNC_FLAG
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsSyncFlags)
            End Get
        End Property

        Public ReadOnly Property BusNumber As Integer
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsBusNumber)
            End Get
        End Property

        Public ReadOnly Property DeviceAddress As Integer
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsDeviceAddress)
            End Get
        End Property


        ''' <Summary>
        '''     If the the device is serialized, represents the string value of \ref USB_DEVICE_DESCRIPTOR::iSerialNumber. For
        '''     Devices without a \b iSerialNumber, represents the unique \b InstanceID assigned by \b Windows.
        ''' </Summary>
        Public ReadOnly Property SerialNumber As String
            Get
                Return Marshal.PtrToStringAnsi(New IntPtr(mHandlePtr.ToInt64() + ofsSerialNumber))
            End Get
        End Property

        Public Overrides Function ToString() As String
            Return String.Format("DriverID: {0}" & Microsoft.VisualBasic.Constants.vbLf & "DeviceInterfaceGUID: {1}" & Microsoft.VisualBasic.Constants.vbLf & "DeviceID: {2}" & Microsoft.VisualBasic.Constants.vbLf & "ClassGUID: {3}" & Microsoft.VisualBasic.Constants.vbLf & "Mfg: {4}" & Microsoft.VisualBasic.Constants.vbLf & "DeviceDesc: {5}" & Microsoft.VisualBasic.Constants.vbLf & "Service: {6}" & Microsoft.VisualBasic.Constants.vbLf & "SymbolicLink: {7}" & Microsoft.VisualBasic.Constants.vbLf & "DevicePath: {8}" & Microsoft.VisualBasic.Constants.vbLf & "LUsb0FilterIndex: {9}" & Microsoft.VisualBasic.Constants.vbLf & "Connected: {10}" & Microsoft.VisualBasic.Constants.vbLf & "SyncFlags: {11}" & Microsoft.VisualBasic.Constants.vbLf & "BusNumber: {12}" & Microsoft.VisualBasic.Constants.vbLf & "DeviceAddress: {13}" & Microsoft.VisualBasic.Constants.vbLf & "SerialNumber: {14}" & Microsoft.VisualBasic.Constants.vbLf, DriverID, DeviceInterfaceGUID, DeviceID, ClassGUID, Mfg, DeviceDesc, Service, SymbolicLink, DevicePath, LUsb0FilterIndex, Connected, SyncFlags.ToString(), BusNumber, DeviceAddress, SerialNumber)
        End Function
    End Structure

    ''' <Summary>Device list/hot-plug pattern match structure.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Size:=1024)>
    Public Structure KLST_PATTERN_MATCH
        ''' <Summary>Pattern match a device instance id.</Summary>
        <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
        Public DeviceID As String

        ''' <Summary>Pattern match a device interface guid.</Summary>
        <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
        Public DeviceInterfaceGUID As String

        ''' <Summary>Pattern match a symbolic link.</Summary>
        <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
        Public ClassGUID As String

        Public Overrides Function ToString() As String
            Return String.Format("DeviceID: {0}" & Microsoft.VisualBasic.Constants.vbLf & "DeviceInterfaceGUID: {1}" & Microsoft.VisualBasic.Constants.vbLf & "ClassGUID: {2}" & Microsoft.VisualBasic.Constants.vbLf, DeviceID, DeviceInterfaceGUID, ClassGUID)
        End Function
    End Structure

    ''' <Summary>A structure representing the standard USB device descriptor.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Pack:=1)>
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

        Public Overrides Function ToString() As String
            Return String.Format("bLength: {0}" & Microsoft.VisualBasic.Constants.vbLf & "bDescriptorType: {1}" & Microsoft.VisualBasic.Constants.vbLf & "bcdUSB: {2}" & Microsoft.VisualBasic.Constants.vbLf & "bDeviceClass: {3}" & Microsoft.VisualBasic.Constants.vbLf & "bDeviceSubClass: {4}" & Microsoft.VisualBasic.Constants.vbLf & "bDeviceProtocol: {5}" & Microsoft.VisualBasic.Constants.vbLf & "bMaxPacketSize0: {6}" & Microsoft.VisualBasic.Constants.vbLf & "idVendor: {7}" & Microsoft.VisualBasic.Constants.vbLf & "idProduct: {8}" & Microsoft.VisualBasic.Constants.vbLf & "bcdDevice: {9}" & Microsoft.VisualBasic.Constants.vbLf & "iManufacturer: {10}" & Microsoft.VisualBasic.Constants.vbLf & "iProduct: {11}" & Microsoft.VisualBasic.Constants.vbLf & "iSerialNumber: {12}" & Microsoft.VisualBasic.Constants.vbLf & "bNumConfigurations: {13}" & Microsoft.VisualBasic.Constants.vbLf, bLength, bDescriptorType.ToString("X2") & "h", bcdUSB.ToString("X4") & "h", bDeviceClass.ToString("X2") & "h", bDeviceSubClass.ToString("X2") & "h", bDeviceProtocol.ToString("X2") & "h", bMaxPacketSize0, idVendor.ToString("X4") & "h", idProduct.ToString("X4") & "h", bcdDevice.ToString("X4") & "h", iManufacturer, iProduct, iSerialNumber, bNumConfigurations)
        End Function
    End Structure

    ''' <Summary>A structure representing the standard USB endpoint descriptor.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Pack:=1)>
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

        Public Overrides Function ToString() As String
            Return String.Format("bLength: {0}" & Microsoft.VisualBasic.Constants.vbLf & "bDescriptorType: {1}" & Microsoft.VisualBasic.Constants.vbLf & "bEndpointAddress: {2}" & Microsoft.VisualBasic.Constants.vbLf & "bmAttributes: {3}" & Microsoft.VisualBasic.Constants.vbLf & "wMaxPacketSize: {4}" & Microsoft.VisualBasic.Constants.vbLf & "bInterval: {5}" & Microsoft.VisualBasic.Constants.vbLf, bLength, bDescriptorType.ToString("X2") & "h", bEndpointAddress.ToString("X2") & "h", bmAttributes.ToString("X2") & "h", wMaxPacketSize, bInterval)
        End Function
    End Structure

    ''' <Summary>A structure representing the standard USB configuration descriptor.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Pack:=1)>
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

        ''' <Summary>
        '''     Maximum power consumption of the USB device from this bus in this configuration when the device is fully
        '''     operation.
        ''' </Summary>
        Public MaxPower As Byte

        Public Overrides Function ToString() As String
            Return String.Format("bLength: {0}" & Microsoft.VisualBasic.Constants.vbLf & "bDescriptorType: {1}" & Microsoft.VisualBasic.Constants.vbLf & "wTotalLength: {2}" & Microsoft.VisualBasic.Constants.vbLf & "bNumInterfaces: {3}" & Microsoft.VisualBasic.Constants.vbLf & "bConfigurationValue: {4}" & Microsoft.VisualBasic.Constants.vbLf & "iConfiguration: {5}" & Microsoft.VisualBasic.Constants.vbLf & "bmAttributes: {6}" & Microsoft.VisualBasic.Constants.vbLf & "MaxPower: {7}" & Microsoft.VisualBasic.Constants.vbLf, bLength, bDescriptorType.ToString("X2") & "h", wTotalLength, bNumInterfaces, bConfigurationValue, iConfiguration, bmAttributes.ToString("X2") & "h", MaxPower)
        End Function
    End Structure

    ''' <Summary>A structure representing the standard USB interface descriptor.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Pack:=1)>
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

        Public Overrides Function ToString() As String
            Return String.Format("bLength: {0}" & Microsoft.VisualBasic.Constants.vbLf & "bDescriptorType: {1}" & Microsoft.VisualBasic.Constants.vbLf & "bInterfaceNumber: {2}" & Microsoft.VisualBasic.Constants.vbLf & "bAlternateSetting: {3}" & Microsoft.VisualBasic.Constants.vbLf & "bNumEndpoints: {4}" & Microsoft.VisualBasic.Constants.vbLf & "bInterfaceClass: {5}" & Microsoft.VisualBasic.Constants.vbLf & "bInterfaceSubClass: {6}" & Microsoft.VisualBasic.Constants.vbLf & "bInterfaceProtocol: {7}" & Microsoft.VisualBasic.Constants.vbLf & "iInterface: {8}" & Microsoft.VisualBasic.Constants.vbLf, bLength, bDescriptorType.ToString("X2") & "h", bInterfaceNumber, bAlternateSetting, bNumEndpoints, bInterfaceClass.ToString("X2") & "h", bInterfaceSubClass.ToString("X2") & "h", bInterfaceProtocol.ToString("X2") & "h", iInterface)
        End Function
    End Structure

    ''' <Summary>A structure representing the standard USB string descriptor.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Unicode, Pack:=1)>
    Public Structure USB_STRING_DESCRIPTOR
        ''' <Summary>Size of this descriptor (in bytes)</Summary>
        Public bLength As Byte

        ''' <Summary>Descriptor type</Summary>
        Public bDescriptorType As Byte

        ''' <Summary>Content of the string</Summary>
        <MarshalAs(UnmanagedType.ByValTStr, SizeConst:=KLST_STRING_MAX_LEN)>
        Public bString As String

        Public Overrides Function ToString() As String
            Return String.Format("bLength: {0}" & Microsoft.VisualBasic.Constants.vbLf & "bDescriptorType: {1}" & Microsoft.VisualBasic.Constants.vbLf & "bString: {2}" & Microsoft.VisualBasic.Constants.vbLf, bLength, bDescriptorType.ToString("X2") & "h", bString)
        End Function
    End Structure

    ''' <Summary>A structure representing the common USB descriptor.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Pack:=1)>
    Public Structure USB_COMMON_DESCRIPTOR
        ''' <Summary>Size of this descriptor (in bytes)</Summary>
        Public bLength As Byte

        ''' <Summary>Descriptor type</Summary>
        Public bDescriptorType As Byte

        Public Overrides Function ToString() As String
            Return String.Format("bLength: {0}" & Microsoft.VisualBasic.Constants.vbLf & "bDescriptorType: {1}" & Microsoft.VisualBasic.Constants.vbLf, bLength, bDescriptorType.ToString("X2") & "h")
        End Function
    End Structure

    ''' <Summary>Allows hardware manufacturers to define groupings of interfaces.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Pack:=1)>
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

        Public Overrides Function ToString() As String
            Return String.Format("bLength: {0}" & Microsoft.VisualBasic.Constants.vbLf & "bDescriptorType: {1}" & Microsoft.VisualBasic.Constants.vbLf & "bFirstInterface: {2}" & Microsoft.VisualBasic.Constants.vbLf & "bInterfaceCount: {3}" & Microsoft.VisualBasic.Constants.vbLf & "bFunctionClass: {4}" & Microsoft.VisualBasic.Constants.vbLf & "bFunctionSubClass: {5}" & Microsoft.VisualBasic.Constants.vbLf & "bFunctionProtocol: {6}" & Microsoft.VisualBasic.Constants.vbLf & "iFunction: {7}" & Microsoft.VisualBasic.Constants.vbLf, bLength, bDescriptorType.ToString("X2") & "h", bFirstInterface, bInterfaceCount, bFunctionClass.ToString("X2") & "h", bFunctionSubClass.ToString("X2") & "h", bFunctionProtocol.ToString("X2") & "h", iFunction)
        End Function
    End Structure

    ''' <Summary>USB core driver API information structure.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi)>
    Public Structure KUSB_DRIVER_API_INFO
        ''' <Summary>\readonly Driver id of the driver api.</Summary>
        Public DriverID As Integer

        ''' <Summary>\readonly Number of valid functions contained in the driver API.</Summary>
        Public FunctionCount As Integer

        Public Overrides Function ToString() As String
            Return String.Format("DriverID: {0}" & Microsoft.VisualBasic.Constants.vbLf & "FunctionCount: {1}" & Microsoft.VisualBasic.Constants.vbLf, DriverID, FunctionCount)
        End Function
    End Structure

    ''' <Summary>Driver API function set structure.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Size:=512)>
    Public Structure KUSB_DRIVER_API
        ''' <Summary>Driver API information.</Summary>
        Public Info As KUSB_DRIVER_API_INFO
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public Init As KUSB_InitDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public Free As KUSB_FreeDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public ClaimInterface As KUSB_ClaimInterfaceDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public ReleaseInterface As KUSB_ReleaseInterfaceDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public SetAltInterface As KUSB_SetAltInterfaceDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public GetAltInterface As KUSB_GetAltInterfaceDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public GetDescriptor As KUSB_GetDescriptorDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public ControlTransfer As KUSB_ControlTransferDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public SetPowerPolicy As KUSB_SetPowerPolicyDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public GetPowerPolicy As KUSB_GetPowerPolicyDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public SetConfiguration As KUSB_SetConfigurationDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public GetConfiguration As KUSB_GetConfigurationDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public ResetDevice As KUSB_ResetDeviceDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public Initialize As KUSB_InitializeDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public SelectInterface As KUSB_SelectInterfaceDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public GetAssociatedInterface As KUSB_GetAssociatedInterfaceDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public Clone As KUSB_CloneDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public QueryInterfaceSettings As KUSB_QueryInterfaceSettingsDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public QueryDeviceInformation As KUSB_QueryDeviceInformationDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public SetCurrentAlternateSetting As KUSB_SetCurrentAlternateSettingDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public GetCurrentAlternateSetting As KUSB_GetCurrentAlternateSettingDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public QueryPipe As KUSB_QueryPipeDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public SetPipePolicy As KUSB_SetPipePolicyDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public GetPipePolicy As KUSB_GetPipePolicyDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public ReadPipe As KUSB_ReadPipeDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public WritePipe As KUSB_WritePipeDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public ResetPipe As KUSB_ResetPipeDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public AbortPipe As KUSB_AbortPipeDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public FlushPipe As KUSB_FlushPipeDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public IsoReadPipe As KUSB_IsoReadPipeDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public IsoWritePipe As KUSB_IsoWritePipeDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public GetCurrentFrameNumber As KUSB_GetCurrentFrameNumberDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public GetOverlappedResult As KUSB_GetOverlappedResultDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public GetProperty As KUSB_GetPropertyDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public IsochReadPipe As KUSB_IsochReadPipeDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public IsochWritePipe As KUSB_IsochWritePipeDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public QueryPipeEx As KUSB_QueryPipeExDelegate
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public GetSuperSpeedPipeCompanionDescriptor As KUSB_GetSuperSpeedPipeCompanionDescriptorDelegate
    End Structure

    ''' <Summary>Hot plug parameter structure.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Size:=2048)>
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
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public OnHotPlug As KHOT_PLUG_CB

        ''' <Summary>\b WM_POWERBROADCAST event callback function invoked when a power-management event has occurred.</Summary>
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public OnPowerBroadcast As KHOT_POWER_BROADCAST_CB

        Public Overrides Function ToString() As String
            Return String.Format("UserHwnd: {0}" & Microsoft.VisualBasic.Constants.vbLf & "UserMessage: {1}" & Microsoft.VisualBasic.Constants.vbLf & "Flags: {2}" & Microsoft.VisualBasic.Constants.vbLf, UserHwnd.ToString("X16") & "h", UserMessage.ToString("X8") & "h", Flags.ToString())
        End Function
    End Structure

    <StructLayout(LayoutKind.Sequential)>
    Public Structure KSTM_XFER_CONTEXT
        Private ReadOnly mHandlePtr As IntPtr

        Public Sub New(ByVal Handle As IntPtr)
            mHandlePtr = Handle
        End Sub

        Public ReadOnly Property Pointer As IntPtr
            Get
                Return mHandlePtr
            End Get
        End Property

        ''' <summary>KSTM_XFER_CONTEXT_MAP is used for calculating field offsets only</summary>
        <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi)>
        Private Structure KSTM_XFER_CONTEXT_MAP
            ''' <Summary>Internal stream buffer.</Summary>
            Private ReadOnly Buffer As IntPtr

            ''' <Summary>Size of internal stream buffer.</Summary>
            Private ReadOnly BufferSize As Integer

            ''' <Summary>Number of bytes to write or number of bytes read.</Summary>
            Private ReadOnly TransferLength As Integer

            ''' <Summary>User defined state.</Summary>
            Private ReadOnly UserState As IntPtr
        End Structure

        Private Shared ReadOnly ofsBuffer As Integer = Marshal.OffsetOf(GetType(KSTM_XFER_CONTEXT_MAP), "Buffer").ToInt32()
        Private Shared ReadOnly ofsBufferSize As Integer = Marshal.OffsetOf(GetType(KSTM_XFER_CONTEXT_MAP), "BufferSize").ToInt32()
        Private Shared ReadOnly ofsTransferLength As Integer = Marshal.OffsetOf(GetType(KSTM_XFER_CONTEXT_MAP), "TransferLength").ToInt32()
        Private Shared ReadOnly ofsUserState As Integer = Marshal.OffsetOf(GetType(KSTM_XFER_CONTEXT_MAP), "UserState").ToInt32()


        ''' <Summary>Internal stream buffer.</Summary>
        Public ReadOnly Property Buffer As IntPtr
            Get
                Return Marshal.ReadIntPtr(mHandlePtr, ofsBuffer)
            End Get
        End Property


        ''' <Summary>Size of internal stream buffer.</Summary>
        Public ReadOnly Property BufferSize As Integer
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsBufferSize)
            End Get
        End Property


        ''' <Summary>Number of bytes to write or number of bytes read.</Summary>
        Public ReadOnly Property TransferLength As Integer
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsTransferLength)
            End Get
        End Property


        ''' <Summary>User defined state.</Summary>
        Public Property UserState As IntPtr
            Get
                Return Marshal.ReadIntPtr(mHandlePtr, ofsUserState)
            End Get
            Set(ByVal value As IntPtr)
                Marshal.WriteIntPtr(mHandlePtr, ofsUserState, value)
            End Set
        End Property

        Public Overrides Function ToString() As String
            Return String.Format("Buffer: {0}" & Microsoft.VisualBasic.Constants.vbLf & "BufferSize: {1}" & Microsoft.VisualBasic.Constants.vbLf & "TransferLength: {2}" & Microsoft.VisualBasic.Constants.vbLf & "UserState: {3}" & Microsoft.VisualBasic.Constants.vbLf, Buffer.ToString("X16") & "h", BufferSize, TransferLength, UserState.ToString("X16") & "h")
        End Function
    End Structure

    <StructLayout(LayoutKind.Sequential)>
    Public Structure KSTM_INFO
        Private ReadOnly mHandlePtr As IntPtr

        Public Sub New(ByVal Handle As IntPtr)
            mHandlePtr = Handle
        End Sub

        Public ReadOnly Property Pointer As IntPtr
            Get
                Return mHandlePtr
            End Get
        End Property

        ''' <summary>KSTM_INFO_MAP is used for calculating field offsets only</summary>
        <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi)>
        Private Structure KSTM_INFO_MAP
            ''' <Summary>\ref KUSB_HANDLE this stream uses.</Summary>
            Private ReadOnly UsbHandle As IntPtr

            ''' <Summary>This parameter corresponds to the bEndpointAddress field in the endpoint descriptor.</Summary>
            Private ReadOnly PipeID As Byte

            ''' <Summary>Maximum transfer read/write request allowed pending.</Summary>
            Private ReadOnly MaxPendingTransfers As Integer

            ''' <Summary>Maximum transfer sage size.</Summary>
            Private ReadOnly MaxTransferSize As Integer

            ''' <Summary>Maximum number of I/O request allowed pending.</Summary>
            Private ReadOnly MaxPendingIO As Integer

            ''' <Summary>Populated with the endpoint descriptor for the specified \c PipeID.</Summary>
            Private ReadOnly EndpointDescriptor As USB_ENDPOINT_DESCRIPTOR

            ''' <Summary>Populated with the driver api for the specified \c UsbHandle.</Summary>
            Private ReadOnly DriverAPI As KUSB_DRIVER_API

            ''' <Summary>Populated with the device file handle for the specified \c UsbHandle.</Summary>
            Private ReadOnly DeviceHandle As IntPtr

            ''' <Summary>Stream handle.</Summary>
            Private ReadOnly StreamHandle As IntPtr

            ''' <Summary>Stream info user defined state.</Summary>
            Private ReadOnly UserState As IntPtr
        End Structure

        Private Shared ReadOnly ofsUsbHandle As Integer = Marshal.OffsetOf(GetType(KSTM_INFO_MAP), "UsbHandle").ToInt32()
        Private Shared ReadOnly ofsPipeID As Integer = Marshal.OffsetOf(GetType(KSTM_INFO_MAP), "PipeID").ToInt32()
        Private Shared ReadOnly ofsMaxPendingTransfers As Integer = Marshal.OffsetOf(GetType(KSTM_INFO_MAP), "MaxPendingTransfers").ToInt32()
        Private Shared ReadOnly ofsMaxTransferSize As Integer = Marshal.OffsetOf(GetType(KSTM_INFO_MAP), "MaxTransferSize").ToInt32()
        Private Shared ReadOnly ofsMaxPendingIO As Integer = Marshal.OffsetOf(GetType(KSTM_INFO_MAP), "MaxPendingIO").ToInt32()
        Private Shared ReadOnly ofsEndpointDescriptor As Integer = Marshal.OffsetOf(GetType(KSTM_INFO_MAP), "EndpointDescriptor").ToInt32()
        Private Shared ReadOnly ofsDriverAPI As Integer = Marshal.OffsetOf(GetType(KSTM_INFO_MAP), "DriverAPI").ToInt32()
        Private Shared ReadOnly ofsDeviceHandle As Integer = Marshal.OffsetOf(GetType(KSTM_INFO_MAP), "DeviceHandle").ToInt32()
        Private Shared ReadOnly ofsStreamHandle As Integer = Marshal.OffsetOf(GetType(KSTM_INFO_MAP), "StreamHandle").ToInt32()
        Private Shared ReadOnly ofsUserState As Integer = Marshal.OffsetOf(GetType(KSTM_INFO_MAP), "UserState").ToInt32()


        ''' <Summary>\ref KUSB_HANDLE this stream uses.</Summary>
        Public ReadOnly Property UsbHandle As IntPtr
            Get
                Return Marshal.ReadIntPtr(mHandlePtr, ofsUsbHandle)
            End Get
        End Property


        ''' <Summary>This parameter corresponds to the bEndpointAddress field in the endpoint descriptor.</Summary>
        Public ReadOnly Property PipeID As Byte
            Get
                Return Marshal.ReadByte(mHandlePtr, ofsPipeID)
            End Get
        End Property


        ''' <Summary>Maximum transfer read/write request allowed pending.</Summary>
        Public ReadOnly Property MaxPendingTransfers As Integer
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsMaxPendingTransfers)
            End Get
        End Property


        ''' <Summary>Maximum transfer sage size.</Summary>
        Public ReadOnly Property MaxTransferSize As Integer
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsMaxTransferSize)
            End Get
        End Property


        ''' <Summary>Maximum number of I/O request allowed pending.</Summary>
        Public ReadOnly Property MaxPendingIO As Integer
            Get
                Return Marshal.ReadInt32(mHandlePtr, ofsMaxPendingIO)
            End Get
        End Property


        ''' <Summary>Populated with the endpoint descriptor for the specified \c PipeID.</Summary>
        Public ReadOnly Property EndpointDescriptor As USB_ENDPOINT_DESCRIPTOR
            Get
                Return CType(Marshal.PtrToStructure(New IntPtr(mHandlePtr.ToInt64() + ofsEndpointDescriptor), GetType(USB_ENDPOINT_DESCRIPTOR)), USB_ENDPOINT_DESCRIPTOR)
            End Get
        End Property


        ''' <Summary>Populated with the driver api for the specified \c UsbHandle.</Summary>
        Public ReadOnly Property DriverAPI As KUSB_DRIVER_API
            Get
                Return CType(Marshal.PtrToStructure(New IntPtr(mHandlePtr.ToInt64() + ofsDriverAPI), GetType(KUSB_DRIVER_API)), KUSB_DRIVER_API)
            End Get
        End Property


        ''' <Summary>Populated with the device file handle for the specified \c UsbHandle.</Summary>
        Public ReadOnly Property DeviceHandle As IntPtr
            Get
                Return Marshal.ReadIntPtr(mHandlePtr, ofsDeviceHandle)
            End Get
        End Property


        ''' <Summary>Stream handle.</Summary>
        Public ReadOnly Property StreamHandle As IntPtr
            Get
                Return Marshal.ReadIntPtr(mHandlePtr, ofsStreamHandle)
            End Get
        End Property


        ''' <Summary>Stream info user defined state.</Summary>
        Public Property UserState As IntPtr
            Get
                Return Marshal.ReadIntPtr(mHandlePtr, ofsUserState)
            End Get
            Set(ByVal value As IntPtr)
                Marshal.WriteIntPtr(mHandlePtr, ofsUserState, value)
            End Set
        End Property

        Public Overrides Function ToString() As String
            Return String.Format("UsbHandle: {0}" & Microsoft.VisualBasic.Constants.vbLf & "PipeID: {1}" & Microsoft.VisualBasic.Constants.vbLf & "MaxPendingTransfers: {2}" & Microsoft.VisualBasic.Constants.vbLf & "MaxTransferSize: {3}" & Microsoft.VisualBasic.Constants.vbLf & "MaxPendingIO: {4}" & Microsoft.VisualBasic.Constants.vbLf & "DeviceHandle: {5}" & Microsoft.VisualBasic.Constants.vbLf & "StreamHandle: {6}" & Microsoft.VisualBasic.Constants.vbLf & "UserState: {7}" & Microsoft.VisualBasic.Constants.vbLf, UsbHandle.ToString("X16") & "h", PipeID.ToString("X2") & "h", MaxPendingTransfers, MaxTransferSize, MaxPendingIO, DeviceHandle.ToString("X16") & "h", StreamHandle.ToString("X16") & "h", UserState.ToString("X16") & "h")
        End Function
    End Structure

    ''' <Summary>Stream callback structure.</Summary>
    <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Size:=64)>
    Public Structure KSTM_CALLBACK
        ''' <Summary>Executed when a transfer error occurs.</Summary>
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public [Error] As KSTM_ERROR_CB

        ''' <Summary>Executed to submit a transfer.</Summary>
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public Submit As KSTM_SUBMIT_CB

        ''' <Summary>Executed when a valid transfer completes.</Summary>
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public Complete As KSTM_COMPLETE_CB

        ''' <Summary>Executed for every transfer context when the stream is started with \ref StmK_Start.</Summary>
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public Started As KSTM_STARTED_CB

        ''' <Summary>Executed for every transfer context when the stream is stopped with \ref StmK_Stop.</Summary>
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public Stopped As KSTM_STOPPED_CB

        ''' <Summary>Executed immediately after a transfer completes.</Summary>
        <MarshalAs(UnmanagedType.FunctionPtr)>
        Public BeforeComplete As KSTM_BEFORE_COMPLETE_CB
    End Structure

#End Region

#Region "Delegates"

    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KLIB_HANDLE_CLEANUP_CB(
    <[In]> ByVal Handle As IntPtr, ByVal HandleType As KLIB_HANDLE_TYPE, ByVal UserContext As IntPtr) As Integer
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KISO_ENUM_PACKETS_CB(ByVal PacketIndex As UInteger,
    <[In]> ByRef IsoPacket As KISO_PACKET, ByVal UserState As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KISOCH_ENUM_PACKETS_CB(ByVal PacketIndex As UInteger, ByRef Offset As UInteger, ByRef Length As UInteger, ByRef Status As UInteger, ByVal UserState As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KLST_ENUM_DEVINFO_CB(
    <[In]> ByVal DeviceList As KLST_HANDLE,
    <[In]> ByVal DeviceInfo As KLST_DEVINFO_HANDLE, ByVal Context As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_InitDelegate(<Out> ByRef InterfaceHandle As KUSB_HANDLE,
    <[In]> ByVal DevInfo As KLST_DEVINFO_HANDLE) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_FreeDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_ClaimInterfaceDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal NumberOrIndex As Byte, ByVal IsIndex As Boolean) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_ReleaseInterfaceDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal NumberOrIndex As Byte, ByVal IsIndex As Boolean) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_SetAltInterfaceDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal NumberOrIndex As Byte, ByVal IsIndex As Boolean, ByVal AltSettingNumber As Byte) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_GetAltInterfaceDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal NumberOrIndex As Byte, ByVal IsIndex As Boolean, <Out> ByRef AltSettingNumber As Byte) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_GetDescriptorDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal DescriptorType As Byte, ByVal Index As Byte, ByVal LanguageID As UShort, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_ControlTransferDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal SetupPacket As WINUSB_SETUP_PACKET, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_SetPowerPolicyDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PolicyType As UInteger, ByVal ValueLength As UInteger, ByVal Value As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_GetPowerPolicyDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PolicyType As UInteger, ByRef ValueLength As UInteger, ByVal Value As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_SetConfigurationDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal ConfigurationNumber As Byte) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_GetConfigurationDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, <Out> ByRef ConfigurationNumber As Byte) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_ResetDeviceDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_InitializeDelegate(ByVal DeviceHandle As IntPtr, <Out> ByRef InterfaceHandle As KUSB_HANDLE) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_SelectInterfaceDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal NumberOrIndex As Byte, ByVal IsIndex As Boolean) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_GetAssociatedInterfaceDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal AssociatedInterfaceIndex As Byte, <Out> ByRef AssociatedInterfaceHandle As KUSB_HANDLE) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_CloneDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, <Out> ByRef DstInterfaceHandle As KUSB_HANDLE) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_QueryInterfaceSettingsDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal AltSettingIndex As Byte, <Out> ByRef UsbAltInterfaceDescriptor As USB_INTERFACE_DESCRIPTOR) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_QueryDeviceInformationDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal InformationType As UInteger, ByRef BufferLength As UInteger, ByVal Buffer As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_SetCurrentAlternateSettingDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal AltSettingNumber As Byte) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_GetCurrentAlternateSettingDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, <Out> ByRef AltSettingNumber As Byte) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_QueryPipeDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal AltSettingNumber As Byte, ByVal PipeIndex As Byte, <Out> ByRef PipeInformation As WINUSB_PIPE_INFORMATION) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_QueryPipeExDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal AlternateSettingNumber As Byte, ByVal PipeIndex As Byte, <Out> ByRef PipeInformationEx As WINUSB_PIPE_INFORMATION_EX) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_GetSuperSpeedPipeCompanionDescriptorDelegate(
    <[In]> ByVal Handle As KUSB_HANDLE, ByVal AltSettingNumber As Byte, ByVal PipeIndex As Byte, <Out> ByRef PipeCompanionDescriptor As USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_SetPipePolicyDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PipeID As Byte, ByVal PolicyType As UInteger, ByVal ValueLength As UInteger, ByVal Value As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_GetPipePolicyDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PipeID As Byte, ByVal PolicyType As UInteger, ByRef ValueLength As UInteger, ByVal Value As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_ReadPipeDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PipeID As Byte, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_WritePipeDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PipeID As Byte, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_ResetPipeDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PipeID As Byte) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_AbortPipeDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PipeID As Byte) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_FlushPipeDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PipeID As Byte) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_IsoReadPipeDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PipeID As Byte, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, ByVal Overlapped As IntPtr,
    <[In]> ByVal IsoContext As KISO_CONTEXT) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_IsoWritePipeDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PipeID As Byte, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, ByVal Overlapped As IntPtr,
    <[In]> ByVal IsoContext As KISO_CONTEXT) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_GetCurrentFrameNumberDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, <Out> ByRef FrameNumber As UInteger) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_GetOverlappedResultDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal Overlapped As IntPtr, <Out> ByRef lpNumberOfBytesTransferred As UInteger, ByVal bWait As Boolean) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_GetPropertyDelegate(
    <[In]> ByVal InterfaceHandle As KUSB_HANDLE, ByVal PropertyType As KUSB_PROPERTY, ByRef PropertySize As UInteger, ByVal Value As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_IsochReadPipeDelegate(
    <[In]> ByVal IsochHandle As KISOCH_HANDLE, ByVal DataLength As UInteger, ByRef FrameNumber As UInteger, ByVal NumberOfPackets As UInteger, ByVal Overlapped As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KUSB_IsochWritePipeDelegate(
    <[In]> ByVal IsochHandle As KISOCH_HANDLE, ByVal DataLength As UInteger, ByRef FrameNumber As UInteger, ByVal NumberOfPackets As UInteger, ByVal Overlapped As IntPtr) As Boolean
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Sub KHOT_PLUG_CB(
    <[In]> ByVal HotHandle As KHOT_HANDLE,
    <[In]> ByVal DeviceInfo As KLST_DEVINFO_HANDLE, ByVal PlugType As KLST_SYNC_FLAG)
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Sub KHOT_POWER_BROADCAST_CB(
    <[In]> ByVal HotHandle As KHOT_HANDLE,
    <[In]> ByVal DeviceInfo As KLST_DEVINFO_HANDLE, ByVal PbtEvent As UInteger)
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KSTM_ERROR_CB(
    <[In]> ByVal StreamInfo As KSTM_INFO,
    <[In]> ByVal XferContext As KSTM_XFER_CONTEXT, ByVal XferContextIndex As Integer, ByVal ErrorCode As Integer) As Integer
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KSTM_SUBMIT_CB(
    <[In]> ByVal StreamInfo As KSTM_INFO,
    <[In]> ByVal XferContext As KSTM_XFER_CONTEXT, ByVal XferContextIndex As Integer, ByVal Overlapped As IntPtr) As Integer
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KSTM_STARTED_CB(
    <[In]> ByVal StreamInfo As KSTM_INFO,
    <[In]> ByVal XferContext As KSTM_XFER_CONTEXT, ByVal XferContextIndex As Integer) As Integer
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KSTM_STOPPED_CB(
    <[In]> ByVal StreamInfo As KSTM_INFO,
    <[In]> ByVal XferContext As KSTM_XFER_CONTEXT, ByVal XferContextIndex As Integer) As Integer
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KSTM_COMPLETE_CB(
    <[In]> ByVal StreamInfo As KSTM_INFO,
    <[In]> ByVal XferContext As KSTM_XFER_CONTEXT, ByVal XferContextIndex As Integer, ByVal ErrorCode As Integer) As Integer
    <UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet:=CharSet.Ansi, SetLastError:=True)>
    Public Delegate Function KSTM_BEFORE_COMPLETE_CB(
    <[In]> ByVal StreamInfo As KSTM_INFO,
    <[In]> ByVal XferContext As KSTM_XFER_CONTEXT, ByVal XferContextIndex As Integer, ByRef ErrorCode As Integer) As KSTM_COMPLETE_RESULT

#End Region

    Public Class LstK
        Implements IDisposable

        Protected mbDisposed As Boolean
        Protected mHandleStruct As KLST_HANDLE

        Protected Sub New()
        End Sub

        ''' <Summary>Initializes a new usb device list containing all supported devices.</Summary>
        Public Sub New(ByVal Flags As KLST_FLAG)
            Dim success = LstK_Init(mHandleStruct, Flags)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
        End Sub

        ''' <Summary>Initializes a new usb device list containing only devices matching a specific class GUID.</Summary>
        Public Sub New(ByVal Flags As KLST_FLAG, ByRef PatternMatch As KLST_PATTERN_MATCH)
            Dim success = LstK_InitEx(mHandleStruct, Flags, PatternMatch)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
        End Sub

        ''' <summary>Gets the handle class structure.</summary>
        Public ReadOnly Property Handle As KLST_HANDLE
            Get
                Return mHandleStruct
            End Get
        End Property

        ''' <summary>Explicitly closes and frees the handle.</summary>
        Public Overridable Sub Dispose() Implements IDisposable.Dispose
            Dispose(True)
            GC.SuppressFinalize(Me)
        End Sub

        Protected Overrides Sub Finalize()
            Dispose(False)
        End Sub

        ''' <summary>Calls the dispose method.</summary>
        Public Overridable Sub Free()
            Dispose()
        End Sub

        Protected Overridable Sub Dispose(ByVal disposing As Boolean)
            If Not mbDisposed Then
                If mHandleStruct.Pointer <> IntPtr.Zero Then
                    LstK_Free(mHandleStruct)
                    Call Debug.Print("{0} Dispose: Freed Handle:{1:X16}h Explicit:{2}", [GetType]().Name, mHandleStruct.Pointer.ToInt64(), disposing)
                Else
                    Call Debug.Print("{0} Dispose: [WARNING] Handle is null", [GetType]().Name)
                End If

                mHandleStruct = New KLST_HANDLE(IntPtr.Zero)
                mbDisposed = True
            End If
        End Sub

        ''' <Summary>Initializes a new usb device list containing all supported devices.</Summary>
        Protected Function Init(ByVal Flags As KLST_FLAG) As Boolean
            Dim success = LstK_Init(mHandleStruct, Flags)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
            Return True
        End Function

        ''' <Summary>Initializes a new usb device list containing only devices matching a specific class GUID.</Summary>
        Protected Function InitEx(ByVal Flags As KLST_FLAG, ByRef PatternMatch As KLST_PATTERN_MATCH) As Boolean
            Dim success = LstK_InitEx(mHandleStruct, Flags, PatternMatch)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
            Return True
        End Function

        ''' <Summary>Enumerates \ref KLST_DEVINFO elements of a \ref KLST_HANDLE.</Summary>
        Public Overridable Function Enumerate(ByVal EnumDevListCB As KLST_ENUM_DEVINFO_CB, ByVal Context As IntPtr) As Boolean
            Return LstK_Enumerate(mHandleStruct, EnumDevListCB, Context)
        End Function

        ''' <Summary>Gets the \ref KLST_DEVINFO element for the current position.</Summary>
        Public Overridable Function Current(<Out> ByRef DeviceInfo As KLST_DEVINFO_HANDLE) As Boolean
            Return LstK_Current(mHandleStruct, DeviceInfo)
        End Function

        ''' <Summary>Advances the device list current \ref KLST_DEVINFO position.</Summary>
        Public Overridable Function MoveNext(<Out> ByRef DeviceInfo As KLST_DEVINFO_HANDLE) As Boolean
            Return LstK_MoveNext(mHandleStruct, DeviceInfo)
        End Function

        ''' <Summary>Sets the device list to its initial position, which is before the first element in the list.</Summary>
        Public Overridable Sub MoveReset()
            LstK_MoveReset(mHandleStruct)
        End Sub

        ''' <Summary>Find a device by vendor and product id</Summary>
        Public Overridable Function FindByVidPid(ByVal Vid As Integer, ByVal Pid As Integer, <Out> ByRef DeviceInfo As KLST_DEVINFO_HANDLE) As Boolean
            Return LstK_FindByVidPid(mHandleStruct, Vid, Pid, DeviceInfo)
        End Function

        ''' <Summary>Counts the number of device info elements in a device list.</Summary>
        Public Overridable Function Count(ByRef pCount As UInteger) As Boolean
            Return LstK_Count(mHandleStruct, pCount)
        End Function
    End Class

    Public Class HotK
        Implements IDisposable

        Protected mbDisposed As Boolean
        Protected mHandleStruct As KHOT_HANDLE

        Protected Sub New()
        End Sub

        ''' <Summary>Creates a new hot-plug handle for USB device arrival/removal event monitoring.</Summary>
        Public Sub New(ByRef InitParams As KHOT_PARAMS)
            Dim success = HotK_Init(mHandleStruct, InitParams)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
        End Sub

        ''' <summary>Gets the handle class structure.</summary>
        Public ReadOnly Property Handle As KHOT_HANDLE
            Get
                Return mHandleStruct
            End Get
        End Property

        ''' <summary>Explicitly closes and frees the handle.</summary>
        Public Overridable Sub Dispose() Implements IDisposable.Dispose
            Dispose(True)
            GC.SuppressFinalize(Me)
        End Sub

        Protected Overrides Sub Finalize()
            Dispose(False)
        End Sub

        ''' <summary>Calls the dispose method.</summary>
        Public Overridable Sub Free()
            Dispose()
        End Sub

        Protected Overridable Sub Dispose(ByVal disposing As Boolean)
            If Not mbDisposed Then
                If mHandleStruct.Pointer <> IntPtr.Zero Then
                    HotK_Free(mHandleStruct)
                    Call Debug.Print("{0} Dispose: Freed Handle:{1:X16}h Explicit:{2}", [GetType]().Name, mHandleStruct.Pointer.ToInt64(), disposing)
                Else
                    Call Debug.Print("{0} Dispose: [WARNING] Handle is null", [GetType]().Name)
                End If

                mHandleStruct = New KHOT_HANDLE(IntPtr.Zero)
                mbDisposed = True
            End If
        End Sub

        ''' <Summary>Creates a new hot-plug handle for USB device arrival/removal event monitoring.</Summary>
        Protected Function Init(ByRef InitParams As KHOT_PARAMS) As Boolean
            Dim success = HotK_Init(mHandleStruct, InitParams)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
            Return True
        End Function

        ''' <Summary>Frees all hot-plug handles initialized with \ref HotK_Init.</Summary>
        Public Shared Sub FreeAll()
            HotK_FreeAll()
        End Sub
    End Class

    Public Class UsbK
        Implements IDisposable

        Protected driverAPI As KUSB_DRIVER_API
        Protected mbDisposed As Boolean
        Protected mHandleStruct As KUSB_HANDLE

        Protected Sub New()
        End Sub

        ''' <Summary>Creates/opens a libusbK interface handle from the device list. This is a preferred method.</Summary>
        Public Sub New(ByVal DevInfo As KLST_DEVINFO_HANDLE)
            Dim success = LibK_LoadDriverAPI(driverAPI, DevInfo.DriverID)
            If Not success Then Throw New Exception(String.Format("{0} failed loading Driver API. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            success = driverAPI.Init(mHandleStruct, DevInfo)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing usb device. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
        End Sub

        ''' <Summary>Creates a libusbK handle for the device specified by a file handle.</Summary>
        Public Sub New(ByVal DeviceHandle As IntPtr, ByVal driverID As KUSB_DRVID)
            Dim success = LibK_LoadDriverAPI(driverAPI, driverID)
            If Not success Then Throw New Exception(String.Format("{0} failed loading Driver API. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            success = driverAPI.Initialize(DeviceHandle, mHandleStruct)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing usb device. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
        End Sub

        ''' <summary>Gets the handle class structure.</summary>
        Public ReadOnly Property Handle As KUSB_HANDLE
            Get
                Return mHandleStruct
            End Get
        End Property

        ''' <summary>Explicitly closes and frees the handle.</summary>
        Public Overridable Sub Dispose() Implements IDisposable.Dispose
            Dispose(True)
            GC.SuppressFinalize(Me)
        End Sub

        Protected Overrides Sub Finalize()
            Dispose(False)
        End Sub

        ''' <summary>Calls the dispose method.</summary>
        Public Overridable Sub Free()
            Dispose()
        End Sub

        Protected Overridable Sub Dispose(ByVal disposing As Boolean)
            If Not mbDisposed Then
                If mHandleStruct.Pointer <> IntPtr.Zero Then
                    UsbK_Free(mHandleStruct)
                    Call Debug.Print("{0} Dispose: Freed Handle:{1:X16}h Explicit:{2}", [GetType]().Name, mHandleStruct.Pointer.ToInt64(), disposing)
                Else
                    Call Debug.Print("{0} Dispose: [WARNING] Handle is null", [GetType]().Name)
                End If

                mHandleStruct = New KUSB_HANDLE(IntPtr.Zero)
                mbDisposed = True
            End If
        End Sub

        ''' <Summary>Creates/opens a libusbK interface handle from the device list. This is a preferred method.</Summary>
        Protected Function Init(ByVal DevInfo As KLST_DEVINFO_HANDLE) As Boolean
            Dim success = LibK_LoadDriverAPI(driverAPI, DevInfo.DriverID)
            If Not success Then Throw New Exception(String.Format("{0} failed loading Driver API. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            success = driverAPI.Init(mHandleStruct, DevInfo)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing usb device. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
            Return True
        End Function

        ''' <Summary>Claims the specified interface by number or index.</Summary>
        Public Overridable Function ClaimInterface(ByVal NumberOrIndex As Byte, ByVal IsIndex As Boolean) As Boolean
            Return driverAPI.ClaimInterface(mHandleStruct, NumberOrIndex, IsIndex)
        End Function

        ''' <Summary>Releases the specified interface by number or index.</Summary>
        Public Overridable Function ReleaseInterface(ByVal NumberOrIndex As Byte, ByVal IsIndex As Boolean) As Boolean
            Return driverAPI.ReleaseInterface(mHandleStruct, NumberOrIndex, IsIndex)
        End Function

        ''' <Summary>Sets the alternate setting of the specified interface.</Summary>
        Public Overridable Function SetAltInterface(ByVal NumberOrIndex As Byte, ByVal IsIndex As Boolean, ByVal AltSettingNumber As Byte) As Boolean
            Return driverAPI.SetAltInterface(mHandleStruct, NumberOrIndex, IsIndex, AltSettingNumber)
        End Function

        ''' <Summary>Gets the alternate setting for the specified interface.</Summary>
        Public Overridable Function GetAltInterface(ByVal NumberOrIndex As Byte, ByVal IsIndex As Boolean, <Out> ByRef AltSettingNumber As Byte) As Boolean
            Return driverAPI.GetAltInterface(mHandleStruct, NumberOrIndex, IsIndex, AltSettingNumber)
        End Function

        ''' <Summary>Gets the requested descriptor. This is a synchronous operation.</Summary>
        Public Overridable Function GetDescriptor(ByVal DescriptorType As Byte, ByVal Index As Byte, ByVal LanguageID As Integer, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger) As Boolean
            Return driverAPI.GetDescriptor(mHandleStruct, DescriptorType, Index, LanguageID, Buffer, BufferLength, LengthTransferred)
        End Function

        ''' <Summary>Gets the requested descriptor. This is a synchronous operation.</Summary>
        Public Overridable Function GetDescriptor(ByVal DescriptorType As Byte, ByVal Index As Byte, ByVal LanguageID As Integer, ByVal Buffer As Array, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger) As Boolean
            Return driverAPI.GetDescriptor(mHandleStruct, DescriptorType, Index, LanguageID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred)
        End Function

        ''' <Summary>Transmits control data over a default control endpoint.</Summary>
        Public Overridable Function ControlTransfer(ByVal SetupPacket As WINUSB_SETUP_PACKET, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As IntPtr) As Boolean
            Return driverAPI.ControlTransfer(mHandleStruct, SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped)
        End Function

        ''' <Summary>Transmits control data over a default control endpoint.</Summary>
        Public Overridable Function ControlTransfer(ByVal SetupPacket As WINUSB_SETUP_PACKET, ByVal Buffer As Array, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As IntPtr) As Boolean
            Return driverAPI.ControlTransfer(mHandleStruct, SetupPacket, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped)
        End Function

        ''' <Summary>Transmits control data over a default control endpoint.</Summary>
        Public Overridable Function ControlTransfer(ByVal SetupPacket As WINUSB_SETUP_PACKET, ByVal Buffer As Array, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As KOVL_HANDLE) As Boolean
            Return driverAPI.ControlTransfer(mHandleStruct, SetupPacket, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped.Pointer)
        End Function

        ''' <Summary>Transmits control data over a default control endpoint.</Summary>
        Public Overridable Function ControlTransfer(ByVal SetupPacket As WINUSB_SETUP_PACKET, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As KOVL_HANDLE) As Boolean
            Return driverAPI.ControlTransfer(mHandleStruct, SetupPacket, Buffer, BufferLength, LengthTransferred, Overlapped.Pointer)
        End Function

        ''' <Summary>Sets the power policy for a device.</Summary>
        Public Overridable Function SetPowerPolicy(ByVal PolicyType As UInteger, ByVal ValueLength As UInteger, ByVal Value As IntPtr) As Boolean
            Return driverAPI.SetPowerPolicy(mHandleStruct, PolicyType, ValueLength, Value)
        End Function

        ''' <Summary>Sets the power policy for a device.</Summary>
        Public Overridable Function SetPowerPolicy(ByVal PolicyType As UInteger, ByVal ValueLength As UInteger, ByVal Value As Array) As Boolean
            Return driverAPI.SetPowerPolicy(mHandleStruct, PolicyType, ValueLength, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))
        End Function

        ''' <Summary>Gets the power policy for a device.</Summary>
        Public Overridable Function GetPowerPolicy(ByVal PolicyType As UInteger, ByRef ValueLength As UInteger, ByVal Value As IntPtr) As Boolean
            Return driverAPI.GetPowerPolicy(mHandleStruct, PolicyType, ValueLength, Value)
        End Function

        ''' <Summary>Gets the power policy for a device.</Summary>
        Public Overridable Function GetPowerPolicy(ByVal PolicyType As UInteger, ByRef ValueLength As UInteger, ByVal Value As Array) As Boolean
            Return driverAPI.GetPowerPolicy(mHandleStruct, PolicyType, ValueLength, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))
        End Function

        ''' <Summary>Sets the device configuration number.</Summary>
        Public Overridable Function SetConfiguration(ByVal ConfigurationNumber As Byte) As Boolean
            Return driverAPI.SetConfiguration(mHandleStruct, ConfigurationNumber)
        End Function

        ''' <Summary>Gets the device current configuration number.</Summary>
        Public Overridable Function GetConfiguration(<Out> ByRef ConfigurationNumber As Byte) As Boolean
            Return driverAPI.GetConfiguration(mHandleStruct, ConfigurationNumber)
        End Function

        ''' <Summary>Resets the usb device of the specified interface handle. (port cycle).</Summary>
        Public Overridable Function ResetDevice() As Boolean
            Return driverAPI.ResetDevice(mHandleStruct)
        End Function

        ''' <Summary>Creates a libusbK handle for the device specified by a file handle.</Summary>
        Protected Function Initialize(ByVal DeviceHandle As IntPtr, ByVal driverID As KUSB_DRVID) As Boolean
            Dim success = LibK_LoadDriverAPI(driverAPI, driverID)
            If Not success Then Throw New Exception(String.Format("{0} failed loading Driver API. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            success = driverAPI.Initialize(DeviceHandle, mHandleStruct)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing usb device. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
            Return True
        End Function

        ''' <Summary>Selects the specified interface by number or index as the current interface.</Summary>
        Public Overridable Function SelectInterface(ByVal NumberOrIndex As Byte, ByVal IsIndex As Boolean) As Boolean
            Return driverAPI.SelectInterface(mHandleStruct, NumberOrIndex, IsIndex)
        End Function

        ''' <Summary>Retrieves a handle for an associated interface.</Summary>
        Public Overridable Function GetAssociatedInterface(ByVal AssociatedInterfaceIndex As Byte, <Out> ByRef AssociatedInterfaceHandle As KUSB_HANDLE) As Boolean
            Return driverAPI.GetAssociatedInterface(mHandleStruct, AssociatedInterfaceIndex, AssociatedInterfaceHandle)
        End Function

        ''' <Summary>Clones the specified interface handle.</Summary>
        Public Overridable Function Clone(<Out> ByRef DstInterfaceHandle As KUSB_HANDLE) As Boolean
            Return driverAPI.Clone(mHandleStruct, DstInterfaceHandle)
        End Function

        ''' <Summary>
        '''     Retrieves the interface descriptor for the specified alternate interface settings for a particular interface
        '''     handle.
        ''' </Summary>
        Public Overridable Function QueryInterfaceSettings(ByVal AltSettingIndex As Byte, <Out> ByRef UsbAltInterfaceDescriptor As USB_INTERFACE_DESCRIPTOR) As Boolean
            Return driverAPI.QueryInterfaceSettings(mHandleStruct, AltSettingIndex, UsbAltInterfaceDescriptor)
        End Function

        ''' <Summary>Retrieves information about the physical device that is associated with a libusbK handle.</Summary>
        Public Overridable Function QueryDeviceInformation(ByVal InformationType As UInteger, ByRef BufferLength As UInteger, ByVal Buffer As IntPtr) As Boolean
            Return driverAPI.QueryDeviceInformation(mHandleStruct, InformationType, BufferLength, Buffer)
        End Function

        ''' <Summary>Sets the alternate setting of an interface.</Summary>
        Public Overridable Function SetCurrentAlternateSetting(ByVal AltSettingNumber As Byte) As Boolean
            Return driverAPI.SetCurrentAlternateSetting(mHandleStruct, AltSettingNumber)
        End Function

        ''' <Summary>Gets the current alternate interface setting for an interface.</Summary>
        Public Overridable Function GetCurrentAlternateSetting(<Out> ByRef AltSettingNumber As Byte) As Boolean
            Return driverAPI.GetCurrentAlternateSetting(mHandleStruct, AltSettingNumber)
        End Function

        ''' <Summary>Retrieves information about a pipe that is associated with an interface.</Summary>
        Public Overridable Function QueryPipe(ByVal AltSettingNumber As Byte, ByVal PipeIndex As Byte, <Out> ByRef PipeInformation As WINUSB_PIPE_INFORMATION) As Boolean
            Return driverAPI.QueryPipe(mHandleStruct, AltSettingNumber, PipeIndex, PipeInformation)
        End Function

        ''' <Summary>Retrieves information about a pipe that is associated with an interface.</Summary>
        Public Overridable Function QueryPipeEx(ByVal AltSettingNumber As Byte, ByVal PipeIndex As Byte, <Out> ByRef PipeInformationEx As WINUSB_PIPE_INFORMATION_EX) As Boolean
            Return driverAPI.QueryPipeEx(mHandleStruct, AltSettingNumber, PipeIndex, PipeInformationEx)
        End Function

        ''' <Summary>Retrieves a pipes super speed endpoint companion descriptor associated with an interface.</Summary>
        Public Overridable Function GetSuperSpeedPipeCompanionDescriptor(ByVal AltSettingNumber As Byte, ByVal PipeIndex As Byte, <Out> ByRef PipeCompanionDescriptor As USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR) As Boolean
            Return driverAPI.GetSuperSpeedPipeCompanionDescriptor(mHandleStruct, AltSettingNumber, PipeIndex, PipeCompanionDescriptor)
        End Function

        ''' <Summary>
        '''     Sets the policy for a specific pipe associated with an endpoint on the device. This is a synchronous
        '''     operation.
        ''' </Summary>
        Public Overridable Function SetPipePolicy(ByVal PipeID As Byte, ByVal PolicyType As UInteger, ByVal ValueLength As UInteger, ByVal Value As IntPtr) As Boolean
            Return driverAPI.SetPipePolicy(mHandleStruct, PipeID, PolicyType, ValueLength, Value)
        End Function

        ''' <Summary>
        '''     Sets the policy for a specific pipe associated with an endpoint on the device. This is a synchronous
        '''     operation.
        ''' </Summary>
        Public Overridable Function SetPipePolicy(ByVal PipeID As Byte, ByVal PolicyType As UInteger, ByVal ValueLength As UInteger, ByVal Value As Array) As Boolean
            Return driverAPI.SetPipePolicy(mHandleStruct, PipeID, PolicyType, ValueLength, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))
        End Function

        ''' <Summary>Gets the policy for a specific pipe (endpoint).</Summary>
        Public Overridable Function GetPipePolicy(ByVal PipeID As Byte, ByVal PolicyType As UInteger, ByRef ValueLength As UInteger, ByVal Value As IntPtr) As Boolean
            Return driverAPI.GetPipePolicy(mHandleStruct, PipeID, PolicyType, ValueLength, Value)
        End Function

        ''' <Summary>Gets the policy for a specific pipe (endpoint).</Summary>
        Public Overridable Function GetPipePolicy(ByVal PipeID As Byte, ByVal PolicyType As UInteger, ByRef ValueLength As UInteger, ByVal Value As Array) As Boolean
            Return driverAPI.GetPipePolicy(mHandleStruct, PipeID, PolicyType, ValueLength, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))
        End Function

        ''' <Summary>Reads data from the specified pipe.</Summary>
        Public Overridable Function ReadPipe(ByVal PipeID As Byte, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As IntPtr) As Boolean
            Return driverAPI.ReadPipe(mHandleStruct, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped)
        End Function

        ''' <Summary>Reads data from the specified pipe.</Summary>
        Public Overridable Function ReadPipe(ByVal PipeID As Byte, ByVal Buffer As Array, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As IntPtr) As Boolean
            Return driverAPI.ReadPipe(mHandleStruct, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped)
        End Function

        ''' <Summary>Reads data from the specified pipe.</Summary>
        Public Overridable Function ReadPipe(ByVal PipeID As Byte, ByVal Buffer As Array, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As KOVL_HANDLE) As Boolean
            Return driverAPI.ReadPipe(mHandleStruct, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped.Pointer)
        End Function

        ''' <Summary>Reads data from the specified pipe.</Summary>
        Public Overridable Function ReadPipe(ByVal PipeID As Byte, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As KOVL_HANDLE) As Boolean
            Return driverAPI.ReadPipe(mHandleStruct, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped.Pointer)
        End Function

        ''' <Summary>Writes data to a pipe.</Summary>
        Public Overridable Function WritePipe(ByVal PipeID As Byte, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As IntPtr) As Boolean
            Return driverAPI.WritePipe(mHandleStruct, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped)
        End Function

        ''' <Summary>Writes data to a pipe.</Summary>
        Public Overridable Function WritePipe(ByVal PipeID As Byte, ByVal Buffer As Array, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As IntPtr) As Boolean
            Return driverAPI.WritePipe(mHandleStruct, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped)
        End Function

        ''' <Summary>Writes data to a pipe.</Summary>
        Public Overridable Function WritePipe(ByVal PipeID As Byte, ByVal Buffer As Array, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As KOVL_HANDLE) As Boolean
            Return driverAPI.WritePipe(mHandleStruct, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, LengthTransferred, Overlapped.Pointer)
        End Function

        ''' <Summary>Writes data to a pipe.</Summary>
        Public Overridable Function WritePipe(ByVal PipeID As Byte, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, <Out> ByRef LengthTransferred As UInteger, ByVal Overlapped As KOVL_HANDLE) As Boolean
            Return driverAPI.WritePipe(mHandleStruct, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped.Pointer)
        End Function

        ''' <Summary>Resets the data toggle and clears the stall condition on a pipe.</Summary>
        Public Overridable Function ResetPipe(ByVal PipeID As Byte) As Boolean
            Return driverAPI.ResetPipe(mHandleStruct, PipeID)
        End Function

        ''' <Summary>Aborts all of the pending transfers for a pipe.</Summary>
        Public Overridable Function AbortPipe(ByVal PipeID As Byte) As Boolean
            Return driverAPI.AbortPipe(mHandleStruct, PipeID)
        End Function

        ''' <Summary>Discards any data that is cached in a pipe.</Summary>
        Public Overridable Function FlushPipe(ByVal PipeID As Byte) As Boolean
            Return driverAPI.FlushPipe(mHandleStruct, PipeID)
        End Function

        ''' <Summary>Reads from an isochronous pipe.</Summary>
        Public Overridable Function IsoReadPipe(ByVal PipeID As Byte, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, ByVal Overlapped As IntPtr, ByVal IsoContext As KISO_CONTEXT) As Boolean
            Return driverAPI.IsoReadPipe(mHandleStruct, PipeID, Buffer, BufferLength, Overlapped, IsoContext)
        End Function

        ''' <Summary>Reads from an isochronous pipe.</Summary>
        Public Overridable Function IsoReadPipe(ByVal PipeID As Byte, ByVal Buffer As Array, ByVal BufferLength As UInteger, ByVal Overlapped As IntPtr, ByVal IsoContext As KISO_CONTEXT) As Boolean
            Return driverAPI.IsoReadPipe(mHandleStruct, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, Overlapped, IsoContext)
        End Function

        ''' <Summary>Reads from an isochronous pipe.</Summary>
        Public Overridable Function IsoReadPipe(ByVal PipeID As Byte, ByVal Buffer As Array, ByVal BufferLength As UInteger, ByVal Overlapped As KOVL_HANDLE, ByVal IsoContext As KISO_CONTEXT) As Boolean
            Return driverAPI.IsoReadPipe(mHandleStruct, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, Overlapped.Pointer, IsoContext)
        End Function

        ''' <Summary>Reads from an isochronous pipe.</Summary>
        Public Overridable Function IsoReadPipe(ByVal PipeID As Byte, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, ByVal Overlapped As KOVL_HANDLE, ByVal IsoContext As KISO_CONTEXT) As Boolean
            Return driverAPI.IsoReadPipe(mHandleStruct, PipeID, Buffer, BufferLength, Overlapped.Pointer, IsoContext)
        End Function

        ''' <Summary>Writes to an isochronous pipe.</Summary>
        Public Overridable Function IsoWritePipe(ByVal PipeID As Byte, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, ByVal Overlapped As IntPtr, ByVal IsoContext As KISO_CONTEXT) As Boolean
            Return driverAPI.IsoWritePipe(mHandleStruct, PipeID, Buffer, BufferLength, Overlapped, IsoContext)
        End Function

        ''' <Summary>Writes to an isochronous pipe.</Summary>
        Public Overridable Function IsoWritePipe(ByVal PipeID As Byte, ByVal Buffer As Array, ByVal BufferLength As UInteger, ByVal Overlapped As IntPtr, ByVal IsoContext As KISO_CONTEXT) As Boolean
            Return driverAPI.IsoWritePipe(mHandleStruct, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, Overlapped, IsoContext)
        End Function

        ''' <Summary>Writes to an isochronous pipe.</Summary>
        Public Overridable Function IsoWritePipe(ByVal PipeID As Byte, ByVal Buffer As Array, ByVal BufferLength As UInteger, ByVal Overlapped As KOVL_HANDLE, ByVal IsoContext As KISO_CONTEXT) As Boolean
            Return driverAPI.IsoWritePipe(mHandleStruct, PipeID, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), BufferLength, Overlapped.Pointer, IsoContext)
        End Function

        ''' <Summary>Writes to an isochronous pipe.</Summary>
        Public Overridable Function IsoWritePipe(ByVal PipeID As Byte, ByVal Buffer As IntPtr, ByVal BufferLength As UInteger, ByVal Overlapped As KOVL_HANDLE, ByVal IsoContext As KISO_CONTEXT) As Boolean
            Return driverAPI.IsoWritePipe(mHandleStruct, PipeID, Buffer, BufferLength, Overlapped.Pointer, IsoContext)
        End Function

        ''' <Summary>Retrieves the current USB frame number.</Summary>
        Public Overridable Function GetCurrentFrameNumber(<Out> ByRef FrameNumber As UInteger) As Boolean
            Return driverAPI.GetCurrentFrameNumber(mHandleStruct, FrameNumber)
        End Function

        ''' <Summary>Reads from an isochronous pipe. Supports LibusbK or WinUsb</Summary>
        Public Overridable Function IsochReadPipe(ByVal IsochHandle As KISOCH_HANDLE, ByVal DataLength As UInteger, ByRef FrameNumber As UInteger, ByVal NumberOfPackets As UInteger, ByVal Overlapped As IntPtr) As Boolean
            Return driverAPI.IsochReadPipe(IsochHandle, DataLength, FrameNumber, NumberOfPackets, Overlapped)
        End Function

        ''' <Summary>Reads from an isochronous pipe. Supports LibusbK or WinUsb</Summary>
        Public Overridable Function IsochReadPipe(ByVal IsochHandle As KISOCH_HANDLE, ByVal DataLength As UInteger, ByRef FrameNumber As UInteger, ByVal NumberOfPackets As UInteger, ByVal Overlapped As KOVL_HANDLE) As Boolean
            Return driverAPI.IsochReadPipe(IsochHandle, DataLength, FrameNumber, NumberOfPackets, Overlapped.Pointer)
        End Function

        ''' <Summary>Writes to an isochronous pipe. Supports LibusbK or WinUsb</Summary>
        Public Overridable Function IsochWritePipe(ByVal IsochHandle As KISOCH_HANDLE, ByVal DataLength As UInteger, ByRef FrameNumber As UInteger, ByVal NumberOfPackets As UInteger, ByVal Overlapped As IntPtr) As Boolean
            Return driverAPI.IsochWritePipe(IsochHandle, DataLength, FrameNumber, NumberOfPackets, Overlapped)
        End Function

        ''' <Summary>Writes to an isochronous pipe. Supports LibusbK or WinUsb</Summary>
        Public Overridable Function IsochWritePipe(ByVal IsochHandle As KISOCH_HANDLE, ByVal DataLength As UInteger, ByRef FrameNumber As UInteger, ByVal NumberOfPackets As UInteger, ByVal Overlapped As KOVL_HANDLE) As Boolean
            Return driverAPI.IsochWritePipe(IsochHandle, DataLength, FrameNumber, NumberOfPackets, Overlapped.Pointer)
        End Function

        ''' <Summary>Retrieves the results of an overlapped operation on the specified libusbK handle.</Summary>
        Public Overridable Function GetOverlappedResult(ByVal Overlapped As IntPtr, <Out> ByRef lpNumberOfBytesTransferred As UInteger, ByVal bWait As Boolean) As Boolean
            Return driverAPI.GetOverlappedResult(mHandleStruct, Overlapped, lpNumberOfBytesTransferred, bWait)
        End Function

        ''' <Summary>Retrieves the results of an overlapped operation on the specified libusbK handle.</Summary>
        Public Overridable Function GetOverlappedResult(ByVal Overlapped As KOVL_HANDLE, <Out> ByRef lpNumberOfBytesTransferred As UInteger, ByVal bWait As Boolean) As Boolean
            Return driverAPI.GetOverlappedResult(mHandleStruct, Overlapped.Pointer, lpNumberOfBytesTransferred, bWait)
        End Function

        ''' <Summary>Gets a USB device (driver specific) property from usb handle.</Summary>
        Public Overridable Function GetProperty(ByVal PropertyType As KUSB_PROPERTY, ByRef PropertySize As UInteger, ByVal Value As IntPtr) As Boolean
            Return driverAPI.GetProperty(mHandleStruct, PropertyType, PropertySize, Value)
        End Function

        ''' <Summary>Gets a USB device (driver specific) property from usb handle.</Summary>
        Public Overridable Function GetProperty(ByVal PropertyType As KUSB_PROPERTY, ByRef PropertySize As UInteger, ByVal Value As Array) As Boolean
            Return driverAPI.GetProperty(mHandleStruct, PropertyType, PropertySize, Marshal.UnsafeAddrOfPinnedArrayElement(Value, 0))
        End Function
    End Class

    Public Class OvlK
        Implements IDisposable

        Protected mbDisposed As Boolean
        Protected mHandleStruct As KOVL_POOL_HANDLE

        Protected Sub New()
        End Sub

        ''' <Summary>Creates a new overlapped pool.</Summary>
        Public Sub New(ByVal UsbHandle As KUSB_HANDLE, ByVal MaxOverlappedCount As Integer, ByVal Flags As KOVL_POOL_FLAG)
            Dim success = OvlK_Init(mHandleStruct, UsbHandle, MaxOverlappedCount, Flags)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
        End Sub

        ''' <summary>Gets the handle class structure.</summary>
        Public ReadOnly Property Handle As KOVL_POOL_HANDLE
            Get
                Return mHandleStruct
            End Get
        End Property

        ''' <summary>Explicitly closes and frees the handle.</summary>
        Public Overridable Sub Dispose() Implements IDisposable.Dispose
            Dispose(True)
            GC.SuppressFinalize(Me)
        End Sub

        Protected Overrides Sub Finalize()
            Dispose(False)
        End Sub

        ''' <summary>Calls the dispose method.</summary>
        Public Overridable Sub Free()
            Dispose()
        End Sub

        Protected Overridable Sub Dispose(ByVal disposing As Boolean)
            If Not mbDisposed Then
                If mHandleStruct.Pointer <> IntPtr.Zero Then
                    OvlK_Free(mHandleStruct)
                    Call Debug.Print("{0} Dispose: Freed Handle:{1:X16}h Explicit:{2}", [GetType]().Name, mHandleStruct.Pointer.ToInt64(), disposing)
                Else
                    Call Debug.Print("{0} Dispose: [WARNING] Handle is null", [GetType]().Name)
                End If

                mHandleStruct = New KOVL_POOL_HANDLE(IntPtr.Zero)
                mbDisposed = True
            End If
        End Sub

        ''' <Summary>Gets a preallocated \c OverlappedK structure from the specified/default pool.</Summary>
        Public Overridable Function Acquire(<Out> ByRef OverlappedK As KOVL_HANDLE) As Boolean
            Return OvlK_Acquire(OverlappedK, mHandleStruct)
        End Function

        ''' <Summary>Returns an \c OverlappedK structure to it's pool.</Summary>
        Public Shared Function Release(ByVal OverlappedK As KOVL_HANDLE) As Boolean
            Return OvlK_Release(OverlappedK)
        End Function

        ''' <Summary>Creates a new overlapped pool.</Summary>
        Protected Function Init(ByVal UsbHandle As KUSB_HANDLE, ByVal MaxOverlappedCount As Integer, ByVal Flags As KOVL_POOL_FLAG) As Boolean
            Dim success = OvlK_Init(mHandleStruct, UsbHandle, MaxOverlappedCount, Flags)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
            Return True
        End Function

        ''' <Summary>Returns the internal event handle used to signal IO operations.</Summary>
        Public Shared Function GetEventHandle(ByVal OverlappedK As KOVL_HANDLE) As IntPtr
            Return OvlK_GetEventHandle(OverlappedK)
        End Function

        ''' <Summary>Waits for overlapped I/O completion, and performs actions specified in \c WaitFlags.</Summary>
        Public Shared Function Wait(ByVal OverlappedK As KOVL_HANDLE, ByVal TimeoutMS As Integer, ByVal WaitFlags As KOVL_WAIT_FLAG, <Out> ByRef TransferredLength As UInteger) As Boolean
            Return OvlK_Wait(OverlappedK, TimeoutMS, WaitFlags, TransferredLength)
        End Function

        ''' <Summary>
        '''     Waits for overlapped I/O completion on the oldest acquired OverlappedK handle and performs actions specified
        '''     in \c WaitFlags.
        ''' </Summary>
        Public Overridable Function WaitOldest(<Out> ByRef OverlappedK As KOVL_HANDLE, ByVal TimeoutMS As Integer, ByVal WaitFlags As KOVL_WAIT_FLAG, <Out> ByRef TransferredLength As UInteger) As Boolean
            Return OvlK_WaitOldest(mHandleStruct, OverlappedK, TimeoutMS, WaitFlags, TransferredLength)
        End Function

        ''' <Summary>Waits for overlapped I/O completion, cancels on a timeout error.</Summary>
        Public Shared Function WaitOrCancel(ByVal OverlappedK As KOVL_HANDLE, ByVal TimeoutMS As Integer, <Out> ByRef TransferredLength As UInteger) As Boolean
            Return OvlK_WaitOrCancel(OverlappedK, TimeoutMS, TransferredLength)
        End Function

        ''' <Summary>
        '''     Waits for overlapped I/O completion, cancels on a timeout error and always releases the OvlK handle back to
        '''     its pool.
        ''' </Summary>
        Public Shared Function WaitAndRelease(ByVal OverlappedK As KOVL_HANDLE, ByVal TimeoutMS As Integer, <Out> ByRef TransferredLength As UInteger) As Boolean
            Return OvlK_WaitAndRelease(OverlappedK, TimeoutMS, TransferredLength)
        End Function

        ''' <Summary>Checks for i/o completion; returns immediately. (polling)</Summary>
        Public Shared Function IsComplete(ByVal OverlappedK As KOVL_HANDLE) As Boolean
            Return OvlK_IsComplete(OverlappedK)
        End Function

        ''' <Summary>Initializes an overlappedK for re-use. The overlappedK is not return to its pool.</Summary>
        Public Shared Function ReUse(ByVal OverlappedK As KOVL_HANDLE) As Boolean
            Return OvlK_ReUse(OverlappedK)
        End Function
    End Class

    Public Class StmK
        Implements IDisposable

        Protected mbDisposed As Boolean
        Protected mHandleStruct As KSTM_HANDLE

        Protected Sub New()
        End Sub

        ''' <Summary>Initializes a new uni-directional pipe stream.</Summary>
        Public Sub New(ByVal UsbHandle As KUSB_HANDLE, ByVal PipeID As Byte, ByVal MaxTransferSize As Integer, ByVal MaxPendingTransfers As Integer, ByVal MaxPendingIO As Integer, ByRef Callbacks As KSTM_CALLBACK, ByVal Flags As KSTM_FLAG)
            Dim success = StmK_Init(mHandleStruct, UsbHandle, PipeID, MaxTransferSize, MaxPendingTransfers, MaxPendingIO, Callbacks, Flags)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
        End Sub

        ''' <summary>Gets the handle class structure.</summary>
        Public ReadOnly Property Handle As KSTM_HANDLE
            Get
                Return mHandleStruct
            End Get
        End Property

        ''' <summary>Explicitly closes and frees the handle.</summary>
        Public Overridable Sub Dispose() Implements IDisposable.Dispose
            Dispose(True)
            GC.SuppressFinalize(Me)
        End Sub

        Protected Overrides Sub Finalize()
            Dispose(False)
        End Sub

        ''' <summary>Calls the dispose method.</summary>
        Public Overridable Sub Free()
            Dispose()
        End Sub

        Protected Overridable Sub Dispose(ByVal disposing As Boolean)
            If Not mbDisposed Then
                If mHandleStruct.Pointer <> IntPtr.Zero Then
                    StmK_Free(mHandleStruct)
                    Call Debug.Print("{0} Dispose: Freed Handle:{1:X16}h Explicit:{2}", [GetType]().Name, mHandleStruct.Pointer.ToInt64(), disposing)
                Else
                    Call Debug.Print("{0} Dispose: [WARNING] Handle is null", [GetType]().Name)
                End If

                mHandleStruct = New KSTM_HANDLE(IntPtr.Zero)
                mbDisposed = True
            End If
        End Sub

        ''' <Summary>Initializes a new uni-directional pipe stream.</Summary>
        Protected Function Init(ByVal UsbHandle As KUSB_HANDLE, ByVal PipeID As Byte, ByVal MaxTransferSize As Integer, ByVal MaxPendingTransfers As Integer, ByVal MaxPendingIO As Integer, ByRef Callbacks As KSTM_CALLBACK, ByVal Flags As KSTM_FLAG) As Boolean
            Dim success = StmK_Init(mHandleStruct, UsbHandle, PipeID, MaxTransferSize, MaxPendingTransfers, MaxPendingIO, Callbacks, Flags)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
            Return True
        End Function

        ''' <Summary>Starts the internal stream thread.</Summary>
        Public Overridable Function Start() As Boolean
            Return StmK_Start(mHandleStruct)
        End Function

        ''' <Summary>Stops the internal stream thread.</Summary>
        Public Overridable Function [Stop](ByVal TimeoutCancelMS As Integer) As Boolean
            Return StmK_Stop(mHandleStruct, TimeoutCancelMS)
        End Function

        ''' <Summary>Reads data from the stream buffer.</Summary>
        Public Overridable Function Read(ByVal Buffer As IntPtr, ByVal Offset As Integer, ByVal Length As Integer, <Out> ByRef TransferredLength As UInteger) As Boolean
            Return StmK_Read(mHandleStruct, Buffer, Offset, Length, TransferredLength)
        End Function

        ''' <Summary>Reads data from the stream buffer.</Summary>
        Public Overridable Function Read(ByVal Buffer As Array, ByVal Offset As Integer, ByVal Length As Integer, <Out> ByRef TransferredLength As UInteger) As Boolean
            Return StmK_Read(mHandleStruct, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), Offset, Length, TransferredLength)
        End Function

        ''' <Summary>Writes data to the stream buffer.</Summary>
        Public Overridable Function Write(ByVal Buffer As IntPtr, ByVal Offset As Integer, ByVal Length As Integer, <Out> ByRef TransferredLength As UInteger) As Boolean
            Return StmK_Write(mHandleStruct, Buffer, Offset, Length, TransferredLength)
        End Function

        ''' <Summary>Writes data to the stream buffer.</Summary>
        Public Overridable Function Write(ByVal Buffer As Array, ByVal Offset As Integer, ByVal Length As Integer, <Out> ByRef TransferredLength As UInteger) As Boolean
            Return StmK_Write(mHandleStruct, Marshal.UnsafeAddrOfPinnedArrayElement(Buffer, 0), Offset, Length, TransferredLength)
        End Function
    End Class

    Public Class IsoK
        Implements IDisposable

        Private Shared ReadOnly ofsFlags As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "Flags").ToInt32()
        Private Shared ReadOnly ofsStartFrame As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "StartFrame").ToInt32()
        Private Shared ReadOnly ofsErrorCount As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "ErrorCount").ToInt32()
        Private Shared ReadOnly ofsNumberOfPackets As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "NumberOfPackets").ToInt32()
        Private Shared ReadOnly ofsUrbHdrStatus As Integer = Marshal.OffsetOf(GetType(KISO_CONTEXT_MAP), "UrbHdrStatus").ToInt32()
        Protected mbDisposed As Boolean
        Protected mHandleStruct As KISO_CONTEXT

        Protected Sub New()
        End Sub


        ''' <Summary>Creates a new isochronous transfer context for libusbK only.</Summary>
        Public Sub New(ByVal NumberOfPackets As Integer, ByVal StartFrame As Integer)
            Dim success = IsoK_Init(mHandleStruct, NumberOfPackets, StartFrame)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
        End Sub

        ''' <summary>Gets the handle class structure.</summary>
        Public ReadOnly Property Handle As KISO_CONTEXT
            Get
                Return mHandleStruct
            End Get
        End Property


        ''' <Summary>Additional ISO transfer flags. See \ref KISO_FLAG.</Summary>
        Public Property Flags As KISO_FLAG
            Get
                Return Marshal.ReadInt32(mHandleStruct.Pointer, ofsFlags)
            End Get
            Set(ByVal value As KISO_FLAG)
                Marshal.WriteInt32(mHandleStruct.Pointer, ofsFlags, value)
            End Set
        End Property


        ''' <Summary>Specifies the frame number that the transfer should begin on (0 for ASAP).</Summary>

        Public Property StartFrame As UInteger
            Get
                Return Marshal.ReadInt32(mHandleStruct.Pointer, ofsStartFrame)
            End Get
            Set(ByVal value As UInteger)
                Marshal.WriteInt32(mHandleStruct.Pointer, ofsStartFrame, CInt(value))
            End Set
        End Property


        ''' <Summary>
        '''     Contains the number of packets that completed with an error condition on return from the host controller
        '''     driver.
        ''' </Summary>

        Public Property ErrorCount As Short
            Get
                Return Marshal.ReadInt16(mHandleStruct.Pointer, ofsErrorCount)
            End Get
            Set(ByVal value As Short)
                Marshal.WriteInt16(mHandleStruct.Pointer, ofsErrorCount, value)
            End Set
        End Property


        ''' <Summary>Specifies the number of packets that are described by the variable-length array member \c IsoPacket.</Summary>

        Public Property NumberOfPackets As Short
            Get
                Return Marshal.ReadInt16(mHandleStruct.Pointer, ofsNumberOfPackets)
            End Get
            Set(ByVal value As Short)
                Marshal.WriteInt16(mHandleStruct.Pointer, ofsNumberOfPackets, value)
            End Set
        End Property


        ''' <Summary>Contains the URB Hdr.Status value on return from the host controller driver.</Summary>

        Public Property UrbHdrStatus As UInteger
            Get
                Return Marshal.ReadInt32(mHandleStruct.Pointer, ofsUrbHdrStatus)
            End Get
            Set(ByVal value As UInteger)
                Marshal.WriteInt32(mHandleStruct.Pointer, ofsUrbHdrStatus, CInt(value))
            End Set
        End Property

        ''' <summary>Explicitly closes and frees the handle.</summary>
        Public Overridable Sub Dispose() Implements IDisposable.Dispose
            Dispose(True)
            GC.SuppressFinalize(Me)
        End Sub

        Protected Overrides Sub Finalize()
            Dispose(False)
        End Sub

        ''' <summary>Calls the dispose method.</summary>
        Public Overridable Sub Free()
            Dispose()
        End Sub

        Protected Overridable Sub Dispose(ByVal disposing As Boolean)
            If Not mbDisposed Then
                If mHandleStruct.Pointer <> IntPtr.Zero Then
                    IsoK_Free(mHandleStruct)
                    Call Debug.Print("{0} Dispose: Freed Handle:{1:X16}h Explicit:{2}", [GetType]().Name, mHandleStruct.Pointer.ToInt64(), disposing)
                Else
                    Call Debug.Print("{0} Dispose: [WARNING] Handle is null", [GetType]().Name)
                End If

                mHandleStruct = New KISO_CONTEXT(IntPtr.Zero)
                mbDisposed = True
            End If
        End Sub

        ''' <Summary>Creates a new isochronous transfer context for libusbK only.</Summary>
        Protected Function Init(ByVal NumberOfPackets As Integer, ByVal StartFrame As Integer) As Boolean
            Dim success = IsoK_Init(mHandleStruct, NumberOfPackets, StartFrame)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
            Return True
        End Function

        ''' <Summary>Convenience function for setting the offset of all ISO packets of an isochronous transfer context.</Summary>
        Public Overridable Function SetPackets(ByVal PacketSize As Integer) As Boolean
            Return IsoK_SetPackets(mHandleStruct, PacketSize)
        End Function

        ''' <Summary>Convenience function for setting all fields of a \ref KISO_PACKET.</Summary>
        Public Overridable Function SetPacket(ByVal PacketIndex As Integer, ByRef IsoPacket As KISO_PACKET) As Boolean
            Return IsoK_SetPacket(mHandleStruct, PacketIndex, IsoPacket)
        End Function

        ''' <Summary>Convenience function for getting all fields of a \ref KISO_PACKET.</Summary>
        Public Overridable Function GetPacket(ByVal PacketIndex As Integer, <Out> ByRef IsoPacket As KISO_PACKET) As Boolean
            Return IsoK_GetPacket(mHandleStruct, PacketIndex, IsoPacket)
        End Function

        ''' <Summary>Convenience function for enumerating ISO packets of an isochronous transfer context.</Summary>
        Public Overridable Function EnumPackets(ByVal pEnumPackets As KISO_ENUM_PACKETS_CB, ByVal StartPacketIndex As Integer, ByVal UserState As IntPtr) As Boolean
            Return IsoK_EnumPackets(mHandleStruct, pEnumPackets, StartPacketIndex, UserState)
        End Function

        ''' <Summary>Convenience function for re-using an isochronous transfer context in a subsequent request.</Summary>
        Public Overridable Function ReUse() As Boolean
            Return IsoK_ReUse(mHandleStruct)
        End Function

        <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Ansi, Pack:=1)>
        Private Structure KISO_CONTEXT_MAP
            ''' <Summary>Additional ISO transfer flags. See \ref KISO_FLAG.</Summary>
            Private ReadOnly Flags As KISO_FLAG

            ''' <Summary>Specifies the frame number that the transfer should begin on (0 for ASAP).</Summary>
            Private ReadOnly StartFrame As UInteger

            ''' <Summary>
            '''     Contains the number of packets that completed with an error condition on return from the host controller
            '''     driver.
            ''' </Summary>
            Private ReadOnly ErrorCount As Short

            ''' <Summary>Specifies the number of packets that are described by the variable-length array member \c IsoPacket.</Summary>
            Private ReadOnly NumberOfPackets As Short

            ''' <Summary>Contains the URB Hdr.Status value on return from the host controller driver.</Summary>
            Private ReadOnly UrbHdrStatus As UInteger
        End Structure
    End Class

    Public Class IsochK
        Implements IDisposable

        Protected mbDisposed As Boolean
        Protected mHandleStruct As KISOCH_HANDLE

        Protected Sub New()
        End Sub

        ''' <Summary>Creates a new isochronous transfer handle for libusbK or WinUSB.</Summary>
        Public Sub New(ByVal InterfaceHandle As KUSB_HANDLE, ByVal PipeId As Byte, ByVal MaxNumberOfPackets As UInteger, ByVal TransferBuffer As IntPtr, ByVal TransferBufferSize As UInteger)
            Dim success = IsochK_Init(mHandleStruct, InterfaceHandle, PipeId, MaxNumberOfPackets, TransferBuffer, TransferBufferSize)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
        End Sub

        ''' <summary>Gets the handle class structure.</summary>
        Public ReadOnly Property Handle As KISOCH_HANDLE
            Get
                Return mHandleStruct
            End Get
        End Property

        ''' <summary>Explicitly closes and frees the handle.</summary>
        Public Overridable Sub Dispose() Implements IDisposable.Dispose
            Dispose(True)
            GC.SuppressFinalize(Me)
        End Sub

        Protected Overrides Sub Finalize()
            Dispose(False)
        End Sub

        ''' <summary>Calls the dispose method.</summary>
        Public Overridable Sub Free()
            Dispose()
        End Sub

        Protected Overridable Sub Dispose(ByVal disposing As Boolean)
            If Not mbDisposed Then
                If mHandleStruct.Pointer <> IntPtr.Zero Then
                    IsochK_Free(mHandleStruct)
                    Call Debug.Print("{0} Dispose: Freed Handle:{1:X16}h Explicit:{2}", [GetType]().Name, mHandleStruct.Pointer.ToInt64(), disposing)
                Else
                    Call Debug.Print("{0} Dispose: [WARNING] Handle is null", [GetType]().Name)
                End If

                mHandleStruct = New KISOCH_HANDLE(IntPtr.Zero)
                mbDisposed = True
            End If
        End Sub

        ''' <Summary>Creates a new isochronous transfer handle for libusbK or WinUSB.</Summary>
        Protected Function Init(ByVal InterfaceHandle As KUSB_HANDLE, ByVal PipeId As Byte, ByVal MaxNumberOfPackets As UInteger, ByVal TransferBuffer As IntPtr, ByVal TransferBufferSize As UInteger) As Boolean
            Dim success = IsochK_Init(mHandleStruct, InterfaceHandle, PipeId, MaxNumberOfPackets, TransferBuffer, TransferBufferSize)
            If Not success Then Throw New Exception(String.Format("{0} failed initializing. ErrorCode={1:X8}h", [GetType]().Name, Marshal.GetLastWin32Error()))
            Call Debug.Print("{0} Init: handle 0x{1:X16}", [GetType]().Name, mHandleStruct.Pointer.ToInt64())
            Return True
        End Function

        ''' <Summary>Convenience function for setting the offsets and lengths of all ISO packets of an isochronous transfer handle.</Summary>
        Public Overridable Function SetPacketOffsets(ByVal PacketSize As UInteger) As Boolean
            Return IsochK_SetPacketOffsets(mHandleStruct, PacketSize)
        End Function

        ''' <Summary>Convenience function for setting all fields in an isochronous transfer packet.</Summary>
        Public Overridable Function SetPacket(ByVal PacketIndex As UInteger, ByVal Offset As UInteger, ByVal Length As UInteger, ByVal Status As UInteger) As Boolean
            Return IsochK_SetPacket(mHandleStruct, PacketIndex, Offset, Length, Status)
        End Function

        ''' <Summary>Convenience function for getting all fields in an isochronous transfer packet.</Summary>
        Public Overridable Function GetPacket(ByVal PacketIndex As UInteger, <Out> ByRef Offset As UInteger, <Out> ByRef Length As UInteger, <Out> ByRef Status As UInteger) As Boolean
            Return IsochK_GetPacket(mHandleStruct, PacketIndex, Offset, Length, Status)
        End Function

        ''' <Summary>Convenience function for enumerating ISO packets of an isochronous transfer context.</Summary>
        Public Overridable Function EnumPackets(ByVal pEnumPackets As KISOCH_ENUM_PACKETS_CB, ByVal StartPacketIndex As UInteger, ByVal UserState As IntPtr) As Boolean
            Return IsochK_EnumPackets(mHandleStruct, pEnumPackets, StartPacketIndex, UserState)
        End Function

        ''' <Summary>Helper function for isochronous packet/transfer calculations.</Summary>
        Public Shared Function CalcPacketInformation(ByVal IsHighSpeed As Boolean, ByRef PipeInformationEx As WINUSB_PIPE_INFORMATION_EX, <Out> ByRef PacketInformation As KISOCH_PACKET_INFORMATION) As Boolean
            Return IsochK_CalcPacketInformation(IsHighSpeed, PipeInformationEx, PacketInformation)
        End Function

        ''' <Summary>Gets the number of iso packets that will be used.</Summary>
        Public Overridable Function GetNumberOfPackets(<Out> ByRef NumberOfPackets As UInteger) As Boolean
            Return IsochK_GetNumberOfPackets(mHandleStruct, NumberOfPackets)
        End Function

        ''' <Summary>Sets the number of iso packets that will be used.</Summary>
        Public Overridable Function SetNumberOfPackets(ByVal NumberOfPackets As UInteger) As Boolean
            Return IsochK_SetNumberOfPackets(mHandleStruct, NumberOfPackets)
        End Function
    End Class
End Namespace
