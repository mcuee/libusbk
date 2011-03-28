@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

ECHO Executing %~n0 tasks..

REM NOTE:
REM - _BUILDARCH = x86, amd64, or ia64
REM - G_TARGET_OUTPUT_ARCH = i386,  amd64, or ia64
REM

IF /I "!G_DIST!" NEQ "finalize" goto Done

IF EXIST "!K_LIBUSB10_DEP_DIR!" IF EXIST "!K_LIBUSB0_DEP_DIR!" (
	PUSHD !CD!
	CD /D "!K_LIBUSB10_DEP_DIR!\msvc"
	CALL ddk_build
	IF NOT EXIST "!K_LIBUSB10_OUTPUT_DIR!\!K_LIBUSB10_NAME!.lib" (
		POPD
		goto Error
	)
	COPY /Y "!K_LIBUSB10_OUTPUT_DIR!\!K_LIBUSB10_NAME!.lib" "!G_BUILD_OUTPUT_BASE_ABS_DIR!\!DDKBUILDENV!\lib\!G_TARGET_OUTPUT_ARCH!"
	
	CALL ddk_build DLL
	IF NOT EXIST "!K_LIBUSB10_OUTPUT_DIR!\!K_LIBUSB10_NAME!.dll" (
		POPD
		goto Error
	)
	POPD
	
	COPY /Y "!K_LIBUSB10_OUTPUT_DIR!\!K_LIBUSB10_NAME!.dll" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\"
	COPY /Y "!K_LIBUSB10_OUTPUT_DIR!\!K_LIBUSB10_NAME!.lib" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\"

	IF /I "!_BUILDARCH!" EQU "x86" (
		COPY /Y "!K_LIBUSB0_DEP_DIR!\bin\!_BUILDARCH!\libusb0_x86.dll" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\libusb0.dll"
		COPY /Y "!K_LIBUSB0_DEP_DIR!\lib\msvc\libusb.lib" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\libusb0.lib"
		
		REM - make.cfg is configured so the x86 dll is the very last to build.
		REM - This batch file is ran while inside of the x86 ddk environment,
		REM - thus it is used to build libwdi and inf-wizard was well.
		CALL :FinalizeDistribution
	) ELSE (
		COPY /Y "!K_LIBUSB0_DEP_DIR!\bin\!_BUILDARCH!\libusb0.dll" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\"
		IF /I "!_BUILDARCH!" EQU "ia64" (
			COPY /Y "!K_LIBUSB0_DEP_DIR!\lib\msvc_i64\libusb.lib" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\libusb0.lib"
		) ELSE (
			COPY /Y "!K_LIBUSB0_DEP_DIR!\lib\msvc_x64\libusb.lib" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\libusb0.lib"
		)
	)
	GOTO Done
)

:Error
	ECHO Cannot create final dist. Missing !K_LIBUSB10_NAME!/libusb-win32.
	ECHO Update %0 with the correct directories.
	GOTO Done

:Done
GOTO :EOF

:FinalizeDistribution
	ECHO [FinalizeDistribution]
	ECHO.
	
	REM - Remove pdb and exp files; leave the driver pdb.
	DEL /S /Q !G_BUILD_OUTPUT_BASE_ABS_DIR!\!DDKBUILDENV!\dll\*.pdb
	DEL /S /Q !G_BUILD_OUTPUT_BASE_ABS_DIR!\!DDKBUILDENV!\*.exp

	REM - Copy in the libusb-win32 includes
	CALL .\make_tasks\deploy_dep.cmd "!K_LIBUSB0_DEP_DIR!" "!G_BUILD_OUTPUT_BASE_ABS_DIR!" "!K_LIBUSB0_NAME!" ".\make_tasks\!K_LIBUSB0_NAME!.dep.lst"

	REM - Copy in the libusb-1.x includes
	CALL .\make_tasks\deploy_dep.cmd "!K_LIBUSB10_DEP_DIR!" "!G_BUILD_OUTPUT_BASE_ABS_DIR!" "!K_LIBUSB10_NAME!" ".\make_tasks\!K_LIBUSB10_NAME!.dep.lst"
	
	REM - Copy in the libusbK includes
	CALL .\make_tasks\deploy_dep.cmd "!K_LIBUSBK_DEP_DIR!" "!G_BUILD_OUTPUT_BASE_ABS_DIR!" "!K_LIBUSBK_NAME!" ".\make_tasks\!K_LIBUSBK_NAME!.dep.lst"
	
	REM - Get rid of those i386 dirs.
	CALL :RenameOutputSubDirs "\sys\i386" "\sys\x86" "\lib\i386" "\lib\x86" "\dll\i386" "\dll\x86"
	
	REM - Move the customized libusbK inf-wizard files into the libwdi directory.
	CALL :SwapFile "!K_LIBWDI_DIR!\msvc\config.h" libwdi ".\src\libwdi\config.h"
	CALL :SwapFile "!K_LIBWDI_DIR!\examples\inf_wizard.c" libwdi ".\src\libwdi\inf_wizard.c"
	CALL :SwapFile "!K_LIBWDI_DIR!\examples\inf_wizard_rc.rc" libwdi ".\src\libwdi\inf_wizard_rc.rc"
	
	REM - Build libwdiK and inf-wizardK.
	PUSHD !CD!
	CD /D "!K_LIBWDI_DIR!"
	SET K_LIBUSBK_DIR=!G_BUILD_OUTPUT_BASE_ABS_DIR!\!DDKBUILDENV!
	SET C_DEFINES=/DLIBUSBK_DIR=\"!K_LIBUSBK_DIR:\=/!\" /DLIBUSB0_DIR=\"!K_LIBUSB0_DEP_DIR:\=/!\" /DINFWIZARD_LIBUSBK=1
	SET RC_DEFINES=!C_DEFINES!
	CALL ddk_build inf_wizard_only
	SET C_DEFINES=
	SET RC_DEFINES=
	POPD
	
	REM - Restore the files that were replaced.
	CALL :RestoreFile "!K_LIBWDI_DIR!\examples\inf_wizard_rc.rc" libwdi
	CALL :RestoreFile "!K_LIBWDI_DIR!\examples\inf_wizard.c" libwdi
	CALL :RestoreFile "!K_LIBWDI_DIR!\msvc\config.h" libwdi

	IF NOT EXIST "!K_LIBWDI_DIR!\examples\inf-wizard.exe" (
		ECHO ERROR - "!K_LIBWDI_DIR!\examples\inf-wizard.exe" not found.
		GOTO :EOF
	)
	COPY /Y "!K_LIBWDI_DIR!\examples\inf-wizard.exe" "!K_LIBUSBK_DIR!\libusbK-inf-wizard.exe"
	
	SET TOKVAR_LTAG=@
	SET TOKVAR_RTAG=@
	IF EXIST "!G_BUILD_OUTPUT_BASE_ABS_DIR!\!K_LIBUSBK_SETUP_NAME!.iss" DEL "!G_BUILD_OUTPUT_BASE_ABS_DIR!\!K_LIBUSBK_SETUP_NAME!.iss" >NUL
	!DCMD! -et ".\src\setup.iss.in" "!G_BUILD_OUTPUT_BASE_ABS_DIR!\!K_LIBUSBK_SETUP_NAME!.iss"
	SET TOKVAR_LTAG=
	SET TOKVAR_RTAG=
	COPY /Y .\make_tasks\libusbK.bmp "!G_BUILD_OUTPUT_BASE_ABS_DIR!\"
	!K_ISCC_EXE! "!G_BUILD_OUTPUT_BASE_ABS_DIR!\!K_LIBUSBK_SETUP_NAME!.iss"

GOTO :EOF

:RenameOutputSubDirs
	IF EXIST "!G_BUILD_OUTPUT_BASE_ABS_DIR!\!DDKBUILDENV!%~2" RMDIR /S /Q "!G_BUILD_OUTPUT_BASE_ABS_DIR!\!DDKBUILDENV!%~2"
	MOVE /Y "!G_BUILD_OUTPUT_BASE_ABS_DIR!\!DDKBUILDENV!%~1" "!G_BUILD_OUTPUT_BASE_ABS_DIR!\!DDKBUILDENV!%~2"
	SHIFT /1
	SHIFT /1
	IF "%~1" NEQ "" GOTO RenameOutputSubDirs
GOTO :EOF


:SwapFile
	IF NOT EXIST "%~1" IF EXIST "%~1.%~2" MOVE /Y "%~1.%~2" "%~1"
	MOVE /Y "%~1" "%~1.%~2"
	COPY /Y "%~3" "%~1"
GOTO :EOF

:RestoreFile
	MOVE /Y "%~1.%~2" "%~1"
GOTO :EOF

