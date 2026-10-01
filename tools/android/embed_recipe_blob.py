#!/usr/bin/env python3
from pathlib import Path
import sys

src = Path(sys.argv[1])
out = Path(sys.argv[2])
data = src.read_bytes()
with out.open("w", newline="\n") as f:
    f.write("/* Generated file: portable asset recipe metadata only. */\n")
    f.write("#include <stddef.h>\n#include <stdint.h>\n")
    f.write("const uint8_t gAndroidMk64RecipeBlob[] = {\n")
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        f.write("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",\n")
    f.write("};\n")
    f.write(f"const size_t gAndroidMk64RecipeBlobSize = {len(data)};\n")
print(f"[android] embedded recipe metadata: {len(data)} bytes")
