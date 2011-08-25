@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
SET _PACKAGE_DIR=%~dp0\package\
SET _FIRMWARE_SRC=%~dp0\USB_Device_Benchmark\Firmware\
SET CFG_H_EXT=!_FIRMWARE_SRC!usb_config_external.h

IF NOT EXIST "!_PACKAGE_DIR!" MKDIR "!_PACKAGE_DIR!"

CALL :StartConfig
CALL :AddConfig DUAL_INTERFACE
CALL :AddConfig INTF0 EP_BULK
CALL :AddConfig INTF1 EP_BULK
CALL :AddConfig USBGEN_EP_SIZE_INTF0 32
CALL :AddConfig USBGEN_EP_SIZE_INTF1 32
CALL :EndConfig
CALL :MakeOutput "BmFW - 2Intf[Bulk_32 Bulk_32]"

CALL :StartConfig
CALL :AddConfig DUAL_INTERFACE_WITH_ASSOCIATION
CALL :AddConfig INTF0 EP_BULK
CALL :AddConfig INTF1 EP_BULK
CALL :AddConfig USBGEN_EP_SIZE_INTF0 32
CALL :AddConfig USBGEN_EP_SIZE_INTF1 32
CALL :EndConfig
CALL :MakeOutput "BmFW - 2AssociatedIntf[Bulk_32 Bulk_32]"

CALL :StartConfig
CALL :AddConfig SINGLE_INTERFACE_WITH_ALTSETTINGS
CALL :AddConfig USBGEN_EP_SIZE_INTF0 64
CALL :AddConfig VENDOR_BUFFER_ENABLED
CALL :EndConfig
CALL :MakeOutput "BmFW - 1Intf[Bulk_64],VendorBuffer,2AltSettings"

CALL :StartConfig
CALL :AddConfig INTF0 EP_INT
CALL :AddConfig USBGEN_EP_SIZE_INTF0 32
CALL :AddConfig VENDOR_BUFFER_ENABLED
CALL :EndConfig
CALL :MakeOutput "BmFW - 1Intf[Interrupt_32],VendorBuffer"

CALL :StartConfig
CALL :AddConfig SINGLE_INTERFACE_WITH_ALTSETTINGS
CALL :AddConfig INTF0 EP_ISO
CALL :AddConfig USBGEN_EP_SIZE_INTF0 256
CALL :AddConfig VENDOR_BUFFER_ENABLED
CALL :EndConfig
CALL :MakeOutput "BmFW - 1Intf[ISO_256],VendorBuffer,2AltSettings"

CALL :StartConfig
CALL :AddConfig INTF0 EP_ISO
CALL :AddConfig USBGEN_EP_SIZE_INTF0 64
CALL :AddConfig VENDOR_BUFFER_ENABLED
CALL :EndConfig
CALL :MakeOutput "BmFW - 1Intf[ISO_64],VendorBuffer"

:Test
CALL :StartConfig
CALL :AddConfig INTF0 EP_BULK
CALL :AddConfig USBGEN_EP_SIZE_INTF0 32
CALL :AddConfig VENDOR_BUFFER_ENABLED
CALL :EndConfig
CALL :MakeOutput "BmFW - 1Intf[Bulk_32],VendorBuffer"

CALL :StartConfig
CALL :EndConfig

GOTO :EOF

:StartConfig
ECHO #ifndef _USB_CFG_EXTERNAL_H>"!CFG_H_EXT!"
ECHO #define _USB_CFG_EXTERNAL_H>>"!CFG_H_EXT!"
GOTO :EOF

:EndConfig
ECHO #endif>>"!CFG_H_EXT!"
GOTO :EOF

:AddConfig
ECHO #define %*>>"!CFG_H_EXT!"
GOTO :EOF

:MakeOutput
ECHO.
ECHO Making %~1..
ECHO.

PUSHD !CD!
CD "!_FIRMWARE_SRC!"
CALL clean.cmd
devenv _Benchmark.sln /build "Release|Win32"
POPD

IF NOT EXIST "!_PACKAGE_DIR!%~1\" MKDIR "!_PACKAGE_DIR!%~1\"
DEL /Q "!_PACKAGE_DIR!%~1\*"
COPY /Y "!_FIRMWARE_SRC!output\*" "!_PACKAGE_DIR!%~1\"

GOTO :EOF
