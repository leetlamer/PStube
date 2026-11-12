#!/usr/bin/env python3
"""Package v0.17 without modifying archived v0.01 release files."""
from pathlib import Path
import hashlib,json,shutil,tarfile,zipfile
APP=Path(__file__).resolve().parents[1];ROOT=APP.parent;DIST=APP/'dist'
VERSION='v0.17';NAME='PStube-PS5-Test-'+VERSION
stage=DIST/NAME;stage.mkdir(exist_ok=True)
app=stage/'PStube-PS5-Test';(app/'sce_sys').mkdir(parents=True,exist_ok=True)
shutil.copy2(DIST/(NAME+'.elf'),app/'eboot.elf')
shutil.copy2(APP/'sce_sys/icon0.png',app/'sce_sys/icon0.png')
shutil.copytree(APP/'assets',app/'assets',dirs_exist_ok=True)
for name in ['LICENSE','third_party/SDL-LICENSE.txt','docs/BETA_V017.md']:
 shutil.copy2(APP/name,stage/('README.md' if name.endswith('.md') else Path(name).name))
for guide in ['BETA_V017_TEST_RESULTS.md','BETA_FEATURE_MATRIX.md','BETA_V014.md','BETA_V013.md']:
 shutil.copy2(APP/'docs'/guide,stage/guide)
shutil.copy2(APP/'build/codecs/ffmpeg-7.1.5/COPYING.LGPLv2.1',stage/'FFmpeg-LGPL-2.1.txt')
shutil.copy2(DIST/'PStube-PS5-Platform-v0.17.elf',stage/'PStube-PS5-Platform-v0.17.elf')
shutil.copytree(APP/'third_party/licenses',stage/'licenses',dirs_exist_ok=True)
shutil.copy2(APP/'assets/OFL.txt',stage/'licenses/Gontserrat-OFL.txt')
shutil.copy2(APP/'assets/stb-LICENSE.txt',stage/'licenses/stb.txt')
shutil.copy2(APP/'third_party/network-manifest.json',stage/'network-manifest.json')
shutil.copy2(APP/'build/codecs/manifest.json',stage/'decoder-build-manifest.json')
files=sorted(p for p in stage.rglob('*') if p.is_file() and p.name!='SHA256SUMS')
(stage/'SHA256SUMS').write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(stage)}\n' for p in files))
with zipfile.ZipFile(DIST/(NAME+'.zip'),'w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(stage.rglob('*')):
  if p.is_file():z.write(p,p.relative_to(stage))
# App objects + static libraries permit relinking; exact source and SDK included.
public_docs={'BETA_FEATURE_MATRIX.md','BETA_V013.md','BETA_V014.md','BETA_V016.md','BETA_V016_TEST_RESULTS.md','BETA_V017.md','BETA_V017_TEST_RESULTS.md'}
def source_filter(item):
 parts=Path(item.name).parts
 if '__pycache__' in parts or Path(item.name).name in ['CURRENT_WORK_CHECKPOINT.md','.DS_Store']:return None
 if len(parts)>2 and parts[:2]==('pstube-ps5','docs') and parts[-1] not in public_docs:return None
 return item
with tarfile.open(DIST/(NAME+'-source-relink.tar.gz'),'w:gz') as t:
 for name in ['src','scripts','tests','docs','third_party','README.md','LICENSE','THIRD_PARTY.md','assets','sce_sys','branding']:
  t.add(APP/name,arcname='pstube-ps5/'+name,filter=source_filter)
 for name in ['commands.json','build-manifest.json','elf-verification.json','v017-host-tests.log','v017-build.log','PStube-PS5-Test-v0.17.o','PStube-PS5-Platform-v0.17.o','json.cpp.o','core.cpp.o','app_state.cpp.o','app_ui.cpp.o','oauth.cpp.o','tv_oauth.cpp.o','live_hls.cpp.o','console_curl.o','codecs/install','codecs/manifest.json','codecs/configure-command.json','codecs/ffmpeg/config.h','codecs/ffmpeg/config_components.h','codecs/ffmpeg/ffbuild/config.mak']:
  t.add(APP/'build'/name,arcname='pstube-ps5/build/'+name)
 t.add(APP/'test-results/beta-v017-evidence.json',arcname='pstube-ps5/test-results/beta-v017-evidence.json')
 for capture in ['remote-v017-reconnect-final','remote-v017-1440-8','remote-v017-1440-10','remote-v017-native4k','remote-v017-long','remote-v017-capture']:
  evidence=APP/'test-results'/capture/'session.log'
  if evidence.exists():t.add(evidence,arcname='pstube-ps5/test-results/'+capture+'/session.log')
 for name in ['ffmpeg-7.1.5.tar.xz','sdk-0.45.zip','ps5-payload-sdk-v0.45.zip']:
  t.add(ROOT/'reference/archives'/name,arcname='reference/archives/'+name)
 for lib in ['curl','ssl','crypto','psl','zstd','z']:
  path=Path('vendor/pacbrew-v0.40.2/user/homebrew/lib')/('lib'+lib+'.a');t.add(ROOT/path,arcname=str(path))
 path=Path('vendor/pacbrew-v0.40.2/user/homebrew/include/curl');t.add(ROOT/path,arcname=str(path))
 t.add(APP/'THIRD_PARTY.md',arcname='THIRD_PARTY.md')
for name in [NAME+'.zip',NAME+'-source-relink.tar.gz',NAME+'.elf']:
 p=DIST/name;print(hashlib.sha256(p.read_bytes()).hexdigest(),p.name,p.stat().st_size)
# Verify packaged file integrity, names, fixtures and ELF match.
with zipfile.ZipFile(DIST/(NAME+'.zip')) as z:
 assert z.testzip() is None
 assert z.read('PStube-PS5-Test/sce_sys/icon0.png')==(APP/'branding/GlossyBluePStubePlayButton.png').read_bytes()
 assert hashlib.sha256(z.read('PStube-PS5-Test/eboot.elf')).digest()==hashlib.sha256((DIST/(NAME+'.elf')).read_bytes()).digest()
 fixtures=json.loads(z.read('PStube-PS5-Test/assets/fixtures.json'))
 for f in fixtures['workloads']:
  assert hashlib.sha256(z.read('PStube-PS5-Test/assets/'+f['file'])).hexdigest()==f['sha256']
 for line in z.read('SHA256SUMS').decode().splitlines():
  digest,name=line.split('  ',1)
  assert hashlib.sha256(z.read(name)).hexdigest()==digest,name
 provenance=json.loads(z.read('PStube-PS5-Test/assets/smarttube/provenance.json'))
 for item in provenance['files']:
  assert hashlib.sha256(z.read('PStube-PS5-Test/assets/smarttube/'+item['file'])).hexdigest()==item['sha256']
print('PASS: ZIP layout, all manifest hashes, fixture hashes and matching ELF')

artifacts=sorted(p for p in DIST.iterdir() if p.is_file() and (p.suffix in ['.zip','.elf'] or p.name.endswith('.tar.gz')))
(DIST/'SHA256SUMS').write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in artifacts))
