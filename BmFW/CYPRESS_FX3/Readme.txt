How to build and flash:
-------------------------------------------------------------------------------
1) install the cypress FX3 development kit:
https://www.cypress.com/documentation/software-and-drivers/ez-usb-fx3-software-development-kit

2) Import the sub-folders in this directory into your workspace:
File->Import->Existing Projects into Workspace

Firmware Projects:
-------------------------------------------------------------------------------
All of the BMFW firmware projects are slightly modified version of the Cypress
test firmware.

BMFW_BulkLoopAuto:
- Supports the kBench "Loop" test only
- Loops data from one bulk endpoint to another

BMFW_IsochLoopAuto:
- Supports the kBench "Loop" test only
- Loops data from one isochronous endpoint to another

BMFW_BulkSourceSink:
- Supports the kBench "Loop, Read, and Write" modes
- The IN and OUT endpoints are entirely independent from one another.
  IE: When running the "Loop" test, data is not actually looped.
- Data received by the IN endpoint is dicarded
- Data send by the OUT endpoint follows a specific pattern.  See the
  xfer-iso example documentation for more information.

BMFW_IsoSourceSink:
- Supports the kBench "Loop, Read, and Write" modes
- The IN and OUT endpoints are entirely independent from one another.
  IE: When running the "Loop" test, data is not actually looped.
- Data received by the IN endpoint is dicarded
- Data send by the OUT endpoint follows a specific pattern.  See the
  xfer-iso example documentation for more information.
  
