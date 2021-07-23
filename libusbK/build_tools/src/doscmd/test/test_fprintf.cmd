@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
SET DIR_WORKING=!CD!
SET DIR_TEST=%~dp0
IF NOT EXIST "!DOSCMD!" SET DOSCMD=!DIR_TEST!\doscmd.exe
IF NOT EXIST "!DOSCMD!" SET DOSCMD=!DIR_TEST!\..\build_tools\doscmd.exe

IF NOT EXIST "!DOSCMD!" (
	ECHO.
	ECHO doscmd.exe not found.
	ECHO.
	GOTO :EOF
)

PUSHD !CD!
CD /D !DIR_TEST!
!DOSCMD! -ff CON "@@ DOSCMD: You Are Here\x21\n\n"

CALL :ShowColor 0 "Black" 8
CALL :ShowColor 8 "Gray"
ECHO.

CALL :ShowColor 1 "Blue"
CALL :ShowColor 9 "Light Blue"
ECHO.

CALL :ShowColor 2 "Green"
CALL :ShowColor A "Light Green"
ECHO.

CALL :ShowColor 3 "Aqua"
CALL :ShowColor B "Light Aqua"
ECHO.

CALL :ShowColor 4 "Red"
CALL :ShowColor C "Light Red"
ECHO.

CALL :ShowColor 5 "Purple"
CALL :ShowColor D "Light Purple"
ECHO.

CALL :ShowColor 6 "Yellow"
CALL :ShowColor E "Light Yellow"
ECHO.

CALL :ShowColor 7 "White"
CALL :ShowColor F "Bright White"
ECHO.
ECHO.

!DOSCMD! -ff CON "\kE1@s \x21\x21\n" "Now we will do it all at once"
pause
!DOSCMD! -ff CON "\k07\n"
ECHO !DOSCMD! -ff CON !_FFS! !_ARGS! > cbang.bat
!DOSCMD! -ff CON !_FFS! !_ARGS!
POPD

GOTO :EOF

:ShowColor
	SET _FG=%~1
	SET _DISP="%~2                         "
	SET _BG=%~3
	IF NOT DEFINED _BG SET _BG=0
	SET _FF="@s \k0F= \k!_BG!!_FG!@s\k07"
	!DOSCMD! -ff CON !_FF! "!_FG!" "!_DISP:~1,12!"
	
	IF DEFINED _ARGS (
		SET _ARGS=!_ARGS! "%~1" "%~2"
	) ELSE (
		SET _ARGS="%~1" "%~2"
	)

	IF DEFINED _FFS (
		SET _FFS="!_FFS:~1,-1!@s \k0F= \k!_BG!%~1@-12s\k07"
	) ELSE (
		SET _FFS="@s \k0F= \k!_BG!%~1@-12s\k07"
	)
	IF DEFINED _NEEDS_CR (
		SET _FFS="!_FFS:~1,-1!\n"
		SET _NEEDS_CR=
	) ELSE (
		SET _NEEDS_CR=1
	)
	
GOTO :EOF

	