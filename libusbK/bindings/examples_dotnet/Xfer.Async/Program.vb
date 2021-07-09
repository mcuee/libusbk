Imports System
Imports System.Collections.Generic
Imports System.Diagnostics
Imports System.Runtime.InteropServices
Imports Test.Devices
Imports libusbK
Imports libusbK.Examples

' ReSharper disable InconsistentNaming

Namespace Xfer.Async
    Friend Class Program
#Region "TODO USER: Set the test parameters for your device."

        Public Shared Test As AsyncTestParameters = New AsyncTestParameters(&H04d8, &Hfa2e, 0, &H81, 9, Nothing, -1, 3)

#End Region

        public Shared Sub Main()
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

            ' In most cases, you should clear the pipe timeout policy before using asynchronous I/O. (INFINITE)
            '  This decreases overhead in the driver and generally we managed the timeout ourselves from user code.
            '  
            '  Set the PIPE_TRANSFER_TIMEOUT policy to INFINITE:
            ' 
            Dim pipeTimeoutMS = {0}
            usb.SetPipePolicy(Test.PipeId, PipePolicyType.PIPE_TRANSFER_TIMEOUT, Marshal.SizeOf(GetType(Integer)), pipeTimeoutMS)


            ' In some cases, you may want to set the RAW_IO pipe policy.  This will impose more restrictions on the
            '  allowable transfer buffer sizes but will improve performance. (See documentation for restrictions)
            '  
            '  Set the RAW_IO policy to TRUE:
            ' 
            Dim useRawIO = {1}
            usb.SetPipePolicy(Test.PipeId, PipePolicyType.RAW_IO, Marshal.SizeOf(GetType(Integer)), useRawIO)
            Dim totalSubmittedTransfers = 0
            Dim totalCompletedTransfers = 0

            ' We need a different buffer for each MaxPendingIO slot because they will all be submitted to the driver 
            '  (outstanding) at the same time.
            ' 
            Dim transferBuffers = New Byte(Test.MaxPendingIO - 1)() {}
            Dim ovlPool As OvlK = New OvlK(usb.Handle, Test.MaxPendingIO, KOVL_POOL_FLAG.NONE)
            Dim transferBufferGCList As List(Of GCHandle) = New List(Of GCHandle)(Test.MaxPendingIO)
            success = True

            ' Start transferring data synchronously; one transfer at a time until the test limit (MaxTransfersTotal) is hit.
            While success AndAlso totalCompletedTransfers < Test.MaxTransfersTotal
                Dim ovlHandle As KOVL_HANDLE
                Dim transferred As UInteger

                While success AndAlso totalSubmittedTransfers < Test.MaxTransfersTotal
                    ' Get the next KOVL_HANDLE
                    If Not ovlPool.Acquire(ovlHandle) Then
                        ' This example expects a failure of NoMoreItems when the pool has no more handles to acquire.  This is
                        ' our indication that it is time to begin waiting for a completion.
                        Call Debug.Assert(Marshal.GetLastWin32Error() = NoMoreItems)
                        Exit While
                    End If

                    ' Get the next transfer buffer
                    Dim transferBufferIndex = totalSubmittedTransfers Mod Test.MaxPendingIO

                    If transferBuffers(transferBufferIndex) Is Nothing Then
                        ' This index has not been allocated yet.  We need a different buffer for each MaxPendingIO
                        '  slot because they will all be submitted to the driver (outstanding) at the same time.
                        ' 

                        Console.WriteLine("Allocating transfer buffer at index:{0}", transferBufferIndex)
                        transferBuffers(transferBufferIndex) = New Byte(Test.TransferBufferSize - 1) {}

                        ' Transfer buffers should be pinned so the garbage collector cannot move the memory.
                        transferBufferGCList.Add(GCHandle.Alloc(transferBuffers(transferBufferIndex), GCHandleType.Pinned))
                    End If

                    Dim transferBuffer = transferBuffers(transferBufferIndex)
                    Dim not_used_for_async As UInteger

                    If (Test.PipeId And USB_ENDPOINT_DIRECTION_MASK) > 0 Then
                        success = usb.ReadPipe(Test.PipeId, transferBuffer, transferBuffer.Length, not_used_for_async, ovlHandle)
                    Else
                        FillMyBufferForWrite(transferBuffer, transferred)
                        success = usb.WritePipe(Test.PipeId, transferBuffer, transferred, not_used_for_async, ovlHandle)
                    End If

                    If Marshal.GetLastWin32Error() = IoPending Then
                        success = True
                        totalSubmittedTransfers += 1
                        Console.WriteLine("Pending  #{0:0000} {1} bytes.", totalSubmittedTransfers, transferBuffer.Length)
                    Else
                        Console.WriteLine("Pending  #{0:0000} failed. ErrorCode={1:X8}h", totalSubmittedTransfers, Marshal.GetLastWin32Error())
                    End If
                End While

                If Not success Then Exit While

                ' Wait for the oldest transfer to complete and release the ovlHandle to the pool so it can be re-acquired.
                success = ovlPool.WaitOldest(ovlHandle, 1000, KOVL_WAIT_FLAG.RELEASE_ALWAYS, transferred)
                totalCompletedTransfers += 1

                If success Then
                    If (Test.PipeId And USB_ENDPOINT_DIRECTION_MASK) > 0 Then
                        ProcessMyBufferFromRead(transferBuffers((totalCompletedTransfers - 1) Mod Test.MaxPendingIO), transferred)
                    End If

                    Console.WriteLine("Complete #{0:0000} {1} bytes.", totalCompletedTransfers, transferred)
                Else
                    Console.WriteLine("Complete #{0:0000} Wait failed. ErrorCode={1:X8}h", totalCompletedTransfers, Marshal.GetLastWin32Error())
                End If
            End While

            If Not success Then Console.WriteLine("An error occured transferring data. ErrorCode: {0:X8}h", Marshal.GetLastWin32Error())
            ovlPool.Free()

            ' Free the GC handles allocated for the transfer buffers
            For Each gcHandle In transferBufferGCList
                gcHandle.Free()
            Next

Done:
            usb.Free()
        End Sub

#Region "TODO USER: Use these functions to process and fill the transfer buffers"

        Private Shared Sub ProcessMyBufferFromRead(ByVal transferBuffer As Byte(), ByVal transferred As UInteger)
        End Sub

        Private Shared Sub FillMyBufferForWrite(ByVal transferBuffer As Byte(), <Out> ByRef length As UInteger)
            length = CUInt(transferBuffer.Length)
        End Sub

#End Region
    End Class
End Namespace
