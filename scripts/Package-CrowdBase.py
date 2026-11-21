from pathlib import Path
import zipfile,json,hashlib,re,argparse,subprocess
root=Path(__file__).resolve().parents[1]
out=root/'dist'
out.mkdir(exist_ok=True)
args=argparse.ArgumentParser()
args.add_argument('--tag')
options=args.parse_args()
version=re.search(r'project\(CrowdBase VERSION (\d+\.\d+\.\d+)',(root/'CMakeLists.txt').read_text()).group(1)
if options.tag and options.tag!='v'+version:raise SystemExit('Release tag must match the CMake project version')
source_zip=out/f'CrowdBase-{version}-Source.zip'
source_files=[]
for folder in ['Source','resources','third_party','scripts','.github','docs']:
    for p in (root/folder).rglob('*'):
        if p.is_file() and '.git' not in p.parts and '__pycache__' not in p.parts:source_files.append(p)
source_files += [root/name for name in ['CMakeLists.txt','Build-source.ps1','.gitignore','README.md','LICENSE.txt','JUCE-LICENSE.md']]
assert (root/'third_party/JUCE/extras/Build/CMake/JUCEModuleSupport.cmake') in source_files
with zipfile.ZipFile(source_zip,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for p in source_files:z.write(p,'CrowdBase/'+p.relative_to(root).as_posix())
exe=root/'build/CrowdBaseStandalone_artefacts/Release/CrowdBase.exe'
vst=root/'build/CrowdBase_artefacts/Release/VST3/CrowdBase.vst3'
package=out/f'CrowdBase-{version}-Windows-x64.zip'
with zipfile.ZipFile(package,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    z.write(exe,'CrowdBase.exe')
    for p in vst.rglob('*'):
        if p.is_file():z.write(p,p.relative_to(vst.parent).as_posix())
    for name in ['README.md','LICENSE.txt','JUCE-LICENSE.md']:z.write(root/name,name)
    z.write(root/'scripts/Install-CrowdBase.ps1','scripts/Install-CrowdBase.ps1')
    z.write(root/'scripts/Install-CrowdBase.cmd','Install-CrowdBase.cmd')
    z.write(source_zip,source_zip.name)
    for name in ['dsp-report.json','host-report.json']:z.write(root/'tests'/name,'verification/'+name)
    z.write(root/'tests/output/CrowdBase-60s-demo.wav','CrowdBase-60s-demo.wav')
    z.write(root/'tests/output/CrowdBase-skin-0.png','CrowdBase-preview.png')
revision=subprocess.run(['git','-C',str(root),'rev-parse','HEAD'],capture_output=True,text=True)
report={'version':version,'platform':'Windows x64','commit':revision.stdout.strip() if revision.returncode==0 else None,'source_files':len(source_files),'files':[]}
for p in [package,source_zip,exe,vst/'Contents/x86_64-win/CrowdBase.vst3']:
    report['files'].append({'path':str(p),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(out/'manifest.json').write_text(json.dumps(report,indent=2))
(out/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in [package,source_zip]),encoding='utf-8')
print(json.dumps(report,indent=2))
