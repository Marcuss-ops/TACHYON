@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "PS_EXE=powershell.exe"

where pwsh >nul 2>&1
if %ERRORLEVEL%==0 set "PS_EXE=pwsh"

"%PS_EXE%" -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build.ps1" %*
exit /b %ERRORLEVEL%
