#region Copyright(c) Travis Robinson
// Copyright (c) 2012 Travis Robinson <libusbdotnet@gmail.com>
// All rights reserved.
// 
// UsbStream.cs
// 
// Created:      03.05.2012
// Last Updated: 03.05.2012
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
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using libusbK;


namespace Xfer.UsbStream
{
    public class UsbStream : Stream
    {
        protected readonly byte mPipeId;
        protected long mCurrentPosition;
        protected StmK mStm;
        protected long mTotalTransferred;
        protected UsbK mUsb;

        protected override void Dispose(bool disposing)
        {
            if (!mbDisposed)
            {
                mStm.Dispose();
                mStm = null;
                mUsb = null;
                mbDisposed = true;
            }
            base.Dispose(disposing);
        }


        #region Public Members
        public UsbStream(UsbK usb,
                         byte pipeId,
                         int maxTransferSize,
                         int maxPendingTransfers,
                         int MaxPendingIO,
                         bool useTimeout,
                         UInt16 timeout)
        {
            mUsb = usb;
            mPipeId = pipeId;

            mCallbacks.Complete = StmComplete;
            if (((mPipeId & AllKConstants.USB_ENDPOINT_DIRECTION_MASK) > 0))
                mCallbacks.Submit = StmSubmitRead;
            else
                mCallbacks.Submit = StmSubmitWrite;

            KSTM_FLAG flags = useTimeout ? KSTM_FLAG.USE_TIMEOUT | (KSTM_FLAG) timeout : KSTM_FLAG.NONE;

            mStm = new StmK(mUsb.Handle,
                            pipeId,
                            maxTransferSize,
                            maxPendingTransfers,
                            MaxPendingIO,
                            ref mCallbacks,
                            flags);
        }

        public override bool CanRead
        {
            get
            {
                return ((mPipeId & AllKConstants.USB_ENDPOINT_DIRECTION_MASK) > 0);
            }
        }

        public override bool CanSeek
        {
            get
            {
                return false;
            }
        }

        public override bool CanWrite
        {
            get
            {
                return ((mPipeId & AllKConstants.USB_ENDPOINT_DIRECTION_MASK) == 0);
            }
        }

        public override long Length
        {
            get
            {
                return (CanRead) ? mTotalTransferred : mCurrentPosition;
            }
        }

        public int OutstandingCount
        {
            get
            {
                return mOutstandingCount;
            }
        }

        public override long Position
        {
            get
            {
                return (CanWrite) ? mTotalTransferred : mCurrentPosition;
            }
            set
            {
                throw new NotSupportedException();
            }
        }

        public override int Read(byte[] buffer,
                                 int offset,
                                 int count)
        {
            int transferred;
            if (!mStm.Read(buffer,
                           offset,
                           count,
                           out transferred))
                return 0;

            mCurrentPosition += transferred;

            return transferred;
        }

        public bool Start()
        {
            return mStm.Start();
        }

        public bool Stop(int timeoutCancelMs)
        {
            return mStm.Stop(timeoutCancelMs);
        }

        public override void Write(byte[] buffer,
                                   int offset,
                                   int count)
        {
            int transferred = 0;

            if (!mStm.Write(buffer,
                            offset,
                            count,
                            out transferred))
            {
                // This exception (and the one below) is caused by a lack of 'PendingTransfer' slots.
                throw new Exception(String.Format("Failed writing to usb stream. ErrorCode={0:X8}h",
                                                  Marshal.GetLastWin32Error()));
            }
            mCurrentPosition += transferred;

            // If the KSTM_FLAG.NO_PARTIAL_XFERS is *not* set, StmK will transfer as many bytes as it can. This class always sets
            // this flag so the below code should never execute.
            if (transferred != count)
                throw new Exception(String.Format("Not all bytes were written. Expected:{0} Transferred:{1}",
                                                  count,
                                                  transferred));
        }
        #endregion


        #region Not Implemented
        public override void Flush()
        {
        }

        public override long Seek(long offset,
                                  SeekOrigin origin)
        {
            throw new NotSupportedException();
        }

        public override void SetLength(long value)
        {
            throw new NotSupportedException();
        }
        #endregion


        #region Private Members
        private int StmComplete(KSTM_INFO stmInfo,
                                KSTM_XFER_CONTEXT stmXferContext,
                                int stmXferContextIndex,
                                int errorCode)
        {
            if (errorCode == ErrorCodes.Success)
                mTotalTransferred += stmXferContext.TransferLength;

            Interlocked.Decrement(ref mOutstandingCount);

            return errorCode;
        }

        private int StmSubmitRead(KSTM_INFO info,
                                  KSTM_XFER_CONTEXT xferContext,
                                  int xferContextIndex,
                                  IntPtr nativeOverlapped)
        {
            int notUsedForAsync;
            int bufferSize = xferContext.BufferSize;

            mUsb.ReadPipe(mPipeId,
                          xferContext.Buffer,
                          bufferSize,
                          out notUsedForAsync,
                          nativeOverlapped);
            int errorCode = Marshal.GetLastWin32Error();

            Debug.Assert(errorCode == ErrorCodes.IoPending,
                         String.Format("TODO USER: Handle submit read errors here. ErrorCode:{0:X8}h",
                                       errorCode));
            Interlocked.Increment(ref mOutstandingCount);
            return errorCode;
        }

        private int StmSubmitWrite(KSTM_INFO info,
                                   KSTM_XFER_CONTEXT xferContext,
                                   int xferContextIndex,
                                   IntPtr nativeOverlapped)
        {
            int notUsedForAsync;
            int bufferSize = xferContext.TransferLength;

            mUsb.WritePipe(mPipeId,
                           xferContext.Buffer,
                           bufferSize,
                           out notUsedForAsync,
                           nativeOverlapped);
            int errorCode = Marshal.GetLastWin32Error();

            Debug.Assert(errorCode == ErrorCodes.IoPending,
                         String.Format("TODO USER: Handle submit write errors here. ErrorCode:{0:X8}h",
                                       errorCode));
            Interlocked.Increment(ref mOutstandingCount);
            return errorCode;
        }

        private readonly KSTM_CALLBACK mCallbacks;
        private int mOutstandingCount;
        private bool mbDisposed;
        #endregion
    }
}