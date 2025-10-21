@echo off
echo Unblocking PowerShell scripts...
powershell -ExecutionPolicy Bypass -Command "Get-ChildItem -Path '%~dp0*.ps1' | Unblock-File"
echo.
echo Running installation...
powershell -ExecutionPolicy Bypass -File "%~dp0install.ps1"
pause
