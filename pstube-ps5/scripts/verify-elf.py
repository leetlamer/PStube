#!/usr/bin/env python3
"""Check the loader-facing ELF shape without claiming PS5 runtime compatibility."""
import json,struct,sys
from pathlib import Path

def verify(path):
    data=Path(path).read_bytes()
    assert data[:7]==b'\x7fELF\x02\x01\x01', 'ELF64 little endian required'
    h=struct.unpack_from('<16sHHIQQQIHHHHHH',data)
    assert h[1:4]==(3,62,1), 'x86-64 ET_DYN PIE required'
    entry,phoff,shoff=h[4:7];phsize,phcount,shsize,shcount=h[9:13]
    assert phsize==56 and phcount and shsize==64 and shcount
    assert phoff+phsize*phcount<=len(data) and shoff+shsize*shcount<=len(data)
    loads=[];dynamic=None
    for i in range(phcount):
        p=struct.unpack_from('<IIQQQQQQ',data,phoff+i*phsize)
        kind,flags,offset,addr,_,filesz,memsz,align=p
        assert filesz<=memsz and offset+filesz<=len(data)
        if kind==1:
            assert align==0x4000 and addr%align==offset%align
            loads.append(p)
        if kind==2:dynamic=p
    assert any(p[1]&1 and p[3]<=entry<p[3]+p[6] for p in loads), 'entry outside executable segment'
    assert dynamic, 'dynamic imports missing'
    sections=[struct.unpack_from('<IIQQQQIIQQ',data,shoff+i*shsize) for i in range(shcount)]
    dependencies=[];undefined=set();relocations=0
    for section in sections:
        _,kind,_,_,offset,size,link,_,_,entsize=section
        assert kind==8 or offset+size<=len(data)
        if kind not in (2,6,11):continue
        assert link<len(sections)
        strings=sections[link];table=data[strings[4]:strings[4]+strings[5]]
        if kind==6:
            for at in range(offset,offset+size,16):
                tag,value=struct.unpack_from('<qQ',data,at)
                if tag==1:dependencies.append(table[value:].split(b'\0',1)[0].decode())
        elif entsize==24:
            for at in range(offset,offset+size,24):
                name,info,_,index,_,_=struct.unpack_from('<IBBHQQ',data,at)
                symbol=table[name:].split(b'\0',1)[0].decode()
                if symbol and index==0 and info>>4==1:undefined.add(symbol)
    for section in sections:
        if section[1]==4:
            assert section[9]==24
            for at in range(section[4],section[4]+section[5],24):
                offset,info,_=struct.unpack_from('<QQq',data,at)
                assert any(p[3]<=offset<p[3]+p[6] for p in loads), 'relocation target outside image'
                assert info&0xffffffff in (1,6,7,8,16,17,18), 'unexpected relocation type'
                relocations+=1
    assert dependencies and relocations and undefined
    return {'file':str(path),'entry':hex(entry),'load_segments':len(loads),'dependencies':dependencies,
            'undefined_imports':sorted(undefined),'relocations':relocations,'bytes':len(data),'hardware_tested':False}
if __name__=='__main__':
    for path in sys.argv[1:]:print(json.dumps(verify(path),indent=2))
