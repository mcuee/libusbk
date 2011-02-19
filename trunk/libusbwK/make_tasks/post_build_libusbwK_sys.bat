@ECHO OFF
ECHO Executing %~n0 tasks..

REM - Copy the WdfCoInstallerXXXXX.dll to the output directory.
COPY /Y "!G_WDK_DIR!\redist\wdf\!_BUILDARCH!\WdfCoInstaller?????.dll" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\">NUL
