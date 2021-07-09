Imports System
Imports System.Collections.Generic
Imports System.Runtime.InteropServices
Imports System.Threading
Imports Test.Devices
Imports libusbK
Imports libusbK.Examples

Namespace Read.Isochronous
    ' ReSharper disable InconsistentNaming

    Friend Class Program
#Region "TODO USER: Set the test parameters for your device."

        'public static IsoTestParameters Test = new IsoTestParameters(0x04d8, 0xfa2e, 0, 0x82, 3, 64,null, 24);
        Public Shared Test As IsoTestParameters = New IsoTestParameters(&H04b4, &H00fD, 1, &H83, 3, 12, Nothing, 24)

#End Region

        public Shared Sub Main(ByVal args As String())
            Dim success As Boolean
            Dim pipeInfo As WINUSB_PIPE_INFORMATION_EX
            Dim usb As UsbK
            Dim interfaceDescriptor As USB_INTERFACE_DESCRIPTOR

            ' Find and configure the device.
            If Not Test.ConfigureDevice(pipeInfo, usb, interfaceDescriptor) Then Return

#If BMFW
            ' TODO FOR USER: Remove this block if not using benchmark firmware.
            ' This configures devices running benchmark firmware for streaming DeviceToHost transfers.
            Console.WriteLine("Configuring for benchmark device..")
            Dim testType As BM_TEST_TYPE = Test.Devices.BM_TEST_TYPE.READ
            success = Test.Devices.Benchmark.Configure(usb, Test.Devices.BM_COMMAND.SET_TEST, interfaceDescriptor.bInterfaceNumber, testType)

            If Not success Then
                Console.WriteLine("Bench_Configure failed.")
            End If
#End If
            ' Create the ISO transfer queue.  This class manages the pending and outstanding transfer lists.
            Dim readXfers As ReadIsoTransferQueue = New ReadIsoTransferQueue(usb, pipeInfo, Test.MaxOutstandingTransfers, Test.IsoPacketsPerTransfer)

            If Not Test.ShowTestReady() Then
                GoTo Done
            End If

            ' Always issue a reset pipe prior to re/starting an ISO stream.
            usb.ResetPipe(pipeInfo.PipeId)

            ' This example will not manage the start frame manually, but this is
            ' how I might caculate the starting point.
            usb.GetCurrentFrameNumber(readXfers.FrameNumber)
            ' Add some start latency
            readXfers.FrameNumber += CUInt(Test.MaxOutstandingTransfers)

            ' Start FrameNumber at an interval of 8.
            If readXfers.FrameNumber Mod 8 > 0 Then readXfers.FrameNumber = CUInt((readXfers.FrameNumber And Not &H7) + 8)

            ' This is a counter/timer used only for statistics gathering.
            Thread.Sleep(0)
            Call Test.Dcs.Start()

            ' Transfer processing loop
            Do
                Dim isoTransferItem As IsoTransferItem
                Dim transferred As UInteger
                Dim errorCode As Integer
                ' While buffers exist in the completed list, submit them. 
                While readXfers.Completed.Count > 0 AndAlso readXfers.Outstanding.Count < Test.MaxOutstandingTransfers
                    errorCode = readXfers.SubmitNextRead()

                    If errorCode <> 0 Then
                        Console.WriteLine("IsoReadPipe failed. ErrorCode: {0:X8}h", errorCode)
                        GoTo Done
                    End If
                End While

                ' Wait for the oldest transfer to complete.
                errorCode = readXfers.WaitRead(isoTransferItem, 1000, transferred)

                If errorCode <> 0 Then
                    Console.WriteLine("OvlPool.Wait failed. ErrorCode: {0:X8}h", errorCode)
                    GoTo Done
                End If

                ' Report iso status.
                IsoXferReport(readXfers, isoTransferItem, transferred)
                If readXfers.CompletedCount = 1 Then Call Test.Dcs.Start()
            Loop While readXfers.CompletedCount < Test.MaxTransfersTotal

Done:
            readXfers.Destroy()
            Call Test.Free()
            usb.Free()
        End Sub

        Private Shared Sub IsoXferReport(ByVal readIsoTransfers As ReadIsoTransferQueue, ByVal isoTransferItem As IsoTransferItem, ByVal transferLength As UInteger)
            Dim packetPos As Integer
            Test.Dcs.Stop(transferLength)

            If readIsoTransfers.LastStartFrame = 0 Then
                Test.LogLn("#{0}: StartFrame={1:X8}h TransferLength={2} BPS-average:{3}", readIsoTransfers.CompletedCount, isoTransferItem.FrameNumber, transferLength, Math.Round(Test.Dcs.Bps, 2))
            Else
                Dim lastFrameCount = isoTransferItem.FrameNumber - readIsoTransfers.LastStartFrame
                Test.LogLn("#{0}: StartFrame={1:X8}h TransferLength={2} BPS-average:{3} LastFrameCount={4}", readIsoTransfers.CompletedCount, isoTransferItem.FrameNumber, transferLength, Math.Round(Test.Dcs.Bps, 2), lastFrameCount)
            End If

            Dim numPackets As UInteger
            isoTransferItem.Iso.GetNumberOfPackets(numPackets)

            For packetPos = 0 To numPackets - 1
                Dim isoPacket As KISO_PACKET
                Dim packetOffset As UInteger
                Dim packetLength As UInteger
                Dim packetStatus As UInteger
                isoTransferItem.Iso.GetPacket(packetPos, packetOffset, packetLength, packetStatus)

                If packetLength > 1 Then
                    ' This is somewhat specific to data that is returned by the benchmark firmware.
                    Dim firstPacketByte = isoTransferItem.Buffer(packetOffset)
                    Dim secondPacketByte = isoTransferItem.Buffer(packetOffset + 1)
                    Test.LogLn("  [{0:000}] Length={1} B0={2:X2}h B1={3:X2}h", packetPos, packetLength, firstPacketByte, secondPacketByte)
                Else
                    Test.LogLn("  [{0:000}] Empty Packet", packetPos)
                End If
            Next

            readIsoTransfers.CompletedCount += 1
            readIsoTransfers.LastStartFrame = isoTransferItem.FrameNumber
        End Sub
    End Class

    Friend Class IsoTransferItem
        Public ReadOnly Container As ReadIsoTransferQueue

        Public Sub New(ByVal container As ReadIsoTransferQueue)
            Me.Container = container
        End Sub

        Public Buffer As Byte()
        Public BufferGC As GCHandle
        Public Iso As IsochK
        Public Ovl As KOVL_HANDLE
        Public FrameNumber As UInteger
    End Class

    Friend Class ReadIsoTransferQueue
#Region "Transfer lists"

        Public Completed As LinkedList(Of IsoTransferItem) = New LinkedList(Of IsoTransferItem)()
        Public Outstanding As LinkedList(Of IsoTransferItem) = New LinkedList(Of IsoTransferItem)()

#End Region

#Region "Core queue members"

        Public ReadOnly DataBufferSize As Integer
        Public ReadOnly OvlPool As OvlK
        Public ReadOnly PipeInfo As WINUSB_PIPE_INFORMATION_EX
        Public ReadOnly Usb As UsbK

#End Region

#Region "Frame management"

        Public FrameNumber As UInteger
        Public LastStartFrame As UInteger

#End Region

#Region "Statisitics"

        Public CompletedCount As UInteger
        Public TotalSubmittedCount As UInteger

#End Region

        Public Sub New(ByVal usb As UsbK, ByRef pipeInfo As WINUSB_PIPE_INFORMATION_EX, ByVal maxPendingTransfers As Integer, ByVal numberOfPackets As UInteger)
            Me.PipeInfo = pipeInfo
            Me.Usb = usb
            Dim deviceSpeedBuffer = New Byte(0) {}
            Dim deviceSpeedLength As UInteger = 1
            Dim gcDeviceSpeed = GCHandle.Alloc(deviceSpeedBuffer, GCHandleType.Pinned)
            Me.Usb.QueryDeviceInformation(DeviceInformationType.DEVICE_SPEED, deviceSpeedLength, Marshal.UnsafeAddrOfPinnedArrayElement(deviceSpeedBuffer, 0))
            gcDeviceSpeed.Free()
            OvlPool = New OvlK(usb.Handle, maxPendingTransfers, KOVL_POOL_FLAG.NONE)
            DataBufferSize = CInt(pipeInfo.MaximumBytesPerInterval * numberOfPackets)

            For pos = 0 To maxPendingTransfers - 1
                Dim isoTransferItem As IsoTransferItem = New IsoTransferItem(Me)
                isoTransferItem.Buffer = New Byte(DataBufferSize - 1) {}
                isoTransferItem.BufferGC = GCHandle.Alloc(isoTransferItem.Buffer, GCHandleType.Pinned)
                isoTransferItem.Iso = New IsochK(usb.Handle, pipeInfo.PipeId, numberOfPackets, isoTransferItem.BufferGC.AddrOfPinnedObject(), isoTransferItem.Buffer.Length)
                isoTransferItem.Iso.SetPacketOffsets(pipeInfo.MaximumBytesPerInterval)
                OvlPool.Acquire(isoTransferItem.Ovl)
                Completed.AddLast(isoTransferItem)
            Next
        End Sub

        Public Sub Destroy()
            ' Cancel any outstanding IO and release the OvlK.
            For Each myIsoBuffer In Outstanding
                Dim transferred As UInteger
                OvlK.WaitAndRelease(myIsoBuffer.Ovl, 0, transferred)
            Next

            For Each myIsoBuffer In Outstanding
                myIsoBuffer.Iso.Free()
                myIsoBuffer.BufferGC.Free()
            Next

            Outstanding.Clear()

            For Each myIsoBuffer In Completed
                myIsoBuffer.Iso.Free()
                myIsoBuffer.BufferGC.Free()
            Next

            Completed.Clear()
            OvlPool.Free()
        End Sub

        Public Function SubmitNextRead() As Integer
            ' Pull from head of Completed list and push to end of Outstanding list
            Dim isoTransferItem = Completed.First.Value
            Completed.RemoveFirst()
            Outstanding.AddLast(isoTransferItem)

            ' Prepare the OvlK handle to be submitted.
            OvlK.ReUse(isoTransferItem.Ovl)

            ' Set the frame number this transfer will be submitted on (used for logging only)
            isoTransferItem.FrameNumber = isoTransferItem.Container.FrameNumber

            ' submit the transfer. use 0 for length and 0 for number of packets to use the values the isoch handle was initialized with
            Usb.IsochReadPipe(isoTransferItem.Iso.Handle, 0, isoTransferItem.Container.FrameNumber, 0, isoTransferItem.Ovl)

            ' IsochReadPipe will always return false so we need to check the error code
            Dim errorCode As Integer = Marshal.GetLastWin32Error()

            ' 997 is ERROR_IO_PENDING. For async, this means so far so good.
            ' IE: The transfer is submitted but we won't know if a problem occurs until it is completed.
            If errorCode <> IoPending Then Return errorCode
            TotalSubmittedCount += 1
            Return 0
        End Function

        Public Function WaitRead(<Out> ByRef isoTransferItem As IsoTransferItem, ByVal i As Integer, <Out> ByRef transferred As UInteger) As Integer
            isoTransferItem = Outstanding.First.Value
            Dim success = OvlK.Wait(isoTransferItem.Ovl, 1000, KOVL_WAIT_FLAG.NONE, transferred)

            If Not success Then
                Return Marshal.GetLastWin32Error()
            End If

            Outstanding.RemoveFirst()
            Completed.AddLast(isoTransferItem)
            Return 0
        End Function
    End Class
End Namespace
