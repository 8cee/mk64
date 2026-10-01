#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "yamls" / "courses"
OUT = ROOT / "assets" / "course_metadata"
OUT.mkdir(parents=True, exist_ok=True)

def scalar(value):
    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] and value[0] in ("'", '"'):
        return value[1:-1]
    return value

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
    name = scalar(field("name"))
    debug = scalar(field("debug_name"))
    cup = field("cup")
    cup_index = field("cup_index")
    course_length = scalar(field("course_length"))
    sky_colors = field("sky_colors")
    sky_colors2 = field("sky_colors2")

    if not name or name.startswith("#"):
        name = ""
    if not debug or debug.startswith("#"):
        debug = ""
    if not cup or cup == "null":
        cup = "-1"
    if not cup_index or cup_index == "null":
        cup_index = "-1"
    if not course_length or course_length == "null":
        course_length = ""
    if not sky_colors:
        sky_colors = "[0, 0, 0, 0, 0, 0]"
    if not sky_colors2:
        sky_colors2 = "[0, 0, 0, 0, 0, 0]"

    courses.append((i, name, debug, cup, cup_index, course_length, sky_colors, sky_colors2))

courses.sort()
max_id = max(i for i, *_ in courses)
by_id = {
    i: (name, debug, cup, cup_index, course_length, sky_colors, sky_colors2)
    for i, name, debug, cup, cup_index, course_length, sky_colors, sky_colors2 in courses
}

def q(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

names = []
debug_names = []
cups = []
cup_indices = []
lengths = []
sky = []
sky2 = []

for i in range(max_id + 1):
    name, debug, cup, cup_index, course_length, sky_colors, sky_colors2 = by_id.get(
        i, ("", "", "-1", "-1", "", "[0, 0, 0, 0, 0, 0]", "[0, 0, 0, 0, 0, 0]")
    )
    names.append(q(name) + ",")
    debug_names.append(q(debug) + ",")
    cups.append(cup + ",")
    cup_indices.append(cup_index + ",")
    lengths.append(q(course_length) + ",")
    sky.append("{ " + sky_colors.strip("[]") + " },")
    sky2.append("{ " + sky_colors2.strip("[]") + " },")

(OUT / "gCourseNames.inc.c").write_text("\n".join(names) + "\n")
(OUT / "gCourseDebugNames.inc.c").write_text("\n".join(debug_names) + "\n")
(OUT / "gCupSelectionByCourseId.inc.c").write_text("\n".join(cups) + "\n")
(OUT / "gPerCupIndexByCourseId.inc.c").write_text("\n".join(cup_indices) + "\n")
(OUT / "sCourseLengths.inc.c").write_text("\n".join(lengths) + "\n")
(OUT / "sSkyColors.inc.c").write_text("\n".join(sky) + "\n")
(OUT / "sSkyColors2.inc.c").write_text("\n".join(sky2) + "\n")

print(f"[android] generated course metadata for {max_id + 1} course ids")
