#Region "Copyright(c) Travis Robinson"

' Copyright (c) 2011-2012 Travis Robinson <libusbdotnet@gmail.com>
' All rights reserved.
' 
' List.Devices
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
Imports libusbK

Namespace List.Devices
    Friend Class Program
        public Shared Sub Main()
            Dim deviceIndex = 0
            Dim deviceCount As UInteger = 0
            Dim deviceInfo As KLST_DEVINFO_HANDLE
            Dim lst As LstK = New LstK(KLST_FLAG.NONE)
            lst.Count(deviceCount)

            While lst.MoveNext(deviceInfo)
                ' Write some information about the device.
                Dim displayLine = String.Format("{0} of {1}: {2}", Threading.Interlocked.Increment(deviceIndex), deviceCount, deviceInfo.DeviceDesc)
                Console.WriteLine(displayLine)
                Console.WriteLine("- " & deviceInfo.DeviceID)
                Console.WriteLine()
            End While

            If deviceCount = 0 Then Console.WriteLine("No devices found!" & Microsoft.VisualBasic.Constants.vbLf)
            lst.Free()
        End Sub
    End Class
End Namespace
