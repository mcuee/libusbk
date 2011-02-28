@ECHO OFF
ECHO Executing %~n0 tasks..

REM - Copy the WdfCoInstallerXXXXX.dll to the output directory.
COPY /Y "!G_WDK_DIR!\redist\wdf\!_BUILDARCH!\WdfCoInstaller?????.dll" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\">NUL
SET TOKVAR_LTAG=#
SET TOKVAR_RTAG=#
!DCMD! -et ".\src\libusbK.sys\example.inf.in" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\example.inf"
SET TOKVAR_LTAG=
SET TOKVAR_RTAG=
