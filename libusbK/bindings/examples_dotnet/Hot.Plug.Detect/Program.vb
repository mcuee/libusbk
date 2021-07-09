#Region "Copyright(c) Travis Robinson"

' Copyright (c) 2012 Travis Robinson <libusbdotnet@gmail.com>
' All rights reserved.
' 
' Hot.Plug.Detect
' 
' Last Updated: 04.28.2012
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
Imports libusbK

Namespace Hot.Plug.Detect
    Friend Class Program
        ' ReSharper disable InconsistentNaming

#Region "For Window Applications"
        Private Const WM_USER As Integer = &H400
        Private Const WM_USER_HOT_BASE As Integer = WM_USER
        Private Const WM_USER_HOT_REMOVAL As Integer = WM_USER_HOT_BASE
        Private Const WM_USER_HOT_ARRIVAL As Integer = WM_USER_HOT_BASE + 1
#End Region

        Public Shared Sub Main()
            Dim hotInitParams As KHOT_PARAMS = New KHOT_PARAMS()

            ' In the real world, you would want to filter for only *your* device(s).
            hotInitParams.PatternMatch.DeviceInterfaceGUID = "*"

            ' The PLUG_ALL_ON_INIT flag will force plug events for matching devices that are already connected.
            hotInitParams.Flags = KHOT_FLAG.PLUG_ALL_ON_INIT
            hotInitParams.OnHotPlug = AddressOf OnHotPlug

            ' Instead of a callback you can send/post messages directly to a window handle.
            ' hotInitParams.UserHwnd = MyForm.Handle;
            ' hotInitParams.UserMessage = WM_USER_HOT_BASE;
            ' 

            Console.WriteLine("Monitoring libusbK arrival/removal events.")
            Console.WriteLine("[PatternMatch]")
            Console.WriteLine(hotInitParams.PatternMatch.ToString())
            Console.WriteLine("Press [ENTER] to exit..")

            While Console.KeyAvailable
                Console.ReadKey(True)
            End While

            ' You may set your initial hot handle user context like this.
            ' This example is using it to count connected devices and detect the first OnHotPlug event (Int32.MaxValue).
            LibK_SetDefaultContext(KLIB_HANDLE_TYPE.HOTK, New IntPtr(Integer.MaxValue))

            ' Start hot-plug detection.
            Dim hot As HotK = New HotK(hotInitParams)
            Console.ReadLine()
            hot.Free()
        End Sub

        Private Shared Sub OnHotPlug(ByVal hotHandle As KHOT_HANDLE, ByVal deviceInfo As KLST_DEVINFO_HANDLE, ByVal plugType As KLST_SYNC_FLAG)
            Dim plugText As String
            Dim totalPluggedDeviceCount As Integer = CInt(hotHandle.GetContext().ToInt64())

            If totalPluggedDeviceCount = Integer.MaxValue Then
                Console.WriteLine("OnHotPlug is being called for the first time on handle:{0}", hotHandle.Pointer)
                totalPluggedDeviceCount = 0
            End If

            Select Case plugType
                Case KLST_SYNC_FLAG.ADDED
                    plugText = "Arrival"
                    totalPluggedDeviceCount += 1
                Case KLST_SYNC_FLAG.REMOVED
                    plugText = "Removal"
                    totalPluggedDeviceCount -= 1
                Case Else
                    Throw New ArgumentOutOfRangeException("plugType")
            End Select

            hotHandle.SetContext(New IntPtr(totalPluggedDeviceCount))
            Console.WriteLine(Microsoft.VisualBasic.Constants.vbLf & "[OnHotPlug] Device {0}:{1} " & Microsoft.VisualBasic.Constants.vbLf, plugText, deviceInfo)
            Console.WriteLine("Total Plugged Device Count: {0}", totalPluggedDeviceCount)
        End Sub
    End Class
End Namespace
