#!/usr/bin/env python3
"""Build PS5 diagnostics with the official community SDK runtime and linker script."""
import hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];APP=ROOT/'pstube-ps5'
SDK=ROOT/'vendor/ps5-payload-sdk';BUILD=APP/'build';DIST=APP/'dist'
sys.path.insert(0,str(APP/'scripts'))
import importlib.util
spec=importlib.util.spec_from_file_location('verify',APP/'scripts/verify-elf.py');verify=importlib.util.module_from_spec(spec);spec.loader.exec_module(verify)

def run(args):
    args=list(map(str,args));print('+',' '.join(args),flush=True)
    subprocess.run(args,cwd=APP,check=True,timeout=120)
    return args

def main():
    BUILD.mkdir(exist_ok=True);DIST.mkdir(exist_ok=True)
    run([sys.executable,APP/"scripts/embed-smarttube-icons.py"])
    archive=ROOT/'reference/archives/ps5-payload-sdk-v0.45.zip'
    assert hashlib.sha256(archive.read_bytes()).hexdigest()=='5fac64093d0e3e69f2584ce7935f24f703f622d9681f1495c85b9f00d1291a2c'
    cc=os.environ.get('PS5_CXX','/usr/bin/clang++');linker=Path(os.environ.get('PS5_LD',ROOT/'build/tools/ld.lld'))
    flags=['-target','x86_64-sie-ps5','-std=c++17','-O2','-g','-fvisibility-nodllstorageclass=default',
           '-isysroot',str(SDK),'-isystem',str(SDK/'target/include/c++/v1'),'-isystem',str(SDK/'target/include'),'-fno-stack-protector','-fno-plt',
           '-femulated-tls','-fno-exceptions','-fno-rtti','-Wall','-Wextra','-Werror']
    commands=[];reports=[]
    ports=ROOT/'vendor/pacbrew-v0.40.2/user/homebrew'
    shared=[]
    for file in ['json.cpp','core.cpp','app_state.cpp','app_ui.cpp','oauth.cpp','tv_oauth.cpp','live_hls.cpp']:
        obj=BUILD/(file+'.o');commands.append(run([cc,*flags,'-I'+str(APP/'third_party'),'-c',APP/'src/shared'/file,'-o',obj]));shared.append(obj)
    pins=json.loads((APP/'third_party/network-manifest.json').read_text())
    for library,digest in pins['libraries'].items():assert hashlib.sha256((ports/'lib'/('lib'+library+'.a')).read_bytes()).hexdigest()==digest
    shim=BUILD/'console_curl.o'
    commands.append(run(['/usr/bin/clang',*[f for f in flags if f!='-std=c++17'],'-I'+str(ports/'include'),'-c',APP/'src/console_curl.c','-o',shim]))
    for name,source,extra in [('PStube-PS5-Test-v0.20',APP/'src/main.cpp',['-DPSTUBE_PROTOTYPE','-DPSTUBE_AV_PLAYBACK','-DPSTUBE_LOCAL_PLAYBACK','-I'+str(APP/'build/codecs/install/include'),'-I'+str(ports/'include')]),('PStube-PS5-Platform-v0.20',APP/'src/main.cpp',[]),('sdk-hello-world',SDK/'samples/hello_world/main.c',[])]:
        obj=BUILD/(name+'.o');elf=DIST/(name+'.elf')
        compiler='/usr/bin/clang' if source.suffix=='.c' else cc
        active=[f for f in flags if f!='-std=c++17'] if source.suffix=='.c' else flags
        commands.append(run([compiler,*active,*extra,'-c',source,'-o',obj]))
        codecLibs=[APP/'build/codecs/install/lib/libavformat.a',APP/'build/codecs/install/lib/libswresample.a',APP/'build/codecs/install/lib/libswscale.a',APP/'build/codecs/install/lib/libavcodec.a',APP/'build/codecs/install/lib/libavutil.a'] if '-DPSTUBE_LOCAL_PLAYBACK' in extra else []
        networkLibs=[*shared,SDK/'target/lib/libc++.a',SDK/'target/lib/libc++abi.a',SDK/'target/lib/libunwind.a',shim,'--start-group',*[ports/'lib'/('lib'+n+'.a') for n in ['curl','ssl','crypto','psl','zstd','z']],'--end-group','--wrap=fcntl'] if '-DPSTUBE_AV_PLAYBACK' in extra else []
        commands.append(run([linker,'-m','elf_x86_64','-pie','-T',SDK/'ldscripts/elf_x86_64.x','--eh-frame-hdr','-z','max-page-size=0x4000','-mllvm','-emulated-tls','--hash-style=gnu','-o',elf,obj,*codecLibs,*networkLibs,SDK/'target/lib/crt1.o','-L',SDK/'target/lib','-ldl','-lc','-lkernel_web','-lSceLibcInternal','-lSceNet','-lpthread']))
        reports.append(verify.verify(elf))
    (BUILD/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
    (BUILD/'elf-verification.json').write_text(json.dumps(reports,indent=2)+'\n')
    hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in DIST.glob('*.elf')}
    (DIST/'SHA256SUMS').write_text(''.join(f'{h}  {n}\n' for n,h in sorted(hashes.items())))
    (BUILD/'build-manifest.json').write_text(json.dumps({'sdk_version':'v0.45','sdk_archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'compiler':subprocess.check_output([cc,'--version'],text=True),'linker_sha256':hashlib.sha256(linker.read_bytes()).hexdigest(),'hardware_tested':False,'sha256':hashes},indent=2)+'\n')
    print('PASS: three PS5 ELF builds and structural validation. Console testing pending.')
if __name__=='__main__':main()
