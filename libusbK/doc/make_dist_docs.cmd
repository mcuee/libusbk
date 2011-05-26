@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
SET HELP_PACKAGE_NAME=libusbK-help

CALL :FindFileInPath DOXYGEN_EXE doxygen.exe
IF "!DOXYGEN_EXE!" EQU "" (
	ECHO [ERROR] Doxygen not found in path.
	GOTO :EOF
)

DEL /S /Q .\output\*

TYPE chm.doxyfile|"!DOXYGEN_EXE!" -
IF "!ERRORLEVEL!" NEQ "0" (
	ECHO [ERROR] Doxygen failed generating chm.doxyfile.
	GOTO :EOF
)
COPY /Y .\output\html\*.chm .\output
DEL /S /Q .\output\html\*

TYPE html.doxyfile|"!DOXYGEN_EXE!" -
IF "!ERRORLEVEL!" NEQ "0" (
	ECHO [ERROR] Doxygen failed generating html.doxyfile.
	GOTO :EOF
)
DEL /S /Q .\output\html\*.chm

MOVE /Y .\output .\docs

IF EXIST "!HELP_PACKAGE_NAME!.7z" DEL /Q "!HELP_PACKAGE_NAME!.7z"
..\build_tools\7za.exe a -r -y "!HELP_PACKAGE_NAME!.7z" .\docs*

IF NOT EXIST "..\package\output\" MKDIR "..\package\output\"
MOVE /Y ".\docs\!HELP_PACKAGE_NAME!.chm" "..\package\output\"
MOVE /Y "!HELP_PACKAGE_NAME!.7z" "..\package\"

RMDIR /S /Q .\docs
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
