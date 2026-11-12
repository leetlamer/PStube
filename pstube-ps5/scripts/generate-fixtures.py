#!/usr/bin/env python3
"""Create deterministic synthetic VP9 workloads; no external video downloads."""
from pathlib import Path
import subprocess,json,hashlib
ROOT=Path(__file__).resolve().parents[2];APP=ROOT/'pstube-ps5';OUT=APP/'assets';OUT.mkdir(exist_ok=True)
records=[]
for height,width in [(1080,1920),(1440,2560),(2160,3840)]:
 p=OUT/f'vp9-{height}.ivf'
 if not p.exists():
  subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-y','-f','lavfi','-i',f'testsrc2=size={width}x{height}:rate=60','-frames:v','120','-an','-c:v','libvpx-vp9','-deadline','realtime','-cpu-used','8','-row-mt','1','-threads','6','-lag-in-frames','0','-auto-alt-ref','0','-g','60','-pix_fmt','yuv420p','-b:v','0','-crf','35','-f','ivf',str(p)],check=True,timeout=180)
 records.append({'file':p.name,'width':width,'height':height,'fps':60,'frames':120,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size})
(OUT/'fixtures.json').write_text(json.dumps({'source':'FFmpeg testsrc2; synthetic, not representative of all YouTube content','workloads':records},indent=2)+'\n')
print('Synthetic VP9 fixtures ready')
