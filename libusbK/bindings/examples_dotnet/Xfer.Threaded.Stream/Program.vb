Imports System
Imports System.Runtime.InteropServices
Imports System.Threading
Imports Test.Devices
Imports libusbK
Imports libusbK.Examples

' ReSharper disable InconsistentNaming

Namespace Xfer.Stm
    Friend Class Program
#Region "TODO USER: Set the test parameters for your device."

        Public Shared Test As StmTestParameters = New StmTestParameters(&H04d8, &Hfa2e, 0, &H81, 1024, Nothing, -1, 4, 64)

#End Region

        Public Shared Sub Main()
            Dim success As Boolean
            Dim pipeInfo As WINUSB_PIPE_INFORMATION_EX
            Dim usb As UsbK
            Dim interfaceDescriptor As USB_INTERFACE_DESCRIPTOR

            ' Find and configure the device.
            If Not Test.ConfigureDevice(pipeInfo, usb, interfaceDescriptor) Then Return
            If Test.TransferBufferSize = -1 Then Test.TransferBufferSize = pipeInfo.MaximumPacketSize * 64

#If BMFW
            ' TODO FOR USER: Remove this block if not using benchmark firmware.
            ' This configures devices running benchmark firmware for streaming DeviceToHost transfers.
            Console.WriteLine("Configuring for benchmark device..")
            Dim testType As BM_TEST_TYPE = If((Test.PipeId And &H80) > 0, Test.Devices.BM_TEST_TYPE.READ, Test.Devices.BM_TEST_TYPE.WRITE)
            success = Test.Devices.Benchmark.Configure(usb, Test.Devices.BM_COMMAND.SET_TEST, interfaceDescriptor.bInterfaceNumber, testType)

            If Not success Then
                Console.WriteLine("Bench_Configure failed.")
            End If
#End If
            If Not Test.ShowTestReady() Then
                GoTo Done
            End If

            Dim callback As KSTM_CALLBACK = New KSTM_CALLBACK()
            Dim stm As StmK = New StmK(usb.Handle, pipeInfo.PipeId, Test.TransferBufferSize, Test.MaxPendingTransfers, Test.MaxPendingIO, callback, KSTM_FLAG.USE_TIMEOUT Or CType(3000, KSTM_FLAG))
            Dim tempBuffer = New Byte(Test.TransferBufferSize - 1) {}
            Thread.Sleep(0)
            ' This is just a counter/timer for statistics gathering.
            Call Test.Dcs.Start()
            success = stm.Start()
            Dim totalTransferCount As Long = 0

            While success
                Dim transferred As UInteger

                If (pipeInfo.PipeId And &H80) = &H80 Then
                    success = stm.Read(tempBuffer, 0, tempBuffer.Length, transferred)
                    If Not success Then Exit While
                Else
                    success = stm.Write(tempBuffer, 0, tempBuffer.Length, transferred)
                    If Not success Then Exit While
                End If

                Dim dataPrefix = String.Format("  Data Prefix: [{0:X2} {1:X2} {2:X2} {3:X2} {4:X2} {5:X2} {6:X2} {7:X2}] ", tempBuffer(0), tempBuffer(1), tempBuffer(2), tempBuffer(3), tempBuffer(4), tempBuffer(5), tempBuffer(6), tempBuffer(7))
                Console.WriteLine(If(totalTransferCount > Test.MaxTransfersTotal, "#{0}: [Stream Stopped] {1} transferred. {2}", "#{0}: {1} transferred. {2}"), totalTransferCount.ToString("0000"), transferred, dataPrefix)
                totalTransferCount += 1
                If totalTransferCount = Test.MaxTransfersTotal Then success = stm.Stop(3000)
            End While

            Console.WriteLine("Done. TotalTransfers:{0} ErrorCode:{1:X8}h", totalTransferCount, Marshal.GetLastWin32Error())
            stm.Free()
Done:
            usb.Free()
        End Sub
    End Class
End Namespace
