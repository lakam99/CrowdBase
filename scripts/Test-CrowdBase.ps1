param([string]$Build='build')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root "$Build/CrowdBaseStandalone_artefacts/Release/CrowdBase.exe"
$vst=Join-Path $root "$Build/CrowdBase_artefacts/Release/VST3/CrowdBase.vst3"
New-Item -ItemType Directory -Force (Join-Path $root 'tests/output') | Out-Null
Push-Location $root
try {
    $p=Start-Process -FilePath $exe -ArgumentList '--test tests/dsp-report.json' -WindowStyle Hidden -PassThru
    if(!$p.WaitForExit(30000)){$p.Kill();throw 'DSP test exceeded 30 seconds'}
    if($p.ExitCode -ne 0){throw 'DSP tests failed; see tests/dsp-report.json'}
    $arguments=@('--host',('"'+$vst+'"'),'--render','tests/output/CrowdBase-60s-demo.wav','--snapshot','tests/output/hosted-editor.png','--report','tests/host-report.json')
    $p=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
    if(!$p.WaitForExit(45000)){$p.Kill();throw 'VST3 host test exceeded 45 seconds'}
    if($p.ExitCode -ne 0){throw 'VST3 host tests failed; see tests/host-report.json'}
    foreach($skin in 0..2){
        $p=Start-Process -FilePath $exe -ArgumentList "--snapshot tests/output/CrowdBase-skin-$skin.png --skin $skin" -WindowStyle Hidden -PassThru
        if(!$p.WaitForExit(15000)){$p.Kill();throw 'UI snapshot timed out'}
        if($p.ExitCode -ne 0){throw 'UI snapshot failed'}
    }
    Get-Content tests/dsp-report.json
    Get-Content tests/host-report.json
} finally {Pop-Location}
