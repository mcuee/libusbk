#Region "Copyright(c) Travis Robinson"

' Copyright (c) 2011-2012 Travis Robinson <libusbdotnet@gmail.com>
' All rights reserved.
' 
' Hot.Plug.Detect.GUI
' 
' Last Updated: 03.08.2012
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
Imports System.Windows.Forms
Imports libusbK

Namespace Hot.Plug.Detect.GUI
    Public Partial Class Form1
        Inherits Form

        Private Const WM_USER As Integer = &H400
        Private Const WM_USER_HOT_BASE As Integer = WM_USER
        Private Const WM_USER_HOT_REMOVAL As Integer = WM_USER_HOT_BASE
        Private Const WM_USER_HOT_ARRIVAL As Integer = WM_USER_HOT_BASE + 1
        Private mHotK As HotK
        Private mHotParams As KHOT_PARAMS

        Public Sub New()
            InitializeComponent()
        End Sub

        Private Sub Form1_Load(ByVal sender As Object, ByVal e As EventArgs)
            mHotParams.PatternMatch.DeviceInterfaceGUID = "*"

            ' Set the Hwnd handle.
            mHotParams.UserHwnd = Handle

            ' Set the base user message.  This can be any value greater than or equal to 0x400
            mHotParams.UserMessage = WM_USER_HOT_BASE

            ' After initializing, send plug events for devices that are currently connected.
            mHotParams.Flags = KHOT_FLAG.PLUG_ALL_ON_INIT

            ' This will cause HotK to use PostMessage instead of SendMessage.
            mHotParams.Flags = mHotParams.Flags Or KHOT_FLAG.POST_USER_MESSAGE
            mHotK = New HotK(mHotParams)
        End Sub

        Private Sub OnHotPlugInvoked(ByVal hotHandle As KHOT_HANDLE, ByVal deviceInfo As KLST_DEVINFO_HANDLE, ByVal plugType As KLST_SYNC_FLAG)
            Dim symbolicLink = deviceInfo.SymbolicLink

            Select Case plugType
                Case KLST_SYNC_FLAG.ADDED
                    Dim iRow = dgvDevices.Rows.Add(New Object() {symbolicLink, deviceInfo.DeviceDesc, deviceInfo.DeviceID})
                    dgvDevices.Rows(iRow).Cells(1).ToolTipText = deviceInfo.ToString()
                    dgvDevices.Rows(iRow).Cells(2).ToolTipText = deviceInfo.Common.ToString()
                Case KLST_SYNC_FLAG.REMOVED

                    For Each row As DataGridViewRow In dgvDevices.Rows
                        If Not Equals(TryCast(row.Cells(0).Value, String), symbolicLink) Then Continue For
                        dgvDevices.Rows.Remove(row)
                        Exit For
                    Next

                Case Else
                    Throw New ArgumentOutOfRangeException("plugType")
            End Select
        End Sub

        Protected Overrides Sub WndProc(ByRef m As Message)
            ' When using the HotK UserHwnd and UserMsg, the add/remove events are seperated into to different messages.
            '  Removal = UserMsg + 0
            '  Arrival = UserMsg + 1
            ' 
            If m.Msg = WM_USER_HOT_REMOVAL OrElse m.Msg = WM_USER_HOT_ARRIVAL Then
                Dim hotHandle As KHOT_HANDLE = New KHOT_HANDLE(m.WParam)
                Dim deviceInfo As KLST_DEVINFO_HANDLE = New KLST_DEVINFO_HANDLE(m.LParam)
                Dim plugType = If(m.Msg = WM_USER_HOT_REMOVAL, KLST_SYNC_FLAG.REMOVED, KLST_SYNC_FLAG.ADDED)
                OnHotPlugInvoked(hotHandle, deviceInfo, plugType)
                Return
            End If

            MyBase.WndProc(m)
        End Sub

        Private Sub Form1_FormClosing(ByVal sender As Object, ByVal e As FormClosingEventArgs)
            mHotK.Dispose()
        End Sub
    End Class
End Namespace
