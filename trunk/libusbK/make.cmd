@ECHO OFF
REM :: 
REM :: WDK build/package/distribution script
REM :: Written by Travis Robinson - 12/20/2010
REM ::

SET /A MAKE_RUN_COUNT=MAKE_RUN_COUNT+1
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

SET BUILD_ERRORLEVEL=0
SET WOUND_GUID=B49955DF-0623-4b65-A6E3-B5A91F3D2CFB
SET CreateTempFileList=

REM :: REQUIRED dependency - DOSCMD  
IF NOT DEFINED DCMD (
	SET MAKE_CMD_PATH=%~dp0
	PUSHD %CD%
	CD /D "!MAKE_CMD_PATH!"
	FOR /F "usebackq eol=; tokens=* delims=" %%D IN (`dir /A-D /B /S "doscmd.exe" "!MAKE_CMD_PATH!\doscmd.exe"`) DO IF NOT DEFINED DCMD SET DCMD=%%~D
	IF NOT DEFINED DCMD CALL :FindFileInPath DCMD doscmd.exe
	POPD
	IF DEFINED DCMD (
		REM - doscmd needs added to the path to work properly as a command in a for loop; this change is temporary
		CALL :PathSplit "!DCMD!"
		SET G_DOSCMD_DIR=!_DIR_!
		CALL :TrimExR "\" G_DOSCMD_DIR
		SET PATH=!G_DOSCMD_DIR!;!PATH!
		SET DCMD=doscmd.exe
		IF "!MAKE_RUN_COUNT!" EQU "1" !DCMD! --version
	) ELSE (
		ECHO DOSCMD not found.  DOSCMD is required for the WDK build process.
		GOTO :EOF
	)
)

REM :: START OF PRE/INIT SECTION ---------------------------------------
IF "%~1" NEQ "!WOUND_GUID!" (
	REM - first set the default 
	SET G_MAKE_CFG_FILE=!CD!\make.cfg
	IF NOT EXIST "!G_MAKE_CFG_FILE!" SET G_MAKE_CFG_FILE=%~dp0\make.cfg
	
	REM - Arguments are loaded twice so you can pass "MAKE_CFG_FILE=yourdir\yourmakefile.cfg"
	REM - load arguments
	CALL :ArgumentsToEnv 1 %*

	IF NOT EXIST "!G_MAKE_CFG_FILE!" (
		CALL :Print E "MAKCFG" "Configuration file not found.\n\tFile: @s\n\tA configuration file located at the root of your WDK 'sources' tree is required." "!G_MAKE_CFG_FILE!"
		GOTO DoneWithErrors
	)
	
	REM - Make sure the current dir is the directory of the cfg file.
	CALL :PathSplit "!G_MAKE_CFG_FILE!"
	SET MAKE_ORGINAL_WORKING_DIR=!CD!
	PUSHD "!CD!"
	IF /I "!CD!" NEQ "!_DIR_!" CD /D "!_DIR_!"
	SET _MAKE_DIR_=!_DIR_!
	
	REM - load config file
	CALL :LoadCfgFile "!G_MAKE_CFG_FILE!"
	IF !BUILD_ERRORLEVEL! NEQ 0 GOTO DoneWithErrors

	REM - load arguments [arguments always override]
	CALL :ArgumentsToEnv 1 %*
	IF !BUILD_ERRORLEVEL! NEQ 0 GOTO DoneWithErrors
	
	REM - If the dist arg was specified, build all platforms/archs listed in the DIST_BUILD_LIST env var.
	IF DEFINED G_DIST (

		IF NOT EXIST "!G_PACKAGE_DIR!" MKDIR "!G_PACKAGE_DIR!"
		CALL :ToAbsPath  G_PACKAGE_ABS_DIR "!G_PACKAGE_DIR!"
		
		IF NOT EXIST "!G_PACKAGE_TEMP_DIR!" MKDIR "!G_PACKAGE_TEMP_DIR!"
		CALL :ToAbsPath  G_PACKAGE_TEMP_ABS_DIR "!G_PACKAGE_TEMP_DIR!"

		REM - default to a FRE build if none was specified.
		IF "!G_FRE!!G_CHK!" EQU "" SET G_FRE=FRE
		CALL :ExecuteUserTask pre dist
		
		FOR /F "usebackq eol=; tokens=* delims=" %%K IN (`!DCMD! "-sp=;" "!G_DIST_BUILD_LIST!"`) DO (
			SET DIST_PLAT=%%~K
			CALL :Trim DIST_PLAT
			IF "!DIST_PLAT!" NEQ "" (
				ECHO DIST:!DIST_PLAT!
				CALL :SafeCMD %0 !WOUND_GUID! build !DIST_PLAT!
				IF DEFINED G_VERSION SET G_VERSION=
				IF !BUILD_ERRORLEVEL! NEQ 0 GOTO DoneWithErrors
			)
		)
		CALL :ExecuteUserTask post dist

	) ELSE (
		CALL :SafeCMD make.cmd !WOUND_GUID!
		IF !BUILD_ERRORLEVEL! NEQ 0 GOTO DoneWithErrors
	)
	
	:DoneWithErrors
	:DoneWithSuccess
	
	REM - This script makes heavy use of temp files.  If you CTRL-C it there will
	REM - be several temp files abandoned in the TEMP dir.
	CALL :DestroyAllTempFiles
	POPD
	
	GOTO :EOF
)

REM :: END OF PRE/INIT SECTION -----------------------------------------
REM :: START OF MAIN SECTION -------------------------------------------

	REM - load the arguments. this will include arguments that may have been passed
	REM - into make.cmd after it is wound. see the dist command above.
	CALL :ArgumentsToEnv 2 %*
	IF !BUILD_ERRORLEVEL! NEQ 0 GOTO Processing_Done

	REM - When a command is executed with SafeCMD it sets E_RET_FILE to a temp file
	REM - which is checked for an error code after the command completes.
	IF DEFINED G_E_RET_FILE SET E_SND_FILE=!G_E_RET_FILE!

	REM - add env option vars to WDK_ENV_OPTIONS [only ones explicitly passed by the user]

	CALL :AddIfDef WDK_ENV_OPTIONS G_FRE G_CHK G_64 G_X64 G_WIN7 G_WLH G_WXP G_WNET G_W2K

	REM - if none were defined, use the default build options.
	IF NOT DEFINED WDK_ENV_OPTIONS SET WDK_ENV_OPTIONS=!G_WDK_DEF_ENV_OPTIONS!
	
	CALL :AddIfDef WDK_ENV_OPTIONS G_NO_OACR

	REM - Check if any commands are defined
	SET __CHECK_CMD=
	CALL :AddIfDef __CHECK_CMD G_HELP G_FORMATCODE G_CLEAN G_BUILD G_VERSION G_ZIP

	REM - If no commands are defined, show the help.
	IF NOT DEFINED __CHECK_CMD SET G_HELP=help

	REM - call the commands that are defined; in this order.
	IF DEFINED G_HELP CALL :ShowUsage !G_HELP!
	IF DEFINED G_VERSION CALL :Version !G_VERSION!
	IF DEFINED G_FORMATCODE CALL :FormatCode !G_FORMATCODE!
	IF DEFINED G_CLEAN CALL :Clean !G_CLEAN!
	IF DEFINED G_BUILD CALL :Build !G_BUILD!
	IF DEFINED G_ZIP CALL :Zip !G_ZIP!
 
	:Processing_Done
	CALL :DestroyAllTempFiles

GOTO :EOF

REM :: END OF MAIN SECTION ---------------------------------------------
REM :: START OF COMMAND SECTION ----------------------------------------

:Version
	Call :PathSplit "!G_MAKE_CFG_FILE!"
	SET __Version_File=!_DIR_!!_FILE_!.versions
	IF NOT EXIST "!__Version_File!" GOTO Version_Done

	FOR /F "usebackq eol=; tokens=1,2,3,4 delims=;" %%A IN ('!G_VERSION!') DO (
		SET __VERSION_TARGET_NAME=%%~A
		SET __VERSION_TARGET_ACTION=%%~B
		SET __VERSION_TARGET_ACTION_OPTION=%%~C
		SET __VERSION_TARGET_ACTION_OPTION_VALUE=%%~D
		GOTO Version_Lookup
	)
	GOTO Version_NotFound

	:Version_Lookup
	CALL :VersionLookup "!__VERSION_TARGET_NAME!" "!__Version_File!"
	IF "!VersionLookup!" EQU "1" GOTO Version_Found

	:Version_NotFound:
	CALL :Print D "VERCTL" "Failed location version information for @s in @s." "%~1" "!__Version_File!"
	GOTO Version_Done

	:Version_Found:
	CALL :OutputToFile T __Version_Line @NULL FINDSTR /I /N /R /C:"^[ ]*!TARGET_VERSION_NAME![ ]*=" !__Version_File!
	IF NOT DEFINED __Version_Line_Error FOR /F "eol=; tokens=1 delims=:" %%A IN (!__Version_Line!) DO IF NOT DEFINED __VERSION_TARGET_LINE SET __VERSION_TARGET_LINE=%%~A
	CALL :Print D "VERCTL" "verson line = @s (@s), target = @s, version = @s\n\tin-file = @s\n\tout-file = @s" "!__VERSION_TARGET_LINE!" "!TARGET_VERSION_NAME!" "!TARGET_VERSION_LINK_NAME!" "!TARGET_VERSION!" "!TARGET_VERSION_IN_FILE!" "!TARGET_VERSION_OUT_FILE!"

	REM - a linked target can't change the version info; this is a safety feature
	IF /I "!TARGET_VERSION_NAME!" EQU "!TARGET_VERSION_LINK_NAME!" (
		IF NOT DEFINED __VERSION_TARGET_ACTION_OPTION SET __VERSION_TARGET_ACTION_OPTION=NANO
		IF /I "!__VERSION_TARGET_ACTION!" EQU "inc" SET __VER_ADD_AMT=+1
		IF /I "!__VERSION_TARGET_ACTION!" EQU "dec" SET __VER_ADD_AMT=-1
		IF /I "!__VERSION_TARGET_ACTION!" EQU "set" SET __VER_ADD_AMT=set
		IF NOT DEFINED TARGET_VERSION_!__VERSION_TARGET_ACTION_OPTION! (
			CALL :Print E "VERCTL" "Invalid version action option @s." "!__VERSION_TARGET_ACTION_OPTION!"
			GOTO Version_Done
		)
		IF "!__VER_ADD_AMT!" EQU "set" (
			SET /A TARGET_VERSION_!__VERSION_TARGET_ACTION_OPTION!=__VERSION_TARGET_ACTION_OPTION_VALUE
		) ELSE (
			SET /A TARGET_VERSION_!__VERSION_TARGET_ACTION_OPTION!=TARGET_VERSION_!__VERSION_TARGET_ACTION_OPTION!!__VER_ADD_AMT!
		)
		CALL :OutputToFile T __Version_NewLine @NULL ECHO !TARGET_VERSION_LINK_NAME! = !TARGET_VERSION_MAJOR!.!TARGET_VERSION_MINOR!.!TARGET_VERSION_MICRO!.!TARGET_VERSION_NANO!; !TARGET_VERSION_IN_FILE!; !TARGET_VERSION_OUT_FILE!;
		SET /A __VERSION_TARGET_LINE=__VERSION_TARGET_LINE-1
		CALL :CreateTempFile __Version_NewFile
		!DCMD! -fl=!__VERSION_TARGET_LINE!;update; "!__Version_File!" "!__Version_NewFile!" "!__Version_NewLine!"
		IF EXIST "!__Version_NewFile!" (
			CALL :PathSplit "!__Version_NewFile!"
			IF "!_FILESIZE_!" GTR "12" (
				COPY /Y "!__Version_NewFile!" "!__Version_File!" 2>NUL>NUL
				IF "!ERRORLEVEL!" NEQ "0" GOTO Version_Error
				SET TARGET_VERSION=!TARGET_VERSION_MAJOR!.!TARGET_VERSION_MINOR!.!TARGET_VERSION_MICRO!.!TARGET_VERSION_NANO!
				CALL :Print S "VERCTL" "@s version changed to @s" "!TARGET_VERSION_NAME!" "!TARGET_VERSION!"
				GOTO Version_Done
			)
		)
		GOTO Version_Error
	)
	GOTO Version_Done
	
	:Version_Error
	CALL :Print E "VERCTL" "Unable to locate target @s in version file @s." "!__VERSION_TARGET_NAME! " "!__Version_File!"
	GOTO Version_Done
	
	:Version_Done
	CALL :Undefine __VERSION* __CHECK_NAME TARGET_VERSION_NAME __VER_ADD_AMT
GOTO :EOF

REM - %~1 = version string i.e '1.2.3.4'
REM - [SETS] TARGET_VERSION; TARGET_VERSION_MAJOR; TARGET_VERSION_MINOR; TARGET_VERSION_MICRO; TARGET_VERSION_NANO;
:VersionStringParse
	SET __VersionStringParse=%~1
	FOR /F "eol=; tokens=1,2,3,4 delims=.;" %%A IN ("!__VersionStringParse!") DO (
		SET TARGET_VERSION_MAJOR=%%A
		SET TARGET_VERSION_MINOR=%%B
		SET TARGET_VERSION_MICRO=%%C
		SET TARGET_VERSION_NANO=%%D
		CALL :Trim TARGET_VERSION_MAJOR TARGET_VERSION_MINOR TARGET_VERSION_MICRO TARGET_VERSION_NANO
	)
	SET __VersionStringParse=
GOTO :EOF

REM - %1 = version target name to look for
REM - %2 = version file to look in
REM - [SETS] TARGET_VERSION_LINK_NAME; TARGET_VERSION_NAME; TARGET_VERSION_IN_FILE; TARGET_VERSION_OUT_FILE; 
REM - [SETS] TARGET_VERSION; TARGET_VERSION_MAJOR; TARGET_VERSION_MINOR; TARGET_VERSION_MICRO; TARGET_VERSION_NANO;
REM - [SETS] __Version_Line to a filename with the [REAL] matched version record
REM - [RETURNS] VersionLookup = 1 if found, 0 if not
:VersionLookup
	CALL :Undefine TARGET_VERSION_LINK_NAME TARGET_VERSION TARGET_VERSION_MAJOR TARGET_VERSION_MINOR TARGET_VERSION_NANO
	SET TARGET_VERSION_NAME=%~1
	:VersionLookup_Link
	CALL :OutputToFile T __Version_Line @NULL FINDSTR /I /R /C:"^[ ]*!TARGET_VERSION_NAME![ ]*=" %~2
	FOR /F "eol=; tokens=1,2,3 delims=;" %%A IN (!__Version_Line!) DO (
		FOR /F "usebackq eol=; tokens=1,2 delims==" %%E IN ('%%~A') DO (
			IF NOT DEFINED TARGET_VERSION_LINK_NAME (
				SET TARGET_VERSION_LINK_NAME=%%~E
				
				REM - add new columns to the versions file here
				REM - modify the %%A FOR loop tokens [1,2,3,etc] as you add columns
				SET TARGET_VERSION_IN_FILE=%%~B
				SET TARGET_VERSION_OUT_FILE=%%~C
				CALL :Trim TARGET_VERSION_LINK_NAME TARGET_VERSION_IN_FILE TARGET_VERSION_OUT_FILE
			)				
			SET TARGET_VERSION_NAME=%%~E
			SET __CHECK_NAME=%%~F
			CALL :Trim TARGET_VERSION_NAME __CHECK_NAME
		)
		REM - If this is a linked version line, the 'version' field is the linked name.
		FINDSTR /I /R /C:"^[ ]*!__CHECK_NAME![ ]*=" %~2 2>NUL>NUL
		IF "!ERRORLEVEL!" NEQ "1" (
			CALL :Print D "VERCTL" "looking up linked version name for @s.." "!TARGET_VERSION_NAME!"
			SET TARGET_VERSION_NAME=!__CHECK_NAME!
			GOTO VersionLookup_Link
		)
		SET TARGET_VERSION=!__CHECK_NAME!
		CALL :VersionStringParse !TARGET_VERSION!
		IF DEFINED TARGET_VERSION_NANO GOTO VersionLookup_Found
		GOTO VersionLookup_NotFound
	)
	:VersionLookup_NotFound
	SET VersionLookup=0
	CALL :Undefine TARGET_VERSION*
	GOTO :EOF

	:VersionLookup_Found
	SET VersionLookup=1
GOTO :EOF

:FormatCode
	IF NOT EXIST "!G_FORMATCODE_EXE!" (
		SET AskYesNo_Default=1
		CALL :AskYesNo "\k0F@s not found. Look for it in path?" "!G_FORMATCODE_EXE!"
		IF NOT !AskYesNo! EQU 1 GOTO :EOF 
		SET __FORMATCODE_EXE=!G_FORMATCODE_EXE!
		CALL :FindFileInPath G_FORMATCODE_EXE "!G_FORMATCODE_EXE!"
		IF NOT EXIST "!G_FORMATCODE_EXE!" (
			CALL :Print E "FMTCOD" "Failed locating @s in path\r\n\t\k0ADownload Astyle at astyle.sourceforge.net" "!__FORMATCODE_EXE!"
			CALL :Print "FMTCOD" "make.cmd looks for optional programs in the path and the 'build_tools' sub-dir."
			GOTO :EOF
		) ELSE (
			CALL :Print S "FMTCOD" "Updating @s\n\twith FORMATCODE_EXE=@s" "!G_MAKE_CFG_FILE!" "!G_FORMATCODE_EXE!"
			!DCMD! --update-cfg=FORMATCODE_EXE "!G_MAKE_CFG_FILE!" "!G_FORMATCODE_EXE!"
		)
	)
	FOR /F "usebackq eol=; tokens=* delims=" %%K IN (`!DCMD! "-sp=;" "!G_FORMATCODE_ARGS!"`) DO (
		"!G_FORMATCODE_EXE!" %%K
		IF NOT "!ERRORLEVEL!" EQU "0" (
			CALL :SetError 1
			CALL :Print E "ERROR" "FormatCode failed: @s @s" "!G_FORMATCODE_EXE!" "%%~K"
			GOTO :EOF
		)
	)

GOTO :EOF

:Clean
	CALL :Print "CLEAN" "Cleaning targets and temporary files.."

	CALL :DefineDefaultDirEscapes __Clean_
	CALL :CreateTempFile __Clean_AllDirList
	CALL :CreateTempFile __Clean_AllFileList
	
	IF /I "!G_CLEAN!" EQU "" SET G_CLEAN=all;
	IF /I "!G_CLEAN!" EQU "clean" SET G_CLEAN=all;
	
	CALL :ExecuteUserTask pre clean
	
	FOR /F "usebackq eol=; tokens=1 delims=" %%A IN (`!DCMD! "-sp=;" "!G_CLEAN!"`) DO (
		SET __CLEAN_SET=%%~A
		CALL :Trim __CLEAN_SET
		CALL :Clean_!__CLEAN_SET!
	)
	
	REM - Even if there is nothing to clean both a dir and file list will exists with 0 bytes.
	IF NOT EXIST "!__Clean_AllDirList!" (
		CALL :Print E "CLEAN" "Invalid clean option@s." " !G_CLEAN!"
	) ELSE (
		CALL :RemoveList dir __Clean_AllDirList
		CALL :RemoveList file __Clean_AllFileList
	)
	
	CALL :Undefine __Clean_*
	CALL :ExecuteUserTask post clean
GOTO :EOF

	:Clean_Full
		CALL :Clean_All
		REM - Remove the entire output directory on a full clean
		ECHO !__Clean_BuildOutputDirRel!>>!__Clean_AllDirList!
		IF EXIST "!G_PACKAGE_TEMP_DIR!\*" DEL /S /Q "!G_PACKAGE_TEMP_DIR!\*"
	GOTO :EOF
	
	:Clean_All
		CALL :Clean_Src
		CALL :Clean_Bin
	GOTO :EOF
	
	:Clean_Bin
	:Clean_Output
	CALL :SyncPathList D __Clean_AllDirList  "!G_BUILD_OUTPUT_BASE_DIR!" "/AD-H-R-S /S"  G_CLEAN_BIN_EXP "/I" @NULL ""
	CALL :SyncPathList F __Clean_AllFileList "!G_BUILD_OUTPUT_BASE_DIR!" "/A-D-H-R-S /S" G_CLEAN_BIN_EXP "/I" @NULL ""
	GOTO :EOF
	
	:Clean_Src
	:Clean_Source
	REM - Src uses a 'final' exclude expression so as not to tamper with the output dir.
	CALL :CreateTempFile __Clean_RegExpExcludeList
	!DCMD! -ff "!__Clean_RegExpExcludeList!" "@s\n@s\n" "!__Clean_BuildOutputDirEsc!" "!__Clean_PackageDirEsc!"
	CALL :SyncPathList D __Clean_AllDirList  "!CD!" "/AD-H-R-S /S"  G_CLEAN_SRC_EXP "/I" __Clean_RegExpExcludeList "/I /V"
	CALL :SyncPathList F __Clean_AllFileList "!CD!" "/A-D-H-R-S /S" G_CLEAN_SRC_EXP "/I" __Clean_RegExpExcludeList "/I /V"
	GOTO :EOF
GOTO :EOF


:Zip
	IF NOT EXIST "!G_PACKAGE_DIR!" MKDIR "!G_PACKAGE_DIR!"
	IF NOT EXIST "!G_PACKAGE_DIR!" (
		CALL :SetError 1
		CALL :Print E "ZIP" "Failed creating package directory @s.\n\tCheck the PACKAGE_DIR setting in make.cfg." "!G_PACKAGE_DIR!"
		GOTO :EOF
	)
	SET __Zip_TempDir=!TEMP!\Zip_Temp_%RANDOM%

	CALL :DefineDefaultDirEscapes __Zip_
	
	SET __Zip_FileCount=0

	IF /I "!G_ZIP!" EQU "" SET G_ZIP=all;
	IF /I "!G_ZIP!" EQU "zip" SET G_ZIP=all;
	
	SET __Zip_TempDir=!TEMP!\Zip_Temp_%RANDOM%
	
	Call :PathSplit "!G_MAKE_CFG_FILE!"
	SET __Version_File=!_DIR_!!_FILE_!.versions
	CALL :VersionLookup "PACKAGE" "!_DIR_!!_FILE_!.versions"
	IF "!VersionLookup!" EQU "1" (
		SET F_PACKAGE_VERSION=!TARGET_VERSION!
		CALL :LoadCfgFile "!G_MAKE_CFG_FILE!" PACKAGE
	)
	
	CALL :ExecuteUserTask pre zip
	
	FOR /F "usebackq eol=; tokens=1 delims=" %%A IN (`!DCMD! "-sp=;" "!G_ZIP!"`) DO (
		SET __Zip_UserOption=%%~A
		CALL :Trim __Zip_UserOption
		CALL :Zip_!__Zip_UserOption!
	)
	
	IF "!__Zip_FileCount!" EQU "0" GOTO Zip_Error
	IF NOT EXIST "!__Zip_TempDir!" GOTO Zip_Error
	
	CALL :LoadCfgFile "!G_MAKE_CFG_FILE!" PACKAGE_NAME_FORMAT
	
	SET __Zip_AbsFilename=!__Zip_PackageDir!\!G_PACKAGE_NAME_FORMAT!.!G_PACKAGE_ZIP_EXT!
	IF EXIST "!__Zip_AbsFilename!" DEL /Q "!__Zip_AbsFilename!"

	SET __Zip_Cmd=7z.exe
	!__Zip_Cmd! 2>NUL>NUL
	IF "!ERRORLEVEL!" NEQ "0" (
		SET __Zip_Cmd=7za.exe
		!__Zip_Cmd! 2>NUL>NUL
	)
	IF "!ERRORLEVEL!" NEQ "0" (
		CALL :Print E "ZIP" "7z.exe not found.\n\tDownload 7Zip at @s." "www.7-zip.org"
		CALL :Print "ZIP" "make.cmd looks for optional programs in the path and the 'build_tools' sub-dir."
		GOTO Zip_Done
	)
	
	PUSHD !CD!
	CD /D !__Zip_TempDir!
	CALL :SafeCall !__Zip_Cmd! a -mx9 -r "!__Zip_AbsFilename!" *
	POPD

	IF "!BUILD_ERRORLEVEL!" NEQ "0" GOTO Zip_Error
	CALL :Print S "ZIP" "Done\x21 @s (@d files)" "!__Zip_AbsFilename!" "!__Zip_FileCount!"
	GOTO Zip_Done
	
	:Zip_Error
	CALL :Print E "ZIP" "Invalid zip option@s or other failure.\n\tUse the 'build_quiet=0' argument for more information." " !G_ZIP!"
	CALL :Print "ZIP" "make.cmd looks for optional programs in the path and the 'build_tools' sub-dir."
	
	:Zip_Done
	IF EXIST "!__Zip_TempDir!" RMDIR /S /Q "!__Zip_TempDir!"	
	CALL :Undefine __Zip_*
	CALL :ExecuteUserTask post zip
GOTO :EOF

	:Zip_All
		SET F_PACKAGE_NAME=!G_PACKAGE_ALL_NAME!
		CALL :LoadCfgFile "!G_MAKE_CFG_FILE!" PACKAGE_ALL_SUBDIR_FORMAT
		SET __Zip_TempDir=!__Zip_TempDir!\!G_PACKAGE_ALL_SUBDIR_FORMAT!
		CALL :Zip_Src "!G_PACKAGE_SRC_NAME!"
		CALL :Zip_Package "@NONE@"
	GOTO :EOF
	
	:Zip_Bin
	:Zip_Output
	IF NOT EXIST "!G_BUILD_OUTPUT_BASE_DIR!" (
		CALL :Print W "ZIP" "Base output directory @s does not exist." "!G_BUILD_OUTPUT_BASE_DIR!"
		GOTO :EOF
	)
	CALL :CreateTempFile __Zip_FileList
	CALL :SyncPathList F __Zip_FileList "!G_BUILD_OUTPUT_BASE_DIR!" "/A-D-H-R-S /S" G_CLEAN_BIN_EXP "/I /V" @NULL ""
	
	SET F_PACKAGE_NAME=!G_PACKAGE_BIN_NAME!
	CALL :LoadCfgFile "!G_MAKE_CFG_FILE!" PACKAGE_NAME_FORMAT

	IF "%~1" NEQ "" (
		CALL :ZipAddFileSet "!__Zip_TempDir!\%~1\!G_BUILD_OUTPUT_BASE_DIR!" "!G_BUILD_OUTPUT_BASE_DIR!" __Zip_FileList __Zip_FileCount
	) ELSE (
		CALL :ZipAddFileSet "!__Zip_TempDir!\!G_PACKAGE_NAME_FORMAT!\!G_BUILD_OUTPUT_BASE_DIR!" "!G_BUILD_OUTPUT_BASE_DIR!" __Zip_FileList __Zip_FileCount
	)
	GOTO :EOF
	
	:Zip_Src
	:Zip_Source
	CALL :CreateTempFile __Zip_FileList
	CALL :CreateTempFile __Zip_RegExpExcludeList
	!DCMD! -ff "!__Zip_RegExpExcludeList!" "@s\n@s\n" "!__Zip_BuildOutputDirEsc!" "!__Zip_PackageDirEsc!"
	
	CALL :SyncPathList F __Zip_FileList "!CD!" "/A-D-H-R-S /S" G_CLEAN_SRC_EXP "/I /V" __Zip_RegExpExcludeList "/I /V"
	IF "%~1" NEQ "" (
		CALL :ZipAddFileSet "!__Zip_TempDir!\%~1" "!CD!" __Zip_FileList __Zip_FileCount
	) ELSE (
		SET F_PACKAGE_NAME=!G_PACKAGE_SRC_NAME!
		CALL :LoadCfgFile "!G_MAKE_CFG_FILE!" PACKAGE_NAME_FORMAT
		CALL :ZipAddFileSet "!__Zip_TempDir!\!G_PACKAGE_NAME_FORMAT!" "!CD!" __Zip_FileList __Zip_FileCount
	)
	GOTO :EOF

	:Zip_Package
	IF NOT EXIST "!G_PACKAGE_TEMP_DIR!" (
		CALL :Print W "ZIP" "Package temp directory @s does not exist." "!G_PACKAGE_TEMP_DIR!"
		GOTO :EOF
	)
	CALL :ToAbsPath  G_PACKAGE_ABS_DIR "!G_PACKAGE_DIR!"
	CALL :ToAbsPath  G_PACKAGE_TEMP_ABS_DIR "!G_PACKAGE_TEMP_DIR!"

	CALL :CreateTempFile __Zip_FileList
	CALL :SyncPathList F __Zip_FileList "!G_PACKAGE_TEMP_ABS_DIR!" "/A-D-H-R-S /S" G_CLEAN_PACKAGE_TEMP_EXP "/I /V" @NULL ""
	

	IF "%~1" EQU "@NONE@" (
		CALL :ZipAddFileSet "!__Zip_TempDir!" "!G_PACKAGE_TEMP_DIR!" __Zip_FileList __Zip_FileCount
	) ELSE IF "%~1" NEQ "" (
		CALL :ZipAddFileSet "!__Zip_TempDir!\%~1" "!G_PACKAGE_TEMP_DIR!" __Zip_FileList __Zip_FileCount
	) ELSE (
		SET F_PACKAGE_NAME=!G_PACKAGE_BIN_NAME!
		CALL :LoadCfgFile "!G_MAKE_CFG_FILE!" PACKAGE_NAME_FORMAT
		CALL :ZipAddFileSet "!__Zip_TempDir!\!G_PACKAGE_NAME_FORMAT!" "!G_PACKAGE_TEMP_DIR!" __Zip_FileList __Zip_FileCount
	)
	GOTO :EOF
	
GOTO :EOF

REM - %1 = package directory. all files and dirs under this are zipped
REM - %2 = root source dir for this set
REM -      i.e. !CD! or output; can be relative or absolute; all files for a set must be contained in this.
REM - %3 = env var name of file containing the list of files to copy
REM - %4 = env var name of file counter 
:ZipAddFileSet
	SET __ZipAddFileSet_Files=!%~3!
	SET __ZipAddFileSet_Count=0
	SET __ZipAddFileSet_BaseDir=%~1
	SET __ZipAddFileSet_RootDir=%~2
	CALL :TrimExR "\" __ZipAddFileSet_BaseDir
	
	
	FOR /F "eol=; tokens=* delims=" %%A IN (!__ZipAddFileSet_Files!) DO (
		SET /A __ZipAddFileSet_Count=__ZipAddFileSet_Count+1
		PUSHD !CD!
		CD /D "!__ZipAddFileSet_RootDir!"
		CALL :ToRelPath __ZipAddFileSet_AbsDir "%%~dpA"
		SET __ZipAddFileSet_AbsDir=!__ZipAddFileSet_BaseDir!\!__ZipAddFileSet_AbsDir!
		IF NOT EXIST "!__ZipAddFileSet_AbsDir!" MKDIR "!__ZipAddFileSet_AbsDir!"
		POPD
		COPY /Y "%%~A" "!__ZipAddFileSet_AbsDir!%%~nxA" >NUL
		IF "!ERRORLEVEL!" NEQ "0" CALL :Print W "ZIP" "Failed copying file:\n\t@s to @s." "%%~A" "!__ZipAddFileSet_AbsDir!%%~nxA"
	)
	
	SET /A %~4=%~4+!__ZipAddFileSet_Count!
	CALL :Undefine __ZipAddFileSet_*
GOTO :EOF

:Build_StartFailed
	CALL :SetError 1
	CALL :Print E "BUILD" "Failed setting up the WDK environment."
GOTO :EOF
	
:Build
	CALL :KitSetup
	IF "!BUILD_ERRORLEVEL!" NEQ "0" GOTO Build_StartFailed
	IF NOT DEFINED DDKBUILDENV GOTO Build_StartFailed

	REM - G_BUILD_OUTPUT_BASE_DIR is either absolute or relative to make.cfg. this is the base path of where output files will be placed.
	IF NOT EXIST "!G_BUILD_OUTPUT_BASE_DIR!" MKDIR "!G_BUILD_OUTPUT_BASE_DIR!"
	IF NOT EXIST "!G_BUILD_OUTPUT_BASE_DIR!" (
		CALL :SetError 1
		CALL :Print E "BUILD" "Failed creating base output dir@s." " !G_BUILD_OUTPUT_BASE_DIR!"
		GOTO :EOF
	)	
	IF EXIST "build!BUILD_ALT_DIR!.err" DEL /Q "build!BUILD_ALT_DIR!.err"
	IF EXIST "build!BUILD_ALT_DIR!.wrn" DEL /Q "build!BUILD_ALT_DIR!.wrn"
	
	REM - TODO: OUTPUT_FILE_LIST is not used yet but it's the start of a crucial part
	REM - for auto generating parts of the installer script.
	SET BUILD_OUTPUT_FILE_LIST=""
	SET BUILD_OUTPUT_COUNT=0
	
	REM - NOTE: Shorthand ovveride for G_WDK_SOURCES_LIST.
	IF DEFINED G_SRCS SET G_WDK_SOURCES_LIST=!G_SRCS!
	
	REM - This is the file.ext.sources file processing loop.  
	REM - It builds the sources listed in WDK_SOURCES_LIST from begining to end [in order]
	FOR /F "usebackq eol=; tokens=* delims=" %%I IN (`!DCMD! "-sp=;" "!G_WDK_SOURCES_LIST!"`) DO (
		SET __SOURCES_FILENAME=%%I
		CALL :TrimL __SOURCES_FILENAME
		IF "!__SOURCES_FILENAME!" NEQ "" (

			SET __SOURCES_FILENAME=!__SOURCES_FILENAME!.sources
			SET __SOURCES_COUNT=0
			
			REM - Locate the next sources file with dir; make sure it's really there.  and get the 
			FOR /F "usebackq eol=; tokens=* delims=" %%J IN (`dir /A-D /S /B "!__SOURCES_FILENAME!"`) DO (
				SET __SOURCES_NAME=%%~nJ
				SET __SOURCES_FILE=%%~fJ
				SET __SOURCES_DIR=%%~dpJ
				SET /A "__SOURCES_COUNT=__SOURCES_COUNT+1"
			)

			REM - Do some validation; there can be only one.
			IF !__SOURCES_COUNT! NEQ 1 (
				IF !__SOURCES_COUNT! GTR 1 CALL :Print E "BUILD" "Multiple sources found for @s.\r\n\tAll 'filename.ext.sources' files must all have a unique name\x21" "!__SOURCES_FILENAME!"
				IF !__SOURCES_COUNT! LSS 1 CALL :Print E "BUILD" "Missing sources file @s.\r\n\tA sources files listed in the WDK_SOURCES_LIST make.cfg setting was not found\x21" "!__SOURCES_FILENAME!"
				CALL :SetError 1
				GOTO :EOF
			)
			TITLE Building !__SOURCES_NAME! - !_BUILDARCH! - !DDK_TARGET_OS!..
			CALL :Print "BUILD" "@s \k03(\k0B@s\k03) - \k0B@s" "!__SOURCES_NAME!" "!_BUILDARCH!" "!DDK_TARGET_OS!"
			CALL :Build_LoadTargetEnv "!__SOURCES_FILE!" "!G_BUILD_OUTPUT_BASE_DIR!"
	
			REM - version tagging. this should go in pre tasks when/if there is one.
			CALL :PathSplit "!G_MAKE_CFG_FILE!"
			SET __BUILD_VERSION_FILE=!_DIR_!!_FILE_!.versions
			SET VERSION_FILENAME=
			IF EXIST "!__BUILD_VERSION_FILE!" (
				SET VERSION_FILENAME="!__SOURCES_NAME!"
				CALL :Print D "BUILD" "VERSION_FILE=@s TARGET_NAME=@s" "!__BUILD_VERSION_FILE!" !VERSION_FILENAME!
				!DCMD! -vc=tag "!__BUILD_VERSION_FILE!" !VERSION_FILENAME!
			)
	
			REM - build the sources file.
			REM - If an error occured in Build_SourcesFile, BUILD_ERRORLEVEL will be set
			CALL :Build_SourcesFile "!__SOURCES_FILE!"
			IF !BUILD_ERRORLEVEL! NEQ 0 GOTO Build_Stop
			
			REM - Check for errors, report warnings, sign target
			CALL :Build_PostTasks "!TARGET_FINAL_OUTPUT_DIR!\!__SOURCES_NAME!"
			IF !BUILD_ERRORLEVEL! NEQ 0 GOTO Build_Stop

			SET /A BUILD_OUTPUT_COUNT=BUILD_OUTPUT_COUNT+1
		)
	)
	
	REM - if nothing was built try and use a regular sources file that doesn't need renamed.
	IF "!BUILD_OUTPUT_COUNT!" EQU "0" CALL :SafeCall build !G_WDK_BUILD_OPTIONS!
	
	:Build_Stop
	SET >build_env_vars.lst
	SET __SOURCES_NAME=
	SET __SOURCES_FILE=
	SET __SOURCES_DIR=
	SET __TARGET_PATH=

	IF NOT EXIST "build!BUILD_ALT_DIR!.err" (
		IF !BUILD_ERRORLEVEL! EQU 0 (
			IF "!BUILD_OUTPUT_COUNT!" GTR "0" (
				CALL :Print S "BUILD" "\k02(\k0A!BUILD_ALT_DIR:_= !\k02)\k0A Build successful\x21 @d target(s) built.\n" "!BUILD_OUTPUT_COUNT!"
				GOTO :EOF
			)
		)
	)
	CALL :Print E "BUILD" "\k04(\k0C!BUILD_ALT_DIR:_= !\k04)\k0C Build failed\x21."
	IF EXIST "build!BUILD_ALT_DIR!.err" (
		ECHO.
		TYPE "build!BUILD_ALT_DIR!.err"
		ECHO.
	)
	IF EXIST "build!BUILD_ALT_DIR!.log" (
		SET AskYesNo_Default=0
		CALL :AskYesNo "\k0FDisplay entire log?"
		IF !AskYesNo! EQU 1 TYPE "build!BUILD_ALT_DIR!.log"
	)
	CALL :SetError 1
GOTO :EOF

REM :: Sets global, dynamic target env vars.  These are updated before each target is built.
REM - %1 = sources file name to be built.  Must be abs.
REM - %2 = Relative or absolute base output dir
:Build_LoadTargetEnv
	CALL :ToAbsPath  G_BUILD_OUTPUT_BASE_ABS_DIR "%~2"

	REM - The sources file must use $(G_TARGET_OUTPUT_NAME) for the target name.
	REM - To change the target name, change the name of the 'file.ext.sources' file.
	SET G_TARGET_OUTPUT_FILENAME=%~n1
	SET G_TARGET_OUTPUT_FRIENDLYNAME=!G_TARGET_OUTPUT_FILENAME:.=_!
	SET G_TARGET_OUTPUT_NAME=!G_TARGET_OUTPUT_FILENAME:~0,-4!
	SET G_TARGET_OUTPUT_FILENAME_EXT=!G_TARGET_OUTPUT_FILENAME:~-3!
	
	REM - This one will always point to the CORRECT ddk 'final' output subdir.
	REM - i386/x86 is a special feature exclusive to microsoft.
	SET G_TARGET_OUTPUT_ARCH=!_BUILDARCH!
	IF /I "!_BUILDARCH!" EQU "x86" SET G_TARGET_OUTPUT_ARCH=i386
	
	REM - TARGET_OUTPUT_ABS_DIR is what TARGETPATH should be set to in the sources file.
	CALL :LoadCfgFile "!G_MAKE_CFG_FILE!" TARGET_OUTPUT_ABS_DIR
	
	REM - TARGET_FINAL_OUTPUT_DIR is where the files will end up after WDK has added the arch.
	REM - This is sortof a hidden global.  It is special because it can't really be changed.
	SET TARGET_FINAL_OUTPUT_DIR=!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!
		
GOTO :EOF

REM :: END COMMAND SECTION ---------------------------------------------
REM :: START BUILD SUB FUNCTIONS ---------------------------------------

REM :: Called by the Build_PostTasks function to sign target dll/sys/exe files when 'sign' is used on the command line.
REM - %1 = filepath to sign
:Build_Sign
	IF DEFINED G_SIGN (
		REM - sign only dll, sys and exe files.
		SET __SIGN_IS_SIGNABLE=
		IF /I "%~x1" EQU ".sys" SET __SIGN_IS_SIGNABLE=1
		IF /I "%~x1" EQU ".dll" SET __SIGN_IS_SIGNABLE=1
		IF /I "%~x1" EQU ".exe" SET __SIGN_IS_SIGNABLE=1
		IF NOT DEFINED __SIGN_IS_SIGNABLE (
			CALL :Print "SIGN" "Skipping @s file.." "%~x1"
			GOTO :EOF
		)
		PUSHD !CD!
		CD /D %~dp1
		CALL :SignFile "%~nx1"
		POPD
	)
GOTO :EOF

:Build_SetupWdfEnv
SET KMDFVERSION_MAJOR=1
SET KMDFVERSION_MINOR=10
:Build_SetupWdfEnv_NextVersion
SET /A "KMDFVERSION_MINOR=KMDFVERSION_MINOR-1"
IF "!KMDFVERSION_MINOR!" EQU "0" GOTO :EOF

SET KMDFCOINSTALLERVERSION=0!KMDFVERSION_MAJOR!00!KMDFVERSION_MINOR!
SET KMDFVERSION=!KMDFVERSION_MAJOR!.!KMDFVERSION_MINOR!
IF EXIST "!G_WDK_DIR!\redist\wdf\!_BUILDARCH!\WdfCoInstaller!KMDFCOINSTALLERVERSION!.dll" GOTO :EOF
GOTO Build_SetupWdfEnv_NextVersion

GOTO :EOF

REM :: Copies build err, log, and wrn files to the current directory
REM - %1 = Absolute filepath for a 'sources' file
REM - [USES] G_WDK_BUILD_OPTIONS, BUILD_ALT_DIR
:Build_SourcesFile
	IF NOT EXIST "%~1" (
		CALL :Print E "BUILD" "Sources file not found '@s'" "%~1"
		CALL :SetError 1
		GOTO :EOF
	)
	
	SET __SRCS_DIR=%~dp1
	CALL :TrimExR "\" __SRCS_DIR
	FOR /F "usebackq eol=; tokens=* delims=" %%A IN (`!DCMD! -rp d "!__SRCS_DIR!\\" d "!G_BUILD_OUTPUT_BASE_ABS_DIR!"`) DO IF "%%~A" NEQ "" SET G_TARGET_OUTPUT_REL_DIR=%%A

	REM - copy the name.ext.sources file to sources
	IF EXIST "%~dp1\sources" DEL /Q "%~dp1\sources"
	!DCMD! -ff "%~dp1\sources" "# ++ @s - @s\n" "AUTO-GENERATED" "%~nx1"
	!DCMD! -ff "%~dp1\sources" "@s@s\n@s@s\n" "TARGET_OUTPUT_FILENAME_EXT=" "!G_TARGET_OUTPUT_FILENAME_EXT!" "TARGET_OUTPUT_BASE_DIR=" "!G_TARGET_OUTPUT_REL_DIR!"
	CALL :Tokenizer "%~1" "%~dp1\sources" "G_TARGET_OUTPUT"

	REM - Clear the archive bit for everything in the output folder
	ATTRIB -A /S "!G_BUILD_OUTPUT_BASE_ABS_DIR!\*" 2>NUL>NUL

	Call :PathSplit "!G_MAKE_CFG_FILE!"
	SET __Version_File=!_DIR_!!_FILE_!.versions
	CALL :VersionLookup "PACKAGE" "!_DIR_!!_FILE_!.versions"
	
	REM - Call any user pre-build tasks here.
	CALL :ExecuteUserTask pre build !G_TARGET_OUTPUT_NAME!
	CALL :ExecuteUserTask pre build !G_TARGET_OUTPUT_FRIENDLYNAME!
	
	REM - Switch to the dir the sources file is in and build; this way we do not need dirs files.
	PUSHD !CD!
	CD /D %~dp1
	SET TOKVAR_LTAG=#
	SET TOKVAR_RTAG=#
	CALL :Build_SetupWdfEnv
	CALL :SafeCall build !G_WDK_BUILD_OPTIONS!
	SET TOKVAR_LTAG=
	SET TOKVAR_RTAG=
	POPD
	
	REM - Track the last file, for each type
	SET WDK_BUILD_LAST_!G_TARGET_OUTPUT_FILENAME_EXT!_TARGETFILE=!TARGET_FINAL_OUTPUT_DIR!\%~n1

	REM - Keep the current set of logs in the current directory.
	IF EXIST "%~dp1\build!BUILD_ALT_DIR!.err" COPY /Y "%~dp1\build!BUILD_ALT_DIR!.err" ".\" >NUL
	IF EXIST "%~dp1\build!BUILD_ALT_DIR!.wrn" COPY /Y "%~dp1\build!BUILD_ALT_DIR!.wrn" ".\" >NUL
	IF EXIST "%~dp1\build!BUILD_ALT_DIR!.log" TYPE "%~dp1\build!BUILD_ALT_DIR!.log" >>build!BUILD_ALT_DIR!.log
	
GOTO :EOF

REM :: Checks for build errors and executes user tasks; signs binaries;
REM - %1 = filepath of built target; this is also passed to Build_Sign
REM - [USES] BUILD_ALT_DIR
REM - [USES] G_TARGET_OUTPUT_FRIENDLYNAME
REM - [USES] G_TARGET_OUTPUT_NAME
:Build_PostTasks

	CALL :ExecuteUserTask post build !G_TARGET_OUTPUT_NAME!
	
	REM - If an error occurs we are all done
	IF EXIST "build!BUILD_ALT_DIR!.err" (
		CALL :SetError 1
		GOTO :EOF
	)

	REM - If an warning occurs; display it
	IF EXIST "build!BUILD_ALT_DIR!.wrn" (
		TYPE "build!BUILD_ALT_DIR!.wrn"
		ECHO.
	)
	IF NOT DEFINED G_WDK_NO_MAKEDIRS MAKEDIRS "!CD!">NUL
	
	REM - This is a good place to sign since we know where the files are at.
	CALL :Build_Sign "%~1"

	REM - Call any user pre-build tasks here.
	CALL :ExecuteUserTask post build !G_TARGET_OUTPUT_FRIENDLYNAME!

GOTO :EOF

:KitSetupFound_Callback
	SET /A __KitSetup_Count=%~5+1
	SET __KitSetup_Found=%~3
	CALL :TrimExR "\" __KitSetup_Found
	REM - setenv.bat must exist in a bin subdir of the root WDK install folder.
	IF EXIST "!__KitSetup_Found!\bin\setenv.bat" (
		ECHO !__KitSetup_Count!;!__KitSetup_Found!>>"!%~1!"
		CALL :Print "WDK #!__KitSetup_Count!" "@s" "!__KitSetup_Found!"
	)
GOTO :EOF

:KitKill
	REM - WDK has destroyed these values so we need a fresh copy
	CALL :RegEnum ":RegGetValue_CallBack" __KitKill_SystemPath "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
	CALL :RegEnum ":RegGetValue_CallBack" __KitKill_UserPath   "HKCU\Environment"                                                  "Path"
	CALL :RegEnum ":RegGetValue_CallBack" __KitKill_UserName   "HKCU\Volatile Environment"                                         "UserName"
	IF NOT DEFINED __KitKill_SystemPath GOTO KitKill_Error
	IF NOT DEFINED __KitKill_UserPath   GOTO KitKill_Error
	IF NOT DEFINED __KitKill_UserName   GOTO KitKill_Error
	
	PUSHD !CD!
	CD /D !PUBLIC_ROOT!\bin
	REM - Find everything that setenv.bat can possibly 'set'
	CALL :OutputToFile T __KitKill_SetEnvList @NULL FINDSTR /I /R /B /C:"[ ]*set[ ]" setenv.bat
	POPD
	IF NOT DEFINED __KitKill_SetEnvList_Error (
		REM - Keep spamming the user with this until they stop doing it.
		SET __KitKill_Text=
		SET __KitKill_Text=!__KitKill_Text! "WDK Environment is already configured."
		SET __KitKill_Text=!__KitKill_Text! "Run make.cmd from a normal command prompt to avoid this delay."
		SET __KitKill_Text=!__KitKill_Text! "De-configuring WDK environment.  Please Wait.."
		CALL :Print SW "KILENV" "\k0E@s\n\t\k0F@s\n\t@s\n" !__KitKill_Text!
		
		REM - Find all the 'set var-name' lines in setenv.bat that match varnames in the environment and undefine them.
		
		REM - Don't care what the values are, only the value names
		CALL :CreateTempFile __KitKill_AllList
		FOR /F "eol=; tokens=1,2 delims== " %%A IN (!__KitKill_SetEnvList!) DO ECHO %%B=>>"!__KitKill_AllList!"
		
		REM - Dump the current env
		CALL :CreateTempFile __KitKill__ActiveList
		SET >!__KitKill__ActiveList!
		
		REM - Find all the values names setenv has added/changed
		CALL :OutputToFile T __KitKill_MangledList @NULL FINDSTR /I /B /G:"!__KitKill_AllList!" !__KitKill__ActiveList!
		
		REM - Do the killin; but not PATH or USERNAME.
		FOR /F "eol=; tokens=1 delims==" %%A IN (!__KitKill_MangledList!) DO IF /I "%%~A" NEQ "PATH" IF /I "%%~A" NEQ "USERNAME" SET %%~A=
		
		REM - Repair PATH and USERNAME
		CALL :TrimEx ";" __KitKill_SystemPath __KitKill_UserPath
		SET UserName=!__KitKill_UserName!
		SET PATH=!G_DOSCMD_DIR!;!__KitKill_SystemPath!;!__KitKill_UserPath!;
		TITLE !COMSPEC!

		CALL :Undefine __KitKill_*
		GOTO :EOF
	)
		
	:KitKill_Error
	CALL :SetError 1
	
GOTO :EOF

:KitGetDefaultBuildOptions
	SET __KitSetup_OS=
	SET __KitSetup_Arch=

	IF /I "!_BUILDARCH!" EQU "amd64" SET __KitSetup_Arch=x64
	IF /I "!_BUILDARCH!" EQU "ia64" SET __KitSetup_Arch=64
	IF /I "!DDK_TARGET_OS!" EQU "Win2K" SET __KitSetup_OS=W2K
	IF /I "!DDK_TARGET_OS!" EQU "WinXP" SET __KitSetup_OS=WXP
	IF /I "!DDK_TARGET_OS!" EQU "Win7" SET __KitSetup_OS=WIN7
	IF NOT DEFINED __KitSetup_OS SET __KitSetup_OS=!DDK_TARGET_OS!
	IF NOT DEFINED __KitSetup_Arch IF /I "!__KitSetup_OS!" NEQ "W2K" IF /I "!__KitSetup_OS!" NEQ "WXP" SET __KitSetup_Arch=x86
	SET G_WDK_DEF_ENV_OPTIONS=
	CALL :AddIfDef G_WDK_DEF_ENV_OPTIONS DDKBUILDENV __KitSetup_Arch __KitSetup_OS
	SET WDK_ENV_OPTIONS=!G_WDK_DEF_ENV_OPTIONS!
GOTO :EOF

REM :: Called by the build function to configure the WDK environment; searches registry for WDK installations.
:KitSetup
	IF DEFINED DDKBUILDENV (
		IF EXIST "!PUBLIC_ROOT!\bin\setenv.bat" (
				CALL :KitGetDefaultBuildOptions
				SET G_WDK_DIR=!PUBLIC_ROOT!
				!DCMD! -uc=WDK_DIR "!G_MAKE_CFG_FILE!" "!G_WDK_DIR!"
				!DCMD! -uc=WDK_DEF_ENV_OPTIONS "!G_MAKE_CFG_FILE!" "!G_WDK_DEF_ENV_OPTIONS!"
			CALL :KitKill
			IF "!BUILD_ERRORLEVEL!" EQU "0" GOTO KitSetup_Start
		)
		CALL :SetError 1
		GOTO :EOF
	)
	
	:KitSetup_Start
	IF EXIST "!G_WDK_DIR!\bin\setenv.bat" GOTO KitSetup_Continue
	
	REM - The value  we are looking for is in the Wow6432Node software key so below will
	REM - scan for the value name we want starting in HKLM\SOFTWARE and do post validation
	REM - to ensure that it's really a WDK install dir.
	CALL :Print "SETENV" "Searching for WDK installations, please wait.."
	CALL :CreateTempFile __KitSetup_List
	CALL :RegEnum ":KitSetupFound_Callback" __KitSetup_List "HKLM\SOFTWARE" "setup-install-location" REG_SZ
	IF EXIST "!__KitSetup_List!" (
		!DCMD! -ff CON "\k0BSelect the DDK to use as the default"
		CALL :GetC __KitSetup_Selected "1" "-" "!__KitSetup_Count!" "1"
	) ELSE IF "!__KitSetup_Count!" EQU "1" (
		SET __KitSetup_Selected=1
	) ELSE (
		SET __KitSetup_Selected=
	)
	IF DEFINED __KitSetup_Selected (
		CALL :OutputToFile T __KitSetup_SelectedList @NULL FINDSTR /R /B /C:"!__KitSetup_Selected!;" !__KitSetup_List!
		IF NOT DEFINED __KitSetup_SelectedList_Error (
			SET G_WDK_DIR=
			FOR /F "eol=; tokens=1,* delims=;" %%A IN (!__KitSetup_SelectedList!) DO IF NOT DEFINED G_WDK_DIR SET G_WDK_DIR=%%~B
			IF EXIST "!G_WDK_DIR!" (
				!DCMD! -uc=WDK_DIR "!G_MAKE_CFG_FILE!" "!G_WDK_DIR!"
				IF "%ERRORLEVEL%" EQU "0" (
					CALL :Print S "SETENV" "Updated default WDK dir in @s." "!G_MAKE_CFG_FILE!"
					CALL :Undefine __KitSetup_*
					GOTO KitSetup_Continue
				) ELSE (
					CALL :Print E "SETENV" "Failed updating @s" "!G_MAKE_CFG_FILE!"
				)
			)
		)
	)
	:KitSetup_Error
	CALL :Undefine __KitSetup_*
	CALL :SetError 1
	GOTO :EOF

	:KitSetup_Continue
	PUSHD !CD!
	CALL :Print "SETENV" "Configuring WDK build environment. \k03(\k0B@s\k03)" "!WDK_ENV_OPTIONS!"
	CD /D "!G_WDK_DIR!\bin"
	CALL :SafeCall setenv.bat !G_WDK_DIR! !WDK_ENV_OPTIONS!
	POPD
GOTO :EOF

REM :: END OF BUILD SUB FUNCTIONS --------------------------------------
REM :: START OF UTILITY FUNCTIONS --------------------------------------

REM :: Called when a build error or other unrecoverable error occurs; notifies the parent make.cmd there was problems.
:SetError
	SET BUILD_ERRORLEVEL=%1
	IF DEFINED E_SND_FILE (
		IF EXIST "!E_SND_FILE!" (
			ECHO %~1 >!E_SND_FILE!
		)
	)
)
GOTO :EOF

:DefineDefaultDirEscapes
	SET %1BuildOutputDir=/@NULL/
	SET %1PackageDir=/@NULL/
	IF EXIST "!G_BUILD_OUTPUT_BASE_DIR!" CALL :ToAbsPath %1BuildOutputDir "!G_BUILD_OUTPUT_BASE_DIR!"
	IF EXIST "!G_PACKAGE_DIR!" CALL :ToAbsPath %1PackageDir "!G_PACKAGE_DIR!"
	CALL :ToRelPath %1BuildOutputDirRel "!%1BuildOutputDir!"
	CALL :ToRelPath %1PackageDirRel "!%1PackageDir!"
	CALL :RegexEscape %1BuildOutputDirEsc "!%1BuildOutputDirRel!"
	CALL :RegexEscape %1PackageDirEsc "!%1PackageDirRel!"
GOTO :EOF

REM :: Looks for and executes user 'bat' files which are excuted vefore and after every major tasks.
REM - %1 = pre/post
REM - %2 = category. IE. build / dist /etc.
REM - %3 = friendly name
:ExecuteUserTask
	SET __BAT_FILE=.\make_tasks\%~1_%~2_%~3.bat
	SET __CFG_FILE=.\make_tasks\%~1_%~2_%~3.cfg
	IF "%~3" EQU "" (
		SET __BAT_FILE=.\make_tasks\%~1_%~2.bat
		SET __CFG_FILE=.\make_tasks\%~1_%~2.cfg
	)
	CALL :Print D "USRCFG" "Looking for user config @s.." "!__CFG_FILE!"
	CALL :Print D "USRBAT" "Looking for user task @s.." "!__BAT_FILE!"
	IF EXIST "!__CFG_FILE!" CALL :LoadCfgFileEx "set-prefix=K_" "!__CFG_FILE!"
	IF EXIST "!__BAT_FILE!" CALL "!__BAT_FILE!" %*
	SET __BAT_FILE=
	SET __CFG_FILE=
GOTO :EOF

REM :: Signs a binary using the cert options defined in make.cfg or passed as arguments.
REM - %1 = Full path of file to sign.
REM [USES] G_WDK_DIR, G_SIGN_CERT_FILE, G_SIGN_CERT_NAME, G_SIGN_CERT_OPTIONS
:SignFile
	CALL :FindFileInPath SIGN_EXE SignTool.exe
	IF NOT EXIST "!SIGN_EXE!" SET SIGN_EXE=!G_WDK_DIR!\bin\x86\SignTool.exe
	IF NOT EXIST "!SIGN_EXE!" FOR /F "usebackq eol=; tokens=* delims=" %%A IN (`dir /S /A-D /B "!G_WDK_DIR!\SignTool.exe"`) DO IF NOT EXIST "!SIGN_EXE!" SET SIGN_EXE=%%~A
	IF NOT EXIST "!SIGN_EXE!" (
		CALL :Print E "SIGN" "@s not found\x21 WDK installation at @s is either mangled or missing." "SignTool.exe" "!G_WDK_DIR!"
	) ELSE (
		PUSHD !CD!
		CD /D %~dp1
		!SIGN_EXE! !G_SIGN_CERT_ARGS! %~nx1
		IF DEFINED G_SIGN_CERT_VERIFY_ARGS !SIGN_EXE! !G_SIGN_CERT_VERIFY_ARGS! %~nx1
		POPD
	)
	SET SIGN_EXE=
GOTO :EOF

REM :: Executes a command line using CMD /C and returns an error code.
REM    After make.cmd has been wound, it can call SetError to report status
REM    back to the parent make.cmd if a critical action fails.
:SafeCMD
	SET E_RET_FILE=!TEMP!\E_RET_FILE_!RANDOM!.mrk
	IF EXIST "!E_RET_FILE!" DEL /Q "!E_RET_FILE!"
	ECHO 0 >!E_RET_FILE!
	CALL CMD /C %* E_RET_FILE="!E_RET_FILE!"
	FOR /F "eol=; tokens=1 delims= " %%I IN (!E_RET_FILE!) DO (
		SET BUILD_ERRORLEVEL=%%I
		GOTO SafeCMD_Done
	)
	
	:SafeCMD_Done
	IF EXIST "!E_RET_FILE!" DEL /Q "!E_RET_FILE!"

GOTO :EOF

REM :: Calls a function safely when running quiet; output is sent to a temp file; of any problems occur the output can be shown to the user.
REM    Checks ERRORLEVEL; calls 'SetError' and displays error with commandline if a problem occurs;
REM [SETS] SafeCall_Command to the last command executed
REM [SETS] SafeCall_OutputLog to the name of the tmp file stdout was stored to
REM [SETS] SafeCall_OutputErr to the name of the tmp file stderr was stored to
:SafeCall
	SET SafeCall_Command=%*
	SET SafeCall_Command=!SafeCall_Command:"=`!
	IF "!G_BUILD_QUIET!" EQU "1" (
		CALL :CreateTempFile __SafeCallOutputLog log
		CALL :CreateTempFile __SafeCallOutputErr err
		CALL %* 2>"!__SafeCallOutputErr!">"!__SafeCallOutputLog!"
	) ELSE (
		CALL %*
	)
	IF NOT "!ERRORLEVEL!" EQU "0" (
		CALL :SetError 1
		CALL :Print E "ERROR" "Failed executing command:@s" " %~1 "
	)
GOTO :EOF

REM :: Removes a list of sub directories or files recursively
REM - %1 = Remove type. can be 'dir' or 'file'
REM - %2 = env var name of the file list to remove; %1 indicates whether its a list of files or dirs
:RemoveList
	SET __RemoveList_FailCount=0
	SET __RemoveList_Count=0
	SET __RemoveList_Type=%~1
	SET __RemoveListFile=!%~2!

	IF /I "!__RemoveList_Type!" EQU "file" SET __RemoveList_DirCmd=/S /A-D
	IF /I "!__RemoveList_Type!" EQU "dir" SET __RemoveList_DirCmd=/S /AD
	FOR /F "eol=; tokens=* delims=" %%I IN (!__RemoveListFile!) DO (
		IF EXIST "%%~I" (
			IF /I "!__RemoveList_Type!" EQU "file" (
				ATTRIB -R -S -H "%%~I" 2>NUL>NUL
				DEL /S /Q %%I 2>NUL>NUL
			)
			IF /I "!__RemoveList_Type!" EQU "dir" RMDIR /S /Q %%I 2>NUL>NUL
			SET __REL_PATH=%%~I
			CALL :TrimExR "\" __REL_PATH
			IF EXIST "!__REL_PATH!"	(
				SET /A "__RemoveList_FailCount=__RemoveList_FailCount+1"
				CALL :Print E "CLEAN" "\k0EUnable to remove !__RemoveList_Type! @s." "!__REL_PATH!" 
			) ELSE (
				SET /A "__RemoveList_Count=__RemoveList_Count+1"
				CALL :Print S "CLEAN" "Removed !__RemoveList_Type! @s" "!__REL_PATH!" 
				SET __REL_PATH=
			)
		) ELSE (
			CALL :Print D "CLEAN" "@s @s no longer exists." "!__RemoveList_Type!" "%%~I" 
		)
	)
	:RemoveList_Done
	IF "!__RemoveList_FailCount!" EQU "0" CALL :Print S "CLEAN" "\k0AAll !__RemoveList_Type!s clean."
	CALL :Undefine __RemoveList*
GOTO :EOF

REM :: Creates a bare '/B' directory list and runs include/exlude regexps against it to produce final output.
REM - %1 = F or D [files or directories];
REM - %2 = env var name of file which will contain the final output;
REM        Must already be defined; ie CreateTempFile, OutputToFile
REM - %3 = root directory.  This will be passed to the initial 'DIR' command. i.e. DIR %1\*
REM        It will be stripped from the filelist and replaced with a '.' char
REM - %4 = additional arguments for DIR. '/S' for recursive, '/AD' dirs only '/A-D' files only
REM - %5 = clean exp var name
REM - %6 = findstr options for main clean expr [in %3] '/I' to ignore case.
REM - %7 = final [can be include or exclude] exp file list var name [@NULL to skip]
REM	- %8 = final exp findstr options [/v for exclude]
:SyncPathList
	REM - write the clean expressions to a temp file; one line per.
	SET __Sync_Path_OutFile=!%2!
	SET __Sync_Path_Type=%~1
	CALL :CreateTempFile __Sync_Path_RelList
	CALL :OutputToFile T __Sync_Path_AbsList @NULL DIR %~4 /B "%~3\*"
	FOR /F "eol=; tokens=* delims=" %%A IN (!__Sync_Path_AbsList!) DO (
		CALL :ToRelPath __Sync_Path_RelPath "%%~A"
		IF /I "!__Sync_Path_Type!" EQU "D" ECHO !__Sync_Path_RelPath!\>>"!__Sync_Path_RelList!"
		IF /I "!__Sync_Path_Type!" EQU "F" ECHO !__Sync_Path_RelPath!>>"!__Sync_Path_RelList!"
	)

	CALL :OutputToFile T __Sync_Path_RegExpList @NULL !DCMD! --get-commandline=start=0;split=0x0A;strip_quotes; !%5!
	ECHO.>>!__Sync_Path_RegExpList!
	CALL :SyncList %2 __Sync_Path_RegExpList "/I %~6" "%~7" "/I %~8" __Sync_Path_RelList
	CALL :Undefine __Sync_Path_*
GOTO :EOF


REM :: Runs include/exlude regexps against command output output to produce final output.
REM - %1 = Env var name of a file that will be appended with the final output
REM - %2 = Env var name of a file containing the list of initial [main] findstr regexps [one per line]
REM - %3 = findstr options for main regexps [in %1] '/I' to ignore case.
REM - %4 = Env var name of a file containing the list of secondary findstr regexps [one per line] [@NULL to skip]
REM - %5 = findstr options for secondary regexps [in %3] '/I' to ignore case.
REM - %6 = Env var name of file text that will be synced
:SyncList
	IF "%~4" EQU "@NULL" (
		CALL :OutputToFile A %~1 @NULL FINDSTR %~3 /R /G:"!%~2!" !%~6!
	) ELSE (
		CALL :OutputToFile T __Sync_List_1 @NULL FINDSTR %~3 /R /G:"!%~2!" !%~6!
		REM - this is the 'finalizer' exp which includes/excludes lines from the already filtered list
		CALL :OutputToFile A %~1 @NULL FINDSTR %~5 /R /G:"!%~4!" !__Sync_List_1!
	)
	CALL :Undefine __Sync_List_1
GOTO :EOF

REM :: Finds a file in the PATH env var
REM - %1 = Env pointer set to the full pathname of the file that was found.
REM - %2 = Filename to look for
REM [SETS] DIR_%1 to the full directory the file was found in.
:FindFileInPath
	IF "%~1" EQU "" GOTO :EOF
	SET %1=%~f$PATH:2
	SET DIR_%1=%~dp$PATH:2
	SHIFT /1
	SHIFT /1
	GOTO FindFileInPath
GOTO :EOF

REM :: Enumerates a registry key/sub keys for a specific value name
REM - %1 = Callback function name [Called for each match]
REM - %2 = Env var name of User context [lol]
REM - %3 = Registry key to enumerate
REM - %4 = value name to look for
REM - %5 = [OP] value type to look for
:RegEnum
	SET __RegEnum_ValueName=%~4
	SET __RegEnum_Callback=%~1
	SET __RegEnum_Context=%~2
	
	IF "%~5" NEQ "" SET __RegEnum_Type=/t %~5

	CALL :OutputToFile T __RegEnum_Out @NULL reg query "%~3" /s /v "%~4" !__RegEnum_Type!
	IF NOT DEFINED __RegEnum_Out_Error (
		SET __RegEnum_Count=0
		FOR /F "eol=H tokens=1,2* delims= " %%A IN (!__RegEnum_Out!) DO (
			SET __RegEnum_CheckValueName=%%~A
			CALL :Trim __RegEnum_CheckValueName
			IF /I "!__RegEnum_CheckValueName!" EQU "!__RegEnum_ValueName!" (
				SET __RegEnum_Type=%%~B
				SET __RegEnum_Value=%%~C
				CALL :Trim __RegEnum_Value __RegEnum_Type
				CALL !__RegEnum_Callback! !__RegEnum_Context! "!__RegEnum_ValueName!" "!__RegEnum_Value!" !__RegEnum_Type! !__RegEnum_Count!
				SET /A __RegEnum_Count=__RegEnum_Count+1
				IF DEFINED !__RegEnum_Callback!_Stop (
					SET !__RegEnum_Callback!_Stop=
					GOTO RegEnum_Done
				)
			)
		)
	) ELSE (
		CALL :Print E "REGENUM" "Unable to find registry value @s in key:\n\t@s." "%~4" "%~3"
	)
	IF "!__RegEnum_Count!" EQU "0" CALL :!__RegEnum_Callback! !__RegEnum_Context! "!__RegEnum_ValueName!"
	
	:RegEnum_Done
	CALL :Undefine __RegEnum_*
GOTO :EOF

:RegGetValue_CallBack
	SET %~1=%~3
GOTO :EOF

REM :: Gets file info.
REM - %1 = full pathname of a file
REM - [SETS] _DRIVE_ to drive
REM - [SETS] _DIR_ drive and diectory
REM - [SETS] _FILE_ filename only [no ext]
REM - [SETS] _FILENAME_ filename only [with ext]
REM - [SETS] _FILEEXT_ file ext only
REM - [SETS] _FILESIZE_ size of the file
:PathSplit
	SET _DRIVE_=%~d1
	SET _DIR_=%~dp1
	SET _FILE_=%~n1
	SET _FILENAME_=%~nx1
	SET _FILEEXT_=%~x1
	SET _FILESIZE_=%~z1
GOTO :EOF

REM :: Generates a new temporary filename that will reside in the 'TEMP' directory;
REM - %1 = Env var name to store filename in; also used to generate the temp filename/
REM - %2 = [OP] The extension added to the filename. i.e 'bat' or 'txt'.  'tmp' is default. 
:CreateTempFile
	IF NOT DEFINED CreateTempFileList SET CreateTempFileList=!TEMP!\__TempFiles_!RANDOM!.lst
	SET __TempFileExt=tmp
	IF "%~2" NEQ "" SET __TempFileExt=%~2
	SET %1=!TEMP!\%1_!RANDOM!.!__TempFileExt!
	IF EXIST "!%1!" DEL /Q "!%1!" 2>NUL>NUL
	SET __TempFileExt=
	ECHO !%1!>>!CreateTempFileList!
GOTO :EOF

REM :: Destroys all temp files that where created with 'CreateTempFile' for the current instance of make.cmd.
:DestroyAllTempFiles
	IF NOT DEFINED CreateTempFileList GOTO DestroyAllTempFilesDone
	IF NOT EXIST !CreateTempFileList! GOTO DestroyAllTempFilesDone
	FOR /F "eol=; tokens=* delims=" %%T IN (!CreateTempFileList!) DO (
		CALL :Print D "TMPFILE" "[@d] Destroying TMP file @s.." "!MAKE_RUN_COUNT!" "%%~nxT"
		IF EXIST "%%~T" DEL /Q "%%~T"
	)
	IF EXIST !CreateTempFileList! DEL /Q !CreateTempFileList!
:DestroyAllTempFilesDone
	SET CreateTempFileList=
GOTO :EOF

REM :: Tags a file with env vars
REM %1 in file.
REM %2 out file.
REM %3 [OP] 'use only' prefix; only tokenize env vars beginning with this string
:Tokenizer
	IF "%~3" EQU "" !DCMD! "--env-tag" "%~1" "%~2"
	IF "%~3" NEQ "" !DCMD! "--env-tag=%~3" "%~1" "%~2"
	IF "%ERRORLEVEL%" NEQ "0" (
		CALL :Print E "TOKEN" "Failed tokenizing file @s to @s."  "%~1" "%~2"
		CALL :SetError 1
	)
GOTO :EOF

REM :: Redirects stderr and stdout to temp files for the given commandline
REM - %1 = 'T' new tempfile
REM        'W' overwrite and re-use existing
REM        'A' append
REM - %2 = env var name to store the temp file name for stdout output
REM - %3 = env var name to store the temp file name for stderr output [@NULL if none]
REM - %4 and up = The commandline to execute and retrieve output from
REM [SETS] %1_Error=1 if the commad failed, undefines %1_Error if successful
:OutputToFile
	SET %2_Error=
	SET __STDOUT_FILE=NUL
	SET __STDERR_FILE=NUL
	
	CALL :CreateTempFile __OutputToFile bat
	CALL !DCMD! --get-commandline=start=3 %*>!__OutputToFile!
	IF "%~2" NEQ "@NULL" (
		IF /I "%~1" EQU "T" (
			CALL :CreateTempFile %2
			SET __STDOUT_FILE=!%~2!
		) ELSE (
			SET __STDOUT_FILE=!%~2!
		)
	)
	
	IF "%~3" NEQ "@NULL" (
		IF /I "%~1" EQU "T" (
			CALL :CreateTempFile %3
			SET __STDERR_FILE=!%~3!
		) ELSE (
			SET __STDERR_FILE=!%~3!
		)
	)
	IF /I "%~1" EQU "A" (
		CALL !__OutputToFile! 2>>!__STDERR_FILE!>>!__STDOUT_FILE!
	) ELSE (
		CALL !__OutputToFile! 2>!__STDERR_FILE!>!__STDOUT_FILE!
	)
	IF "%ERRORLEVEL%" NEQ "0" SET %2_Error=1
GOTO :EOF

REM :: Loads a cfg file [settting=value entities in a file] into env vars.
REM %1      - filepath to load
REM %2 [OP] - prefix string of the variable names to get from the file. [get-prefix[
REM %3 [OP] - prefix string added to the variables that are set. [set-prefix]
:LoadCfgFile
	CALL :CreateTempFile __LoadCfgFile bat
	IF "%~2" EQU "" (
		!DCMD! "--make-tokens=set-prefix=G_" "%~1" "!__LoadCfgFile!"
	) ELSE IF "%~3" EQU "" (
		!DCMD! "--make-tokens=set-prefix=G_;get-prefix=%~2" "%~1" "!__LoadCfgFile!"
	) ELSE (
		!DCMD! "--make-tokens=set-prefix=%~3;get-prefix=%~2" "%~1" "!__LoadCfgFile!"
	)
	
	"!__LoadCfgFile!"
	IF "%ERRORLEVEL%" NEQ "0" (
		CALL :Print E "CFGFILE" "Failed loading cfg file @s." "%~1"
		CALL :SetError 1
		GOTO :EOF
	)
GOTO :EOF

REM %1 - make-tokens arguments
REM %2 - cfg file to load
:LoadCfgFileEx
	CALL :CreateTempFile __LoadCfgFile bat
	!DCMD! "--make-tokens=%~1" "%~2" "!__LoadCfgFile!"
	"!__LoadCfgFile!"
	IF "%ERRORLEVEL%" NEQ "0" (
		CALL :Print E "CFGFILE" "Failed loading cfg file @s." "%~1"
		CALL :SetError 1
		GOTO :EOF
	)
GOTO :EOF

REM :: Gets arguments from the commandline and adds them as env vars.
:ArgumentsToEnv
	CALL :CreateTempFile __ArgumentsToEnv bat
	!DCMD! --make-args=%~1 "!__ArgumentsToEnv!" %*
	"!__ArgumentsToEnv!"
	IF "%ERRORLEVEL%" NEQ "0" (
		CALL :Print E "ARGENV" "Failed loading arguments file @s." "!__ArgumentsToEnv!"
		CALL :SetError 1
	)
GOTO :EOF

REM :: Prints formatted/colored status messages.
REM - %1 = [OP] type [D for debug, E for error, S for success, W for warning, excluded for status]
REM - %1 = 6 char category string (automatically padded/truncated)
REM - %2 = printf format string.
REM %3 and up printf arguments.
:Print
	SET __Print_Args=
	REM - StatuColor = 0, StatusColorDark = 3, TextColor = 6 [_S_CLR index]
	SET _S_CLR=0E 08 0B
	
	IF /I "%~1" EQU "S" ( 
		SET _S_CLR=0A 08 0B
		SHIFT /1
	) ELSE IF /I "%~1" EQU "E" ( 
		SET _S_CLR=CF 08 0C
		SHIFT /1
	) ELSE IF /I "%~1" EQU "W" ( 
		SET _S_CLR=E4 08 0F
		SHIFT /1
	) ELSE IF /I "%~1" EQU "SW" ( 
		SET _S_CLR=F4 08 0B
		SHIFT /1
	) ELSE IF /I "%~1" EQU "D" (
		IF "!G_SCRIPT_DEBUGGING!" NEQ "1" GOTO :EOF
		SET _S_CLR=0D 08 07
		SHIFT /1
	)
	
	:Print_NextArg
	IF "%~3" NEQ "" (
		SET __Print_Args=!__Print_Args! %3
		SHIFT /3
		GOTO Print_NextArg
	)
	!DCMD! -ff stdout "\k!_S_CLR:~0,2!@-6.6s\k!_S_CLR:~3,2!: \k!_S_CLR:~6,2!%~2\k07\r\n" "%~1" !__Print_Args!
	SET __Print_Args=
GOTO :EOF

REM :: Gets one char from the user and returns immediately; 
REM - %1 = Env Pointer to store char in.
REM - %2 = lower limit char to display.
REM - %3 = limit separator char to display.
REM - %4 = upper limit char to display.
REM - %5 = String for [enter = string]  display and value returned if enter is pressed.
REM - [NOTE] This is the only form if input we request from the user
:GetC
	IF DEFINED G_NOGETC (
		SET %1=%~5
		SET GetC_Response=%~5
		GOTO :EOF
	)
	CALL :CreateTempFile __GetC
	!DCMD! -ff CON " \k03(\k0B@s\k03@s\k0B@s\k03) \k03[\k0BEnter\k03=\k0B@s\k03] \k0E:\k0A" "%~2" "%~3" "%~4" "%~5"
	!DCMD! --getkey " " 2>!__GetC!>NUL
	FOR /F "eol=; tokens=* delims=" %%G IN (!__GetC!) DO (
		SET GetC_Response=%%G
		GOTO GetC_FoundKey
	)
	:GetC_FoundKey
	
	REM - Char is converted to ucase; if not a word char returned as 4 char hex string [0x00].
	IF /I "!GetC_Response:~1,1!" NEQ "" (
		SET %1=%~5
		!DCMD! -ff CON "\k07\n"
	) ELSE (
		SET %1=!GetC_Response!
		!DCMD! -ff CON "@s\k07\n" "!GetC_Response!"
	)
	SET GetC_Response=
GOTO :EOF

REM :: Ask a yes/no question.
REM - %1 and up = The question to ask
REM [USES] AskYesNo_Default [0 or 1]
REM [SETS] AskYesNo=1 if yes, 0 if no, AskYesNo_Default if niether y or n was pressed.
:AskYesNo
	!DCMD! -ff CON %*
	IF "!AskYesNo_Default!" EQU "0" (
		SET AskYesNo=0
		SET __AskYesNo_DefString=n
	) ELSE (
		SET AskYesNo=1
		SET AskYesNo_Default=1
		SET __AskYesNo_DefString=y
	)
	CALL :GetC AskYesNo_Response "y" "/" "n" "!__AskYesNo_DefString!"
	SET __AskYesNo_DefString=
	IF /I "!AskYesNo_Response!" EQU "N" SET AskYesNo=0
	IF /I "!AskYesNo_Response!" EQU "Y" SET AskYesNo=1
	SET AskYesNo_Response=
GOTO :EOF

REM :: Converts relative paths to absolute.
:ToAbsPath
	IF "%~1" EQU "" GOTO :EOF
	SET %~1=%~f2
	SHIFT /1
	SHIFT /1
	GOTO ToAbsPath
GOTO :EOF

:ToRelPath
	IF "%~1" EQU "" GOTO :EOF
	SET __ToRelPath=%~2
	CALL :PtrToString __ToRelPath "__ToRelPath:!CD!=."
	SET %~1=!__ToRelPath!
	SHIFT /1
	SHIFT /1
	GOTO ToRelPath
GOTO :EOF

REM :: Escapes a string for FINDSTR/
REM - %1 = Env var name to set with the escaped string text.
REM - %2 and up = Text to escape 
:RegexEscape
	CALL :CreateTempFile __RegexEscape_File bat
	CALL !DCMD! --get-commandline=start=1;strip_quotes;escaped="5C; 2E, 2A, 24, 5E, 5B, 5D";header="SET %~1=" %*>!__RegexEscape_File!
	CALL "!__RegexEscape_File!"
	IF NOT "!ERRORLEVEL!" EQU "0" (
		CALL :SetError 1
		CALL :Print E "ERROR" "Failed executing command: @s" "RegexEscape"
		TYPE "!__RegexEscape_File!"
	)	
GOTO :EOF

REM :: oooooooooooooooooooooooooooooooooooooooooooooooooooooo
REM ::  Basic string functions for trimming ends of a string
REM :: oooooooooooooooooooooooooooooooooooooooooooooooooooooo
:Trim
	CALL :TrimEx " " %*
GOTO :EOF

:TrimL
	CALL :TrimExL " " %*
GOTO :EOF

:TrimR
	CALL :TrimExR " " %*
GOTO :EOF

:TrimEx
	CALL :TrimExL %*
	CALL :TrimExR %*
GOTO :EOF

REM :: In the 'Ex' functions, the first arg is the trim char.
:TrimExL
	SET _W0=!%2!
	IF DEFINED _W0 (
		SET _W1="!_W0:~0,1!"
		IF !_W1! EQU "%~1" (
			SET %2=!_W0:~1!
			GOTO TrimExL
		)
		GOTO TrimExL_Done
	)
	:TrimExL_Done
	SHIFT /2
	IF "%2" NEQ "" GOTO TrimExL
GOTO :EOF

:TrimExR
	SET _W0=!%2!
	IF DEFINED _W0 (
		SET _W1="!_W0:~-1!"
		IF !_W1! EQU "%~1" (
			SET %2=!_W0:~0,-1!
			GOTO TrimExR
		)
		GOTO TrimExR_Done
	)
	:TrimExR_Done
	SHIFT /2
	IF "%2" NEQ "" GOTO TrimExR
GOTO :EOF

REM :: Adds env vars separated by spaces but only if they are defined.
REM - %1 = env var name to store 'defined' values.
REM - %2 and up = Env var names to check/add
:AddIfDef
	IF "%2" EQU "" (
		CALL :Trim %1
		GOTO :EOF
	)
	IF DEFINED %2 SET %1=!%1! !%~2!
	SHIFT /2
	GOTO AddIfDef
GOTO :EOF

REM :: Takes two env var names and set the first equal to the value of the second
REM - %1 = name of env var to set
REM - %2 = name of env var to use as the value for %1
:PtrToString
	SET %~1=!%~2!
GOTO :EOF

REM :: Undefines env vars.  
REM - %1 and up = Env var names to destroy. Appending a '*' will undefine all vars starting with this name
:Undefine
	IF "%~1" EQU "" (
		SET __UNDEFINE=
		GOTO :EOF
	)
	SET __UNDEFINE=%~1
	IF "!__UNDEFINE:~-1!" EQU "*" (
		SET __UNDEFINE=!__UNDEFINE:~0,-1!
		FOR /F "eol=; tokens=1 delims==" %%U IN ('SET !__UNDEFINE!') DO IF DEFINED %%U SET %%U=
	) ELSE (
		IF DEFINED %~1 SET %~1=
	)
	SHIFT /1
	GOTO Undefine
GOTO :EOF

:ShowUsage
SET F=!DCMD! -ff CON
SET A=!F! "@.72s\n"
IF /I "!G_HELP!" EQU "help" SET G_HELP=
IF /I "!G_HELP!" NEQ "" GOTO :ShowUsage_!G_HELP!

%A% "USAGE: make.cmd command[=value] <command-specific-arguments>            "
%A% "                                                                        "
%A% "COMMANDS: build/dist/clean/version/zip/formatcode                       "
%A% "                                                                        "
%A% "For help on a specific command, use the syntax:                         "
%A% "    make.cmd help=command                                               "
%A% "                                                                        "
GOTO :EOF

:ShowUsage_All
:ShowUsage_Build
%A% "USAGE: make.cmd build [fre/chk] [64/x64]                                "
%A% "                      [W2K/WIN7/WLH/WXP/WNET] [no_oacr]                 "
%A% "                                                                        "
IF "!G_HELP!" NEQ "all" GOTO :EOF

:ShowUsage_Dist
%A% "USAGE: make.cmd dist [fre/chk]                                          "
%A% "                                                                        "
IF "!G_HELP!" NEQ "all" GOTO :EOF

GOTO :EOF

