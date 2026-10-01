#!/usr/bin/env python3
import argparse, struct, zipfile
from pathlib import Path

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("zip")
    ap.add_argument("-o","--output",required=True)
    a=ap.parse_args()
    with zipfile.ZipFile(a.zip) as z:
        names=[n for n in z.namelist() if n.upper().endswith("EBOOT.PBP")]
        if not names:
            raise SystemExit("EBOOT.PBP not found in release zip")
        pbp=z.read(names[0])
    if pbp[:4] != b"\x00PBP":
        raise SystemExit("invalid PBP")
    psar=struct.unpack_from("<I",pbp,0x24)[0]
    if psar <= 0 or psar >= len(pbp):
        raise SystemExit("invalid DATA.PSAR offset")
    blob=None
    if pbp[psar:psar+8] == b"MK64PSAR":
        count=struct.unpack_from("<I",pbp,psar+8)[0]
        for i in range(count):
            tag,off,size=struct.unpack_from("<4sII",pbp,psar+12+i*12)
            if tag == b"RCP1":
                blob=pbp[psar+off:psar+off+size]
                break
    elif pbp[psar:psar+8] == b"MK64RCP1":
        blob=pbp[psar:]
    if not blob or blob[:8] != b"MK64RCP1":
        raise SystemExit("RCP1 recipe blob not found")
    vals=struct.unpack_from("<8s10I",blob,0)
    (_,region_size,data_crc,recipe_count,literal_size,pattern_count,
     course_count,block_count,reloc_count,max_block,max_unpacked)=vals
    p=48 + recipe_count*20
    p=(p + literal_size + 3) & ~3
    p += pattern_count*20 + course_count*20 + block_count*12
    outside=0
    for i in range(reloc_count):
        off,target=struct.unpack_from("<II",blob,p+i*8)
        if off & 0x80000000:
            outside += 1
    Path(a.output).write_bytes(blob)
    print(f"[android] recipes: {len(blob)} bytes region=0x{region_size:x} recipes={recipe_count} "
          f"courses={course_count} blocks={block_count} relocs={reloc_count} outside={outside} "
          f"max_block=0x{max_block:x} max_unpacked=0x{max_unpacked:x} crc={data_crc:08x}")

if __name__=="__main__":
    main()
