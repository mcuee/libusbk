@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
REM #
REM # [%1] = dependency src base src dir
REM # [%2] = output base dir
REM # [%3] = dependency output dir name
REM # [%4] = dependency file list
REM #

IF NOT EXIST "%~4" GOTO :EOF

SET __DepBaseSrcDir=%~1
SET __DepBaseOutDir=%~2
SET __DepBaseOutDirName=%~3
SET __DepFileList=%~4

FOR /F "eol=# tokens=1,2 delims=;" %%A IN (!__DepFileList!) DO (
	SET __DepSrcRel=%%~A
	SET __DepDstRel=%%~B
	CALL :TrimEx " " __DepSrcRel __DepDstRel
	IF NOT "!__DepSrcRel!" EQU "" (
		SET __DepSrc=!__DepBaseSrcDir!!__DepSrcRel!
		IF "!__DepDstRel:~0,1!" EQU "@" (
			SET __DepDstRel=!__DepDstRel:~1!
			SET __DepDst=!__DepBaseOutDir!!__DepDstRel!
		) ELSE (
			SET __DepDst=!__DepBaseOutDir!!__DepDstRel!\!__DepBaseOutDirName!
		)
		
		ECHO Deploying !__DepSrc! to !__DepDst!..
		IF NOT EXIST "!__DepSrc!" (
			ECHO [WARNING] Dependency !__DepSrc! not found.
		) ELSE (
			CALL :GetFName "!__DepSrc!"
			IF NOT EXIST "!__DepDst!" MKDIR "!__DepDst!"
			COPY /Y "!__DepSrc!" "!__DepDst!\!_FD!" 2>NUL>NUL
			IF NOT "!ERRORLEVEL!" EQU "0" ECHO [WARNING] Dependency copy failed from !__DepSrc! to "!__DepDst!\!_FD!".
		)
	)
)

REM #
REM # Done.
REM #
GOTO :EOF

REM # [SETS] _GD to the full directory of a file.
:GetDir
	SET _GD=%~dp1
GOTO :EOF

REM # [SETS] _GF to the filename.ext only.
:GetFName
	SET _FD=%~nx1
GOTO :EOF

REM # Removes chars on the left and right.
REM %1 = char to remove
REM %2 = and up env var names [not values] to trim
:TrimEx
	CALL :TrimExL %*
	CALL :TrimExR %*
GOTO :EOF

REM # Removes chars on the left.
REM %1 = char to remove
REM %2 = and up env var names [not values] to trim
:TrimExL
	SET _W0=!%2!
	IF DEFINED _W0 (
		SET _W1="!_W0:~0,1!"
		IF !_W1! EQU "%~1" (
			SET %2=!_W0:~1!
			GOTO TrimExL
		)
	)
	SHIFT /2
	IF "%2" NEQ "" GOTO TrimExL
GOTO :EOF

REM # Removes chars on the right.
REM %1 = char to remove
REM %2 = and up env var names [not values] to trim
:TrimExR
	SET _W0=!%2!
	IF DEFINED _W0 (
		SET _W1="!_W0:~-1!"
		IF !_W1! EQU "%~1" (
			SET %2=!_W0:~0,-1!
			GOTO TrimExR
		)
	)
	SHIFT /2
	IF "%2" NEQ "" GOTO TrimExR
GOTO :EOF
