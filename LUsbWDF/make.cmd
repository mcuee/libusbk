@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
SET WOUND_GUID=B49955DF-0623-4b65-A6E3-B5A91F3D2CFB
SET DIR_WDK_ROOT=Z:\WinDDK
SET DIR_PACKAGE=Z:\packages
SET DIR_BUILD=!CD!\driver\project
SET CLEAN_DIR_PATTERNS=_ReSharper* objfre* objchk* x64 x86 Debug Release
SET CLEAN_FILE_PATTERNS=*.suo *.resharper *.user *.ncb *.orig buildfre* buildchk*
SET CLEAN_ATTRIB_PATTERNS=*.suo
SET BUILD_ERRORLEVEL=0

::Uncomment below to compile with WDK v6001.18002
::
::SET USE_W2K_WDK=TRUE

IF "%~1" NEQ "!WOUND_GUID!" (
	IF /I "%~1" EQU "dist" (
	
		ECHO Cleaning..
		CALL CMD /C make.cmd !WOUND_GUID! clean 2>NUL>NUL
		
		CALL :SafeCMD make.cmd !WOUND_GUID! build x86 WXP fre no_oacr %*
		IF !BUILD_ERRORLEVEL! NEQ 0 GOTO DoneWithErrors
		
		CALL :SafeCMD make.cmd !WOUND_GUID! build amd64 WNET fre no_oacr %*
		IF !BUILD_ERRORLEVEL! NEQ 0 GOTO DoneWithErrors
		
		CALL :SafeCMD make.cmd !WOUND_GUID! build ia64 WNET fre no_oacr %*
		IF !BUILD_ERRORLEVEL! NEQ 0 GOTO DoneWithErrors
	) ELSE (
		CALL CMD /C make.cmd !WOUND_GUID! %*
	)
	
	:DoneWithErrors
	:DoneWithSuccess
	GOTO :EOF
)

PUSHD !CD!
CD /D !DIR_BUILD!
SET DIR_BUILD=!CD!

IF DEFINED USE_W2K_WDK (
	SET BUILD_ARCH=
	SET BUILD_MODE=FRE
	SET TARGET_OS=W2K
	SET WDK_NO_OACR=
	SET DIR_WDK=!DIR_WDK_ROOT!\6001.18002
) ELSE (
	SET BUILD_ARCH=x64
	SET BUILD_MODE=FRE
	SET TARGET_OS=WIN7
	SET DIR_WDK=!DIR_WDK_ROOT!\7600.16385.1
)

SET DIR_WDK_BIN=!DIR_WDK!\bin
SET DIR_DIST_OUTPUT=!DIR_PACKAGE!\LUsbWDF\bin

SET LIBUSB_WDF_SYS=LUsbWDF.sys

SET SIGNTOOL=!DIR_WDK!\bin\!PROCESSOR_ARCHITECTURE!\SignTool.exe
SET CERT_ADD=!DIR_BUILD!\MSCV-GlobalSign.cer
SET CERT_NAME=Akeo Consulting
SET CERT_TIMESTAMP_URL=http://timestamp.globalsign.com/scripts/timstamp.dll
SET SignFile_Options=!SignFile_Options! /t !CERT_TIMESTAMP_URL!

CALL :ParseCommandLine %*

IF /I "!BUILD_COMMAND!" EQU "" CALL :ShowUsage
IF /I "!BUILD_COMMAND!" EQU "formatcode" CALL :FormatCode
IF /I "!BUILD_COMMAND!" EQU "clean" CALL :Clean
IF /I "!BUILD_COMMAND!" EQU "build" CALL :Build

POPD
GOTO :EOF


:Clean
	ECHO Cleaning..
	PUSHD !CD!
	CD /D "!DIR_BUILD!\.."
	CALL :RemoveDirs !CLEAN_DIR_PATTERNS! 2>NUL
	ATTRIB -R -H -S  !CLEAN_ATTRIB_PATTERNS! /S 2>NUL>NUL
	DEL /S /Q  !CLEAN_FILE_PATTERNS! 2>NUL>NUL
	POPD
GOTO :EOF

:Build
	IF /I "!BUILD_MODE!" EQU "" GOTO ShowUsage
	
	PUSHD !CD!
	CALL "!DIR_WDK_BIN!\setenv.bat" !DIR_WDK! !BUILD_MODE! !BUILD_ARCH! !TARGET_OS! !WDK_OPTION_NO_OACR!
	POPD

	CALL :MatchAny "!_BUILDARCH!" "SET DIR_BUILD_OUTPUT=!DIR_BUILD!\obj!BUILD_ALT_DIR!\i386" "" x86 i386
	CALL :MatchAny "!_BUILDARCH!" "SET DIR_BUILD_OUTPUT=!DIR_BUILD!\obj!BUILD_ALT_DIR!\!_BUILDARCH!" "" amd64
	CALL :MatchAny "!_BUILDARCH!" "SET DIR_BUILD_OUTPUT=!DIR_BUILD!\obj!BUILD_ALT_DIR!\!_BUILDARCH!" "" ia64
	
	CALL :MatchAny "!_BUILDARCH!" "SET LIBWDI_BUILDARCH=x86" "" x86 i386
	CALL :MatchAny "!_BUILDARCH!" "SET LIBWDI_BUILDARCH=amd64" "" amd64
	CALL :MatchAny "!_BUILDARCH!" "SET LIBWDI_BUILDARCH=ia64" "" ia64
	
	ECHO Building LibUsbWDF - !_BUILDARCH!..
	IF EXIST "build!BUILD_ALT_DIR!.err" DEL "build!BUILD_ALT_DIR!.err"
	IF EXIST "build!BUILD_ALT_DIR!.wrn" DEL "build!BUILD_ALT_DIR!.wrn"
	CALL build -ceZ 2>NUL>NUL
	IF NOT EXIST "build!BUILD_ALT_DIR!.err" (
		SET __SRC=!DIR_WDK!\redist\wdf\!_BUILDARCH!\WdfCoInstaller01009.dll
		SET __DST=!DIR_BUILD_OUTPUT!\WdfCoInstaller01009.dll
		IF NOT EXIST "!__DST!" COPY /Y "!__SRC!" "!__DST!" >NUL
		IF EXIST "build!BUILD_ALT_DIR!.wrn" (
			ECHO.
			ECHO !BUILD_ALT_DIR! BUILD SUCCEEDED WITH SOME WARNINGS.
			ECHO oooooooooooooooooooooooooooooo W A R N I N G S ooooooooooooooooooooooooooooooo
			TYPE "build!BUILD_ALT_DIR!.wrn"
			ECHO ------------------------------------------------------------------------------
			ECHO.
		) ELSE (
			ECHO !BUILD_ALT_DIR! BUILD SUCCESSFUL.
			ECHO.
		)
		
		IF "!ENABLE_SIGNING!" EQU "1" (
			PUSHD !CD!
			CD /D !DIR_BUILD_OUTPUT!
			CALL :SignFile !LIBUSB_WDF_SYS!
			POPD		
		)
		
		IF EXIST "!DIR_PACKAGE!" (
			SET __SRC=!DIR_BUILD_OUTPUT!\!LIBUSB_WDF_SYS!
			SET __DST=!DIR_DIST_OUTPUT!\!LIBWDI_BUILDARCH!\
			IF NOT EXIST "!__DST!" MKDIR "!__DST!"
			
			IF EXIST "!__SRC!" COPY /Y "!__SRC!" "!__DST!" >NUL
			
			SET __SRC=!DIR_WDK!\redist\wdf\!_BUILDARCH!\WdfCoInstaller01009.dll
			SET __DST=!DIR_DIST_OUTPUT!\!LIBWDI_BUILDARCH!\WdfCoInstaller01009.dll
			IF EXIST "!__SRC!" COPY /Y "!__SRC!" "!__DST!" >NUL

		) ELSE (
			ECHO Distribution directory not set.
		)

	) ELSE (

		TYPE "build!BUILD_ALT_DIR!.err"
		ECHO.
		ECHO ///////////////////////////////
		ECHO !BUILD_ALT_DIR! BUILD FAILED.
		ECHO ///////////////////////////////
		ECHO.
		IF EXIST "build!BUILD_ALT_DIR!.wrn" (
			SET /P __DISP_WARNING="Display WDK warning messages? (y/n) :"
			IF /I "!__DISP_WARNING!" EQU "y" (
				TYPE "build!BUILD_ALT_DIR!.wrn"
			)
		)
		
		CALL :SetError 1
		GOTO :EOF
	)
	
GOTO :EOF

:User_LoadOption
	CALL :MatchAny "%~1" "SET ENABLE_SIGNING=1" "" sign
	IF !MatchAny! EQU 1 GOTO :EOF
	CALL :MatchAny "%~1" "SET BUILD_ARCH=" "" x86 i386
	IF !MatchAny! EQU 1 GOTO :EOF
	CALL :MatchAny "%~1" "SET BUILD_ARCH=64" "" ia64
	IF !MatchAny! EQU 1 GOTO :EOF
	CALL :MatchAny "%~1" "SET BUILD_ARCH=x64" "" x64 amd64 intel64
	IF !MatchAny! EQU 1 GOTO :EOF
	CALL :MatchAny "%~1" "SET TARGET_OS=%~1" "" WIN7 WLH WXP WNET W2K
	IF !MatchAny! EQU 1 GOTO :EOF
	CALL :MatchAny "%~1" "SET BUILD_MODE=FRE" "" fre
	IF !MatchAny! EQU 1 GOTO :EOF
	CALL :MatchAny "%~1" "SET BUILD_MODE=CHK" "" chk debug
	IF !MatchAny! EQU 1 GOTO :EOF
	CALL :MatchAny "%~1" "SET WDK_OPTION_NO_OACR=no_oacr" "" no_oacr
	IF !MatchAny! EQU 1 GOTO :EOF	
	CALL :MatchAny "%~1" "SET BUILD_COMMAND=%~1" "" clean build formatcode
	IF !MatchAny! EQU 1 GOTO :EOF
	CALL :MatchAny "%~1" "SET BUILD_TYPE=%~1" "" dist
	IF !MatchAny! EQU 1 GOTO :EOF
	CALL :MatchAny "%~1" "ECHO Starting.." "" "!WOUND_GUID!"
	IF !MatchAny! EQU 1 GOTO :EOF
	
	ECHO Unknown parameter "%~1"
GOTO :EOF

:MatchAny
	SET MatchAny=0
	SET __MATCH=%~1
	SHIFT /1
	SET __DO_IF_TRUE=%1
	SHIFT /1
	SET __DO_IF_FALSE=%1
	:MatchAny_Next
	SHIFT /1
	IF "%~1" EQU "" GOTO MatchAny_Done
	IF /I "!__MATCH!" EQU "%~1" (
		SET MatchAny=1
		IF !__DO_IF_TRUE! NEQ "" (
			CALL !__DO_IF_TRUE:~1,-1!
		)
		GOTO :EOF
	)
	GOTO MatchAny_Next
	:MatchAny_Done
	IF !__DO_IF_FALSE! NEQ "" (
		CALL !__DO_IF_FALSE:~1,-1!
	)
GOTO :EOF

:ParseCallback_FoundParameterWithValue
	CALL :DBG ParseCallback_FoundParameterWithValue %*
	
	IF /I "%~1" EQU "E_RET_FILE" (
		SET E_SND_FILE=%~2
		GOTO :EOF
	)
	SET __TEMP=%1
	SET __TEMP=!__TEMP:~1,4!
	IF /I "!__TEMP!" EQU "PATH_" (
		SET %~1=%~s2
	) ELSE (
		SET CMD_%~1=%~2
	)
	SET __TEMP=
GOTO :EOF

:ParseCallback_FoundQuotedString
	CALL :DBG ParseCallback_FoundQuotedString %*
	
GOTO :EOF

:ParseCallback_FoundCommand
	CALL :DBG ParseCallback_FoundCommand %*
	
	CALL :User_LoadOption "%~1"
GOTO :EOF

:ParseCommandLine
	SET __EXP=%*
	SET __VALUE=""
	SET __IN_QUOTE=0

	CALL :DBG [ParseCommandLine] __EXP=!__EXP!

	::Set exp to the entire command line, replacing dblqoutes with grave accents.
	SET __EXP=!__EXP:"=`!
	
	CALL :DBG [ParseCommandLine] __EXP=!__EXP!

	:ParseCommandLine_NextChar
	IF DEFINED __EXP (
		::Get the first char in exp.
		SET __CH="!__EXP:~0,1!"
		::Remove the first char from exp. 
		SET __EXP=!__EXP:~1!
	) ELSE (
		::No more chars in exp, loop one last time.
		SET __CH=""
	)

	IF !__CH! EQU "`" (
		::Found a dblquote
		IF "!__IN_QUOTE!" EQU "1" (
			::This is the second dblquote.
			IF DEFINED __PARAM (
				::This dblquoted string had a leading parameter attached; report it.
				CALL :ParseCallback_FoundParameterWithValue "!__PARAM!" !__VALUE!
			) ELSE (
				::No parameters for this dblquoted string; report it.
				CALL :ParseCallback_FoundQuotedString !__VALUE!
			)
			::Flush the buffers.
			SET __VALUE=""
			SET __PARAM=
			SET __IN_QUOTE=0
		) ELSE (
			::This is the first dblquote.
			SET __IN_QUOTE=1
		)
		GOTO ParseCommandLine_NextChar
	) ELSE IF !__CH! EQU " " (
		::Found a space.
		IF "!__IN_QUOTE!" EQU "0" (
			::This space serves as a seperator because we're not in quotes.
			IF "!__VALUE!" NEQ "" (
				:: This is a single unqouted command
				CALL :ParseCallback_FoundCommand !__VALUE!
				SET __VALUE=""
				GOTO ParseCommandLine_NextChar
			)
		)
	) ELSE IF !__CH! EQU "=" (
		::Found an equals.
		IF "!__IN_QUOTE!" EQU "0" (
			::This equals serves as a seperator because we're not in quotes.
			CALL :DBG __CH=!__CH!
			CALL :DBG __PARAM=!__PARAM!
			CALL :DBG __VALUE=!__VALUE!
			CALL :DBG __EXP=!__EXP!
			CALL :DBG __IN_QUOTE=!__IN_QUOTE!
			SET __PARAM=!__VALUE:~1,-1!
			SET __VALUE=""
			GOTO ParseCommandLine_NextChar
		)
	) ELSE IF !__CH! EQU "" (
		::No more chars, pump any remaining through.
		IF DEFINED __PARAM (
			IF !__VALUE! NEQ "" (
				CALL :ParseCallback_FoundParameterWithValue "!__PARAM!" !__VALUE!
			) ELSE (
				CALL :ParseCallback_FoundCommand "!__PARAM!"
			)
		) ELSE IF !__VALUE! NEQ "" (
			CALL :ParseCallback_FoundCommand !__VALUE!
		)
		GOTO ParseCommandLine_Done
	)
	::No delimiters were found; just add this char to the end of our temp value.
	SET __VALUE="!__VALUE:~1,-1!!__CH:~1,-1!"
	GOTO ParseCommandLine_NextChar
	
	:ParseCommandLine_Done
	::Undefined the temp vars.
	SET __VALUE=
	SET __PARAM=
	SET __IN_QUOTE=
	SET __CH=
	SET __EXP=
GOTO :EOF

:SignFile
	!SIGNTOOL! sign /v /ac "!CERT_ADD!" /s my /n "!CERT_NAME!" !SignFile_Options! %1
	!SIGNTOOL! verify /kp /v %1
GOTO :EOF

:RemoveDirs
	SET __TEMP=%~1
	FOR /F "tokens=* delims=" %%I IN ('dir /AD /S /B !__TEMP!') DO (
		ECHO Removing %%~nI
		RMDIR /S /Q %%I
	)
	SHIFT /1
	IF "%~1" NEQ "" GOTO RemoveDirs
GOTO :EOF

:FormatCode
	Astyle.exe -V 2>NUL>NUL
	IF "!ERRORLEVEL!" NEQ "0" (
		ECHO Astyle not found in path. Download it at astyle.sourceforge.net.
		GOTO :EOF
	)
	
	PUSHD !CD!
	CD /D "!DIR_BUILD!\.."

	Astyle.exe --style=allman --lineend=windows --align-pointer=type --indent=tab *.c *.h
	
	POPD
GOTO :EOF

:: Executes a command line using CMD /C and returns an error code.
:SafeCMD
	SET E_RET_FILE=!TEMP!\!RANDOM!.mrk
	DEL /Q "!E_RET_FILE!" 2>NUL>NUL
	ECHO 0 > "!E_RET_FILE!"
	CALL :DBG [SafeCMD] CMD /C %* E_RET_FILE="!E_RET_FILE!"
	CALL CMD /C %* E_RET_FILE="!E_RET_FILE!"
	FOR /F "eol=; tokens=1 delims= " %%I IN (!E_RET_FILE!) DO (
		SET BUILD_ERRORLEVEL=%%I
		GOTO SafeCMD_Done
	)
	
	:SafeCMD_Done
	DEL /Q "!E_RET_FILE!" >NUL

GOTO :EOF

:SetError
	SET BUILD_ERRORLEVEL=%1
	IF DEFINED E_SND_FILE (
		IF EXIST "!E_SND_FILE!" (
			ECHO %1 > "!E_SND_FILE!"
		)
	)
)
GOTO :EOF

:ShowUsage
	ECHO LUsbWDF_make [build/clean] [x86/amd64/ia64]
GOTO :EOF

:DBG
REM	ECHO %*
GOTO :EOF