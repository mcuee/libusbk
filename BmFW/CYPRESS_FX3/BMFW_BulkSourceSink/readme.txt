
                        CYPRESS SEMICONDUCTOR CORPORATION
                                    FX3 SDK

USB BULK SOURCE SINK EXAMPLE
-------------------------------

  This example illustrates the use of the FX3 firmware APIs to implement
  a data source and data sink over a pair of USB Bulk endpoints.

  The device enumerates as a vendor specific USB device with a pair of Bulk
  endpoints (1-OUT and 1-IN). The OUT endpoint acts as data sink and the IN
  endpoint acts as data source to the PC Host.

  The source and sink is achieved with the help of a a DMA MANUAL IN Channel 
  and a DMA MANUAL OUT Channel. 

  Any data received from the host through the DMA MANUAL IN channel is
  discarded. A constant data pattern is continuously loaded into the DMA MANUAL
  OUT channel and sent to the host. 

  The application also supports a set of vendor commands on the control endpoint.
  These are used to demonstrate the vendor command handling as well as to
  test cases like repeated USB connection, device reset etc.

  Files:

    * cyfx_gcc_startup.S   : Start-up code for the ARM-9 core on the FX3 device.
      This assembly source file follows the syntax for the GNU assembler.

    * cyfxbulksrcsink.h    : Constant definitions for the bulk source sink 
      application. The USB connection speed, numbers and properties of the 
      endpoints etc. can be selected through definitions in this file.

    * cyfxbulkdscr.c       : C source file containing the USB descriptors that
      are used by this firmware example. VID and PID is defined in this file.

    * cyfxtx.c             : ThreadX RTOS wrappers and utility functions required
      by the FX3 API library.

    * cyfxbulksrcsink.c    : Main C source file that implements the bulk source sink
      example.

    * makefile             : GNU make compliant build script for compiling this
      example.

  PERFORMANCE OPTIMIZATIONS

  The default example is not optimised for performance in USB 3.0, in order to get 
  better performance the following changes has to be done.

  1. Have only one endpoint (IN or OUT) This avoids any USB host bandwidth issues.

  2. Update the burst length to maximum burst value supported by the USB SS host
     This can be done by modifying the CY_FX_EP_BURST_LENGTH field (1 - 16) in
     cyfxbulksrcsink.h. Updating this value will change the DMA buffer size for all
     speeds. The bursting will be done only for USB super speed operation.

  3. The firmware latencies can be minimized by having a larger buffer. This can
     be done by changing the multiplier defined by CY_FX_DMA_SIZE_MULTIPLIER in
     cyfxbulksrcsink.h. When making these changes, make sure that there is enough
     memory available for buffering.

[]

