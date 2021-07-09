#Region "Copyright(c) Travis Robinson"

' Copyright (c) 2012 Travis Robinson <libusbdotnet@gmail.com>
' All rights reserved.
' 
' UsbStream.cs
' 
' Created:      03.05.2012
' Last Updated: 03.07.2012
' 
' Redistribution and use in source and binary forms, with or without
' modification, are permitted provided that the following conditions are met:
' 
'     * Redistributions of source code must retain the above copyright
'       notice, this list of conditions and the following disclaimer.
' 	  
' THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
' IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
' TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
' PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TRAVIS LEE ROBINSON 
' BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
' CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
' SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
' INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
' CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
' ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
' THE POSSIBILITY OF SUCH DAMAGE.

#End Region

Imports System
Imports System.Diagnostics
Imports System.IO
Imports System.Runtime.InteropServices
Imports libusbK

Namespace Xfer.UsbStream
    ' ReSharper disable InconsistentNaming

    Public Class UsbStream
        Inherits Stream

        Public Shared UseCallbacks As Boolean = True
        Private ReadOnly mCallbacks As KSTM_CALLBACK
        Protected ReadOnly mPipeId As Byte
        Protected mStm As StmK
        Protected mUsb As UsbK
        Private mbDisposed As Boolean

        Public Sub New(ByVal usb As UsbK, ByVal pipeId As Byte, ByVal maxTransferSize As Integer, ByVal maxPendingTransfers As Integer, ByVal maxPendingIo As Integer, ByVal useTimeout As Boolean, ByVal timeout As UShort)
            mUsb = usb
            mPipeId = pipeId

            If UseCallbacks Then
                mCallbacks.Complete = AddressOf StmComplete

                If (mPipeId And USB_ENDPOINT_DIRECTION_MASK) > 0 Then
                    mCallbacks.Submit = AddressOf StmSubmitRead
                Else
                    mCallbacks.Submit = AddressOf StmSubmitWrite
                End If
            End If

            Dim flags = If(useTimeout, KSTM_FLAG.USE_TIMEOUT Or CType(timeout, KSTM_FLAG), KSTM_FLAG.NONE)
            mStm = New StmK(mUsb.Handle, pipeId, maxTransferSize, maxPendingTransfers, maxPendingIo, mCallbacks, flags)
        End Sub

        Public Overrides ReadOnly Property CanRead As Boolean
            Get
                Return (mPipeId And USB_ENDPOINT_DIRECTION_MASK) > 0
            End Get
        End Property

        Public Overrides ReadOnly Property CanSeek As Boolean
            Get
                Return False
            End Get
        End Property

        Public Overrides ReadOnly Property CanWrite As Boolean
            Get
                Return (mPipeId And USB_ENDPOINT_DIRECTION_MASK) = 0
            End Get
        End Property

        Protected Overrides Sub Dispose(ByVal disposing As Boolean)
            If Not mbDisposed Then
                mStm.Dispose()
                mStm = Nothing
                mUsb = Nothing
                mbDisposed = True
            End If

            MyBase.Dispose(disposing)
        End Sub

        Public Overrides Function Read(ByVal buffer As Byte(), ByVal offset As Integer, ByVal count As Integer) As Integer
            Dim transferred As UInteger
            Dim success = mStm.Read(buffer, offset, count, transferred)
            Call Debug.WriteLineIf(Not success, String.Format("Failed reading from usb stream. ErrorCode={0:X8}h", Marshal.GetLastWin32Error()), "ERROR")
            Return If(Not success, 0, CInt(transferred))
        End Function

        Public Function Start() As Boolean
            Return mStm.Start()
        End Function

        Public Function [Stop](ByVal timeoutCancelMs As Integer) As Boolean
            Return mStm.Stop(timeoutCancelMs)
        End Function

        Public Overrides Sub Write(ByVal buffer As Byte(), ByVal offset As Integer, ByVal count As Integer)
            Dim transferred As UInteger
            Dim success = mStm.Write(buffer, offset, count, transferred)

            If Not success Then
                ' This is caused by a lack of 'PendingTransfer' slots.
                Call Debug.WriteLine(String.Format("Failed writing to usb stream. ErrorCode={0:X8}h", Marshal.GetLastWin32Error()), "ERROR")
                Return
            End If

            ' If the KSTM_FLAG.NO_PARTIAL_XFERS is *not* set, StmK will transfer as many bytes as it can and return true.
            Debug.WriteLineIf(transferred <> count, String.Format("Not all bytes were written. Expected:{0} Transferred:{1}", count, transferred), "ERROR")
        End Sub

        Public Overrides Sub Flush()
        End Sub

        Private Function StmComplete(ByVal stmInfo As KSTM_INFO, ByVal xferContext As KSTM_XFER_CONTEXT, ByVal stmXferContextIndex As Integer, ByVal errorCode As Integer) As Integer
            Debug.WriteLineIf(errorCode <> Success, String.Format("Failed completing transfer. PipeID:{0:X2}h ErrorCode:{1:X8}h", mPipeId, errorCode), "ERROR")
            Return errorCode
        End Function

        Private Function StmSubmitRead(ByVal info As KSTM_INFO, ByVal xferContext As KSTM_XFER_CONTEXT, ByVal xferContextIndex As Integer, ByVal nativeOverlapped As IntPtr) As Integer
            Dim notUsedForAsync As UInteger
            Dim bufferSize As UInteger = xferContext.BufferSize
            Dim success = mUsb.ReadPipe(mPipeId, xferContext.Buffer, bufferSize, notUsedForAsync, nativeOverlapped)
            Dim errorCode As Integer = Marshal.GetLastWin32Error()
            If Not success AndAlso errorCode = IoPending Then Return ErrorCodes.Success
            Debug.WriteLine(String.Format("Failed submitting transfer. PipeID:{0:X2}h ErrorCode:{1:X8}h", mPipeId, errorCode), "ERROR")
            Return errorCode
        End Function

        Private Function StmSubmitWrite(ByVal info As KSTM_INFO, ByVal xferContext As KSTM_XFER_CONTEXT, ByVal xferContextIndex As Integer, ByVal nativeOverlapped As IntPtr) As Integer
            Dim notUsedForAsync As UInteger
            Dim bufferSize As UInteger = xferContext.TransferLength
            Dim success = mUsb.WritePipe(mPipeId, xferContext.Buffer, bufferSize, notUsedForAsync, nativeOverlapped)
            Dim errorCode As Integer = Marshal.GetLastWin32Error()
            If Not success AndAlso errorCode = IoPending Then Return ErrorCodes.Success
            Debug.WriteLine(String.Format("Failed submitting transfer. PipeID:{0:X2}h ErrorCode:{1:X8}h", mPipeId, errorCode), "ERROR")
            Return errorCode
        End Function

#Region "Not Supported"

        Public Overrides ReadOnly Property Length As Long
            Get
                Throw New NotSupportedException()
            End Get
        End Property

        Public Overrides Property Position As Long
            Get
                Throw New NotSupportedException()
            End Get
            Set(ByVal value As Long)
                Throw New NotSupportedException()
            End Set
        End Property

        Public Overrides Function Seek(ByVal offset As Long, ByVal origin As SeekOrigin) As Long
            Throw New NotSupportedException()
        End Function

        Public Overrides Sub SetLength(ByVal value As Long)
            Throw New NotSupportedException()
        End Sub

#End Region
    End Class
End Namespace
