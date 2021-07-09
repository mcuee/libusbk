#Region "Copyright(c) Travis Robinson"
' Copyright (c) 2012 Travis Robinson <libusbdotnet@gmail.com>
' All rights reserved.
' 
' HiPerfTimer.cs
' 
' Created:      03.05.2012
' Last Updated: 03.05.2012
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


Imports System.ComponentModel
Imports System.Runtime.InteropServices

Namespace Win32
    Friend Class HiPerfTimer
#Region "Public Members"
        Public Sub New()
            startTime = 0
            stopTime = 0
            length = 0

            If QueryPerformanceFrequency(freq) = False Then
                ' high-performance counter not supported 
                Throw New Win32Exception()
            End If
        End Sub

        ''' <summary>
        '''   Gets the bytes per second
        ''' </summary>
        Public ReadOnly Property Bps As Double
            Get
                Dim duration = Me.Duration
                If duration.Equals(0.0) Then Return 0.0
                Return length / duration
            End Get
        End Property

        ''' <summary>
        '''   Gets the duration of the timer (in seconds)
        ''' </summary>
        Public ReadOnly Property Duration As Double
            Get
                Return (stopTime - startTime) / freq
            End Get
        End Property

        ''' <summary>
        '''   Gets the duration of the timer (in seconds)
        ''' </summary>
        Public ReadOnly Property Ticks As Long
            Get
                Dim tDiff = stopTime - startTime
                tDiff = tDiff * 10000000 / freq
                Return tDiff
            End Get
        End Property
        ''' <summary>
        '''   Start the timer
        ''' </summary>
        Public Sub Start()
            length = 0
            QueryPerformanceCounter(startTime)
        End Sub

        ''' <summary>
        '''   Stop the timer
        ''' </summary>
        Public Sub [Stop]()
            QueryPerformanceCounter(stopTime)
        End Sub

        Public Sub [Stop](ByVal addLength As Integer)
            QueryPerformanceCounter(stopTime)
            length += addLength
        End Sub
#End Region


#Region "Private Members"
        <DllImport("Kernel32.dll")>
        Private Shared Function QueryPerformanceCounter(<Out> ByRef lpPerformanceCount As Long) As Boolean
        End Function

        <DllImport("Kernel32.dll")>
        Private Shared Function QueryPerformanceFrequency(<Out> ByRef lpFrequency As Long) As Boolean
        End Function

        Private ReadOnly freq As Long
        Private length As Long
        Private startTime, stopTime As Long
#End Region
    End Class
End Namespace
