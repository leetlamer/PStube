#!/usr/bin/env python3
from pathlib import Path
import subprocess,json,hashlib
app=Path(__file__).resolve().parents[1];out=app/'assets/av-1080.webm'
subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-y','-f','lavfi','-i','testsrc2=size=1920x1080:rate=60','-f','lavfi','-i','sine=frequency=440:sample_rate=48000','-t','12','-c:v','libvpx-vp9','-deadline','realtime','-cpu-used','8','-row-mt','1','-threads','6','-lag-in-frames','0','-auto-alt-ref','0','-g','60','-crf','35','-b:v','0','-c:a','libopus','-b:a','96k','-ac','2',str(out)],check=True,timeout=180)
(app/'assets/av-fixture.json').write_text(json.dumps({'file':out.name,'source':'synthetic testsrc2 and 440Hz tone','duration':12,'fps':60,'width':1920,'height':1080,'audio':'Opus stereo 48kHz','sha256':hashlib.sha256(out.read_bytes()).hexdigest()},indent=2)+'\n')
