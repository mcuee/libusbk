Imports System
Imports System.Runtime.InteropServices
Imports Test.Devices
Imports libusbK
Imports libusbK.Examples

' ReSharper disable InconsistentNaming

Namespace Xfer.Sync
    Friend Class Program
#Region "TODO USER: Set the test parameters for your device."

        Public Shared Test As StmTestParameters = New StmTestParameters(&H04d8, &Hfa2e, 0, &H01, 1024, Nothing, -1, 4, 64)

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

            ' In most cases, you should to set the pipe timeout policy before using synchronous I/O.
            ' By default, sync transfers wait infinitely for a transfer to complete.

            ' Set the pipe timeout policy to 3000ms
            Dim pipeTimeoutMS = {3000}
            usb.SetPipePolicy(Test.PipeId, PipePolicyType.PIPE_TRANSFER_TIMEOUT, Marshal.SizeOf(GetType(Integer)), pipeTimeoutMS)
            Dim totalTransfers = 0
            success = True
            Dim tempBuffer = New Byte(Test.TransferBufferSize - 1) {}

            While success AndAlso Math.Min(Threading.Interlocked.Increment(totalTransfers), totalTransfers - 1) < Test.MaxTransfersTotal
                Dim transferred As UInteger

                If (Test.PipeId And &H80) > 0 Then
                    success = usb.ReadPipe(Test.PipeId, tempBuffer, tempBuffer.Length, transferred, IntPtr.Zero)
                Else
                    success = usb.WritePipe(Test.PipeId, tempBuffer, tempBuffer.Length, transferred, IntPtr.Zero)
                End If

                Console.WriteLine("#{0:0000} Transferred {1} bytes.", totalTransfers, transferred)
            End While

            If Not success Then
                Console.WriteLine("An error occured transferring data. ErrorCode: {0:X8}h", Marshal.GetLastWin32Error())
            End If

Done:
            usb.Free()
        End Sub
    End Class
End Namespace
