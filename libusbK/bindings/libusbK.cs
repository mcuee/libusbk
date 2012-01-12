#region Copyright (c) Travis Robinson
// Copyright (c) 2011 Travis Robinson <libusbdotnet@gmail.com>
// All rights reserved.
//
// C# libusbK Bindings
// Auto-generated on: 12.03.2011
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


using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;


// ReSharper disable InconsistentNaming
// ReSharper disable CheckNamespace
// ReSharper disable UnassignedReadonlyField

namespace libusbK
{
    public static class Constants
    {
        public const int KLST_STRING_MAX_LEN = 256;
        public const string LIBUSBK_DLL = "libusbK.dll";
    }


    #region Base KLIB Handle
    public abstract class KLIB_HANDLE : SafeHandleZeroOrMinusOneIsInvalid, IKLIB_HANDLE
    {
        protected KLIB_HANDLE(bool ownsHandle) : base(ownsHandle)
        {
        }

        protected KLIB_HANDLE() : this(true)
        {
        }

        protected KLIB_HANDLE(IntPtr handlePtrToWrap) : base(false)
        {
            SetHandle(handlePtrToWrap);
        }


        #region IKLIB_HANDLE Members
        public abstract KLIB_HANDLE_TYPE HandleType { get; }

        public IntPtr GetContext()
        {
            return Functions.LibK_GetContext(handle, HandleType);
        }

        public bool SetContext(IntPtr ContextValue)
        {
            return Functions.LibK_SetContext(handle, HandleType, ContextValue);
        }

        public bool SetCleanupCallback(KLIB_HANDLE_CLEANUP_CB CleanupCallback)
        {
            return Functions.LibK_SetCleanupCallback(handle, HandleType, CleanupCallback);
        }
        #endregion


        protected abstract override bool ReleaseHandle();
    }

    public interface IKLIB_HANDLE
    {
        KLIB_HANDLE_TYPE HandleType { get; }
        IntPtr DangerousGetHandle();
        IntPtr GetContext();
        bool SetContext(IntPtr UserContext);
        bool SetCleanupCallback(KLIB_HANDLE_CLEANUP_CB CleanupCallback);
    }
    #endregion


    #region Class Handles
    public class KHOT_HANDLE : KLIB_HANDLE
    {
        public static KHOT_HANDLE Empty
        {
            get { return new KHOT_HANDLE(); }
        }

        public override KLIB_HANDLE_TYPE HandleType
        {
            get { return KLIB_HANDLE_TYPE.HOTK; }
        }

        protected override bool ReleaseHandle()
        {
            return Functions.HotK_Free(handle);
        }
    }

    public class KUSB_HANDLE : KLIB_HANDLE
    {
        public static KUSB_HANDLE Empty
        {
            get { return new KUSB_HANDLE(); }
        }

        public override KLIB_HANDLE_TYPE HandleType
        {
            get { return KLIB_HANDLE_TYPE.USBK; }
        }

        protected override bool ReleaseHandle()
        {
            {
                return Functions.UsbK_Free(handle);
            }
        }


        #region USB Shared Device Context
        public IntPtr GetSharedContext()
        {
            return Functions.LibK_GetContext(handle, KLIB_HANDLE_TYPE.USBSHAREDK);
        }

        public bool SetSharedContext(IntPtr SharedUserContext)
        {
            return Functions.LibK_SetContext(handle, KLIB_HANDLE_TYPE.USBSHAREDK, SharedUserContext);
        }

        public bool SetSharedCleanupCallback(KLIB_HANDLE_CLEANUP_CB CleanupCallback)
        {
            return Functions.LibK_SetCleanupCallback(handle, KLIB_HANDLE_TYPE.USBSHAREDK, CleanupCallback);
        }
        #endregion
    }

    public class KLST_HANDLE : KLIB_HANDLE
    {
        public static KLST_HANDLE Empty
        {
            get { return new KLST_HANDLE(); }
        }

        public override KLIB_HANDLE_TYPE HandleType
        {
            get { return KLIB_HANDLE_TYPE.LSTK; }
        }

        protected override bool ReleaseHandle()
        {
            return Functions.LstK_Free(handle);
        }
    }

    public class KOVL_POOL_HANDLE : KLIB_HANDLE
    {
        public static KOVL_POOL_HANDLE Empty
        {
            get { return new KOVL_POOL_HANDLE(); }
        }

        public override KLIB_HANDLE_TYPE HandleType
        {
            get { return KLIB_HANDLE_TYPE.OVLPOOLK; }
        }

        protected override bool ReleaseHandle()
        {
            return Functions.OvlK_Free(handle);
        }
    }

    public class KSTM_HANDLE : KLIB_HANDLE
    {
        public static KSTM_HANDLE Empty
        {
            get { return new KSTM_HANDLE(); }
        }

        public override KLIB_HANDLE_TYPE HandleType
        {
            get { return KLIB_HANDLE_TYPE.STMK; }
        }

        protected override bool ReleaseHandle()
        {
            return Functions.StmK_Free(handle);
        }
    }

    public class KISO_CONTEXT : SafeHandleZeroOrMinusOneIsInvalid
    {
        protected KISO_CONTEXT(bool ownsHandle) : base(ownsHandle)
        {
        }

        protected KISO_CONTEXT() : this(true)
        {
        }

        protected KISO_CONTEXT(IntPtr handlePtrToWrap) : base(false)
        {
            SetHandle(handlePtrToWrap);
        }

        protected override bool ReleaseHandle()
        {
            Functions.IsoK_Free(handle);
            handle = IntPtr.Zero;
            return true;
        }
    }
    #endregion


    #region Struct Handles
    public struct KOVL_HANDLE : IKLIB_HANDLE
    {
        private readonly IntPtr handle;

        public static KOVL_HANDLE Empty
        {
            get { return new KOVL_HANDLE(); }
        }


        #region IKLIB_HANDLE Members
        public KLIB_HANDLE_TYPE HandleType
        {
            get { return KLIB_HANDLE_TYPE.OVLK; }
        }

        public IntPtr GetContext()
        {
            return Functions.LibK_GetContext(handle, HandleType);
        }

        public bool SetContext(IntPtr ContextValue)
        {
            return Functions.LibK_SetContext(handle, HandleType, ContextValue);
        }

        public bool SetCleanupCallback(KLIB_HANDLE_CLEANUP_CB CleanupCallback)
        {
            return Functions.LibK_SetCleanupCallback(handle, HandleType, CleanupCallback);
        }

        public IntPtr DangerousGetHandle()
        {
            return handle;
        }
        #endregion
    }

    public struct KLST_DEVINFO_HANDLE : IKLIB_HANDLE
    {
        private static readonly int ofsClassGUID = Marshal.OffsetOf(typeof (KLST_DEVINFO), "ClassGUID").ToInt32();
        private static readonly int ofsCommon = Marshal.OffsetOf(typeof (KLST_DEVINFO), "Common").ToInt32();
        private static readonly int ofsConnected = Marshal.OffsetOf(typeof (KLST_DEVINFO), "Connected").ToInt32();
        private static readonly int ofsDeviceDesc = Marshal.OffsetOf(typeof (KLST_DEVINFO), "DeviceDesc").ToInt32();
        private static readonly int ofsDeviceInterfaceGUID = Marshal.OffsetOf(typeof (KLST_DEVINFO), "DeviceInterfaceGUID").ToInt32();
        private static readonly int ofsDevicePath = Marshal.OffsetOf(typeof (KLST_DEVINFO), "DevicePath").ToInt32();
        private static readonly int ofsDriverID = Marshal.OffsetOf(typeof (KLST_DEVINFO), "DriverID").ToInt32();
        private static readonly int ofsInstanceID = Marshal.OffsetOf(typeof (KLST_DEVINFO), "InstanceID").ToInt32();
        private static readonly int ofsLUsb0FilterIndex = Marshal.OffsetOf(typeof (KLST_DEVINFO), "LUsb0FilterIndex").ToInt32();
        private static readonly int ofsMfg = Marshal.OffsetOf(typeof (KLST_DEVINFO), "Mfg").ToInt32();
        private static readonly int ofsService = Marshal.OffsetOf(typeof (KLST_DEVINFO), "Service").ToInt32();
        private static readonly int ofsSymbolicLink = Marshal.OffsetOf(typeof (KLST_DEVINFO), "SymbolicLink").ToInt32();
        private static readonly int ofsSyncFlags = Marshal.OffsetOf(typeof (KLST_DEVINFO), "SyncFlags").ToInt32();

        private readonly IntPtr handle;

        public KLST_DEVINFO_HANDLE(IntPtr HandleToWrap)
        {
            handle = HandleToWrap;
        }

        public static KLST_DEVINFO_HANDLE Empty
        {
            get { return new KLST_DEVINFO_HANDLE(); }
        }

        public KLST_SYNC_FLAG SyncFlags
        {
            get { return (KLST_SYNC_FLAG) Marshal.ReadInt32(handle, ofsSyncFlags); }
        }

        public bool Connected
        {
            get { return Marshal.ReadInt32(handle, ofsConnected) != 0; }
        }

        public int LUsb0FilterIndex
        {
            get { return Marshal.ReadInt32(handle, ofsLUsb0FilterIndex); }
        }

        public string DevicePath
        {
            get { return Marshal.PtrToStringAnsi(new IntPtr(handle.ToInt64() + ofsDevicePath)); }
        }

        public string SymbolicLink
        {
            get { return Marshal.PtrToStringAnsi(new IntPtr(handle.ToInt64() + ofsSymbolicLink)); }
        }

        public string Service
        {
            get { return Marshal.PtrToStringAnsi(new IntPtr(handle.ToInt64() + ofsService)); }
        }

        public string DeviceDesc
        {
            get { return Marshal.PtrToStringAnsi(new IntPtr(handle.ToInt64() + ofsDeviceDesc)); }
        }

        public string Mfg
        {
            get { return Marshal.PtrToStringAnsi(new IntPtr(handle.ToInt64() + ofsMfg)); }
        }

        public string ClassGUID
        {
            get { return Marshal.PtrToStringAnsi(new IntPtr(handle.ToInt64() + ofsClassGUID)); }
        }

        public string InstanceID
        {
            get { return Marshal.PtrToStringAnsi(new IntPtr(handle.ToInt64() + ofsInstanceID)); }
        }

        public string DeviceInterfaceGUID
        {
            get { return Marshal.PtrToStringAnsi(new IntPtr(handle.ToInt64() + ofsDeviceInterfaceGUID)); }
        }

        public int DriverID
        {
            get { return Marshal.ReadInt32(handle, ofsDriverID); }
        }

        public KLST_DEV_COMMON_INFO Common
        {
            get { return (KLST_DEV_COMMON_INFO) Marshal.PtrToStructure(new IntPtr(handle.ToInt64() + ofsCommon), typeof (KLST_DEV_COMMON_INFO)); }
        }


        #region IKLIB_HANDLE Members
        public KLIB_HANDLE_TYPE HandleType
        {
            get { return KLIB_HANDLE_TYPE.LSTINFOK; }
        }

        public IntPtr GetContext()
        {
            return Functions.LibK_GetContext(handle, HandleType);
        }

        public bool SetContext(IntPtr ContextValue)
        {
            return Functions.LibK_SetContext(handle, HandleType, ContextValue);
        }

        public IntPtr DangerousGetHandle()
        {
            return handle;
        }

        public bool SetCleanupCallback(KLIB_HANDLE_CLEANUP_CB CleanupCallback)
        {
            return Functions.LibK_SetCleanupCallback(handle, HandleType, CleanupCallback);
        }
        #endregion


        public override string ToString()
        {
            return
                string.Format(
                    "{{Common: {0}, DriverID: {1}, DeviceInterfaceGuid: {2}, InstanceId: {3}, ClassGuid: {4}, Mfg: {5}, DeviceDesc: {6}, Service: {7}, SymbolicLink: {8}, DevicePath: {9}, LUsb0FilterIndex: {10}, Connected: {11}, SyncFlags: {12}}}",
                    Common,
                    DriverID,
                    DeviceInterfaceGUID,
                    InstanceID,
                    ClassGUID,
                    Mfg,
                    DeviceDesc,
                    Service,
                    SymbolicLink,
                    DevicePath,
                    LUsb0FilterIndex,
                    Connected,
                    SyncFlags);
        }
    }
    #endregion


    #region Internal Function Imports
    internal static class Functions
    {
        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LibK_GetVersion",
            SetLastError = true)]
        public static extern void LibK_GetVersion([Out] out KLIB_VERSION Version);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LibK_GetContext",
            SetLastError = true)]
        public static extern IntPtr LibK_GetContext([In] IntPtr Handle, KLIB_HANDLE_TYPE HandleType);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LibK_SetContext",
            SetLastError = true)]
        public static extern bool LibK_SetContext([In] IntPtr Handle, KLIB_HANDLE_TYPE HandleType, IntPtr ContextValue);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LibK_SetCleanupCallback"
            , SetLastError = true)]
        public static extern bool LibK_SetCleanupCallback([In] IntPtr Handle, KLIB_HANDLE_TYPE HandleType, KLIB_HANDLE_CLEANUP_CB CleanupCB);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LibK_LoadDriverAPI",
            SetLastError = true)]
        public static extern bool LibK_LoadDriverAPI([Out] out KUSB_DRIVER_API DriverAPI, int DriverID);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LibK_CopyDriverAPI",
            SetLastError = true)]
        public static extern bool LibK_CopyDriverAPI([Out] out KUSB_DRIVER_API DriverAPI, [In] KUSB_HANDLE UsbHandle);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LibK_GetProcAddress",
            SetLastError = true)]
        public static extern bool LibK_GetProcAddress(IntPtr ProcAddress, int DriverID, int FunctionID);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_Init",
            SetLastError = true)]
        public static extern bool UsbK_Init([Out] out KUSB_HANDLE InterfaceHandle, [In] KLST_DEVINFO_HANDLE DevInfo);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_Free",
            SetLastError = true)]
        public static extern bool UsbK_Free([In] IntPtr InterfaceHandle);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_ClaimInterface",
            SetLastError = true)]
        public static extern bool UsbK_ClaimInterface([In] KUSB_HANDLE InterfaceHandle, byte NumberOrIndex, bool IsIndex);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_ReleaseInterface",
            SetLastError = true)]
        public static extern bool UsbK_ReleaseInterface([In] KUSB_HANDLE InterfaceHandle, byte NumberOrIndex, bool IsIndex);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_SetAltInterface",
            SetLastError = true)]
        public static extern bool UsbK_SetAltInterface([In] KUSB_HANDLE InterfaceHandle, byte NumberOrIndex, bool IsIndex, byte AltSettingNumber);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_GetAltInterface",
            SetLastError = true)]
        public static extern bool UsbK_GetAltInterface([In] KUSB_HANDLE InterfaceHandle, byte NumberOrIndex, bool IsIndex, out byte AltSettingNumber);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_GetDescriptor",
            SetLastError = true)]
        public static extern bool UsbK_GetDescriptor([In] KUSB_HANDLE InterfaceHandle,
                                                     byte DescriptorType,
                                                     byte Index,
                                                     ushort LanguageID,
                                                     IntPtr Buffer,
                                                     uint BufferLength,
                                                     out uint LengthTransferred);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_ControlTransfer",
            SetLastError = true)]
        public static extern bool UsbK_ControlTransfer([In] KUSB_HANDLE InterfaceHandle,
                                                       WINUSB_SETUP_PACKET SetupPacket,
                                                       IntPtr Buffer,
                                                       uint BufferLength,
                                                       out uint LengthTransferred,
                                                       IntPtr Overlapped);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_SetPowerPolicy",
            SetLastError = true)]
        public static extern bool UsbK_SetPowerPolicy([In] KUSB_HANDLE InterfaceHandle, uint PolicyType, uint ValueLength, IntPtr Value);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_GetPowerPolicy",
            SetLastError = true)]
        public static extern bool UsbK_GetPowerPolicy([In] KUSB_HANDLE InterfaceHandle, uint PolicyType, ref uint ValueLength, IntPtr Value);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_SetConfiguration",
            SetLastError = true)]
        public static extern bool UsbK_SetConfiguration([In] KUSB_HANDLE InterfaceHandle, byte ConfigurationNumber);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_GetConfiguration",
            SetLastError = true)]
        public static extern bool UsbK_GetConfiguration([In] KUSB_HANDLE InterfaceHandle, out byte ConfigurationNumber);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_ResetDevice",
            SetLastError = true)]
        public static extern bool UsbK_ResetDevice([In] KUSB_HANDLE InterfaceHandle);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_Initialize",
            SetLastError = true)]
        public static extern bool UsbK_Initialize(IntPtr DeviceHandle, [Out] out KUSB_HANDLE InterfaceHandle);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_SelectInterface",
            SetLastError = true)]
        public static extern bool UsbK_SelectInterface([In] KUSB_HANDLE InterfaceHandle, byte NumberOrIndex, bool IsIndex);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi,
            EntryPoint = "UsbK_GetAssociatedInterface", SetLastError = true)]
        public static extern bool UsbK_GetAssociatedInterface([In] KUSB_HANDLE InterfaceHandle,
                                                              byte AssociatedInterfaceIndex,
                                                              [Out] out KUSB_HANDLE AssociatedInterfaceHandle);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_Clone",
            SetLastError = true)]
        public static extern bool UsbK_Clone([In] KUSB_HANDLE InterfaceHandle, [Out] out KUSB_HANDLE DstInterfaceHandle);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi,
            EntryPoint = "UsbK_QueryInterfaceSettings", SetLastError = true)]
        public static extern bool UsbK_QueryInterfaceSettings([In] KUSB_HANDLE InterfaceHandle,
                                                              byte AltSettingNumber,
                                                              [Out] out USB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi,
            EntryPoint = "UsbK_QueryDeviceInformation", SetLastError = true)]
        public static extern bool UsbK_QueryDeviceInformation([In] KUSB_HANDLE InterfaceHandle,
                                                              uint InformationType,
                                                              ref uint BufferLength,
                                                              IntPtr Buffer);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi,
            EntryPoint = "UsbK_SetCurrentAlternateSetting", SetLastError = true)]
        public static extern bool UsbK_SetCurrentAlternateSetting([In] KUSB_HANDLE InterfaceHandle, byte AltSettingNumber);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi,
            EntryPoint = "UsbK_GetCurrentAlternateSetting", SetLastError = true)]
        public static extern bool UsbK_GetCurrentAlternateSetting([In] KUSB_HANDLE InterfaceHandle, out byte AltSettingNumber);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_QueryPipe",
            SetLastError = true)]
        public static extern bool UsbK_QueryPipe([In] KUSB_HANDLE InterfaceHandle,
                                                 byte AltSettingNumber,
                                                 byte PipeIndex,
                                                 [Out] out WINUSB_PIPE_INFORMATION PipeInformation);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_SetPipePolicy",
            SetLastError = true)]
        public static extern bool UsbK_SetPipePolicy([In] KUSB_HANDLE InterfaceHandle, byte PipeID, uint PolicyType, uint ValueLength, IntPtr Value);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_GetPipePolicy",
            SetLastError = true)]
        public static extern bool UsbK_GetPipePolicy([In] KUSB_HANDLE InterfaceHandle,
                                                     byte PipeID,
                                                     uint PolicyType,
                                                     ref uint ValueLength,
                                                     IntPtr Value);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_ReadPipe",
            SetLastError = true)]
        public static extern bool UsbK_ReadPipe([In] KUSB_HANDLE InterfaceHandle,
                                                byte PipeID,
                                                IntPtr Buffer,
                                                uint BufferLength,
                                                out uint LengthTransferred,
                                                IntPtr Overlapped);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_WritePipe",
            SetLastError = true)]
        public static extern bool UsbK_WritePipe([In] KUSB_HANDLE InterfaceHandle,
                                                 byte PipeID,
                                                 IntPtr Buffer,
                                                 uint BufferLength,
                                                 out uint LengthTransferred,
                                                 IntPtr Overlapped);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_ResetPipe",
            SetLastError = true)]
        public static extern bool UsbK_ResetPipe([In] KUSB_HANDLE InterfaceHandle, byte PipeID);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_AbortPipe",
            SetLastError = true)]
        public static extern bool UsbK_AbortPipe([In] KUSB_HANDLE InterfaceHandle, byte PipeID);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_FlushPipe",
            SetLastError = true)]
        public static extern bool UsbK_FlushPipe([In] KUSB_HANDLE InterfaceHandle, byte PipeID);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_IsoReadPipe",
            SetLastError = true)]
        public static extern bool UsbK_IsoReadPipe([In] KUSB_HANDLE InterfaceHandle,
                                                   byte PipeID,
                                                   IntPtr Buffer,
                                                   uint BufferLength,
                                                   IntPtr Overlapped,
                                                   [In] KISO_CONTEXT IsoContext);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_IsoWritePipe",
            SetLastError = true)]
        public static extern bool UsbK_IsoWritePipe([In] KUSB_HANDLE InterfaceHandle,
                                                    byte PipeID,
                                                    IntPtr Buffer,
                                                    uint BufferLength,
                                                    IntPtr Overlapped,
                                                    [In] KISO_CONTEXT IsoContext);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi,
            EntryPoint = "UsbK_GetCurrentFrameNumber", SetLastError = true)]
        public static extern bool UsbK_GetCurrentFrameNumber([In] KUSB_HANDLE InterfaceHandle, out uint FrameNumber);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi,
            EntryPoint = "UsbK_GetOverlappedResult", SetLastError = true)]
        public static extern bool UsbK_GetOverlappedResult([In] KUSB_HANDLE InterfaceHandle,
                                                           IntPtr lpOverlapped,
                                                           out uint lpNumberOfBytesTransferred,
                                                           bool bWait);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "UsbK_GetProperty",
            SetLastError = true)]
        public static extern bool UsbK_GetProperty([In] KUSB_HANDLE InterfaceHandle,
                                                   KUSB_PROPERTY PropertyType,
                                                   ref uint PropertySize,
                                                   IntPtr Property);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LstK_Init",
            SetLastError = true)]
        public static extern bool LstK_Init([Out] out KLST_HANDLE DeviceList, KLST_FLAG Flags);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LstK_Free",
            SetLastError = true)]
        public static extern bool LstK_Free([In] IntPtr DeviceList);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LstK_Enumerate",
            SetLastError = true)]
        public static extern bool LstK_Enumerate([In] KLST_HANDLE DeviceList, KLST_ENUM_DEVINFO_CB EnumDevListCB, IntPtr Context);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LstK_Current",
            SetLastError = true)]
        public static extern bool LstK_Current([In] KLST_HANDLE DeviceList, [Out] out KLST_DEVINFO_HANDLE DeviceInfo);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LstK_MoveNext",
            SetLastError = true)]
        public static extern bool LstK_MoveNext([In] KLST_HANDLE DeviceList, [Out] out KLST_DEVINFO_HANDLE DeviceInfo);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LstK_MoveReset",
            SetLastError = true)]
        public static extern void LstK_MoveReset([In] KLST_HANDLE DeviceList);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LstK_FindByVidPid",
            SetLastError = true)]
        public static extern bool LstK_FindByVidPid([In] KLST_HANDLE DeviceList, int Vid, int Pid, [Out] out KLST_DEVINFO_HANDLE DeviceInfo);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "LstK_Count",
            SetLastError = true)]
        public static extern bool LstK_Count([In] KLST_HANDLE DeviceList, ref uint Count);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "HotK_Init",
            SetLastError = true)]
        public static extern bool HotK_Init([Out] out KHOT_HANDLE Handle, [In, Out] ref KHOT_PARAMS InitParams);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "HotK_Free",
            SetLastError = true)]
        public static extern bool HotK_Free([In] IntPtr Handle);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "HotK_FreeAll",
            SetLastError = true)]
        public static extern void HotK_FreeAll();

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "OvlK_Acquire",
            SetLastError = true)]
        public static extern bool OvlK_Acquire([Out] out KOVL_HANDLE OverlappedK, [In] KOVL_POOL_HANDLE PoolHandle);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "OvlK_Release",
            SetLastError = true)]
        public static extern bool OvlK_Release([In] KOVL_HANDLE OverlappedK);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "OvlK_Init",
            SetLastError = true)]
        public static extern bool OvlK_Init([Out] out KOVL_POOL_HANDLE PoolHandle,
                                            [In] KUSB_HANDLE UsbHandle,
                                            int MaxOverlappedCount,
                                            KOVL_POOL_FLAG Flags);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "OvlK_Free",
            SetLastError = true)]
        public static extern bool OvlK_Free([In] IntPtr PoolHandle);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "OvlK_GetEventHandle",
            SetLastError = true)]
        public static extern IntPtr OvlK_GetEventHandle([In] KOVL_HANDLE OverlappedK);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "OvlK_Wait",
            SetLastError = true)]
        public static extern bool OvlK_Wait([In] KOVL_HANDLE OverlappedK, int TimeoutMS, KOVL_WAIT_FLAG WaitFlags, out uint TransferredLength);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "OvlK_WaitOrCancel",
            SetLastError = true)]
        public static extern bool OvlK_WaitOrCancel([In] KOVL_HANDLE OverlappedK, int TimeoutMS, out uint TransferredLength);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "OvlK_WaitAndRelease",
            SetLastError = true)]
        public static extern bool OvlK_WaitAndRelease([In] KOVL_HANDLE OverlappedK, int TimeoutMS, out uint TransferredLength);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "OvlK_IsComplete",
            SetLastError = true)]
        public static extern bool OvlK_IsComplete([In] KOVL_HANDLE OverlappedK);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "OvlK_ReUse",
            SetLastError = true)]
        public static extern bool OvlK_ReUse([In] KOVL_HANDLE OverlappedK);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "StmK_Init",
            SetLastError = true)]
        public static extern bool StmK_Init([Out] out KSTM_HANDLE StreamHandle,
                                            [In] KUSB_HANDLE UsbHandle,
                                            byte PipeID,
                                            int MaxTransferSize,
                                            int MaxPendingTransfers,
                                            int MaxPendingIO,
                                            [In] ref KSTM_CALLBACK Callbacks,
                                            KSTM_FLAG Flags);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "StmK_Free",
            SetLastError = true)]
        public static extern bool StmK_Free([In] IntPtr StreamHandle);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "StmK_Start",
            SetLastError = true)]
        public static extern bool StmK_Start([In] KSTM_HANDLE StreamHandle);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "StmK_Stop",
            SetLastError = true)]
        public static extern bool StmK_Stop([In] KSTM_HANDLE StreamHandle, int TimeoutCancelMS);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "StmK_Read",
            SetLastError = true)]
        public static extern bool StmK_Read([In] KSTM_HANDLE StreamHandle, IntPtr Buffer, int Offset, int Length, out uint TransferredLength);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "StmK_Write",
            SetLastError = true)]
        public static extern bool StmK_Write([In] KSTM_HANDLE StreamHandle, IntPtr Buffer, int Offset, int Length, out uint TransferredLength);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "IsoK_Init",
            SetLastError = true)]
        public static extern bool IsoK_Init([Out] out KISO_CONTEXT IsoContext, int NumberOfPackets, int StartFrame);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "IsoK_Free",
            SetLastError = true)]
        public static extern bool IsoK_Free([In] IntPtr IsoContext);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "IsoK_SetPackets",
            SetLastError = true)]
        public static extern bool IsoK_SetPackets([In] KISO_CONTEXT IsoContext, int PacketSize);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "IsoK_SetPacket",
            SetLastError = true)]
        public static extern bool IsoK_SetPacket([In] KISO_CONTEXT IsoContext, int PacketIndex, [In] ref KISO_PACKET IsoPacket);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "IsoK_GetPacket",
            SetLastError = true)]
        public static extern bool IsoK_GetPacket([In] KISO_CONTEXT IsoContext, int PacketIndex, [Out] out KISO_PACKET IsoPacket);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "IsoK_EnumPackets",
            SetLastError = true)]
        public static extern bool IsoK_EnumPackets([In] KISO_CONTEXT IsoContext,
                                                   KISO_ENUM_PACKETS_CB EnumPackets,
                                                   int StartPacketIndex,
                                                   IntPtr UserState);

        [DllImport(Constants.LIBUSBK_DLL, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Ansi, EntryPoint = "IsoK_ReUse",
            SetLastError = true)]
        public static extern bool IsoK_ReUse([In] KISO_CONTEXT IsoContext);
    }
    #endregion


    #region Enumerations
    /// <Summary>Values used in the \c bmAttributes field of a \ref USB_ENDPOINT_DESCRIPTOR</Summary>
    public enum USBD_PIPE_TYPE
    {
        /// <Summary>Indicates a control endpoint</Summary>
        UsbdPipeTypeControl,

        /// <Summary>Indicates an isochronous endpoint</Summary>
        UsbdPipeTypeIsochronous,

        /// <Summary>Indicates a bulk endpoint</Summary>
        UsbdPipeTypeBulk,

        /// <Summary>Indicates an interrupt endpoint</Summary>
        UsbdPipeTypeInterrupt,
    }

    /// <Summary>Additional ISO transfer flags.</Summary>
    [Flags]
    public enum KISO_FLAG
    {
        NONE = 0,

        /// <Summary>Do not start the transfer immediately, instead use \ref KISO_CONTEXT::StartFrame.</Summary>
        SET_START_FRAME = 0x00000001,
    }

    /// <Summary>Handle type enumeration.</Summary>
    public enum KLIB_HANDLE_TYPE
    {
        /// <Summary>Hot plug handle. \ref KHOT_HANDLE</Summary>
        HOTK,

        /// <Summary>USB handle. \ref KUSB_HANDLE</Summary>
        USBK,

        /// <Summary>Shared USB handle. \ref KUSB_HANDLE</Summary>
        USBSHAREDK,

        /// <Summary>Device list handle. \ref KLST_HANDLE</Summary>
        LSTK,

        /// <Summary>Device info handle. \ref KLST_DEVINFO_HANDLE</Summary>
        LSTINFOK,

        /// <Summary>Overlapped handle. \ref KOVL_HANDLE</Summary>
        OVLK,

        /// <Summary>Overlapped pool handle. \ref KOVL_POOL_HANDLE</Summary>
        OVLPOOLK,

        /// <Summary>Pipe stream handle. \ref KSTM_HANDLE</Summary>
        STMK,

        /// <Summary>Max handle type count.</Summary>
        COUNT
    }

    /// <Summary>Device list sync flags.</Summary>
    [Flags]
    public enum KLST_SYNC_FLAG
    {
        /// <Summary>Cleared/invalid state.</Summary>
        NONE = 0,

        /// <Summary>Unchanged state,</Summary>
        UNCHANGED = 0x0001,

        /// <Summary>Added (Arrival) state,</Summary>
        ADDED = 0x0002,

        /// <Summary>Removed (Unplugged) state,</Summary>
        REMOVED = 0x0004,

        /// <Summary>Connect changed state.</Summary>
        CONNECT_CHANGE = 0x0008,

        /// <Summary>All states.</Summary>
        MASK = 0x000F,
    }

    /// <Summary>Initialization parameters for \ref LstK_Init</Summary>
    [Flags]
    public enum KLST_FLAG
    {
        /// <Summary>No flags (or 0)</Summary>
        NONE = 0,

        /// <Summary>Enable listings for the raw device interface GUID.{A5DCBF10-6530-11D2-901F-00C04FB951ED}</Summary>
        INCLUDE_RAWGUID = 0x0001,

        /// <Summary>List libusbK devices that not currently connected.</Summary>
        INCLUDE_DISCONNECT = 0x0002,
    }

    /// <Summary>bmRequest.Dir</Summary>
    public enum BMREQUEST_DIR
    {
        HOST_TO_DEVICE = 0,
        DEVICE_TO_HOST = 1,
    }

    /// <Summary>bmRequest.Type</Summary>
    public enum BMREQUEST_TYPE
    {
        /// <Summary>Standard request. See \ref USB_REQUEST_ENUM</Summary>
        STANDARD = 0,

        /// <Summary>Class-specific request.</Summary>
        CLASS = 1,

        /// <Summary>Vendor-specific request</Summary>
        VENDOR = 2,
    }

    /// <Summary>bmRequest.Recipient</Summary>
    public enum BMREQUEST_RECIPIENT
    {
        /// <Summary>Request is for a device.</Summary>
        DEVICE = 0,

        /// <Summary>Request is for an interface of a device.</Summary>
        INTERFACE = 1,

        /// <Summary>Request is for an endpoint of a device.</Summary>
        ENDPOINT = 2,

        /// <Summary>Request is for a vendor-specific purpose.</Summary>
        OTHER = 3,
    }

    /// <Summary>Values for the bits returned by the \ref USB_REQUEST_GET_STATUS request.</Summary>
    public enum USB_GETSTATUS
    {
        /// <Summary>Device is self powered</Summary>
        SELF_POWERED = 0x01,

        /// <Summary>Device can wake the system from a low power/sleeping state.</Summary>
        REMOTE_WAKEUP_ENABLED = 0x02
    }

    /// <Summary>Standard USB descriptor types. For more information, see section 9-5 of the USB 3.0 specifications.</Summary>
    public enum USB_DESCRIPTOR_TYPE
    {
        /// <Summary>Device descriptor type.</Summary>
        DEVICE = 0x01,

        /// <Summary>Configuration descriptor type.</Summary>
        CONFIGURATION = 0x02,

        /// <Summary>String descriptor type.</Summary>
        STRING = 0x03,

        /// <Summary>Interface descriptor type.</Summary>
        INTERFACE = 0x04,

        /// <Summary>Endpoint descriptor type.</Summary>
        ENDPOINT = 0x05,

        /// <Summary>Device qualifier descriptor type.</Summary>
        DEVICE_QUALIFIER = 0x06,

        /// <Summary>Config power descriptor type.</Summary>
        CONFIG_POWER = 0x07,

        /// <Summary>Interface power descriptor type.</Summary>
        INTERFACE_POWER = 0x08,

        /// <Summary>Interface association descriptor type.</Summary>
        INTERFACE_ASSOCIATION = 0x0B,
    }

    /// <Summary>Usb handle specific properties that can be retrieved with \ref UsbK_GetProperty.</Summary>
    public enum KUSB_PROPERTY
    {
        /// <Summary>Get the internal device file handle used for operations such as GetOverlappedResult or DeviceIoControl.</Summary>
        DEVICE_FILE_HANDLE,

        COUNT
    }

    /// <Summary>Supported driver id enumeration.</Summary>
    public enum KUSB_DRVID
    {
        /// <Summary>libusbK.sys driver ID</Summary>
        LIBUSBK,

        /// <Summary>libusb0.sys driver ID</Summary>
        LIBUSB0,

        /// <Summary>WinUSB.sys driver ID</Summary>
        WINUSB,

        /// <Summary>libusb0.sys filter driver ID</Summary>
        LIBUSB0_FILTER,

        /// <Summary>Supported driver count</Summary>
        COUNT
    }

    /// <Summary>Supported function id enumeration.</Summary>
    public enum KUSB_FNID
    {
        /// <Summary>\ref UsbK_Init dynamic driver function id.</Summary>
        Init,

        /// <Summary>\ref UsbK_Free dynamic driver function id.</Summary>
        Free,

        /// <Summary>\ref UsbK_ClaimInterface dynamic driver function id.</Summary>
        ClaimInterface,

        /// <Summary>\ref UsbK_ReleaseInterface dynamic driver function id.</Summary>
        ReleaseInterface,

        /// <Summary>\ref UsbK_SetAltInterface dynamic driver function id.</Summary>
        SetAltInterface,

        /// <Summary>\ref UsbK_GetAltInterface dynamic driver function id.</Summary>
        GetAltInterface,

        /// <Summary>\ref UsbK_GetDescriptor dynamic driver function id.</Summary>
        GetDescriptor,

        /// <Summary>\ref UsbK_ControlTransfer dynamic driver function id.</Summary>
        ControlTransfer,

        /// <Summary>\ref UsbK_SetPowerPolicy dynamic driver function id.</Summary>
        SetPowerPolicy,

        /// <Summary>\ref UsbK_GetPowerPolicy dynamic driver function id.</Summary>
        GetPowerPolicy,

        /// <Summary>\ref UsbK_SetConfiguration dynamic driver function id.</Summary>
        SetConfiguration,

        /// <Summary>\ref UsbK_GetConfiguration dynamic driver function id.</Summary>
        GetConfiguration,

        /// <Summary>\ref UsbK_ResetDevice dynamic driver function id.</Summary>
        ResetDevice,

        /// <Summary>\ref UsbK_Initialize dynamic driver function id.</Summary>
        Initialize,

        /// <Summary>\ref UsbK_SelectInterface dynamic driver function id.</Summary>
        SelectInterface,

        /// <Summary>\ref UsbK_GetAssociatedInterface dynamic driver function id.</Summary>
        GetAssociatedInterface,

        /// <Summary>\ref UsbK_Clone dynamic driver function id.</Summary>
        Clone,

        /// <Summary>\ref UsbK_QueryInterfaceSettings dynamic driver function id.</Summary>
        QueryInterfaceSettings,

        /// <Summary>\ref UsbK_QueryDeviceInformation dynamic driver function id.</Summary>
        QueryDeviceInformation,

        /// <Summary>\ref UsbK_SetCurrentAlternateSetting dynamic driver function id.</Summary>
        SetCurrentAlternateSetting,

        /// <Summary>\ref UsbK_GetCurrentAlternateSetting dynamic driver function id.</Summary>
        GetCurrentAlternateSetting,

        /// <Summary>\ref UsbK_QueryPipe dynamic driver function id.</Summary>
        QueryPipe,

        /// <Summary>\ref UsbK_SetPipePolicy dynamic driver function id.</Summary>
        SetPipePolicy,

        /// <Summary>\ref UsbK_GetPipePolicy dynamic driver function id.</Summary>
        GetPipePolicy,

        /// <Summary>\ref UsbK_ReadPipe dynamic driver function id.</Summary>
        ReadPipe,

        /// <Summary>\ref UsbK_WritePipe dynamic driver function id.</Summary>
        WritePipe,

        /// <Summary>\ref UsbK_ResetPipe dynamic driver function id.</Summary>
        ResetPipe,

        /// <Summary>\ref UsbK_AbortPipe dynamic driver function id.</Summary>
        AbortPipe,

        /// <Summary>\ref UsbK_FlushPipe dynamic driver function id.</Summary>
        FlushPipe,

        /// <Summary>\ref UsbK_IsoReadPipe dynamic driver function id.</Summary>
        IsoReadPipe,

        /// <Summary>\ref UsbK_IsoWritePipe dynamic driver function id.</Summary>
        IsoWritePipe,

        /// <Summary>\ref UsbK_GetCurrentFrameNumber dynamic driver function id.</Summary>
        GetCurrentFrameNumber,

        /// <Summary>\ref UsbK_GetOverlappedResult dynamic driver function id.</Summary>
        GetOverlappedResult,

        /// <Summary>\ref UsbK_GetProperty dynamic driver function id.</Summary>
        GetProperty,

        /// <Summary>Supported function count</Summary>
        COUNT,
    }

    /// <Summary>Hot plug config flags.</Summary>
    [Flags]
    public enum KHOT_FLAG
    {
        /// <Summary>No flags (or 0)</Summary>
        NONE,

        /// <Summary>Notify all devices which match upon a succuessful call to \ref HotK_Init.</Summary>
        PLUG_ALL_ON_INIT = 0x0001,

        /// <Summary>Allow other \ref KHOT_HANDLE instances to consume this match.</Summary>
        PASS_DUPE_INSTANCE = 0x0002,

        /// <Summary>If a \c UserHwnd is specified, use \c PostMessage instead of \c SendMessage.</Summary>
        POST_USER_MESSAGE = 0x0004,
    }

    /// <Summary>\c WaitFlags used by \ref OvlK_Wait.</Summary>
    [Flags]
    public enum KOVL_WAIT_FLAG
    {
        /// <Summary>Do not perform any additional actions upon exiting \ref OvlK_Wait.</Summary>
        NONE = 0,

        /// <Summary>If the i/o operation completes successfully, release the OverlappedK back to it's pool.</Summary>
        RELEASE_ON_SUCCESS = 0x0001,

        /// <Summary>If the i/o operation fails, release the OverlappedK back to it's pool.</Summary>
        RELEASE_ON_FAIL = 0x0002,

        /// <Summary>If the i/o operation fails or completes successfully, release the OverlappedK back to its pool. Perform no actions if it times-out.</Summary>
        RELEASE_ON_SUCCESS_FAIL = 0x0003,

        /// <Summary>If the i/o operation times-out cancel it, but do not release the OverlappedK back to its pool.</Summary>
        CANCEL_ON_TIMEOUT = 0x0004,

        /// <Summary>If the i/o operation times-out, cancel it and release the OverlappedK back to its pool.</Summary>
        RELEASE_ON_TIMEOUT = 0x000C,

        /// <Summary>Always release the OverlappedK back to its pool.  If the operation timed-out, cancel it before releasing back to its pool.</Summary>
        RELEASE_ALWAYS = 0x000F,

        /// <Summary>Uses alterable wait functions.  See http://msdn.microsoft.com/en-us/library/windows/desktop/ms687036%28v=vs.85%29.aspx</Summary>
        ALERTABLE = 0x0010,
    }

    /// <Summary>\c Overlapped pool config flags.</Summary>
    [Flags]
    public enum KOVL_POOL_FLAG
    {
        NONE = 0,
    }

    /// <Summary>Stream config flags.</Summary>
    [Flags]
    public enum KSTM_FLAG
    {
        NONE = 0,
    }

    /// <Summary>Stream config flags.</Summary>
    public enum KSTM_COMPLETE_RESULT
    {
        VALID = 0,
        INVALID,
    }
    #endregion


    #region Structs
    /// <Summary>The \c WINUSB_PIPE_INFORMATION structure contains pipe information that the \ref UsbK_QueryPipe routine retrieves.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct WINUSB_PIPE_INFORMATION
    {
        /// <Summary>A \c USBD_PIPE_TYPE enumeration value that specifies the pipe type</Summary>
        public USBD_PIPE_TYPE PipeType;

        /// <Summary>The pipe identifier (ID)</Summary>
        public byte PipeId;

        /// <Summary>The maximum size, in bytes, of the packets that are transmitted on the pipe</Summary>
        public ushort MaximumPacketSize;

        /// <Summary>The pipe interval</Summary>
        public byte Interval;
    };

    /// <Summary>The \c WINUSB_SETUP_PACKET structure describes a USB setup packet.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct WINUSB_SETUP_PACKET
    {
        /// <Summary>The request type. The values that are assigned to this member are defined in Table 9.2 of section 9.3 of the Universal Serial Bus (USB) specification (www.usb.org).</Summary>
        public byte RequestType;

        /// <Summary>The device request. The values that are assigned to this member are defined in Table 9.3 of section 9.4 of the Universal Serial Bus (USB) specification.</Summary>
        public byte Request;

        /// <Summary>The meaning of this member varies according to the request. For an explanation of this member, see the Universal Serial Bus (USB) specification.</Summary>
        public ushort Value;

        /// <Summary>The meaning of this member varies according to the request. For an explanation of this member, see the Universal Serial Bus (USB) specification.</Summary>
        public ushort Index;

        /// <Summary>The number of bytes to transfer. (not including the \c WINUSB_SETUP_PACKET itself)</Summary>
        public ushort Length;
    };

    /// <Summary>Structure describing an isochronous transfer packet.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct KISO_PACKET
    {
        /// <Summary>Specifies the offset, in bytes, of the buffer for this packet from the beginning of the entire isochronous transfer data buffer.</Summary>
        public uint Offset;

        /// <Summary>Set by the host controller to indicate the actual number of bytes received by the device for isochronous IN transfers. Length not used for isochronous OUT transfers.</Summary>
        public ushort Length;

        /// <Summary>Contains the 16 least significant USBD status bits, on return from the host controller driver, of this transfer packet.</Summary>
        public ushort Status;
    };

    /// <summary>KISO_CONTEXT_MAP is used for calculating field offsets only as it lacks an IsoPackets field.</summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1, Size = 16)]
    internal struct KISO_CONTEXT_MAP
    {
        /// <Summary>Additional ISO transfer flags. See \ref KISO_FLAG.</Summary>
        public KISO_FLAG Flags;

        /// <Summary>Specifies the frame number that the transfer should begin on (0 for ASAP).</Summary>
        public uint StartFrame;

        /// <Summary>Contains the number of packets that completed with an error condition on return from the host controller driver.</Summary>
        public short ErrorCount;

        /// <Summary>Specifies the number of packets that are described by the variable-length array member \c IsoPacket.</Summary>
        public short NumberOfPackets;
    };

    /// <Summary>libusbK verson information structure.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct KLIB_VERSION
    {
        /// <Summary>Major version number.</Summary>
        public int Major;

        /// <Summary>Minor version number.</Summary>
        public int Minor;

        /// <Summary>Micro version number.</Summary>
        public int Micro;

        /// <Summary>Nano version number.</Summary>
        public int Nano;
    };

    /// <Summary>Common usb device information structure</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct KLST_DEV_COMMON_INFO
    {
        /// <Summary>VendorID parsed from \ref KLST_DEVINFO::InstanceID</Summary>
        public int Vid;

        /// <Summary>ProductID parsed from \ref KLST_DEVINFO::InstanceID</Summary>
        public int Pid;

        /// <Summary>Interface number (valid for composite devices only) parsed from \ref KLST_DEVINFO::InstanceID</Summary>
        public int MI;

        // An ID that uniquely identifies a USB device.
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string InstanceID;
    };

    /// <Summary>Semi-opaque device information structure of a device list.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct KLST_DEVINFO
    {
        /// <Summary>Common usb device information</Summary>
        public KLST_DEV_COMMON_INFO Common;

        /// <Summary>Driver id this device element is using</Summary>
        public int DriverID;

        /// <Summary>Device interface GUID</Summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string DeviceInterfaceGUID;

        /// <Summary>Device instance ID.</Summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string InstanceID;

        /// <Summary>Class GUID.</Summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string ClassGUID;

        /// <Summary>Manufacturer name as specified in the INF file.</Summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string Mfg;

        /// <Summary>Device description as specified in the INF file.</Summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string DeviceDesc;

        /// <Summary>Driver service name.</Summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string Service;

        /// <Summary>Unique identifier.</Summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string SymbolicLink;

        /// <Summary>physical device filename used with the Windows \c CreateFile()</Summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string DevicePath;

        /// <Summary>libusb-win32 filter index id.</Summary>
        public int LUsb0FilterIndex;

        /// <Summary>Indicates the devices connection state.</Summary>
        public bool Connected;

        /// <Summary>Synchronization flags. (internal use only)</Summary>
        public KLST_SYNC_FLAG SyncFlags;
    };

    /// <Summary>A structure representing the standard USB device descriptor.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct USB_DEVICE_DESCRIPTOR
    {
        /// <Summary>Size of this descriptor (in bytes)</Summary>
        public byte bLength;

        /// <Summary>Descriptor type</Summary>
        public byte bDescriptorType;

        /// <Summary>USB specification release number in binary-coded decimal.</Summary>
        public ushort bcdUSB;

        /// <Summary>USB-IF class code for the device</Summary>
        public byte bDeviceClass;

        /// <Summary>USB-IF subclass code for the device</Summary>
        public byte bDeviceSubClass;

        /// <Summary>USB-IF protocol code for the device</Summary>
        public byte bDeviceProtocol;

        /// <Summary>Maximum packet size for control endpoint 0</Summary>
        public byte bMaxPacketSize0;

        /// <Summary>USB-IF vendor ID</Summary>
        public ushort idVendor;

        /// <Summary>USB-IF product ID</Summary>
        public ushort idProduct;

        /// <Summary>Device release number in binary-coded decimal</Summary>
        public ushort bcdDevice;

        /// <Summary>Index of string descriptor describing manufacturer</Summary>
        public byte iManufacturer;

        /// <Summary>Index of string descriptor describing product</Summary>
        public byte iProduct;

        /// <Summary>Index of string descriptor containing device serial number</Summary>
        public byte iSerialNumber;

        /// <Summary>Number of possible configurations</Summary>
        public byte bNumConfigurations;
    };

    /// <Summary>A structure representing the standard USB endpoint descriptor.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct USB_ENDPOINT_DESCRIPTOR
    {
        /// <Summary>Size of this descriptor (in bytes)</Summary>
        public byte bLength;

        /// <Summary>Descriptor type</Summary>
        public byte bDescriptorType;

        /// <Summary>The address of the endpoint described by this descriptor.</Summary>
        public byte bEndpointAddress;

        /// <Summary>Attributes which apply to the endpoint when it is configured using the bConfigurationValue.</Summary>
        public byte bmAttributes;

        /// <Summary>Maximum packet size this endpoint is capable of sending/receiving.</Summary>
        public ushort wMaxPacketSize;

        /// <Summary>Interval for polling endpoint for data transfers.</Summary>
        public byte bInterval;
    };

    /// <Summary>A structure representing the standard USB configuration descriptor.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct USB_CONFIGURATION_DESCRIPTOR
    {
        /// <Summary>Size of this descriptor (in bytes)</Summary>
        public byte bLength;

        /// <Summary>Descriptor type</Summary>
        public byte bDescriptorType;

        /// <Summary>Total length of data returned for this configuration</Summary>
        public ushort wTotalLength;

        /// <Summary>Number of interfaces supported by this configuration</Summary>
        public byte bNumInterfaces;

        /// <Summary>Identifier value for this configuration</Summary>
        public byte bConfigurationValue;

        /// <Summary>Index of string descriptor describing this configuration</Summary>
        public byte iConfiguration;

        /// <Summary>Configuration characteristics</Summary>
        public byte bmAttributes;

        /// <Summary>Maximum power consumption of the USB device from this bus in this configuration when the device is fully operation.</Summary>
        public byte MaxPower;
    };

    /// <Summary>A structure representing the standard USB interface descriptor.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct USB_INTERFACE_DESCRIPTOR
    {
        /// <Summary>Size of this descriptor (in bytes)</Summary>
        public byte bLength;

        /// <Summary>Descriptor type</Summary>
        public byte bDescriptorType;

        /// <Summary>Number of this interface</Summary>
        public byte bInterfaceNumber;

        /// <Summary>Value used to select this alternate setting for this interface</Summary>
        public byte bAlternateSetting;

        /// <Summary>Number of endpoints used by this interface (excluding the control endpoint)</Summary>
        public byte bNumEndpoints;

        /// <Summary>USB-IF class code for this interface</Summary>
        public byte bInterfaceClass;

        /// <Summary>USB-IF subclass code for this interface</Summary>
        public byte bInterfaceSubClass;

        /// <Summary>USB-IF protocol code for this interface</Summary>
        public byte bInterfaceProtocol;

        /// <Summary>Index of string descriptor describing this interface</Summary>
        public byte iInterface;
    };

    /// <Summary>A structure representing the standard USB string descriptor.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode, Pack = 1)]
    public struct USB_STRING_DESCRIPTOR
    {
        /// <Summary>Size of this descriptor (in bytes)</Summary>
        public byte bLength;

        /// <Summary>Descriptor type</Summary>
        public byte bDescriptorType;

        /// <Summary>Content of the string</Summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string bString;
    };

    /// <Summary>A structure representing the common USB descriptor.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct USB_COMMON_DESCRIPTOR
    {
        /// <Summary>Size of this descriptor (in bytes)</Summary>
        public byte bLength;

        /// <Summary>Descriptor type</Summary>
        public byte bDescriptorType;
    };

    /// <Summary>Allows hardware manufacturers to define groupings of interfaces.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct USB_INTERFACE_ASSOCIATION_DESCRIPTOR
    {
        /// <Summary>Size of this descriptor (in bytes)</Summary>
        public byte bLength;

        /// <Summary>Descriptor type</Summary>
        public byte bDescriptorType;

        /// <Summary>First interface number of the set of interfaces that follow this descriptor</Summary>
        public byte bFirstInterface;

        /// <Summary>The Number of interfaces follow this descriptor that are considered "associated"</Summary>
        public byte bInterfaceCount;

        /// <Summary>\c bInterfaceClass used for this associated interfaces</Summary>
        public byte bFunctionClass;

        /// <Summary>\c bInterfaceSubClass used for the associated interfaces</Summary>
        public byte bFunctionSubClass;

        /// <Summary>\c bInterfaceProtocol used for the associated interfaces</Summary>
        public byte bFunctionProtocol;

        /// <Summary>Index of string descriptor describing the associated interfaces</Summary>
        public byte iFunction;
    };

    /// <Summary>USB core driver API information structure.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct KUSB_DRIVER_API_INFO
    {
        /// <Summary>\readonly Driver id of the driver api.</Summary>
        public int DriverID;

        /// <Summary>\readonly Number of valid functions contained in the driver API.</Summary>
        public int FunctionCount;
    };

    /// <Summary>Driver API function set structure.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Size = 512)]
    public struct KUSB_DRIVER_API
    {
        /// <Summary>Driver API information.</Summary>
        public KUSB_DRIVER_API_INFO Info;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_InitDelegate Init;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_FreeDelegate Free;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_ClaimInterfaceDelegate ClaimInterface;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_ReleaseInterfaceDelegate ReleaseInterface;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_SetAltInterfaceDelegate SetAltInterface;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_GetAltInterfaceDelegate GetAltInterface;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_GetDescriptorDelegate GetDescriptor;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_ControlTransferDelegate ControlTransfer;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_SetPowerPolicyDelegate SetPowerPolicy;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_GetPowerPolicyDelegate GetPowerPolicy;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_SetConfigurationDelegate SetConfiguration;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_GetConfigurationDelegate GetConfiguration;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_ResetDeviceDelegate ResetDevice;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_InitializeDelegate Initialize;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_SelectInterfaceDelegate SelectInterface;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_GetAssociatedInterfaceDelegate GetAssociatedInterface;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_CloneDelegate Clone;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_QueryInterfaceSettingsDelegate QueryInterfaceSettings;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_QueryDeviceInformationDelegate QueryDeviceInformation;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_SetCurrentAlternateSettingDelegate SetCurrentAlternateSetting;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_GetCurrentAlternateSettingDelegate GetCurrentAlternateSetting;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_QueryPipeDelegate QueryPipe;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_SetPipePolicyDelegate SetPipePolicy;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_GetPipePolicyDelegate GetPipePolicy;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_ReadPipeDelegate ReadPipe;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_WritePipeDelegate WritePipe;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_ResetPipeDelegate ResetPipe;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_AbortPipeDelegate AbortPipe;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_FlushPipeDelegate FlushPipe;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_IsoReadPipeDelegate IsoReadPipe;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_IsoWritePipeDelegate IsoWritePipe;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_GetCurrentFrameNumberDelegate GetCurrentFrameNumber;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_GetOverlappedResultDelegate GetOverlappedResult;

        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KUSB_GetPropertyDelegate GetProperty;
    };

    /// <Summary>Hot plug parameter structure.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Size = 1024)]
    public struct KHOT_PATTERN_MATCH
    {
        /// <Summary>Pattern match a device instance id.</Summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string InstanceID;

        /// <Summary>Pattern match a device interface guid.</Summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string DeviceInterfaceGUID;

        /// <Summary>Pattern match a device path.</Summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = Constants.KLST_STRING_MAX_LEN)]
        public string DevicePath;
    };

    /// <Summary>Hot plug parameter structure.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Size = 2048)]
    public struct KHOT_PARAMS
    {
        /// <Summary>Hot plug event window handle to send/post messages when notifications occur.</Summary>
        public IntPtr UserHwnd;

        /// <Summary>WM_USER message start offset used when sending/posting messages, See details.</Summary>
        public uint UserMessage;

        /// <Summary>Additional init/config parameters</Summary>
        public KHOT_FLAG Flags;

        /// <Summary>File pattern matches for restricting notifcations to a single/group or all supported usb devices.</Summary>
        public KHOT_PATTERN_MATCH PatternMatch;

        /// <Summary>Hot plug event callback function invoked when notifications occur.</Summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KHOT_PLUG_CB OnHotPlug;
    };

    /// <Summary>Stream transfer context structure.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct KSTM_XFER_CONTEXT
    {
        /// <Summary>Internal stream buffer.</Summary>
        public IntPtr Buffer;

        /// <Summary>Size of internal stream buffer.</Summary>
        public int BufferSize;

        /// <Summary>Number of bytes to write or number of bytes read.</Summary>
        public int TransferLength;

        /// <Summary>User defined state.</Summary>
        public IntPtr UserState;
    };

    /// <Summary>Stream information structure.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct KSTM_INFO
    {
        /// <Summary>\ref KUSB_HANDLE this stream uses.</Summary>
        public IntPtr UsbHandle;

        /// <Summary>This parameter corresponds to the bEndpointAddress field in the endpoint descriptor.</Summary>
        public byte PipeID;

        /// <Summary>Maximum transfer read/write request allowed pending.</Summary>
        public int MaxPendingTransfers;

        /// <Summary>Maximum transfer sage size.</Summary>
        public int MaxTransferSize;

        /// <Summary>Maximum number of I/O request allowed pending.</Summary>
        public int MaxPendingIO;

        /// <Summary>Populated with the endpoint descriptor for the specified \c PipeID.</Summary>
        public USB_ENDPOINT_DESCRIPTOR EndpointDescriptor;

        /// <Summary>Populated with the driver api for the specified \c UsbHandle.</Summary>
        public KUSB_DRIVER_API DriverAPI;

        /// <Summary>Populated with the device file handle for the specified \c UsbHandle.</Summary>
        public IntPtr DeviceHandle;
    };

    /// <Summary>Stream callback structure.</Summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Size = 64)]
    public struct KSTM_CALLBACK
    {
        /// <Summary>Executed when a transfer error occurs.</Summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KSTM_ERROR_CB Error;

        /// <Summary>Executed to submit a transfer.</Summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KSTM_SUBMIT_CB Submit;

        /// <Summary>Executed when a valid transfer completes.</Summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KSTM_COMPLETE_CB Complete;

        /// <Summary>Executed for every transfer context when the stream is started with \ref StmK_Start.</Summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KSTM_STARTED_CB Started;

        /// <Summary>Executed for every transfer context when the stream is stopped with \ref StmK_Stop.</Summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KSTM_STOPPED_CB Stopped;

        /// <Summary>Executed immediately after a transfer completes.</Summary>
        [MarshalAs(UnmanagedType.FunctionPtr)]
        public KSTM_BEFORE_COMPLETE_CB BeforeComplete;
    };
    #endregion


    #region Delegates
    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate int KLIB_HANDLE_CLEANUP_CB([In] IntPtr Handle, IntPtr HandleType, IntPtr UserContext);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KISO_ENUM_PACKETS_CB(uint PacketIndex, IntPtr IsoPacket, IntPtr UserState);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KLST_ENUM_DEVINFO_CB([In] IntPtr DeviceList, [In] IntPtr DeviceInfo, IntPtr Context);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_InitDelegate([Out] out KUSB_HANDLE InterfaceHandle, [In] KLST_DEVINFO_HANDLE DevInfo);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_FreeDelegate([In] KUSB_HANDLE InterfaceHandle);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_ClaimInterfaceDelegate([In] KUSB_HANDLE InterfaceHandle, byte NumberOrIndex, bool IsIndex);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_ReleaseInterfaceDelegate([In] KUSB_HANDLE InterfaceHandle, byte NumberOrIndex, bool IsIndex);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_SetAltInterfaceDelegate([In] KUSB_HANDLE InterfaceHandle, byte NumberOrIndex, bool IsIndex, byte AltSettingNumber);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_GetAltInterfaceDelegate([In] KUSB_HANDLE InterfaceHandle, byte NumberOrIndex, bool IsIndex, out byte AltSettingNumber);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_GetDescriptorDelegate(
        [In] KUSB_HANDLE InterfaceHandle,
        byte DescriptorType,
        byte Index,
        ushort LanguageID,
        IntPtr Buffer,
        uint BufferLength,
        out uint LengthTransferred);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_ControlTransferDelegate(
        [In] KUSB_HANDLE InterfaceHandle,
        WINUSB_SETUP_PACKET SetupPacket,
        IntPtr Buffer,
        uint BufferLength,
        out uint LengthTransferred,
        IntPtr Overlapped);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_SetPowerPolicyDelegate([In] KUSB_HANDLE InterfaceHandle, uint PolicyType, uint ValueLength, IntPtr Value);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_GetPowerPolicyDelegate([In] KUSB_HANDLE InterfaceHandle, uint PolicyType, ref uint ValueLength, IntPtr Value);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_SetConfigurationDelegate([In] KUSB_HANDLE InterfaceHandle, byte ConfigurationNumber);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_GetConfigurationDelegate([In] KUSB_HANDLE InterfaceHandle, out byte ConfigurationNumber);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_ResetDeviceDelegate([In] KUSB_HANDLE InterfaceHandle);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_InitializeDelegate(IntPtr DeviceHandle, [Out] out KUSB_HANDLE InterfaceHandle);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_SelectInterfaceDelegate([In] KUSB_HANDLE InterfaceHandle, byte NumberOrIndex, bool IsIndex);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_GetAssociatedInterfaceDelegate(
        [In] KUSB_HANDLE InterfaceHandle, byte AssociatedInterfaceIndex, [Out] out KUSB_HANDLE AssociatedInterfaceHandle);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_CloneDelegate([In] KUSB_HANDLE InterfaceHandle, [Out] out KUSB_HANDLE DstInterfaceHandle);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_QueryInterfaceSettingsDelegate(
        [In] KUSB_HANDLE InterfaceHandle, byte AltSettingNumber, [Out] out USB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_QueryDeviceInformationDelegate(
        [In] KUSB_HANDLE InterfaceHandle, uint InformationType, ref uint BufferLength, IntPtr Buffer);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_SetCurrentAlternateSettingDelegate([In] KUSB_HANDLE InterfaceHandle, byte AltSettingNumber);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_GetCurrentAlternateSettingDelegate([In] KUSB_HANDLE InterfaceHandle, out byte AltSettingNumber);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_QueryPipeDelegate(
        [In] KUSB_HANDLE InterfaceHandle, byte AltSettingNumber, byte PipeIndex, [Out] out WINUSB_PIPE_INFORMATION PipeInformation);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_SetPipePolicyDelegate([In] KUSB_HANDLE InterfaceHandle, byte PipeID, uint PolicyType, uint ValueLength, IntPtr Value);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_GetPipePolicyDelegate([In] KUSB_HANDLE InterfaceHandle, byte PipeID, uint PolicyType, ref uint ValueLength, IntPtr Value
        );

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_ReadPipeDelegate(
        [In] KUSB_HANDLE InterfaceHandle, byte PipeID, IntPtr Buffer, uint BufferLength, out uint LengthTransferred, IntPtr Overlapped);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_WritePipeDelegate(
        [In] KUSB_HANDLE InterfaceHandle, byte PipeID, IntPtr Buffer, uint BufferLength, out uint LengthTransferred, IntPtr Overlapped);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_ResetPipeDelegate([In] KUSB_HANDLE InterfaceHandle, byte PipeID);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_AbortPipeDelegate([In] KUSB_HANDLE InterfaceHandle, byte PipeID);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_FlushPipeDelegate([In] KUSB_HANDLE InterfaceHandle, byte PipeID);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_IsoReadPipeDelegate(
        [In] KUSB_HANDLE InterfaceHandle, byte PipeID, IntPtr Buffer, uint BufferLength, IntPtr Overlapped, [In] KISO_CONTEXT IsoContext);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_IsoWritePipeDelegate(
        [In] KUSB_HANDLE InterfaceHandle, byte PipeID, IntPtr Buffer, uint BufferLength, IntPtr Overlapped, [In] KISO_CONTEXT IsoContext);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_GetCurrentFrameNumberDelegate([In] KUSB_HANDLE InterfaceHandle, out uint FrameNumber);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_GetOverlappedResultDelegate(
        [In] KUSB_HANDLE InterfaceHandle, IntPtr lpOverlapped, out uint lpNumberOfBytesTransferred, bool bWait);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate bool KUSB_GetPropertyDelegate([In] KUSB_HANDLE InterfaceHandle, KUSB_PROPERTY PropertyType, ref uint PropertySize, IntPtr Property
        );

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate void KHOT_PLUG_CB([In] IntPtr HotHandle, [In] IntPtr DeviceInfo, IntPtr PlugType);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate int KSTM_ERROR_CB(IntPtr StreamInfo, IntPtr XferContext, int ErrorCode);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate int KSTM_SUBMIT_CB(IntPtr StreamInfo, IntPtr XferContext, IntPtr Overlapped);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate int KSTM_STARTED_CB(IntPtr StreamInfo, IntPtr XferContext, int XferContextIndex);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate int KSTM_STOPPED_CB(IntPtr StreamInfo, IntPtr XferContext, int XferContextIndex);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate int KSTM_COMPLETE_CB(IntPtr StreamInfo, IntPtr XferContext, int ErrorCode);

    [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Ansi, SetLastError = true)]
    public delegate KSTM_COMPLETE_RESULT KSTM_BEFORE_COMPLETE_CB(IntPtr StreamInfo, IntPtr XferContext, ref int ErrorCode);
    #endregion


    public class LstK
    {
        protected readonly KLST_HANDLE handle;

        public KLST_HANDLE Handle
        {
            get { return handle; }
        }

        public virtual bool Free()
        {
            if (handle.IsInvalid || handle.IsClosed) return false;

            handle.Close();
            return true;
        }

        /// <Summary>Initializes a new usb device list.</Summary>
        public LstK(KLST_FLAG Flags)
        {
            RuntimeHelpers.PrepareConstrainedRegions();

            try
            {
            }
            finally
            {
                bool success = Functions.LstK_Init(out handle, Flags);

                if (!success || handle.IsInvalid || handle.IsClosed)
                {
                    handle.SetHandleAsInvalid();
                    int errorCode = Marshal.GetLastWin32Error();
                    throw new Exception(GetType().Name + " failed. ErrorCode=" + errorCode.ToString("X"));
                }
            }
        }

        /// <Summary>Enumerates \ref KLST_DEVINFO elements of a \ref KLST_HANDLE.</Summary>
        public bool Enumerate(KLST_ENUM_DEVINFO_CB EnumDevListCB, IntPtr Context)
        {
            return Functions.LstK_Enumerate(handle, EnumDevListCB, Context);
        }

        /// <Summary>Gets the \ref KLST_DEVINFO element for the current position.</Summary>
        public bool Current(out KLST_DEVINFO_HANDLE DeviceInfo)
        {
            return Functions.LstK_Current(handle, out DeviceInfo);
        }

        /// <Summary>Advances the device list current \ref KLST_DEVINFO position.</Summary>
        public bool MoveNext(out KLST_DEVINFO_HANDLE DeviceInfo)
        {
            return Functions.LstK_MoveNext(handle, out DeviceInfo);
        }

        /// <Summary>Sets the device list to its initial position, which is before the first element in the list.</Summary>
        public void MoveReset()
        {
            Functions.LstK_MoveReset(handle);
        }

        /// <Summary>Find a device by vendor and product id</Summary>
        public bool FindByVidPid(int Vid, int Pid, out KLST_DEVINFO_HANDLE DeviceInfo)
        {
            return Functions.LstK_FindByVidPid(handle, Vid, Pid, out DeviceInfo);
        }

        /// <Summary>Counts the number of device info elements in a device list.</Summary>
        public bool Count(ref uint Count)
        {
            return Functions.LstK_Count(handle, ref Count);
        }
    }

    public class HotK
    {
        protected readonly KHOT_HANDLE handle;

        public KHOT_HANDLE Handle
        {
            get { return handle; }
        }

        public virtual bool Free()
        {
            if (handle.IsInvalid || handle.IsClosed) return false;

            handle.Close();
            return true;
        }

        /// <Summary>Creates a new hot-plug handle for USB device arrival/removal event monitoring.</Summary>
        public HotK(ref KHOT_PARAMS InitParams)
        {
            RuntimeHelpers.PrepareConstrainedRegions();

            try
            {
            }
            finally
            {
                bool success = Functions.HotK_Init(out handle, ref InitParams);

                if (!success || handle.IsInvalid || handle.IsClosed)
                {
                    handle.SetHandleAsInvalid();
                    int errorCode = Marshal.GetLastWin32Error();
                    throw new Exception(GetType().Name + " failed. ErrorCode=" + errorCode.ToString("X"));
                }
            }
        }

        /// <Summary>Frees all hot-plug handles initialized with \ref HotK_Init.</Summary>
        public void FreeAll()
        {
            Functions.HotK_FreeAll();
        }
    }

    public class UsbK
    {
        protected readonly KUSB_HANDLE handle;

        public KUSB_HANDLE Handle
        {
            get { return handle; }
        }

        public virtual bool Free()
        {
            if (handle.IsInvalid || handle.IsClosed) return false;

            handle.Close();
            return true;
        }

        protected readonly KUSB_DRIVER_API driverAPI;

        /// <Summary>Creates/opens a libusbK interface handle from the device list. This is a preferred method.</Summary>
        public UsbK(KLST_DEVINFO_HANDLE DevInfo)
        {
            if (!Functions.LibK_LoadDriverAPI(out driverAPI, DevInfo.DriverID))
            {
                throw new Exception(GetType().Name + " failed loading Driver API. ErrorCode=" + Marshal.GetLastWin32Error().ToString("X"));
            }

            RuntimeHelpers.PrepareConstrainedRegions();

            try
            {
            }
            finally
            {
                bool success = driverAPI.Init(out handle, DevInfo);

                if (!success || handle.IsInvalid || handle.IsClosed)
                {
                    handle.SetHandleAsInvalid();
                    int errorCode = Marshal.GetLastWin32Error();
                    throw new Exception(GetType().Name + " failed. ErrorCode=" + errorCode.ToString("X"));
                }
            }
        }

        /// <Summary>Claims the specified interface by number or index.</Summary>
        public bool ClaimInterface(byte NumberOrIndex, bool IsIndex)
        {
            return driverAPI.ClaimInterface(handle, NumberOrIndex, IsIndex);
        }

        /// <Summary>Releases the specified interface by number or index.</Summary>
        public bool ReleaseInterface(byte NumberOrIndex, bool IsIndex)
        {
            return driverAPI.ReleaseInterface(handle, NumberOrIndex, IsIndex);
        }

        /// <Summary>Sets the alternate setting of the specified interface.</Summary>
        public bool SetAltInterface(byte NumberOrIndex, bool IsIndex, byte AltSettingNumber)
        {
            return driverAPI.SetAltInterface(handle, NumberOrIndex, IsIndex, AltSettingNumber);
        }

        /// <Summary>Gets the alternate setting for the specified interface.</Summary>
        public bool GetAltInterface(byte NumberOrIndex, bool IsIndex, out byte AltSettingNumber)
        {
            return driverAPI.GetAltInterface(handle, NumberOrIndex, IsIndex, out AltSettingNumber);
        }

        /// <Summary>Gets the requested descriptor. This is a synchronous operation.</Summary>
        public bool GetDescriptor(byte DescriptorType, byte Index, ushort LanguageID, IntPtr Buffer, uint BufferLength, out uint LengthTransferred)
        {
            return driverAPI.GetDescriptor(handle, DescriptorType, Index, LanguageID, Buffer, BufferLength, out LengthTransferred);
        }

        /// <Summary>Transmits control data over a default control endpoint.</Summary>
        public bool ControlTransfer(WINUSB_SETUP_PACKET SetupPacket, IntPtr Buffer, uint BufferLength, out uint LengthTransferred, IntPtr Overlapped)
        {
            return driverAPI.ControlTransfer(handle, SetupPacket, Buffer, BufferLength, out LengthTransferred, Overlapped);
        }

        /// <Summary>Sets the power policy for a device.</Summary>
        public bool SetPowerPolicy(uint PolicyType, uint ValueLength, IntPtr Value)
        {
            return driverAPI.SetPowerPolicy(handle, PolicyType, ValueLength, Value);
        }

        /// <Summary>Gets the power policy for a device.</Summary>
        public bool GetPowerPolicy(uint PolicyType, ref uint ValueLength, IntPtr Value)
        {
            return driverAPI.GetPowerPolicy(handle, PolicyType, ref ValueLength, Value);
        }

        /// <Summary>Sets the device configuration number.</Summary>
        public bool SetConfiguration(byte ConfigurationNumber)
        {
            return driverAPI.SetConfiguration(handle, ConfigurationNumber);
        }

        /// <Summary>Gets the device current configuration number.</Summary>
        public bool GetConfiguration(out byte ConfigurationNumber)
        {
            return driverAPI.GetConfiguration(handle, out ConfigurationNumber);
        }

        /// <Summary>Resets the usb device of the specified interface handle. (port cycle).</Summary>
        public bool ResetDevice()
        {
            return driverAPI.ResetDevice(handle);
        }

        /// <Summary>Creates a libusbK handle for the device specified by a file handle.</Summary>
        public UsbK(IntPtr DeviceHandle, KUSB_DRVID driverID)
        {
            if (!Functions.LibK_LoadDriverAPI(out driverAPI, (int) driverID))
            {
                throw new Exception(GetType().Name + " failed loading Driver API. ErrorCode=" + Marshal.GetLastWin32Error().ToString("X"));
            }

            RuntimeHelpers.PrepareConstrainedRegions();

            try
            {
            }
            finally
            {
                bool success = driverAPI.Initialize(DeviceHandle, out handle);

                if (!success || handle.IsInvalid || handle.IsClosed)
                {
                    handle.SetHandleAsInvalid();
                    int errorCode = Marshal.GetLastWin32Error();
                    throw new Exception(GetType().Name + " failed. ErrorCode=" + errorCode.ToString("X"));
                }
            }
        }

        /// <Summary>Selects the specified interface by number or index as the current interface.</Summary>
        public bool SelectInterface(byte NumberOrIndex, bool IsIndex)
        {
            return driverAPI.SelectInterface(handle, NumberOrIndex, IsIndex);
        }

        /// <Summary>Retrieves a handle for an associated interface.</Summary>
        public bool GetAssociatedInterface(byte AssociatedInterfaceIndex, out KUSB_HANDLE AssociatedInterfaceHandle)
        {
            return driverAPI.GetAssociatedInterface(handle, AssociatedInterfaceIndex, out AssociatedInterfaceHandle);
        }

        /// <Summary>Clones the specified interface handle.</Summary>
        public bool Clone(out KUSB_HANDLE DstInterfaceHandle)
        {
            return driverAPI.Clone(handle, out DstInterfaceHandle);
        }

        /// <Summary>Retrieves the interface descriptor for the specified alternate interface settings for a particular interface handle.</Summary>
        public bool QueryInterfaceSettings(byte AltSettingNumber, out USB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor)
        {
            return driverAPI.QueryInterfaceSettings(handle, AltSettingNumber, out UsbAltInterfaceDescriptor);
        }

        /// <Summary>Retrieves information about the physical device that is associated with a libusbK handle.</Summary>
        public bool QueryDeviceInformation(uint InformationType, ref uint BufferLength, IntPtr Buffer)
        {
            return driverAPI.QueryDeviceInformation(handle, InformationType, ref BufferLength, Buffer);
        }

        /// <Summary>Sets the alternate setting of an interface.</Summary>
        public bool SetCurrentAlternateSetting(byte AltSettingNumber)
        {
            return driverAPI.SetCurrentAlternateSetting(handle, AltSettingNumber);
        }

        /// <Summary>Gets the current alternate interface setting for an interface.</Summary>
        public bool GetCurrentAlternateSetting(out byte AltSettingNumber)
        {
            return driverAPI.GetCurrentAlternateSetting(handle, out AltSettingNumber);
        }

        /// <Summary>Retrieves information about a pipe that is associated with an interface.</Summary>
        public bool QueryPipe(byte AltSettingNumber, byte PipeIndex, out WINUSB_PIPE_INFORMATION PipeInformation)
        {
            return driverAPI.QueryPipe(handle, AltSettingNumber, PipeIndex, out PipeInformation);
        }

        /// <Summary>Sets the policy for a specific pipe associated with an endpoint on the device. This is a synchronous operation.</Summary>
        public bool SetPipePolicy(byte PipeID, uint PolicyType, uint ValueLength, IntPtr Value)
        {
            return driverAPI.SetPipePolicy(handle, PipeID, PolicyType, ValueLength, Value);
        }

        /// <Summary>Gets the policy for a specific pipe (endpoint).</Summary>
        public bool GetPipePolicy(byte PipeID, uint PolicyType, ref uint ValueLength, IntPtr Value)
        {
            return driverAPI.GetPipePolicy(handle, PipeID, PolicyType, ref ValueLength, Value);
        }

        /// <Summary>Reads data from the specified pipe.</Summary>
        public bool ReadPipe(byte PipeID, IntPtr Buffer, uint BufferLength, out uint LengthTransferred, IntPtr Overlapped)
        {
            return driverAPI.ReadPipe(handle, PipeID, Buffer, BufferLength, out LengthTransferred, Overlapped);
        }

        /// <Summary>Writes data to a pipe.</Summary>
        public bool WritePipe(byte PipeID, IntPtr Buffer, uint BufferLength, out uint LengthTransferred, IntPtr Overlapped)
        {
            return driverAPI.WritePipe(handle, PipeID, Buffer, BufferLength, out LengthTransferred, Overlapped);
        }

        /// <Summary>Resets the data toggle and clears the stall condition on a pipe.</Summary>
        public bool ResetPipe(byte PipeID)
        {
            return driverAPI.ResetPipe(handle, PipeID);
        }

        /// <Summary>Aborts all of the pending transfers for a pipe.</Summary>
        public bool AbortPipe(byte PipeID)
        {
            return driverAPI.AbortPipe(handle, PipeID);
        }

        /// <Summary>Discards any data that is cached in a pipe.</Summary>
        public bool FlushPipe(byte PipeID)
        {
            return driverAPI.FlushPipe(handle, PipeID);
        }

        /// <Summary>Reads from an isochronous pipe.</Summary>
        public bool IsoReadPipe(byte PipeID, IntPtr Buffer, uint BufferLength, IntPtr Overlapped, KISO_CONTEXT IsoContext)
        {
            return driverAPI.IsoReadPipe(handle, PipeID, Buffer, BufferLength, Overlapped, IsoContext);
        }

        /// <Summary>Writes to an isochronous pipe.</Summary>
        public bool IsoWritePipe(byte PipeID, IntPtr Buffer, uint BufferLength, IntPtr Overlapped, KISO_CONTEXT IsoContext)
        {
            return driverAPI.IsoWritePipe(handle, PipeID, Buffer, BufferLength, Overlapped, IsoContext);
        }

        /// <Summary>Retrieves the current USB frame number.</Summary>
        public bool GetCurrentFrameNumber(out uint FrameNumber)
        {
            return driverAPI.GetCurrentFrameNumber(handle, out FrameNumber);
        }

        /// <Summary>Retrieves the results of an overlapped operation on the specified libusbK handle.</Summary>
        public bool GetOverlappedResult(IntPtr lpOverlapped, out uint lpNumberOfBytesTransferred, bool bWait)
        {
            return driverAPI.GetOverlappedResult(handle, lpOverlapped, out lpNumberOfBytesTransferred, bWait);
        }

        /// <Summary>Gets a USB device (driver specific) property from usb handle.</Summary>
        public bool GetProperty(KUSB_PROPERTY PropertyType, ref uint PropertySize, IntPtr Property)
        {
            return driverAPI.GetProperty(handle, PropertyType, ref PropertySize, Property);
        }
    }

    public class OvlK
    {
        protected readonly KOVL_POOL_HANDLE handle;

        public KOVL_POOL_HANDLE Handle
        {
            get { return handle; }
        }

        public virtual bool Free()
        {
            if (handle.IsInvalid || handle.IsClosed) return false;

            handle.Close();
            return true;
        }

        /// <Summary>Gets a preallocated \c OverlappedK structure from the specified/default pool.</Summary>
        public bool Acquire(out KOVL_HANDLE OverlappedK)
        {
            return Functions.OvlK_Acquire(out OverlappedK, handle);
        }

        /// <Summary>Returns an \c OverlappedK structure to it's pool.</Summary>
        public bool Release(KOVL_HANDLE OverlappedK)
        {
            return Functions.OvlK_Release(OverlappedK);
        }

        /// <Summary>Creates a new overlapped pool.</Summary>
        public OvlK(KUSB_HANDLE UsbHandle, int MaxOverlappedCount, KOVL_POOL_FLAG Flags)
        {
            RuntimeHelpers.PrepareConstrainedRegions();

            try
            {
            }
            finally
            {
                bool success = Functions.OvlK_Init(out handle, UsbHandle, MaxOverlappedCount, Flags);

                if (!success || handle.IsInvalid || handle.IsClosed)
                {
                    handle.SetHandleAsInvalid();
                    int errorCode = Marshal.GetLastWin32Error();
                    throw new Exception(GetType().Name + " failed. ErrorCode=" + errorCode.ToString("X"));
                }
            }
        }

        /// <Summary>Returns the internal event handle used to signal IO operations.</Summary>
        public IntPtr GetEventHandle(KOVL_HANDLE OverlappedK)
        {
            return Functions.OvlK_GetEventHandle(OverlappedK);
        }

        /// <Summary>Waits for overlapped I/O completion, and performs actions specified in \c WaitFlags.</Summary>
        public bool Wait(KOVL_HANDLE OverlappedK, int TimeoutMS, KOVL_WAIT_FLAG WaitFlags, out uint TransferredLength)
        {
            return Functions.OvlK_Wait(OverlappedK, TimeoutMS, WaitFlags, out TransferredLength);
        }

        /// <Summary>Waits for overlapped I/O completion, cancels on a timeout error.</Summary>
        public bool WaitOrCancel(KOVL_HANDLE OverlappedK, int TimeoutMS, out uint TransferredLength)
        {
            return Functions.OvlK_WaitOrCancel(OverlappedK, TimeoutMS, out TransferredLength);
        }

        /// <Summary>Waits for overlapped I/O completion, cancels on a timeout error and always releases the OvlK handle back to its pool.</Summary>
        public bool WaitAndRelease(KOVL_HANDLE OverlappedK, int TimeoutMS, out uint TransferredLength)
        {
            return Functions.OvlK_WaitAndRelease(OverlappedK, TimeoutMS, out TransferredLength);
        }

        /// <Summary>Checks for i/o completion; returns immediately. (polling)</Summary>
        public bool IsComplete(KOVL_HANDLE OverlappedK)
        {
            return Functions.OvlK_IsComplete(OverlappedK);
        }

        /// <Summary>Initializes an overlappedK for re-use. The overlappedK is not return to its pool.</Summary>
        public bool ReUse(KOVL_HANDLE OverlappedK)
        {
            return Functions.OvlK_ReUse(OverlappedK);
        }
    }

    public class StmK
    {
        protected readonly KSTM_HANDLE handle;

        public KSTM_HANDLE Handle
        {
            get { return handle; }
        }

        public virtual bool Free()
        {
            if (handle.IsInvalid || handle.IsClosed) return false;

            handle.Close();
            return true;
        }

        /// <Summary>Initializes a new uni-directional pipe stream.</Summary>
        public StmK(KUSB_HANDLE UsbHandle,
                    byte PipeID,
                    int MaxTransferSize,
                    int MaxPendingTransfers,
                    int MaxPendingIO,
                    ref KSTM_CALLBACK Callbacks,
                    KSTM_FLAG Flags)
        {
            RuntimeHelpers.PrepareConstrainedRegions();

            try
            {
            }
            finally
            {
                bool success = Functions.StmK_Init(out handle,
                                                   UsbHandle,
                                                   PipeID,
                                                   MaxTransferSize,
                                                   MaxPendingTransfers,
                                                   MaxPendingIO,
                                                   ref Callbacks,
                                                   Flags);

                if (!success || handle.IsInvalid || handle.IsClosed)
                {
                    handle.SetHandleAsInvalid();
                    int errorCode = Marshal.GetLastWin32Error();
                    throw new Exception(GetType().Name + " failed. ErrorCode=" + errorCode.ToString("X"));
                }
            }
        }

        /// <Summary>Starts the internal stream thread.</Summary>
        public bool Start()
        {
            return Functions.StmK_Start(handle);
        }

        /// <Summary>Stops the internal stream thread.</Summary>
        public bool Stop(int TimeoutCancelMS)
        {
            return Functions.StmK_Stop(handle, TimeoutCancelMS);
        }

        /// <Summary>Reads data from the stream buffer.</Summary>
        public bool Read(IntPtr Buffer, int Offset, int Length, out uint TransferredLength)
        {
            return Functions.StmK_Read(handle, Buffer, Offset, Length, out TransferredLength);
        }

        /// <Summary>Writes data to the stream buffer.</Summary>
        public bool Write(IntPtr Buffer, int Offset, int Length, out uint TransferredLength)
        {
            return Functions.StmK_Write(handle, Buffer, Offset, Length, out TransferredLength);
        }
    }

    public class IsoK
    {
        protected readonly KISO_CONTEXT handle;

        public KISO_CONTEXT Handle
        {
            get { return handle; }
        }

        public virtual bool Free()
        {
            if (handle.IsInvalid || handle.IsClosed) return false;

            handle.Close();
            return true;
        }

        private static readonly int ofsFlags = Marshal.OffsetOf(typeof (KISO_CONTEXT_MAP), "Flags").ToInt32();
        private static readonly int ofsStartFrame = Marshal.OffsetOf(typeof (KISO_CONTEXT_MAP), "StartFrame").ToInt32();
        private static readonly int ofsErrorCount = Marshal.OffsetOf(typeof (KISO_CONTEXT_MAP), "ErrorCount").ToInt32();
        private static readonly int ofsNumberOfPackets = Marshal.OffsetOf(typeof (KISO_CONTEXT_MAP), "NumberOfPackets").ToInt32();

        /// <Summary>Additional ISO transfer flags. See \ref KISO_FLAG.</Summary>
        public KISO_FLAG Flags
        {
            get { return (KISO_FLAG) Marshal.ReadInt32(handle.DangerousGetHandle(), ofsFlags); }
            set { Marshal.WriteInt32(handle.DangerousGetHandle(), ofsFlags, (int) value); }
        }

        /// <Summary>Specifies the frame number that the transfer should begin on (0 for ASAP).</Summary>
        public uint StartFrame
        {
            get { return (uint) Marshal.ReadInt32(handle.DangerousGetHandle(), ofsStartFrame); }
            set { Marshal.WriteInt32(handle.DangerousGetHandle(), ofsStartFrame, (Int32) value); }
        }

        /// <Summary>Contains the number of packets that completed with an error condition on return from the host controller driver.</Summary>
        public short ErrorCount
        {
            get { return Marshal.ReadInt16(handle.DangerousGetHandle(), ofsErrorCount); }
            set { Marshal.WriteInt16(handle.DangerousGetHandle(), ofsErrorCount, value); }
        }

        /// <Summary>Specifies the number of packets that are described by the variable-length array member \c IsoPacket.</Summary>
        public short NumberOfPackets
        {
            get { return Marshal.ReadInt16(handle.DangerousGetHandle(), ofsNumberOfPackets); }
            set { Marshal.WriteInt16(handle.DangerousGetHandle(), ofsNumberOfPackets, value); }
        }

        /// <Summary>Creates a new isochronous transfer context.</Summary>
        public IsoK(int NumberOfPackets, int StartFrame)
        {
            RuntimeHelpers.PrepareConstrainedRegions();

            try
            {
            }
            finally
            {
                bool success = Functions.IsoK_Init(out handle, NumberOfPackets, StartFrame);

                if (!success || handle.IsInvalid || handle.IsClosed)
                {
                    handle.SetHandleAsInvalid();
                    int errorCode = Marshal.GetLastWin32Error();
                    throw new Exception(GetType().Name + " failed. ErrorCode=" + errorCode.ToString("X"));
                }
            }
        }

        /// <Summary>Convenience function for setting the offset of all ISO packets of an isochronous transfer context.</Summary>
        public bool SetPackets(int PacketSize)
        {
            return Functions.IsoK_SetPackets(handle, PacketSize);
        }

        /// <Summary>Convenience function for setting all fields of a \ref KISO_PACKET.</Summary>
        public bool SetPacket(int PacketIndex, ref KISO_PACKET IsoPacket)
        {
            return Functions.IsoK_SetPacket(handle, PacketIndex, ref IsoPacket);
        }

        /// <Summary>Convenience function for getting all fields of a \ref KISO_PACKET.</Summary>
        public bool GetPacket(int PacketIndex, out KISO_PACKET IsoPacket)
        {
            return Functions.IsoK_GetPacket(handle, PacketIndex, out IsoPacket);
        }

        /// <Summary>Convenience function for enumerating ISO packets of an isochronous transfer context.</Summary>
        public bool EnumPackets(KISO_ENUM_PACKETS_CB EnumPackets, int StartPacketIndex, IntPtr UserState)
        {
            return Functions.IsoK_EnumPackets(handle, EnumPackets, StartPacketIndex, UserState);
        }

        /// <Summary>Convenience function for re-using an isochronous transfer context in a subsequent request.</Summary>
        public bool ReUse()
        {
            return Functions.IsoK_ReUse(handle);
        }
    }
}