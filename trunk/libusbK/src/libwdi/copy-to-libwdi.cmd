@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
IF "%~1" EQU "" GOTO ShowHelp
SET _INPUT_DIR=%~dp0
SET _OUTPUT_DIR=%~1
IF "!_INPUT_DIR:~-1!" NEQ "\" SET _INPUT_DIR=!_INPUT_DIR!\
IF "!_OUTPUT_DIR:~-1!" NEQ "\" SET _OUTPUT_DIR=!_OUTPUT_DIR!\

IF NOT EXIST "!_OUTPUT_DIR!" GOTO ShowHelp

COPY /Y "!_INPUT_DIR!config.h" "!_OUTPUT_DIR!msvc\config.h"
COPY /Y "!_INPUT_DIR!inf_wizard.c" "!_OUTPUT_DIR!examples\inf_wizard.c"
COPY /Y "!_INPUT_DIR!inf_wizard_rc.rc" "!_OUTPUT_DIR!examples\inf_wizard_rc.rc"
COPY /Y "!_INPUT_DIR!embedder_files.h" "!_OUTPUT_DIR!libwdi\embedder_files.h"
COPY /Y "!_INPUT_DIR!libusbk.inf.in" "!_OUTPUT_DIR!libwdi\libusbk.inf.in"

GOTO :EOF

:ShowHelp
ECHO.
ECHO Copies libwdi user files to libwdi source folders.
ECHO WARNING: This will overwrite certain files in the libwdi source folders.
ECHO.
ECHO USAGE  : %~n0 [root-libwdi-folder]
ECHO EXAMPLE: %~n0 "Z:\GITMAIN\libwdi"
ECHO.

