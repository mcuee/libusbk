#region Copyright(c) Travis Robinson
// Copyright (c) 2012 Travis Robinson <libusbdotnet@gmail.com>
// All rights reserved.
// 
// Program.cs
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
using libusbK;


namespace Hot.Plug.Detect
{
    internal class Program
    {
        #region Private Members
        private static void Main()
        {
            KHOT_PARAMS hotInitParams = new KHOT_PARAMS();
            hotInitParams.PatternMatch.DeviceInterfaceGUID = "*";
            hotInitParams.Flags = KHOT_FLAG.PLUG_ALL_ON_INIT;
            hotInitParams.OnHotPlug = OnHotPlug;

            Console.WriteLine("Monitoring libusbK arrival/removal events.");
            Console.WriteLine("[PatternMatch]");
            Console.WriteLine(hotInitParams.PatternMatch.ToString());
            Console.WriteLine("Press [ENTER] to exit..");

            while (Console.KeyAvailable) Console.ReadKey(true);
            HotK hot = new HotK(ref hotInitParams);

            Console.ReadLine();

            hot.Free();
        }

        private static void OnHotPlug(KHOT_HANDLE hothandle,
                                      KLST_DEVINFO_HANDLE deviceinfo,
                                      KLST_SYNC_FLAG plugtype)
        {
            string plugText;

            int totalPluggedDeviceCount = (int) hothandle.GetContext().ToInt64();

            switch (plugtype)
            {
                case KLST_SYNC_FLAG.ADDED:
                    plugText = "Arrival";
                    totalPluggedDeviceCount++;
                    break;
                case KLST_SYNC_FLAG.REMOVED:
                    plugText = "Removal";
                    totalPluggedDeviceCount--;
                    break;
                default:
                    throw new ArgumentOutOfRangeException("plugtype");
            }

            hothandle.SetContext(new IntPtr(totalPluggedDeviceCount));

            Console.WriteLine("\n[OnHotPlug] Device {0}:{1} \n",
                              plugText,
                              deviceinfo);
            Console.WriteLine("Total Plugged Device Count: {0}",
                              totalPluggedDeviceCount);
        }
        #endregion
    }
}