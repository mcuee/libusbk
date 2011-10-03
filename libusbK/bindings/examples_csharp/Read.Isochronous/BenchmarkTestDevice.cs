using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using libusbK;


// ReSharper disable CheckNamespace
namespace Test.Devices
// ReSharper restore CheckNamespace
{
    public enum BM_COMMAND
    {
        SET_TEST = 0x0E,
        GET_TEST = 0x0F,
    };

    public enum BM_TEST_TYPE
    {
        NONE = 0x00,
        READ = 0x01,
        WRITE = 0x02,
        LOOP = READ | WRITE,
    };

    public static class Benchmark
    {
        //! Custom vendor requests that must be implemented in the benchmark firmware.

        public static bool Configure(UsbK usb, BM_COMMAND command, byte interfaceNumber, ref BM_TEST_TYPE testType)
        {
            uint transferred;
            WINUSB_SETUP_PACKET pkt;
            byte[] data = new byte[1];

            pkt.RequestType = (1 << 7) | (2 << 5);
            pkt.Request = (byte)command;

            pkt.Value = (ushort)testType;
            pkt.Index = interfaceNumber;
            pkt.Length = 1;

            bool success = usb.ControlTransfer(pkt, Marshal.UnsafeAddrOfPinnedArrayElement(data, 0), 1, out transferred, IntPtr.Zero);
            testType = (BM_TEST_TYPE)data[0];
            return success;
        }

    }
}
