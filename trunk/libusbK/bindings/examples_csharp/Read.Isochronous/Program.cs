#region Copyright (c) Travis Robinson
// Copyright (c) 2011 Travis Robinson <libusbdotnet@gmail.com>
// All rights reserved.
// 
// C# Read.Isochronous
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
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading;
using Test.Devices;
using Win32;
using libusbK;


namespace Read.Isochronous
{
    // ReSharper disable InconsistentNaming

    internal class Program
    {
        public static TestParameters Test;

        private static void Main(string[] args)
        {
            LstK deviceList = new LstK(KLST_FLAG.NONE);
            KLST_DEVINFO_HANDLE deviceInfo;

            // TODO User: Set the test parameters for your device.
            Test = new TestParameters(0x03eb, 0x2315, 0x81, 4, 24, 64, "xfer-iso-read.txt");
            Test.FillFromCommandLine(Environment.CommandLine);
            Test.Init();

            Console.WriteLine("Finding usb device by VID/PID.. ({0:X4}h / {1:X4}h)", Test.Vid, Test.Pid);
            bool success = deviceList.FindByVidPid(Test.Vid, Test.Pid, out deviceInfo);
            if (!success)
            {
                Console.WriteLine("Usb device not found.");
                return;
            }

            Console.WriteLine("Opening usb device..");
            UsbK usb = new UsbK(deviceInfo);

            Console.WriteLine("Finding interface and endpoint by PipeId..");
            USB_INTERFACE_DESCRIPTOR interfaceDescriptor = new USB_INTERFACE_DESCRIPTOR();
            WINUSB_PIPE_INFORMATION pipeInfo = new WINUSB_PIPE_INFORMATION();
            byte interfaceIndex = 0;
            while (usb.SelectInterface(interfaceIndex, true))
            {
                byte altSettingNumber = 0;
                while (usb.QueryInterfaceSettings(altSettingNumber, out interfaceDescriptor))
                {
                    byte pipeIndex = 0;
                    while (usb.QueryPipe(altSettingNumber, pipeIndex++, out pipeInfo))
                    {
                        if (pipeInfo.PipeId == Test.PipeId && pipeInfo.MaximumPacketSize > 0 &&
                            pipeInfo.PipeType == USBD_PIPE_TYPE.UsbdPipeTypeIsochronous)
                        {
                            goto FindInterfaceDone;
                        }
                        pipeInfo.PipeId = 0;
                    }
                    altSettingNumber++;
                }
                interfaceIndex++;
            }
            FindInterfaceDone:
            if (pipeInfo.PipeId == 0)
            {
                Console.WriteLine("{0}={1} not found.", "PipeId", Test.PipeId);
                return;
            }

            // Set interface alt setting if needed.
            if (interfaceDescriptor.bAlternateSetting > 0)
            {
                Console.WriteLine("Setting interface #{0} to bAlternateSetting #{1}..",
                                  interfaceDescriptor.bInterfaceNumber,
                                  interfaceDescriptor.bAlternateSetting);
                success = usb.SetAltInterface(interfaceDescriptor.bInterfaceNumber, false, interfaceDescriptor.bAlternateSetting);
                if (!success)
                {
                    Console.WriteLine("SetAltInterface failed.");
                    return;
                }
            }

            // TODO FOR USER: Remove this block if not using benchmark firmware.
            // This configures devices running benchmark firmware for streaming DeviceToHost transfers.
            Console.WriteLine("Configuring for benchmark device..");
            BM_TEST_TYPE testType = BM_TEST_TYPE.READ;
            success = Benchmark.Configure(usb, BM_COMMAND.SET_TEST, interfaceDescriptor.bInterfaceNumber, ref testType);
            if (!success)
            {
                Console.WriteLine("Bench_Configure failed.");
            }

            // Create the ISO transfer queue.  This class manages the pending and outstanding transfer lists.
            ReadIsoTransferQueue readXfers = new ReadIsoTransferQueue(usb, ref pipeInfo, Test.MaxOutstandingTransfers, Test.IsoPacketsPerTransfer);

            Console.WriteLine("Test ready:");
            Console.Write(Test.ToString());

            Console.WriteLine("Press 'Q' to abort or any other key to continue..");
            ConsoleKeyInfo key = Console.ReadKey(true);
            if (key.KeyChar == 'q' || key.KeyChar == 'Q') goto Done;

            Console.WriteLine("Starting test..");
            if (!String.IsNullOrEmpty(Test.LogFilename))
            {
                Console.WriteLine("No more text will be written to console");
                Console.WriteLine("Test reults will appear in the log file.");
            }
            Console.WriteLine();

            // Always issue a reset pipe prior to re/starting an ISO stream.
            usb.ResetPipe(pipeInfo.PipeId);

            // This example will not manage the start frame manually, but this is
            // how I might caculate the starting point.
            usb.GetCurrentFrameNumber(out readXfers.FrameNumber);
            unchecked
            {
                // Add some start latency
                readXfers.FrameNumber += 32;

                // Start FrameNumber at an interval of 8.
                readXfers.FrameNumber -= ((readXfers.FrameNumber)%8);
            }

            // This is just a counter/timer for statistics gathering.

            // Transfer processing loop
            Thread.Sleep(0);
            Test.Dcs.Start();
            do
            {
                IsoTransferItem isoTransferItem;
                uint transferred;
                int errorCode;
                // While buffers exist in the completed list, submit them. 
                while (readXfers.Completed.Count > 0 && readXfers.TotalSubmittedCount < Test.MaxTransfersTotal)
                {
                    errorCode = readXfers.SubmitNextRead();
                    if (errorCode != 0)
                    {
                        Console.WriteLine("IsoReadPipe failed. ErrorCode: {0:X8}h", errorCode);
                        goto Done;
                    }
                }

                if (readXfers.Outstanding.Count == 0)
                {
                    // The MAX_TRANSFERS_TOTAL test limit as hit.
                    Console.WriteLine("Done!");
                    goto Done;
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
            } while (true);

            Done:
            readXfers.Destroy();
            Test.CloseLogFile();

            deviceList.Free();
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
                           isoTransferItem.Iso.StartFrame,
                           transferLength,
                           Math.Round(Test.Dcs.Bps, 2));
            }
            else
            {
                uint lastFrameCount = isoTransferItem.Iso.StartFrame - readIsoTransfers.LastStartFrame;
                Test.LogLn("#{0}: StartFrame={1:X8}h TransferLength={2} BPS-average:{3} LastFrameCount={4}",
                           readIsoTransfers.CompletedCount,
                           isoTransferItem.Iso.StartFrame,
                           transferLength,
                           Math.Round(Test.Dcs.Bps, 2),
                           lastFrameCount);
            }

            int numPackets = isoTransferItem.Iso.NumberOfPackets;
            for (packetPos = 0; packetPos < numPackets; packetPos++)
            {
                KISO_PACKET isoPacket;
                isoTransferItem.Iso.GetPacket(packetPos, out isoPacket);
                if (isoPacket.Length > 1)
                {
                    // This is somewhat specific to data that is returned by the benchmark firmware.
                    byte firstPacketByte = isoTransferItem.Buffer[isoPacket.Offset];
                    byte secondPacketByte = isoTransferItem.Buffer[isoPacket.Offset + 1];
                    Test.LogLn("  [{0:000}] Length={1} B0={2:X2}h B1={3:X2}h", packetPos, isoPacket.Length, firstPacketByte, secondPacketByte);
                }
                else
                {
                    Test.LogLn("  [{0:000}] Empty Packet", packetPos);
                }
            }

            readIsoTransfers.CompletedCount++;
            readIsoTransfers.LastStartFrame = isoTransferItem.Iso.StartFrame;
        }
    }

    internal class IsoTransferItem
    {
        public byte[] Buffer;
        public GCHandle BufferGC;
        public IsoK Iso;
        public KOVL_HANDLE Ovl;
    }

    internal class ReadIsoTransferQueue
    {
        #region Transfer lists
        public LinkedList<IsoTransferItem> Completed = new LinkedList<IsoTransferItem>();
        public LinkedList<IsoTransferItem> Master = new LinkedList<IsoTransferItem>();
        public LinkedList<IsoTransferItem> Outstanding = new LinkedList<IsoTransferItem>();
        #endregion


        #region Core queue members
        public readonly uint DataBufferSize;
        public readonly OvlK OvlPool;
        public readonly WINUSB_PIPE_INFORMATION PipeInfo;
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


        public ReadIsoTransferQueue(UsbK usb, ref WINUSB_PIPE_INFORMATION pipeInfo, int maxPendingTransfers, int numberOfPackets)
        {
            PipeInfo = pipeInfo;
            Usb = usb;
            OvlPool = new OvlK(usb.Handle, maxPendingTransfers, KOVL_POOL_FLAG.NONE);
            DataBufferSize = (uint) (pipeInfo.MaximumPacketSize*numberOfPackets);
            for (int pos = 0; pos < maxPendingTransfers; pos++)
            {
                IsoTransferItem isoTransferItem = new IsoTransferItem();

                isoTransferItem.Buffer = new byte[pipeInfo.MaximumPacketSize*numberOfPackets];
                isoTransferItem.BufferGC = GCHandle.Alloc(isoTransferItem.Buffer, GCHandleType.Pinned);

                isoTransferItem.Iso = new IsoK(numberOfPackets, 0);
                isoTransferItem.Iso.SetPackets(pipeInfo.MaximumPacketSize);

                OvlPool.Acquire(out isoTransferItem.Ovl);

                Master.AddLast(isoTransferItem);
                Completed.AddLast(isoTransferItem);
            }
        }

        public void Destroy()
        {
            // Cancel any outstanding IO and release the OvlK.
            foreach (IsoTransferItem myIsoBuffer in Outstanding)
            {
                uint transferred;
                OvlPool.WaitAndRelease(myIsoBuffer.Ovl, 0, out transferred);
            }
            Completed.Clear();
            Outstanding.Clear();

            foreach (IsoTransferItem myIsoBuffer in Master)
            {
                myIsoBuffer.Iso.Free();
                myIsoBuffer.BufferGC.Free();
            }
            Master.Clear();
            OvlPool.Free();
        }

        private static void SetNextFrameNumber(IsoTransferItem isoTransferItem)
        {
        }

        public int SubmitNextRead()
        {
            // Pull from head of Completed list and push to end of Outstanding list
            IsoTransferItem isoTransferItem = Completed.First.Value;
            Completed.RemoveFirst();
            Outstanding.AddLast(isoTransferItem);

            // If managing a start frame manually, set it here and update the the frame counter for the next transfer.
            SetNextFrameNumber(isoTransferItem);

            // Prepare the OvlK handle to be submitted.
            OvlPool.ReUse(isoTransferItem.Ovl);

            // TODO Travis: libusbK binding is missing several function overloads.
            // The data buffer was pinned earlier when it was allocated.  Always pin managed memory before using it
            // in an asynchronous function to keep the framework from tampering with it.
            Usb.IsoReadPipe(PipeInfo.PipeId,
                            isoTransferItem.BufferGC.AddrOfPinnedObject(),
                            DataBufferSize,
                            isoTransferItem.Ovl.DangerousGetHandle(),
                            isoTransferItem.Iso.Handle);
            int errorCode = Marshal.GetLastWin32Error();

            // TODO Travis: libusbK binding is missing several constants.
            // 997 is ERROR_IO_PENDING. For async, this means so far so good.
            // IE: The transfer is submitted but we won't know if a problem occurs until it is completed.
            if (errorCode != 997) return errorCode;

            TotalSubmittedCount++;

            return 0;
        }

        public int WaitRead(out IsoTransferItem isoTransferItem, int i, out uint transferred)
        {
            isoTransferItem = Outstanding.First.Value;
            bool success = OvlPool.Wait(isoTransferItem.Ovl, 1000, KOVL_WAIT_FLAG.NONE, out transferred);
            if (!success)
            {
                return Marshal.GetLastWin32Error();
            }

            Outstanding.RemoveFirst();
            Completed.AddLast(isoTransferItem);

            return 0;
        }
    }

    internal class TestParameters
    {
        public int Vid;
        public int Pid;
        public byte PipeId;
        public int MaxOutstandingTransfers;
        public int MaxTransfersTotal;
        public int IsoPacketsPerTransfer;
        public string LogFilename;

        private bool fileLoggingEnabled;
        public HiPerfTimer Dcs = new HiPerfTimer();

        private StreamWriter logStream;

        public TestParameters(int vid,
                              int pid,
                              byte pipeId,
                              int maxOutstandingTransfers,
                              int maxTransfersTotal,
                              int isoPacketsPerTransfer,
                              string logFilename)
        {
            Vid = vid;
            Pid = pid;
            PipeId = pipeId;
            MaxOutstandingTransfers = maxOutstandingTransfers;
            MaxTransfersTotal = maxTransfersTotal;
            IsoPacketsPerTransfer = isoPacketsPerTransfer;
            LogFilename = logFilename;
        }

        public void CloseLogFile()
        {
            if (fileLoggingEnabled && logStream != null)
            {
                logStream.Flush();
                logStream.Close();
                logStream = null;
            }
        }

        public void LogLn(String text)
        {
            if (fileLoggingEnabled) logStream.WriteLine(text);
            else Console.WriteLine(text);
        }

        public void LogLn(String format, params object[] args)
        {
            if (fileLoggingEnabled) logStream.WriteLine(format, args);
            else Console.WriteLine(format, args);
        }

        public void Log(String text)
        {
            if (fileLoggingEnabled) logStream.Write(text);
            else Console.Write(text);
        }

        public void Log(String format, params object[] args)
        {
            if (fileLoggingEnabled) logStream.Write(format, args);
            else Console.Write(format, args);
        }

        public override string ToString()
        {
            return
                string.Format(
                    "Vid: {0:X4}h\nPid: {1:X4}h\nPipeId: {2:X2}h\nMaxOutstandingTransfers: {3}\nMaxTransfersTotal: {4}\nIsoPacketsPerTransfer: {5}\nLogFilename: {6}\n",
                    Vid,
                    Pid,
                    PipeId,
                    MaxOutstandingTransfers,
                    MaxTransfersTotal,
                    IsoPacketsPerTransfer,
                    LogFilename);
        }

        public void Init()
        {
            if (!String.IsNullOrEmpty(LogFilename))
            {
                logStream = new StreamWriter(LogFilename);
                fileLoggingEnabled = true;
            }
        }

        public void FillFromCommandLine(string commandLine)
        {
            commandLine = Regex.Replace(commandLine, "^(\".+?\"|\\S+)\\s*", "");
            MatchCollection matches = Regex.Matches(commandLine,
                                                    @"(Vid \W\s*(0x)? (?<VidHex>[0-9A-F]+))  |  (Pid \W\s*(0x)? (?<PidHex>[0-9A-F]+))",
                                                    RegexOptions.ExplicitCapture | RegexOptions.IgnoreCase | RegexOptions.IgnorePatternWhitespace);

            foreach (Match m in matches)
            {
                Group gVid = m.Groups["VidHex"];
                Group gPid = m.Groups["PidHex"];

                if (gVid.Success) Vid = ushort.Parse(gVid.Value, NumberStyles.HexNumber);
                else if (gPid.Success) Pid = ushort.Parse(gPid.Value, NumberStyles.HexNumber);
            }
        }
    }
}