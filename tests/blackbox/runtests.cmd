@echo off
setlocal
set MAKETOOL=%1
if "%MAKETOOL%"=="" set MAKETOOL=jom
for /D %%d in (*) do call :foo %%d
endlocal
goto :eof

:foo
pushd %1
echo --- %1 INIT ----------------------------
call "%MAKETOOL%" /nologo -f test.mk init
if errorlevel 1 goto onError
echo --- %1 TEST
call "%MAKETOOL%" /nologo -f test.mk tests
if errorlevel 1 goto onError
echo --- %1 POST CHECK
call "%MAKETOOL%" /nologo -f test.mk post_check
if errorlevel 1 goto onError
goto onSuccess
:onError
echo --- %1 FAILED
:onSuccess
popd
echo --- %1 END -----------------------------
goto :eof

