#!/usr/bin/env python3
"""Verify pinned community PS5 ports; extract locally without running target tools."""
from pathlib import Path
import hashlib,tarfile,json
ROOT=Path(__file__).resolve().parents[2]
archive=ROOT/'reference/archives/pacbrew-v0.40.2.tar.gz'
expected='a85f65de418a8e6a898c6c3e3c870d50fff7618a200e4dd59ea9692af6ecec4d'
assert hashlib.sha256(archive.read_bytes()).hexdigest()==expected
out=ROOT/'vendor/pacbrew-v0.40.2';out.mkdir(exist_ok=True)
with tarfile.open(archive) as t:
 for member in t.getmembers():
  prefix='opt/ps5-payload-sdk/target/'
  if member.isfile() and member.name.startswith(prefix+'user/homebrew/'):
   member.name=member.name.removeprefix(prefix);t.extract(member,out,filter='data')
libs={n:hashlib.sha256((out/'user/homebrew/lib'/('lib'+n+'.a')).read_bytes()).hexdigest() for n in ['curl','ssl','crypto','psl','zstd','z']}
(ROOT/'pstube-ps5/third_party/network-manifest.json').write_text(json.dumps({'release':'v0.40.2','url':'https://github.com/ps5-payload-dev/pacbrew-repo/releases/download/v0.40.2/ps5-payload-dev.tar.gz','sha256':expected,'libraries':libs},indent=2)+'\n')
print('Verified/extracted pinned PS5 networking libraries')
