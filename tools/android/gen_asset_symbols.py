#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
PARTS = ROOT / "tools" / "android" / "asset_symbols"
OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "android" / "app" / "src" / "main" / "cpp" / "generated_asset_symbols.c"

region_size = int((PARTS / "meta.txt").read_text().strip(), 0)
rows = []
for p in sorted(PARTS.glob("part*.tsv")):
    for line in p.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        name, off = line.split("\t", 1)
        rows.append((name, int(off, 0)))

OUT.parent.mkdir(parents=True, exist_ok=True)
with OUT.open("w", newline="\n") as f:
    f.write("/* Generated from the public portable-recipe symbol map. No ROM data. */\n")
    f.write("#include <stdint.h>\n")
    f.write(f"__attribute__((aligned(16))) unsigned char __assets_start[{region_size}];\n")
    f.write(f'__asm__(".globl __assets_end\\n.set __assets_end, __assets_start + {region_size}\\n");\n')
    for name, off in rows:
        f.write(f'__asm__(".globl {name}\\n.set {name}, __assets_start + {off}\\n");\n')
print(f"[android] generated {OUT} with {len(rows)} asset symbols ({region_size} bytes)")
