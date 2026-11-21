@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Install-CrowdBase.ps1" -PackageRoot "%~dp0."
if errorlevel 1 (
 echo Installation did not complete. See the error above.
 pause
 exit /b 1
)
echo CrowdBase is ready. Rescan your DAW instrument list.
pause
