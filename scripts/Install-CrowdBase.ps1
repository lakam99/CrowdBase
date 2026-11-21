param([string]$PackageRoot=(Split-Path -Parent $PSScriptRoot))
$ErrorActionPreference='Stop'
$sourceExe=Join-Path $PackageRoot 'CrowdBase.exe'
$sourceVst=Join-Path $PackageRoot 'CrowdBase.vst3'
if(!(Test-Path -LiteralPath $sourceExe) -or !(Test-Path -LiteralPath $sourceVst)){throw 'Run this installer from the extracted CrowdBase Windows package.'}
$program=Join-Path $env:LOCALAPPDATA 'Programs/CrowdBase'
$vst=Join-Path $env:LOCALAPPDATA 'Programs/Common/VST3/CrowdBase.vst3'
$running=Get-Process CrowdBase -ErrorAction SilentlyContinue
if($running){throw 'Close CrowdBase before installing an update.'}
# Do not stop any DAW. A locked installed plugin makes the copy fail explicitly.
if(Test-Path -LiteralPath $vst){
    $binary=Join-Path $vst 'Contents/x86_64-win/CrowdBase.vst3'
    if(Test-Path -LiteralPath $binary){$lock=[IO.File]::Open($binary,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None);$lock.Dispose()}
}
New-Item -ItemType Directory -Force $program,(Split-Path -Parent $vst) | Out-Null
Copy-Item -LiteralPath $sourceExe -Destination (Join-Path $program 'CrowdBase.exe') -Force
# Copy directory contents into a named destination, preserving any unrelated plugins.
New-Item -ItemType Directory -Path $vst -Force | Out-Null
Get-ChildItem -LiteralPath $sourceVst | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $vst -Recurse -Force}
Copy-Item -LiteralPath (Join-Path $PackageRoot 'README.md') -Destination $program -Force
$shell=New-Object -ComObject WScript.Shell
$shortcut=$shell.CreateShortcut((Join-Path ([Environment]::GetFolderPath('Programs')) 'CrowdBase.lnk'))
$shortcut.TargetPath=Join-Path $program 'CrowdBase.exe';$shortcut.WorkingDirectory=$program;$shortcut.Save()
Write-Output "Installed standalone: $program"
Write-Output "Installed VST3: $vst"
Write-Output 'Restart or rescan your DAW instrument list to find CrowdBase.'
