#region Copyright (c) Travis Robinson
// Copyright (c) 2011 Travis Robinson <libusbdotnet@gmail.com>
// All rights reserved.
// 
// C# Read.Isochronous
// Auto-generated on: 09.22.2011
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


using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using Win32;
using libusbK;


// ReSharper disable InconsistentNaming

namespace Read.Isochronous
{
    internal class Program
    {
        private const int MY_VID = 0x03eb;
        private const int MY_PID = 0x2315;
        private const byte MY_READ_EP = 0x81;
        private const int MAX_OUTSTANDING_TRANSFERS = 8;
        private const int MAX_TRANSFERS_TOTAL = 24;
        private const int ISO_PACKETS_PER_TRANSFER = 128;

        private static USB_INTERFACE_DESCRIPTOR gInterfaceDescriptor;
        private static WINUSB_PIPE_INFORMATION gPipeInfo;

        private static StreamWriter gLogstream;

        private static readonly HiPerfTimer gDcs = new HiPerfTimer();

        //! Custom vendor requests that must be implemented in the benchmark firmware.

        private static bool Bench_Configure(UsbK usb, BM_COMMAND command, byte interfaceNumber, ref BM_TEST_TYPE testType)
        {
            uint transferred;
            WINUSB_SETUP_PACKET pkt;
            byte[] data = new byte[1];

            pkt.RequestType = (1 << 7) | (2 << 5);
            pkt.Request = (byte) command;

            pkt.Value = (ushort) testType;
            pkt.Index = interfaceNumber;
            pkt.Length = 1;

            bool success = usb.ControlTransfer(pkt, Marshal.UnsafeAddrOfPinnedArrayElement(data, 0), 1, out transferred, IntPtr.Zero);
            testType = (BM_TEST_TYPE) data[0];
            return success;
        }

        private static void Main(string[] args)
        {
            MY_ISO_XFER_LIST MyXfers = new MY_ISO_XFER_LIST();
            LstK deviceList = new LstK(KLST_FLAG.NONE);
            KLST_DEVINFO_HANDLE deviceInfo;

            gLogstream = new StreamWriter("xfer-iso-read.txt");

            bool success = deviceList.FindByVidPid(MY_VID, MY_PID, out deviceInfo);
            if (!success)
            {
                Console.WriteLine("Device not found.");
                return;
            }

            UsbK usb = new UsbK(deviceInfo);

            byte interfaceIndex = 0;
            while (gPipeInfo.PipeId == 0 && usb.SelectInterface(interfaceIndex, true))
            {
                byte altSettingNumber = 0;
                while (gPipeInfo.PipeId == 0 && usb.QueryInterfaceSettings(altSettingNumber, out gInterfaceDescriptor))
                {
                    byte pipeIndex = 0;
                    while (usb.QueryPipe(altSettingNumber, pipeIndex, out gPipeInfo))
                    {
                        if (gPipeInfo.PipeId == MY_READ_EP && gPipeInfo.MaximumPacketSize > 0 && gPipeInfo.PipeType == USBD_PIPE_TYPE.UsbdPipeTypeIsochronous)
                        {
                            break;
                        }
                        pipeIndex++;
                        gPipeInfo = new WINUSB_PIPE_INFORMATION();
                    }
                    altSettingNumber++;
                }
                interfaceIndex++;
            }

            if (gPipeInfo.PipeId == 0)
            {
                Console.WriteLine("Pipe not found.");
                return;
            }

            success = usb.SetAltInterface(gInterfaceDescriptor.bInterfaceNumber, false, gInterfaceDescriptor.bAlternateSetting);
            if (!success)
            {
                Console.WriteLine("SetAltInterface failed.");
                return;
            }

            BM_TEST_TYPE testType = BM_TEST_TYPE.READ;
            success = Bench_Configure(usb, BM_COMMAND.SET_TEST, gInterfaceDescriptor.bInterfaceNumber, ref testType);
            if (!success)
            {
                Console.WriteLine("Bench_Configure failed.");
            }

            MyXfers.OvlPool = new OvlK(usb.Handle, MAX_OUTSTANDING_TRANSFERS, KOVL_POOL_FLAG.NONE);
            MyXfers.DataBufferSize = (uint) (gPipeInfo.MaximumPacketSize*ISO_PACKETS_PER_TRANSFER);
            for (int pos = 0; pos < MAX_OUTSTANDING_TRANSFERS; pos++)
            {
                MY_ISO_BUFFER isoBuffer = new MY_ISO_BUFFER();

                isoBuffer.Buffer = new byte[gPipeInfo.MaximumPacketSize*ISO_PACKETS_PER_TRANSFER];
                isoBuffer.BufferGC = GCHandle.Alloc(isoBuffer.Buffer, GCHandleType.Pinned);

                isoBuffer.Iso = new IsoK(ISO_PACKETS_PER_TRANSFER, 0);
                isoBuffer.Iso.SetPackets(gPipeInfo.MaximumPacketSize);

                MyXfers.OvlPool.Acquire(out isoBuffer.Ovl);

                MyXfers.AddLast(isoBuffer);
                MyXfers.Completed.AddLast(isoBuffer);
            }

            usb.ResetPipe(gPipeInfo.PipeId);

            usb.GetCurrentFrameNumber(out MyXfers.FrameNumber);
            unchecked
            {
                MyXfers.FrameNumber += ISO_PACKETS_PER_TRANSFER * 2;
                MyXfers.FrameNumber -= (MyXfers.FrameNumber % ISO_PACKETS_PER_TRANSFER);
            }

            gDcs.Start();
            do
            {
                MY_ISO_BUFFER isoBuffer;
                uint transferred;

                int errorCode;
                while (MyXfers.Completed.Count > 0 && MyXfers.SubmittedCount < MAX_TRANSFERS_TOTAL)
                {
                    isoBuffer = MyXfers.Completed.First.Value;
                    MyXfers.Completed.RemoveFirst();
                    MyXfers.Outstanding.AddLast(isoBuffer);

                    MyXfers.OvlPool.ReUse(isoBuffer.Ovl);

                    SetNextFrameNumber(MyXfers, isoBuffer);

                    usb.IsoReadPipe(gPipeInfo.PipeId, isoBuffer.BufferGC.AddrOfPinnedObject(), MyXfers.DataBufferSize, isoBuffer.Ovl.DangerousGetHandle(), isoBuffer.Iso.Handle);

                    errorCode = Marshal.GetLastWin32Error();
                    if (errorCode != 997)
                    {
                        Console.WriteLine("IsoReadPipe failed. ErrorCode: {0:X8}h", errorCode);
                        goto Done;
                    }
                    MyXfers.SubmittedCount++;
                }

                if (MyXfers.Outstanding.Count == 0)
                {
                    Console.WriteLine("Done!");
                    goto Done;
                }
                isoBuffer = MyXfers.Outstanding.First.Value;

                success = MyXfers.OvlPool.Wait(isoBuffer.Ovl, 1000, KOVL_WAIT_FLAG.NONE, out transferred);
                if (!success)
                {
                    errorCode = Marshal.GetLastWin32Error();
                    Console.WriteLine("OvlPool.Wait failed. ErrorCode: {0:X8}h", errorCode);
                    goto Done;
                }

                MyXfers.Outstanding.RemoveFirst();
                MyXfers.Completed.AddLast(isoBuffer);

                IsoXferComplete(MyXfers, isoBuffer, transferred);
            } while (true);

            Done:
            IsoXferCleanup(MyXfers);

            if (gLogstream != null)
            {
                gLogstream.Flush();
                gLogstream.Close();
            }

            deviceList.Free();
            usb.Free();
        }

        private static void IsoXferCleanup(MY_ISO_XFER_LIST myIsoTransfers)
        {
            foreach (MY_ISO_BUFFER myIsoBuffer in myIsoTransfers.Outstanding)
            {
                uint transferred;
                myIsoTransfers.OvlPool.WaitAndRelease(myIsoBuffer.Ovl, 0, out transferred);
            }
            myIsoTransfers.Outstanding.Clear();

            foreach (MY_ISO_BUFFER myIsoBuffer in myIsoTransfers)
            {
                myIsoBuffer.Iso.Free();
                myIsoBuffer.BufferGC.Free();
            }
            myIsoTransfers.Completed.Clear();
            myIsoTransfers.Clear();
            myIsoTransfers.OvlPool.Free();
        }

        private static void IsoXferComplete(MY_ISO_XFER_LIST myIsoTransfers, MY_ISO_BUFFER isoBuffer, uint transferLength)
        {
            int packetPos;
            gDcs.Stop((int) transferLength);
            if (myIsoTransfers.LastStartFrame == 0)
            {
                gLogstream.WriteLine("#{0}: StartFrame={1:X8}h TransferLength={2} BPS-average:{3}", 
                    myIsoTransfers.CompletedCount, isoBuffer.Iso.StartFrame, transferLength, Math.Round(gDcs.Bps, 2));
            }
            else
            {
                uint lastFrameCount = isoBuffer.Iso.StartFrame - myIsoTransfers.LastStartFrame;
                gLogstream.WriteLine("#{0}: StartFrame={1:X8}h TransferLength={2} BPS-average:{3} LastFrameCount={4}",
                    myIsoTransfers.CompletedCount, isoBuffer.Iso.StartFrame, transferLength, Math.Round(gDcs.Bps, 2), lastFrameCount);
            }

            int numPackets = isoBuffer.Iso.NumberOfPackets;
            for (packetPos = 0; packetPos < numPackets; packetPos++)
            {
                KISO_PACKET isoPacket;
                isoBuffer.Iso.GetPacket(packetPos, out isoPacket);
                if (isoPacket.Length > 1)
                {
                    byte firstPacketByte = isoBuffer.Buffer[isoPacket.Offset];
                    byte secondPacketByte = isoBuffer.Buffer[isoPacket.Offset + 1];
                    gLogstream.WriteLine("  [{0:000}] Length={1} {2:X2}h {3:X2}h", packetPos, isoPacket.Length, firstPacketByte, secondPacketByte);
                }
                else
                {
                    gLogstream.WriteLine("  [{0:000}] Empty Packet", packetPos);
                }
            }

            myIsoTransfers.CompletedCount++;
            myIsoTransfers.LastStartFrame = isoBuffer.Iso.StartFrame;
        }

        private static void SetNextFrameNumber(MY_ISO_XFER_LIST myIsoTransfers, MY_ISO_BUFFER isoBuffer)
        {
        }
    }

    internal enum BM_COMMAND
    {
        SET_TEST = 0x0E,
        GET_TEST = 0x0F,
    };

    internal class MY_ISO_BUFFER
    {
        public byte[] Buffer;
        public GCHandle BufferGC;
        public IsoK Iso;
        public KOVL_HANDLE Ovl;
    }

    internal class MY_ISO_XFER_LIST : LinkedList<MY_ISO_BUFFER>
    {
        public LinkedList<MY_ISO_BUFFER> Completed = new LinkedList<MY_ISO_BUFFER>();

        public uint CompletedCount;
        public uint DataBufferSize;

        public uint FrameNumber;

        public uint LastStartFrame;
        public LinkedList<MY_ISO_BUFFER> Outstanding = new LinkedList<MY_ISO_BUFFER>();
        public OvlK OvlPool;
        public uint SubmittedCount;
    }

    internal enum BM_TEST_TYPE
    {
        NONE = 0x00,
        READ = 0x01,
        WRITE = 0x02,
        LOOP = READ | WRITE,
    };
}