#!/usr/bin/env python3
"""Check source-tree hygiene and embedded asset provenance without a PS5 SDK."""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
required = ['README.md', 'LICENSE', 'THIRD_PARTY.md', 'CONTRIBUTING.md',
            'docs/BUILDING.md', 'pstube-ps5/src/main.cpp',
            'pstube-ps5/docs/BETA_V019.md', 'pstube-ps5/sce_sys/icon0.png']
for name in required:
    assert (ROOT / name).is_file(), f'Missing {name}'
files = [p for p in ROOT.rglob('*') if p.is_file() and '.git' not in p.relative_to(ROOT).parts
         and not any(part in {'build', 'dist', '__pycache__'} for part in p.relative_to(ROOT).parts)]
for path in files:
    relative = path.relative_to(ROOT)
    assert 'test-results' not in relative.parts, f'Raw console evidence must remain local: {relative}'
    assert path.name != '.DS_Store', f'Finder metadata: {relative}'
    assert path.suffix.lower() not in {'.elf', '.o', '.a', '.pkg', '.zip', '.gz', '.log'}, relative
    if path.suffix == '.json':
        json.loads(path.read_text())
icons = ROOT / 'pstube-ps5/assets/smarttube'
provenance = json.loads((icons / 'provenance.json').read_text())
for item in provenance['files']:
    path = icons / item['file']
    assert hashlib.sha256(path.read_bytes()).hexdigest() == item['sha256'], path.name
assert (ROOT / 'pstube-ps5/sce_sys/icon0.png').read_bytes() == (ROOT / 'pstube-ps5/branding/GlossyBluePStubePlayButton.png').read_bytes()
print(f'PASS: {len(files)} source/assets/docs files; JSON, asset hashes and repository hygiene')
