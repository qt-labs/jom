@echo off
setlocal
set PATH=%PATH%;D:\msys\1.0\bin
nmake -nologo -f genfiles.mk
endlocal

