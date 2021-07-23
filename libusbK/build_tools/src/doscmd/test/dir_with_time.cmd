@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

DIR /S /B %~1\*>dir.lst
FOR /F "eol=; tokens=* delims=" %%A IN (dir.lst) DO ECHO %%~tA;%%~zA;%%~A
DEL dir.lst