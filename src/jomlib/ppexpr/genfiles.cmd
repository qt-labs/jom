@echo off
setlocal
set MSYSPATH=D:\msys
if not exist %MSYSPATH% set MSYSPATH=C:\msys
if not exist %MSYSPATH% echo Can't locate msys directory.
set PATH=%PATH%;%MSYSPATH%\1.0\bin
nmake -nologo -f genfiles.mk
endlocal

