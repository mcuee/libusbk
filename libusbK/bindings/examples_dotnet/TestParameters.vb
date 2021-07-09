#Region "Copyright(c) Travis Robinson"
' Copyright (c) 2012 Travis Robinson <libusbdotnet@gmail.com>
' All rights reserved.
' 
' TestParameters.cs
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
Imports System.Globalization
Imports System.IO
Imports System.Runtime.InteropServices
Imports System.Text.RegularExpressions
Imports Win32

Namespace libusbK.Examples
    Friend MustInherit Class TestParameters
        Protected Sub New(ByVal vid As Integer, ByVal pid As Integer, ByVal altInterfaceId As Integer, ByVal pipeId As Integer, ByVal maxTransfersTotal As Integer, ByVal logFilename As String)
            MI = -1
            Me.Vid = vid
            Me.Pid = pid
            Me.PipeId = pipeId
            Me.AltInterfaceId = altInterfaceId
            Me.MaxTransfersTotal = maxTransfersTotal
            Me.LogFilename = logFilename
        End Sub


#Region "Public Members"
        Public AltInterfaceId As Integer

        Public Function ConfigureDevice(<Out> ByRef pipeInfo As WINUSB_PIPE_INFORMATION_EX, <Out> ByRef usb As UsbK, <Out> ByRef interfaceDescriptor As USB_INTERFACE_DESCRIPTOR) As Boolean
            Dim success As Boolean
            Console.WriteLine("Finding usb device by VID/PID.. ({0:X4}h / {1:X4}h)", Vid, Pid)

            ' Use a patttern match to include only matching devices.
            ' NOTE: You can use the '*' and '?' chars as wildcards for all chars or a single char (respectively). 
            Dim patternMatch As KLST_PATTERN_MATCH = New KLST_PATTERN_MATCH()

            If MI <> -1 Then
                patternMatch.DeviceID = String.Format("USB\VID_{0:X4}&PID_{1:X4}&MI_{2:X2}*", Vid, Pid, MI)
            Else
                patternMatch.DeviceID = String.Format("USB\VID_{0:X4}&PID_{1:X4}*", Vid, Pid)
            End If

            Dim deviceList As LstK = New LstK(KLST_FLAG.NONE, patternMatch)
            Dim deviceInfo As KLST_DEVINFO_HANDLE
            interfaceDescriptor = New USB_INTERFACE_DESCRIPTOR()
            pipeInfo = New WINUSB_PIPE_INFORMATION_EX()
            usb = Nothing

            ' Iterate the devices looking for a matching alt-interface and endpoint id.
            While deviceList.MoveNext(deviceInfo)
                ' libusbK class contructors can throw exceptions; For instance, if the device is
                ' using the WinUsb driver and already in-use by another application.
                Console.WriteLine("Opening usb device..")
                usb = New UsbK(deviceInfo)
                Console.WriteLine("Finding interface and endpoint by PipeId..")
                success = FindPipeAndInterface(usb, interfaceDescriptor, pipeInfo)
                If success Then Exit While
                Console.WriteLine("PipeId {0:X2}h not found on this device.", PipeId)
                usb.Free()
                usb = Nothing
            End While

            If ReferenceEquals(usb, Nothing) Then
                Console.WriteLine("Usb device not found: {0}", patternMatch.DeviceID)
                success = False
                GoTo Done
            End If

            MI = interfaceDescriptor.bInterfaceNumber
            AltInterfaceId = interfaceDescriptor.bAlternateSetting
            PipeId = pipeInfo.PipeId

            ' Set interface alt setting.
            Console.WriteLine("Setting interface #{0} to bAlternateSetting #{1}..", interfaceDescriptor.bInterfaceNumber, interfaceDescriptor.bAlternateSetting)
            success = usb.SetAltInterface(interfaceDescriptor.bInterfaceNumber, False, interfaceDescriptor.bAlternateSetting)

            If Not success Then
                Console.WriteLine("SetAltInterface failed. ErrorCode={0:X8}h", Marshal.GetLastWin32Error())
                Console.WriteLine(interfaceDescriptor.ToString())
            End If

Done:
            deviceList.Free()
            Return success
        End Function

        Public Dcs As HiPerfTimer = New HiPerfTimer()

        Public Sub FillFromCommandLine(ByVal commandLine As String)
            commandLine = Regex.Replace(commandLine, "^("".+?""|\S+)\s*", "")
            Dim exp = String.Empty
            exp += makeArgExpression("Vid", NumberStyles.HexNumber) & "|"
            exp += makeArgExpression("Pid", NumberStyles.HexNumber) & "|"
            exp += makeArgExpression("PipeId", NumberStyles.HexNumber) & "|"
            exp += makeArgExpression("AltId", NumberStyles.Integer) & "|"
            exp += makeArgExpression("MI", NumberStyles.Integer) & "|"
            exp += makeArgExpression("Log", NumberStyles.None)
            m_CommandArguments = Regex.Matches(commandLine, exp, RegexOptions.ExplicitCapture Or RegexOptions.IgnoreCase Or RegexOptions.IgnorePatternWhitespace)

            For Each m As Match In m_CommandArguments
                Dim gVid = m.Groups("Vid")
                Dim gPid = m.Groups("Pid")
                Dim gMI = m.Groups("MI")
                Dim gPipeId = m.Groups("PipeId")
                Dim gAltId = m.Groups("AltId")
                Dim gLog = m.Groups("Log")
                Dim gIsHex = m.Groups("IsHex")

                If gVid.Success Then
                    Vid = If(gIsHex.Success, UShort.Parse(gVid.Value, NumberStyles.HexNumber), UShort.Parse(gVid.Value))
                ElseIf gPid.Success Then
                    Pid = If(gIsHex.Success, UShort.Parse(gPid.Value, NumberStyles.HexNumber), UShort.Parse(gPid.Value))
                ElseIf gMI.Success Then
                    MI = If(gIsHex.Success, UShort.Parse(gMI.Value, NumberStyles.HexNumber), UShort.Parse(gMI.Value))
                ElseIf gPipeId.Success Then
                    PipeId = If(gIsHex.Success, Byte.Parse(gPipeId.Value, NumberStyles.HexNumber), Byte.Parse(gPipeId.Value))
                ElseIf gAltId.Success Then
                    AltInterfaceId = If(gIsHex.Success, Byte.Parse(gAltId.Value, NumberStyles.HexNumber), Byte.Parse(gAltId.Value))
                ElseIf gLog.Success Then
                    LogFilename = gLog.Value
                Else
                    Throw New Exception("Invalid argument:" & m.Value)
                End If
            Next
        End Sub

        Public Function FindPipeAndInterface(ByVal usb As UsbK, <Out> ByRef interfaceDescriptor As USB_INTERFACE_DESCRIPTOR, <Out> ByRef pipeInfo As WINUSB_PIPE_INFORMATION_EX) As Boolean
            If FindPipeAndInterface(usb, interfaceDescriptor, pipeInfo, AltInterfaceId, PipeId) Then
                AltInterfaceId = interfaceDescriptor.bAlternateSetting
                PipeId = pipeInfo.PipeId
                Return True
            End If

            Return False
        End Function

        Public Shared Function FindPipeAndInterface(ByVal usb As UsbK, <Out> ByRef interfaceDescriptor As USB_INTERFACE_DESCRIPTOR, <Out> ByRef pipeInfo As WINUSB_PIPE_INFORMATION_EX, ByVal altInterfaceId As Integer, ByVal pipeId As Integer) As Boolean
            Dim interfaceIndex As Byte = 0
            interfaceDescriptor = New USB_INTERFACE_DESCRIPTOR()
            pipeInfo = New WINUSB_PIPE_INFORMATION_EX()

            While usb.SelectInterface(interfaceIndex, True)
                Dim altSettingNumber As Byte = 0

                While usb.QueryInterfaceSettings(altSettingNumber, interfaceDescriptor)

                    If altInterfaceId = -1 OrElse altInterfaceId = altSettingNumber Then
                        Dim pipeIndex As Byte = 0

                        While usb.QueryPipeEx(altSettingNumber,pipeIndex, pipeInfo)

                            If pipeInfo.MaximumPacketSize > 0 AndAlso pipeId = -1 OrElse pipeInfo.PipeId = pipeId OrElse (pipeId And &HF) = 0 AndAlso (pipeId And &H80) = (pipeInfo.PipeId And &H80) Then
                                GoTo FindInterfaceDone
                            End If
                            pipeIndex=CType((pipeIndex+1), Byte)
                            pipeInfo.PipeId = 0
                        End While
                    End If

                    altSettingNumber += 1
                End While

                interfaceIndex += 1
            End While

FindInterfaceDone:
            Return pipeInfo.PipeId <> 0
        End Function

        Public Sub Free()
            If fileLoggingEnabled AndAlso logStream IsNot Nothing Then
                logStream.Flush()
                logStream.Close()
                logStream = Nothing
            End If
        End Sub

        Public Sub Init()
            If Not String.IsNullOrEmpty(LogFilename) Then
                logStream = New StreamWriter(LogFilename)
                fileLoggingEnabled = True
            End If
        End Sub

        Public Sub Log(ByVal text As String)
            If fileLoggingEnabled Then
                logStream.Write(text)
            Else
                Console.Write(text)
            End If
        End Sub

        Public Sub Log(ByVal format As String, ParamArray args As Object())
            If fileLoggingEnabled Then
                logStream.Write(format, args)
            Else
                Console.Write(format, args)
            End If
        End Sub

        Public LogFilename As String

        Public Sub LogLn(ByVal text As String)
            If fileLoggingEnabled Then
                logStream.WriteLine(text)
            Else
                Console.WriteLine(text)
            End If
        End Sub

        Public Sub LogLn(ByVal format As String, ParamArray args As Object())
            If fileLoggingEnabled Then
                logStream.WriteLine(format, args)
            Else
                Console.WriteLine(format, args)
            End If
        End Sub

        Public MI As Integer
        Public MaxTransfersTotal As Integer
        Public Pid As Integer
        Public PipeId As Integer

        Public Function ShowTestReady() As Boolean
            While Console.KeyAvailable
                Console.ReadKey(True)
            End While

            Console.WriteLine("Test ready:")
            Console.Write(ToString())
            If Not String.IsNullOrEmpty(LogFilename) Then Console.WriteLine("Logging output to file: {0}", LogFilename)
            Console.WriteLine("Press 'Q' to abort or any other key to continue..")
            Dim key = Console.ReadKey(True)
            If key.KeyChar = "q"c OrElse key.KeyChar = "Q"c Then Return False
            Console.WriteLine()
            Console.WriteLine("Starting test..")
            Console.WriteLine()
            Return True
        End Function

        Public Overrides Function ToString() As String
            Return String.Format("Vid: {0:X4}h" & Microsoft.VisualBasic.Constants.vbLf & "Pid: {1:X4}h" & Microsoft.VisualBasic.Constants.vbLf & "MI: {2:X2}h" & Microsoft.VisualBasic.Constants.vbLf & "PipeId: {3:X2}h" & Microsoft.VisualBasic.Constants.vbLf & "MaxTransfersTotal: {4}" & Microsoft.VisualBasic.Constants.vbLf & Microsoft.VisualBasic.Constants.vbLf & "LogFilename: {5}" & Microsoft.VisualBasic.Constants.vbLf, Vid, Pid, MI, PipeId, MaxTransfersTotal, LogFilename)
        End Function

        Public Vid As Integer
#End Region


#Region "Private Members"
        Private fileLoggingEnabled As Boolean
        Private logStream As StreamWriter
        Private m_CommandArguments As MatchCollection

        Private Function makeArgExpression(ByVal argName As String, ByVal argType As NumberStyles) As String
            Const argsep = "\W\s*"

            Select Case argType
                Case NumberStyles.None
                    Return String.Format("(({0}{1}(?<{0}>{2}))\s*)", argName, argsep, "("".+?""|\S+)")
                Case NumberStyles.Integer, NumberStyles.HexNumber
                    Return String.Format("(({0}{1}(?<IsHex>0x)?(?<{0}>[0-9A-F]+))(?<IsHex>h)?\s*)", argName, argsep)
                Case Else
                    Throw New Exception()
            End Select
        End Function
#End Region
    End Class

    Friend Class IsoTestParameters
        Inherits TestParameters
#Region "Public Members"
        Public Sub New(ByVal vid As Integer, ByVal pid As Integer, ByVal altInterfaceId As Integer, ByVal pipeId As Integer, ByVal maxOutstandingTransfers As Integer, ByVal maxTransfersTotal As Integer, ByVal logFilename As String, ByVal isoPacketsPerTransfer As UInteger)
            MyBase.New(vid, pid, altInterfaceId, pipeId, maxTransfersTotal, logFilename)
            Me.IsoPacketsPerTransfer = isoPacketsPerTransfer
            Me.MaxOutstandingTransfers = maxOutstandingTransfers
            FillFromCommandLine(Environment.CommandLine)
            Init()
        End Sub

        Public IsoPacketsPerTransfer As UInteger
        Public MaxOutstandingTransfers As Integer

        Public Overrides Function ToString() As String
            Dim fmt = String.Empty
            fmt += "IsoPacketsPerTransfer: {0}" & Microsoft.VisualBasic.Constants.vbLf
            fmt += "MaxOutstandingTransfers: {1}" & Microsoft.VisualBasic.Constants.vbLf
            Dim args = New Object() {IsoPacketsPerTransfer, MaxOutstandingTransfers}
            Return MyBase.ToString() & String.Format(fmt, args)
        End Function
#End Region
    End Class

    Friend Class StmTestParameters
        Inherits TestParameters
#Region "Public Members"
        Public Sub New(ByVal vid As Integer, ByVal pid As Integer, ByVal altInterfaceId As Integer, ByVal pipeId As Byte, ByVal maxTransfersTotal As Integer, ByVal logFilename As String, ByVal transferBufferSize As Integer, ByVal maxPendingIO As Integer, ByVal maxPendingTransfers As Integer)
            MyBase.New(vid, pid, altInterfaceId, pipeId, maxTransfersTotal, logFilename)
            Me.TransferBufferSize = transferBufferSize
            Me.MaxPendingIO = maxPendingIO
            Me.MaxPendingTransfers = maxPendingTransfers
            FillFromCommandLine(Environment.CommandLine)
            Init()
        End Sub

        Public MaxPendingIO As Integer
        Public MaxPendingTransfers As Integer

        Public Overrides Function ToString() As String
            Dim fmt = String.Empty
            fmt += "TransferBufferSize: {0}" & Microsoft.VisualBasic.Constants.vbLf
            fmt += "MaxPendingIO: {1}" & Microsoft.VisualBasic.Constants.vbLf
            fmt += "MaxPendingTransfers: {2}" & Microsoft.VisualBasic.Constants.vbLf
            Dim args = New Object() {TransferBufferSize, MaxPendingIO, MaxPendingTransfers}
            Return MyBase.ToString() & String.Format(fmt, args)
        End Function

        Public TransferBufferSize As Integer
#End Region
    End Class

    Friend Class AsyncTestParameters
        Inherits TestParameters
#Region "Public Members"
        Public Sub New(ByVal vid As Integer, ByVal pid As Integer, ByVal altInterfaceId As Integer, ByVal pipeId As Byte, ByVal maxTransfersTotal As Integer, ByVal logFilename As String, ByVal transferBufferSize As Integer, ByVal maxPendingIO As Integer)
            MyBase.New(vid, pid, altInterfaceId, pipeId, maxTransfersTotal, logFilename)
            Me.TransferBufferSize = transferBufferSize
            Me.MaxPendingIO = maxPendingIO
            FillFromCommandLine(Environment.CommandLine)
            Init()
        End Sub

        Public MaxPendingIO As Integer

        Public Overrides Function ToString() As String
            Dim fmt = String.Empty
            fmt += "TransferBufferSize: {0}" & Microsoft.VisualBasic.Constants.vbLf
            fmt += "MaxPendingIO: {1}" & Microsoft.VisualBasic.Constants.vbLf
            Dim args = New Object() {TransferBufferSize, MaxPendingIO}
            Return MyBase.ToString() & String.Format(fmt, args)
        End Function

        Public TransferBufferSize As Integer
#End Region
    End Class
End Namespace
