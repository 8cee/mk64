#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
PARTS = ROOT / "tools" / "android" / "asset_symbols"
OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "android" / "app" / "src" / "main" / "cpp" / "generated_asset_symbols.c"

region_size = int((PARTS / "meta.txt").read_text().strip(), 0)
EXTRA_ALIASES = {
    "D_0B002A00": 0x4FBEC4,
    "gTexture7ED50C": 0x4EC61C,
    "common_grand_prix_human_item_curve2": 0x46F68 + 0x64,
    "common_grand_prix_human_item_curve3": 0x46F68 + 0xC8,
    "common_grand_prix_human_item_curve4": 0x46F68 + 0x12C,
    "common_grand_prix_human_item_curve5": 0x46F68 + 0x190,
    "common_grand_prix_human_item_curve6": 0x46F68 + 0x1F4,
    "common_grand_prix_human_item_curve7": 0x46F68 + 0x258,
    "common_grand_prix_human_item_curve8": 0x46F68 + 0x2BC,
    "common_grand_prix_cpu_item_curve2": 0x46C48 + 0x64,
    "common_grand_prix_cpu_item_curve3": 0x46C48 + 0xC8,
    "common_grand_prix_cpu_item_curve4": 0x46C48 + 0x12C,
    "common_grand_prix_cpu_item_curve5": 0x46C48 + 0x190,
    "common_grand_prix_cpu_item_curve6": 0x46C48 + 0x1F4,
    "common_grand_prix_cpu_item_curve7": 0x46C48 + 0x258,
    "common_grand_prix_cpu_item_curve8": 0x46C48 + 0x2BC,
    "common_versus_2_player_item_curve2": 0x46B80 + 0x64,
    "common_versus_3_player_item_curve2": 0x46A54 + 0x64,
    "common_versus_3_player_item_curve3": 0x46A54 + 0xC8,
}

rows = []
for p in sorted(PARTS.glob("part*.tsv")):
    for line in p.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        name, off = line.split("\t", 1)
        rows.append((name, int(off, 0)))

rows.extend((name, off) for name, off in EXTRA_ALIASES.items())

def derive_common_aliases(rows):
    """Expand grouped common-data tables into their YAML sub-symbol aliases."""
    base_offsets = {name: off for name, off in rows}
    text = (ROOT / "yamls/us/common_data.yml").read_text().splitlines()
    tables = {}
    in_tables = False
    current_table = None
    symbols = []
    current_symbol = None
    for raw in text:
        stripped = raw.strip()
        indent = len(raw) - len(raw.lstrip(" "))
        if indent == 2 and stripped == "tables:":
            in_tables = True
            current_table = None
            continue
        if in_tables:
            if indent == 4:
                m = re.match(r"([A-Za-z_0-9]+):\\s*$", stripped)
                if m:
                    current_table = m.group(1)
                    continue
            if indent == 6 and current_table:
                m = re.match(r"range:\\s*\\[\\s*(0x[0-9A-Fa-f]+)\\s*,\\s*(0x[0-9A-Fa-f]+)", stripped)
                if m:
                    tables[current_table] = (int(m.group(1), 16), int(m.group(2), 16))
                    continue
            if indent <= 0:
                in_tables = False
        if not in_tables:
            if indent == 0:
                m = re.match(r"([A-Za-z_0-9]+):\\s*$", stripped)
                current_symbol = m.group(1) if m else None
            elif indent == 2 and current_symbol:
                m = re.match(r"offset:\\s*(0x[0-9A-Fa-f]+)", stripped)
                if m:
                    symbols.append((current_symbol, int(m.group(1), 16)))
    out = []
    for table, (start, end) in tables.items():
        base = base_offsets.get(table)
        if base is None:
            continue
        for name, off in symbols:
            if start <= off <= end:
                out.append((name, base + (off - start)))
    return out

rows.extend(derive_common_aliases(rows))
rows = sorted(set(rows))

OUT.parent.mkdir(parents=True, exist_ok=True)
with OUT.open("w", newline="\n") as f:
    f.write("/* Generated from the public portable-recipe symbol map. No ROM data. */\n")
    f.write("#include <stdint.h>\n")
    f.write(f"__attribute__((aligned(16))) unsigned char __assets_start[{region_size}];\n")
    f.write(f'__asm__(".globl __assets_end\\n.set __assets_end, __assets_start + {region_size}\\n");\n')
    for name, off in rows:
        f.write(f'__asm__(".globl {name}\\n.set {name}, __assets_start + {off}\\n");\n')
print(f"[android] generated {OUT} with {len(rows)} asset symbols ({region_size} bytes)")
