#region Copyright(c) Travis Robinson

// Copyright (c) 2011-2012 Travis Robinson <libusbdotnet@gmail.com>
// All rights reserved.
// 
// Read.Isochronous
// 
// Last Updated: 03.08.2012
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
// 	  
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS LEE ROBINSON 
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
// THE POSSIBILITY OF SUCH DAMAGE.

#endregion

#define BMFW

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;
using Test.Devices;
using libusbK;
using libusbK.Examples;

namespace Read.Isochronous
{
    // ReSharper disable InconsistentNaming

    internal class Program
    {
        #region TODO USER: Set the test parameters for your device.

        //public static IsoTestParameters Test = new IsoTestParameters(0x04d8, 0xfa2e, 0, 0x82, 3, 64,null, 24);
        public static IsoTestParameters Test = new IsoTestParameters(0x04b4, 0x00fd, 1, 0x83, 3, 12, null, 24);

        #endregion

        private static void Main(string[] args)
        {
            bool success;
            WINUSB_PIPE_INFORMATION_EX pipeInfo;
            UsbK usb;
            USB_INTERFACE_DESCRIPTOR interfaceDescriptor;

            // Find and configure the device.
            if (!Test.ConfigureDevice(out pipeInfo, out usb, out interfaceDescriptor)) return;

#if BMFW
            // TODO FOR USER: Remove this block if not using benchmark firmware.
            // This configures devices running benchmark firmware for streaming DeviceToHost transfers.
            Console.WriteLine("Configuring for benchmark device..");
            BM_TEST_TYPE testType = BM_TEST_TYPE.READ;
            success = Benchmark.Configure(usb, BM_COMMAND.SET_TEST, interfaceDescriptor.bInterfaceNumber, ref testType);
            if (!success)
            {
                Console.WriteLine("Bench_Configure failed.");
            }
#endif
            // Create the ISO transfer queue.  This class manages the pending and outstanding transfer lists.
            ReadIsoTransferQueue readXfers = new ReadIsoTransferQueue(usb, ref pipeInfo, Test.MaxOutstandingTransfers, Test.IsoPacketsPerTransfer);

            if (!Test.ShowTestReady()) goto Done;

            // Always issue a reset pipe prior to re/starting an ISO stream.
            usb.ResetPipe(pipeInfo.PipeId);

            // This example will not manage the start frame manually, but this is
            // how I might caculate the starting point.
            usb.GetCurrentFrameNumber(out readXfers.FrameNumber);
            unchecked
            {
                // Add some start latency
                readXfers.FrameNumber += (uint)Test.MaxOutstandingTransfers;

                // Start FrameNumber at an interval of 8.
                if (readXfers.FrameNumber % 8 > 0) readXfers.FrameNumber = (uint) ((readXfers.FrameNumber & (~0x7)) + 8);
            }

            // This is a counter/timer used only for statistics gathering.
            Thread.Sleep(0);
            Test.Dcs.Start();

            // Transfer processing loop
            do
            {
                IsoTransferItem isoTransferItem;
                uint transferred;
                int errorCode;
                // While buffers exist in the completed list, submit them. 
                while (readXfers.Completed.Count > 0 && readXfers.Outstanding.Count < Test.MaxOutstandingTransfers)
                {
                    errorCode = readXfers.SubmitNextRead();
                    if (errorCode != 0)
                    {
                        Console.WriteLine("IsoReadPipe failed. ErrorCode: {0:X8}h", errorCode);
                        goto Done;
                    }
                }

                // Wait for the oldest transfer to complete.
                errorCode = readXfers.WaitRead(out isoTransferItem, 1000, out transferred);
                if (errorCode != 0)
                {
                    Console.WriteLine("OvlPool.Wait failed. ErrorCode: {0:X8}h", errorCode);
                    goto Done;
                }

                // Report iso status.
                IsoXferReport(readXfers, isoTransferItem, transferred);

                if (readXfers.CompletedCount == 1) Test.Dcs.Start();
            } while (readXfers.CompletedCount < Test.MaxTransfersTotal);

            Done:
            readXfers.Destroy();
            Test.Free();

            usb.Free();
        }

        private static void IsoXferReport(ReadIsoTransferQueue readIsoTransfers, IsoTransferItem isoTransferItem, uint transferLength)
        {
            int packetPos;
            Test.Dcs.Stop((int) transferLength);
            if (readIsoTransfers.LastStartFrame == 0)
            {
                Test.LogLn("#{0}: StartFrame={1:X8}h TransferLength={2} BPS-average:{3}",
                           readIsoTransfers.CompletedCount,
                           isoTransferItem.FrameNumber,
                           transferLength,
                           Math.Round(Test.Dcs.Bps, 2));
            }
            else
            {
                uint lastFrameCount = isoTransferItem.FrameNumber - readIsoTransfers.LastStartFrame;
                Test.LogLn("#{0}: StartFrame={1:X8}h TransferLength={2} BPS-average:{3} LastFrameCount={4}",
                           readIsoTransfers.CompletedCount,
                           isoTransferItem.FrameNumber,
                           transferLength,
                           Math.Round(Test.Dcs.Bps, 2),
                           lastFrameCount);
            }

            uint numPackets;
            isoTransferItem.Iso.GetNumberOfPackets(out numPackets);
            for (packetPos = 0; packetPos < numPackets; packetPos++)
            {
                KISO_PACKET isoPacket;
                uint packetOffset;
                uint packetLength;
                uint packetStatus;
                isoTransferItem.Iso.GetPacket((uint) packetPos,out packetOffset, out packetLength, out packetStatus);
                if (packetLength > 1)
                {
                    // This is somewhat specific to data that is returned by the benchmark firmware.
                    byte firstPacketByte = isoTransferItem.Buffer[packetOffset];
                    byte secondPacketByte = isoTransferItem.Buffer[packetOffset + 1];
                    Test.LogLn("  [{0:000}] Length={1} B0={2:X2}h B1={3:X2}h", packetPos, packetLength, firstPacketByte, secondPacketByte);
                }
                else
                {
                    Test.LogLn("  [{0:000}] Empty Packet", packetPos);
                }
            }

            readIsoTransfers.CompletedCount++;
            readIsoTransfers.LastStartFrame = isoTransferItem.FrameNumber;
        }
    }

    internal class IsoTransferItem
    {
        public readonly ReadIsoTransferQueue Container;

        public IsoTransferItem(ReadIsoTransferQueue container)
        {
            Container = container;
        }
        public byte[] Buffer;
        public GCHandle BufferGC;
        public IsochK Iso;
        public KOVL_HANDLE Ovl;
        public uint FrameNumber;
    }

    internal class ReadIsoTransferQueue
    {
        #region Transfer lists

        public LinkedList<IsoTransferItem> Completed = new LinkedList<IsoTransferItem>();
        public LinkedList<IsoTransferItem> Outstanding = new LinkedList<IsoTransferItem>();

        #endregion

        #region Core queue members

        public readonly int DataBufferSize;
        public readonly OvlK OvlPool;
        public readonly WINUSB_PIPE_INFORMATION_EX PipeInfo;
        public readonly UsbK Usb;

        #endregion

        #region Frame management

        public uint FrameNumber;
        public uint LastStartFrame;

        #endregion

        #region Statisitics

        public uint CompletedCount;
        public uint TotalSubmittedCount;

        #endregion

        public ReadIsoTransferQueue(UsbK usb, ref WINUSB_PIPE_INFORMATION_EX pipeInfo, int maxPendingTransfers, uint numberOfPackets)
        {
            PipeInfo = pipeInfo;
            Usb = usb;
            byte[] deviceSpeedBuffer = new byte[1];
            uint deviceSpeedLength = 1;
            GCHandle gcDeviceSpeed = GCHandle.Alloc(deviceSpeedBuffer, GCHandleType.Pinned);
            Usb.QueryDeviceInformation((uint) DeviceInformationType.DEVICE_SPEED, ref deviceSpeedLength, Marshal.UnsafeAddrOfPinnedArrayElement(deviceSpeedBuffer, 0));
            gcDeviceSpeed.Free();
            OvlPool = new OvlK(usb.Handle, maxPendingTransfers, KOVL_POOL_FLAG.NONE);
            DataBufferSize = (int) (pipeInfo.MaximumBytesPerInterval * numberOfPackets);
            for (int pos = 0; pos < maxPendingTransfers; pos++)
            {
                IsoTransferItem isoTransferItem = new IsoTransferItem(this);

                isoTransferItem.Buffer = new byte[DataBufferSize];
                isoTransferItem.BufferGC = GCHandle.Alloc(isoTransferItem.Buffer, GCHandleType.Pinned);

                isoTransferItem.Iso = new IsochK(usb.Handle, pipeInfo.PipeId, numberOfPackets, isoTransferItem.BufferGC.AddrOfPinnedObject(), (uint) isoTransferItem.Buffer.Length);
                isoTransferItem.Iso.SetPacketOffsets(pipeInfo.MaximumBytesPerInterval);

                OvlPool.Acquire(out isoTransferItem.Ovl);

                Completed.AddLast(isoTransferItem);
            }
        }

        public void Destroy()
        {
            // Cancel any outstanding IO and release the OvlK.
            foreach (IsoTransferItem myIsoBuffer in Outstanding)
            {
                uint transferred;
                OvlK.WaitAndRelease(myIsoBuffer.Ovl, 0, out transferred);
            }

            foreach (IsoTransferItem myIsoBuffer in Outstanding)
            {
                myIsoBuffer.Iso.Free();
                myIsoBuffer.BufferGC.Free();
            }
            Outstanding.Clear();

            foreach (IsoTransferItem myIsoBuffer in Completed)
            {
                myIsoBuffer.Iso.Free();
                myIsoBuffer.BufferGC.Free();
            }
            Completed.Clear();

            OvlPool.Free();
        }

        public int SubmitNextRead()
        {
            // Pull from head of Completed list and push to end of Outstanding list
            IsoTransferItem isoTransferItem = Completed.First.Value;
            Completed.RemoveFirst();
            Outstanding.AddLast(isoTransferItem);

            // Prepare the OvlK handle to be submitted.
            OvlK.ReUse(isoTransferItem.Ovl);

            // Set the frame number this transfer will be submitted on (used for logging only)
            isoTransferItem.FrameNumber = isoTransferItem.Container.FrameNumber;

            // submit the transfer. use 0 for length and 0 for number of packets to use the values the isoch handle was initialized with
            Usb.IsochReadPipe(isoTransferItem.Iso.Handle, 0, ref isoTransferItem.Container.FrameNumber, 0, isoTransferItem.Ovl);

            // IsochReadPipe will always return false so we need to check the error code
            int errorCode = Marshal.GetLastWin32Error();

            // 997 is ERROR_IO_PENDING. For async, this means so far so good.
            // IE: The transfer is submitted but we won't know if a problem occurs until it is completed.
            if (errorCode != ErrorCodes.IoPending) return errorCode;

            TotalSubmittedCount++;

            return 0;
        }

        public int WaitRead(out IsoTransferItem isoTransferItem, int i, out uint transferred)
        {
            isoTransferItem = Outstanding.First.Value;
            bool success = OvlK.Wait(isoTransferItem.Ovl, 1000, KOVL_WAIT_FLAG.NONE, out transferred);
            if (!success)
            {
                return Marshal.GetLastWin32Error();
            }

            Outstanding.RemoveFirst();
            Completed.AddLast(isoTransferItem);

            return 0;
        }
    }
}