#region Copyright(c) Travis Robinson
// Copyright (c) 2012 Travis Robinson <libusbdotnet@gmail.com>
// All rights reserved.
// 
// Program.cs
// 
// Created:      02.29.2012
// Last Updated: 03.01.2012
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
using System.Diagnostics;
using System.Runtime.InteropServices;
using Test.Devices;
using libusbK;
using libusbK.Examples;


// ReSharper disable InconsistentNaming

namespace Xfer.Async
{
    internal class Program
    {
        #region TODO USER: Set the test parameters for your device.
        public static StmTestParameters Test = new StmTestParameters(0x04d8, 0xfa2e, 0, 0x82, 4096, null, -1, 3, -1);
        #endregion

        private static void Main()
        {
            bool success;

            WINUSB_PIPE_INFORMATION pipeInfo;
            UsbK usb;
            USB_INTERFACE_DESCRIPTOR interfaceDescriptor;

            // Find and configure the device.
            if (!Test.ConfigureDevice(out pipeInfo, out usb, out interfaceDescriptor)) return;
            if (Test.TransferBufferSize == -1)
                Test.TransferBufferSize = pipeInfo.MaximumPacketSize*64;

#if BMFW
            // TODO FOR USER: Remove this block if not using benchmark firmware.
            // This configures devices running benchmark firmware for streaming DeviceToHost transfers.
            Console.WriteLine("Configuring for benchmark device..");
            BM_TEST_TYPE testType = ((Test.PipeId & 0x80) > 0) ? BM_TEST_TYPE.READ : BM_TEST_TYPE.WRITE;
            success = Benchmark.Configure(usb, BM_COMMAND.SET_TEST, interfaceDescriptor.bInterfaceNumber, ref testType);
            if (!success)
            {
                Console.WriteLine("Bench_Configure failed.");
            }
#endif
            if (!Test.ShowTestReady()) goto Done;

            // In most cases, you should to set the pipe timeout policy before using synchronous I/O.
            // By default, sync transfers wait infinitely on a transfer to complete.

            // Set the pipe timeout policy to 3000ms
            int[] pipeTimeoutMS = new int[] {3000};
            usb.SetPipePolicy(
                (byte) Test.PipeId,
                (int) PipePolicyType.PIPE_TRANSFER_TIMEOUT,
                Marshal.SizeOf(typeof (int)),
                pipeTimeoutMS);

            int totalSubmittedTransfers = 0;
            int totalCompletedTransfers = 0;
            success = true;
            byte[] tempBuffer = new byte[Test.TransferBufferSize];

            // Start transferring data synchronously; one transfer at a time until the test limit (MaxTransfersTotal) is hit.
            KOVL_HANDLE ovlHandle;
            OvlK ovl = new OvlK(usb.Handle, Test.MaxPendingIO, KOVL_POOL_FLAG.NONE);
            while (success && totalCompletedTransfers < Test.MaxTransfersTotal)
            {
                while (success && totalSubmittedTransfers < Test.MaxTransfersTotal)
                {
                    if (!ovl.Acquire(out ovlHandle))
                    {
                        Debug.Assert(Marshal.GetLastWin32Error() == ErrorCodes.NoMoreItems);
                        break;
                    }
                    int not_used_for_async;
                    if ((Test.PipeId & 0x80) > 0)
                        success = usb.ReadPipe((byte) Test.PipeId, tempBuffer, tempBuffer.Length, out not_used_for_async, ovlHandle);
                    else
                        success = usb.WritePipe((byte) Test.PipeId, tempBuffer, tempBuffer.Length, out not_used_for_async, ovlHandle);

                    if (Marshal.GetLastWin32Error() == ErrorCodes.IoPending)
                    {
                        success = true;
                        totalSubmittedTransfers++;
                        Console.WriteLine("Pending  #{0:0000} {1} bytes.", totalSubmittedTransfers, tempBuffer.Length);
                    }
                    else
                        Console.WriteLine("Pending  #{0:0000} failed. ErrorCode={1:X8}h", totalSubmittedTransfers, Marshal.GetLastWin32Error());
                }
                if (!success) break;

                int transferred;
                success = ovl.WaitOldest(out ovlHandle, 1000, KOVL_WAIT_FLAG.RELEASE_ALWAYS, out transferred);

                if (success)
                {
                    totalCompletedTransfers++;
                    Console.WriteLine("Complete #{0:0000} {1} bytes.", totalCompletedTransfers, transferred);
                }
                else
                    Console.WriteLine("Complete #{0:0000} Wait failed. ErrorCode={1:X8}h", totalSubmittedTransfers, Marshal.GetLastWin32Error());
            }

            if (!success)
                Console.WriteLine("An error occured transferring data. ErrorCode: {0:X8}h", Marshal.GetLastWin32Error());

            ovl.Free();

            Done:
            usb.Free();
        }
    }
}