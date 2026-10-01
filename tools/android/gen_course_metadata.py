#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "yamls" / "courses"
OUT = ROOT / "assets" / "course_metadata"
OUT.mkdir(parents=True, exist_ok=True)

courses = []
for path in SRC.glob("*_metadata.yml"):
    text = path.read_text()
    def field(name):
        m = re.search(rf"^\s{{2}}{re.escape(name)}:\s*(.*)$", text, re.M)
        return m.group(1).strip() if m else ""
    cid = field("id")
    if not cid:
        continue
    i = int(cid, 0)
    name = field("name")
    debug = field("debug_name")
    cup = field("cup")
    cup_index = field("cup_index")
    if not name or name.startswith("#"):
        name = ""
    if not debug or debug.startswith("#"):
        debug = ""
    if not cup or cup == "null":
        cup = "-1"
    if not cup_index or cup_index == "null":
        cup_index = "-1"
    courses.append((i, name, debug, cup, cup_index))

courses.sort()
max_id = max(i for i, *_ in courses)
by_id = {i:(name,debug,cup,cup_index) for i,name,debug,cup,cup_index in courses}

def q(s):
    return '"' + s.replace("\\","\\\\").replace('"','\\"') + '"'

names=[]; debug=[]; cups=[]; idx=[]
for i in range(max_id + 1):
    name, dbg, cup, ci = by_id.get(i, ("","","-1","-1"))
    names.append(q(name) + ",")
    debug.append(q(dbg) + ",")
    cups.append(cup + ",")
    idx.append(ci + ",")

(OUT / "gCourseNames.inc.c").write_text("\n".join(names) + "\n")
(OUT / "gCourseDebugNames.inc.c").write_text("\n".join(debug) + "\n")
(OUT / "gCupSelectionByCourseId.inc.c").write_text("\n".join(cups) + "\n")
(OUT / "gPerCupIndexByCourseId.inc.c").write_text("\n".join(idx) + "\n")
print(f"[android] generated course metadata for {max_id + 1} course ids")
