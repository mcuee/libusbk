#Region "Copyright(c) Travis Robinson"

' Copyright (c) 2012 Travis Robinson <libusbdotnet@gmail.com>
' All rights reserved.
' 
' BenchmarkTestDevice.cs
' 
' Created:      03.05.2012
' Last Updated: 03.05.2012
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
Imports System.Runtime.InteropServices
Imports libusbK


' ReSharper disable CheckNamespace

' ReSharper restore CheckNamespace
Namespace Test.Devices
    Public Enum BM_COMMAND
        SET_TEST = &H0E
        GET_TEST = &H0F
    End Enum

    Public Enum BM_TEST_TYPE
        NONE = &H00
        READ = &H01
        WRITE = &H02
        [LOOP] = READ Or WRITE
    End Enum

    Public Module Benchmark
        '! Custom vendor requests that must be implemented in the benchmark firmware.


#Region "Public Members"

        Public Function Configure(ByVal usb As UsbK, ByVal command As BM_COMMAND, ByVal interfaceNumber As Byte, ByRef testType As BM_TEST_TYPE) As Boolean
            Dim transferred As UInteger
            Dim pkt As WINUSB_SETUP_PACKET
            Dim data = New Byte(0) {}
            pkt.RequestType = 1 << 7 Or 2 << 5
            pkt.Request = CByte(command)
            pkt.Value = CUShort(testType)
            pkt.Index = interfaceNumber
            pkt.Length = 1
            Dim success = usb.ControlTransfer(pkt, Marshal.UnsafeAddrOfPinnedArrayElement(data, 0), 1, transferred, IntPtr.Zero)
            testType = CType(data(0), BM_TEST_TYPE)
            Return success
        End Function

#End Region
    End Module
End Namespace
