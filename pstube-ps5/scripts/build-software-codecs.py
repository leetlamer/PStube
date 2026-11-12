#!/usr/bin/env python3
"""Build a PS5-native, LGPL-only FFmpeg VP9 decoder for the software experiment."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,tarfile
ROOT=Path(__file__).resolve().parents[2];APP=ROOT/'pstube-ps5';BASE=APP/'build/codecs';SDK=ROOT/'vendor/ps5-payload-sdk'
BASE.mkdir(parents=True,exist_ok=True)
archive=ROOT/'reference/archives/ffmpeg-7.1.5.tar.xz'
# Reuse the previously pinned archive, not the PS4-compiled libraries.
expected='de668509caf9e35e3cd162473441fdb29538c6d96ed080292b3cf9e6fc5d558f'
actual=hashlib.sha256(archive.read_bytes()).hexdigest();assert expected==actual,(expected,actual)
source=BASE/'ffmpeg-7.1.5'
if not source.exists():
 with tarfile.open(archive) as t:t.extractall(BASE,filter='data')
# Upstream configure's bare sysctl test lacks FreeBSD's mandatory header.
p=source/'configure';s=p.read_text();p.write_text(s.replace('check_func  sysctl\n','check_func_headers sys/sysctl.h sysctl\n'))
build=BASE/'ffmpeg';build.mkdir(exist_ok=True);prefix=BASE/'install'
flags=f'-target x86_64-sie-ps5 -fvisibility-nodllstorageclass=default -isysroot {SDK} -isystem {SDK}/target/include -fno-stack-protector -fno-plt -femulated-tls -include {APP}/src/codec_threads.h'
ldflags=f'-m elf_x86_64 -pie --eh-frame-hdr --script={SDK}/ldscripts/elf_x86_64.x -z max-page-size=0x4000 -L{SDK}/target/lib {SDK}/target/lib/crt1.o'
args=[str(source/'configure'),'--prefix='+str(prefix),'--cc=/usr/bin/clang','--ld='+str(ROOT/'build/tools/ld.lld'),'--ar='+str(ROOT/'build/tools/elf-ar'),'--ranlib=/usr/bin/true','--strip=/usr/bin/true','--x86asmexe='+str(ROOT/'vendor/build-tools/bin/nasm'),'--disable-autodetect','--disable-everything','--disable-programs','--disable-doc','--disable-network','--disable-avdevice','--disable-avfilter','--enable-avformat','--enable-swresample','--enable-swscale','--disable-postproc','--disable-shared','--enable-static','--enable-pic','--disable-symver','--disable-gpl','--disable-version3','--enable-decoder=vp9,opus,aac,h264','--enable-demuxer=matroska,mov,mpegts,aac','--enable-protocol=file','--enable-parser=vp9,h264,aac','--enable-pthreads','--enable-cross-compile','--target-os=freebsd','--arch=x86_64','--cpu=generic','--extra-cflags='+flags,'--extra-ldflags='+ldflags,'--extra-libs=-lc -lkernel_web -lSceLibcInternal -lSceNet -lpthread']
(BASE/'configure-command.json').write_text(json.dumps(args,indent=2)+'\n')
for label,cmd in [('configure',args),('build',['make','-j6']),('install',['make','install'])]:
 print(label,flush=True)
 with (BASE/(label+'.log')).open('w') as f:subprocess.run(cmd,cwd=build,stdout=f,stderr=subprocess.STDOUT,check=True,timeout=600)
(BASE/'manifest.json').write_text(json.dumps({'archive_sha256':actual,'target':'PS5 x86_64','decoder':'vp9','configure':args,'libraries':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (prefix/'lib').glob('*.a')}},indent=2)+'\n')
print('PS5 VP9 software libraries ready')
