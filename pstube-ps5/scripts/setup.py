#!/usr/bin/env python3
"""Verify and install the pinned SDK into this workspace; no global changes."""
from pathlib import Path
import hashlib,subprocess,zipfile
ROOT=Path(__file__).resolve().parents[2]
archive=ROOT/'reference/archives/ps5-payload-sdk-v0.45.zip'
if not archive.is_file():
    raise SystemExit('Download https://github.com/ps5-payload-dev/sdk/releases/download/v0.45/ps5-payload-sdk.zip to reference/archives/ps5-payload-sdk-v0.45.zip')
assert hashlib.sha256(archive.read_bytes()).hexdigest()=='5fac64093d0e3e69f2584ce7935f24f703f622d9681f1495c85b9f00d1291a2c'
with zipfile.ZipFile(archive) as z:
    assert all(not Path(n).is_absolute() and '..' not in Path(n).parts for n in z.namelist())
(ROOT/'vendor').mkdir(exist_ok=True)
# unzip preserves SDK executable permissions and relative symlinks.
subprocess.run(['/usr/bin/unzip','-oq',str(archive),'-d',str(ROOT/'vendor')],check=True,timeout=30)
assert (ROOT/'vendor/ps5-payload-sdk/target/lib/crt1.o').is_file()
print('PASS: pinned PS5 SDK extracted locally; build uses Apple clang and workspace ld.lld')
