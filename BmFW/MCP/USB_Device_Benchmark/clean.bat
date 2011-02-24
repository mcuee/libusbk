@ECHO OFF

PUSHD %CD%
CD .\Firmware

SET REGCLEAN=.\projects\VStudio\Utility\RegexClean
%REGCLEAN% -m"\.(\$\$\$|bkx|cce|cod|cof|err|hex|lde|i|lde|lst|obj|o|rlf|sym|sdb|wat|mcs|mptags|tagsrc|map|elf|ncb|resharper|suo|user)$"
IF EXIST ".\Objects\" RMDIR /S /Q ".\Objects"
%REGCLEAN% -r -d -m"\\_ReSharper\."
%REGCLEAN% -r -d -m"\\(Release|Debug|temp|output)$"
IF EXIST "CleanUp.bat" CALL CleanUp.bat 2>NUL>NUL

CD .\projects\VStudio
CALL clean.cmd

POPD
