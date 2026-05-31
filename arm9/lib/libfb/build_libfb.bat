@echo off
setlocal
set "HOME=%CD%\.msys2-home"
set "TMP=%CD%\.msys2-tmp"
set "TEMP=%CD%\.msys2-tmp"
if not exist "%HOME%" mkdir "%HOME%"
if not exist "%TMP%" mkdir "%TMP%"
del lib\libfb.a
C:\msys64\msys2_shell.cmd -defterm -no-start -here -c "export HOME=\"$PWD/.msys2-home\" TMPDIR=\"$PWD/.msys2-tmp\" TMP=\"$PWD/.msys2-tmp\" TEMP=\"$PWD/.msys2-tmp\"; mkdir -p \"$TMPDIR\" \"$HOME\"; make"
