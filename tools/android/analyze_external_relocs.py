#!/usr/bin/env python3
from pathlib import Path
import struct, sys, bisect

ROOT = Path(__file__).resolve().parents[2]
blob = Path(sys.argv[1]).read_bytes()
parts = ROOT / "tools" / "android" / "asset_symbols"

symbols = []
for p in sorted(parts.glob("part*.tsv")):
    for line in p.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        name, off = line.split("\t", 1)
        symbols.append((int(off, 0), name))
symbols.sort()
offs = [o for o, _ in symbols]

vals = struct.unpack_from("<8s10I", blob, 0)
magic = vals[0]
if magic != b"MK64RCP1":
    raise SystemExit("bad recipe blob")
_, region_size, data_crc, recipe_count, literal_size, pattern_count, course_count, block_count, reloc_count, max_block, max_unpacked = vals
p = 48 + recipe_count * 20
p = (p + literal_size + 3) & ~3
p += pattern_count * 20 + course_count * 20 + block_count * 12

print(f"[android] external relocations ({reloc_count} total):")
n = 0
for i in range(reloc_count):
    off, target = struct.unpack_from("<II", blob, p + i * 8)
    if not (off & 0x80000000):
        continue
    roff = off & 0x7fffffff
    idx = bisect.bisect_right(offs, roff) - 1
    owner_off, owner = symbols[idx] if idx >= 0 else (0, "<unknown>")
    print(f"[android] extrel off=0x{roff:08x} owner={owner}+0x{roff-owner_off:x} target=0x{target:08x}")
    n += 1
print(f"[android] external relocation count={n}")
