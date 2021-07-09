Imports System
Imports System.Diagnostics
Imports System.IO
Imports System.Threading
Imports Test.Devices
Imports libusbK
Imports libusbK.Examples

Namespace Xfer.UsbStream
    Friend Class Program
#Region "TODO USER: Set the test parameters for your device."

        Public Shared Test As StmTestParameters = New StmTestParameters(&H04d8, &Hfa2e, 0, &H81, 1024, Nothing, -1, 4, 64)

#End Region

        Private Shared Function FormatBrokenStrings(ByVal fixedLength As Integer, ByVal preAdd As String, ByRef sPartialRead As String, ByRef sRead As String) As Boolean
            If sRead.Length <> fixedLength Then
                ' partial read.
                If Not String.IsNullOrEmpty(sPartialRead) Then
                    sPartialRead += sRead
                    If sPartialRead.Length <> fixedLength Then Return False
                    sRead = sPartialRead
                    sPartialRead = String.Empty
                Else
                    sPartialRead = sRead
                    Return False
                End If
            End If

            sRead = preAdd & sRead.Substring(preAdd.Length)
            Return True
        End Function

        Public Shared Sub Main()
            Dim success As Boolean
            Dim usb As UsbK
            Dim pipeInfoRead As WINUSB_PIPE_INFORMATION_EX
            Dim interfaceDescriptorRead As USB_INTERFACE_DESCRIPTOR
            Dim pipeInfoWrite As WINUSB_PIPE_INFORMATION_EX
            Dim interfaceDescriptorWrite As USB_INTERFACE_DESCRIPTOR

            ' Find the IN endpoint and configure the device.
            Test.PipeId = Test.PipeId Or USB_ENDPOINT_DIRECTION_MASK
            If Not Test.ConfigureDevice(pipeInfoRead, usb, interfaceDescriptorRead) Then Return

            ' Find the OUT endpoint.
            Test.PipeId = Test.PipeId And Not USB_ENDPOINT_DIRECTION_MASK
            If Not Test.FindPipeAndInterface(usb, interfaceDescriptorWrite, pipeInfoWrite) Then Return
            Debug.Assert(interfaceDescriptorRead.bInterfaceNumber = interfaceDescriptorWrite.bInterfaceNumber, "This example requires the IN and OUT endpoints have the same bInterfaceNumber.")
            Debug.Assert(pipeInfoRead.MaximumPacketSize > 0 AndAlso pipeInfoRead.MaximumPacketSize = pipeInfoWrite.MaximumPacketSize, "This example requires the IN and OUT endpoints have the same MaximumPacketSize.")

            ' We will keep the buffer >= 1024.
            ' To satisfy StmK it must also be an interval of MaximumPacketSize
            If Test.TransferBufferSize = -1 Then Test.TransferBufferSize = pipeInfoWrite.MaximumPacketSize * ((1024 + pipeInfoWrite.MaximumPacketSize - 1) / pipeInfoWrite.MaximumPacketSize)

#If BMFW
            ' TODO FOR USER: Remove this block if not using benchmark firmware.
            ' This configures devices running benchmark firmware for streaming DeviceToHost transfers.
            Console.WriteLine("Configuring for benchmark device..")
            Dim testType As BM_TEST_TYPE = Test.Devices.BM_TEST_TYPE.LOOP
            success = Test.Devices.Benchmark.Configure(usb, Test.Devices.BM_COMMAND.SET_TEST, interfaceDescriptorRead.bInterfaceNumber, testType)

            If Not success Then
                Console.WriteLine("Bench_Configure failed.")
            End If
#End If
            If Not Test.ShowTestReady() Then
                GoTo Done
            End If

            Dim stmRead As UsbStream = New UsbStream(usb, pipeInfoRead.PipeId, Test.TransferBufferSize, Test.MaxPendingTransfers, Test.MaxPendingIO, True, 1)
            Dim stmWrite As UsbStream = New UsbStream(usb, pipeInfoWrite.PipeId, Test.TransferBufferSize, Test.MaxPendingTransfers, Test.MaxPendingIO, True, 3000)
            Dim stmReader As StreamReader = New StreamReader(stmRead)
            Dim stmWriter As StreamWriter = New StreamWriter(stmWrite)
            stmRead.Start()
            stmWrite.Start()
            Dim chTemp = New Char(Test.TransferBufferSize - 1) {}

            While stmReader.Read(chTemp, 0, chTemp.Length) > 0
                Console.WriteLine("Flushing packets..")
            End While

            Dim sWrite = String.Empty
            Dim sPartialRead = String.Empty
            Dim sRead As String
            Const lineFormat = "Tx#{0:000000} This string is being looped through endpoints {1:X2}h and {2:X2}h"
            Dim lineLength = String.Format(lineFormat, 0, 0, 0).Length

            ' This is a counter/timer used only for statistics gathering.
            Thread.Sleep(0)
            Call Test.Dcs.Start()

            For iTransfers = 0 To Test.MaxTransfersTotal - 1
                sWrite = String.Format(lineFormat, iTransfers, pipeInfoWrite.PipeId, pipeInfoRead.PipeId)
                stmWriter.WriteLine(sWrite)
                Console.WriteLine(sWrite)
                If (iTransfers And 8) > 0 Then stmWriter.Flush()
                If iTransfers <= 16 OrElse (String.IsNullOrEmpty((CSharpImpl.__Assign(sRead, stmReader.ReadLine())))) Then Continue For
                If Not FormatBrokenStrings(lineLength, "Rx", sPartialRead, sRead) Then Continue For
                Console.WriteLine(sRead)
            Next

            stmWriter.Flush()

            While Not (String.IsNullOrEmpty((CSharpImpl.__Assign(sRead, stmReader.ReadLine()))))
                If Not FormatBrokenStrings(lineLength, "Rx", sPartialRead, sRead) Then Continue While
                Console.WriteLine(sRead)
            End While

            Call Test.Dcs.Stop()
            Dim ts As TimeSpan = New TimeSpan(Test.Dcs.Ticks)
            Console.WriteLine("Elapsed Time:" & Microsoft.VisualBasic.Constants.vbLf & Microsoft.VisualBasic.Constants.vbTab & "{0} mins" & Microsoft.VisualBasic.Constants.vbLf & Microsoft.VisualBasic.Constants.vbTab & "{1} secs" & Microsoft.VisualBasic.Constants.vbLf & Microsoft.VisualBasic.Constants.vbTab & "{2} msecs", Math.Floor(ts.TotalMinutes), ts.Seconds, ts.Milliseconds)
            stmWriter.Dispose()
            stmReader.Dispose()
Done:
            usb.Free()
        End Sub

        Private Class CSharpImpl
            <Obsolete("Please refactor calling code to use normal Visual Basic assignment")>
            Shared Function __Assign(Of T)(ByRef target As T, value As T) As T
                target = value
                Return value
            End Function
        End Class
    End Class
End Namespace
