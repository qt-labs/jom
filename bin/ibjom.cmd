@echo off
if "%IBJOM_NUMBEROFJOBS%" == "" set IBJOM_NUMBEROFJOBS=20
xgConsole /profile="%~dp0\profile.xml" /command="jom -j%IBJOM_NUMBEROFJOBS% %*"
