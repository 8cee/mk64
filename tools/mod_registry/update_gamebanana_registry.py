#!/usr/bin/env python3
import html
import json
import re
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CONFIG = ROOT / "mods" / "server-config.json"
OUTPUT = ROOT / "mods" / "registry.json"
UA = "MK64-ModRegistry/1.0 (+https://github.com/8cee/mk64)"

def get_json(url: str):
    req = urllib.request.Request(url, headers={"User-Agent": UA, "Accept": "application/json"})
    with urllib.request.urlopen(req, timeout=30) as response:
        return json.load(response)

def strip_html(text):
    if not text:
        return ""
    text = re.sub(r"<br\\s*/?>", "\\n", str(text), flags=re.I)
    text = re.sub(r"<[^>]+>", "", text)
    text = html.unescape(text)
    return " ".join(text.split())[:500]

def first_value(obj, *keys, default=""):
    for key in keys:
        value = obj.get(key)
        if value not in (None, "", [], {}):
            return value
    return default

def list_mod_ids(game_id: int, max_pages: int):
    result = []
    seen = set()
    for page in range(1, max_pages + 1):
        params = urllib.parse.urlencode({
            "page": page,
            "itemtype": "Mod",
            "gameid": game_id,
            "include_updated": "true",
            "format": "json_min",
        })
        data = get_json(f"https://api.gamebanana.com/Core/List/New?{params}")
        if not data:
            break
        added = 0
        for row in data:
            if not isinstance(row, list) or len(row) < 2 or row[0] != "Mod":
                continue
            mod_id = int(row[1])
            if mod_id not in seen:
                seen.add(mod_id)
                result.append(mod_id)
                added += 1
        if added == 0:
            break
    return result

def mod_details(mod_id: int):
    props = ",".join([
        "_idRow",
        "_sName",
        "_sText",
        "_sProfileUrl",
        "_aFiles",
        "_aSubmitter",
        "_bIsNsfw",
    ])
    url = "https://gamebanana.com/apiv11/Mod/{}?{}".format(
        mod_id, urllib.parse.urlencode({"_csvProperties": props})
    )
    return get_json(url)

def normalize_file(file_obj, allowed_exts):
    name = first_value(file_obj, "_sFile", "_sName")
    url = first_value(file_obj, "_sDownloadUrl", "_sUrl")
    if not name or not url:
        return None
    lower = name.lower()
    if not any(lower.endswith(ext) for ext in allowed_exts):
        return None
    if not str(url).startswith("https://"):
        return None
    return {"file_name": name, "download_url": url}

def main():
    cfg = json.loads(CONFIG.read_text(encoding="utf-8"))
    allowed = tuple(x.lower() for x in cfg.get("allowed_extensions", [".zip", ".o2r"]))
    mods = []
    seen = set()

    for game_id in cfg.get("gamebanana_game_ids", []):
        for mod_id in list_mod_ids(int(game_id), int(cfg.get("max_pages", 10))):
            if mod_id in seen:
                continue
            seen.add(mod_id)
            try:
                data = mod_details(mod_id)
            except Exception as exc:
                print(f"skip {mod_id}: details failed: {exc}")
                continue

            if data.get("_bIsNsfw") is True:
                print(f"skip {mod_id}: nsfw")
                continue

            chosen = None
            for file_obj in data.get("_aFiles") or []:
                chosen = normalize_file(file_obj, allowed)
                if chosen:
                    break
            if not chosen:
                print(f"skip {mod_id}: no supported archive")
                continue

            submitter = data.get("_aSubmitter") or {}
            if isinstance(submitter, dict):
                author = first_value(submitter, "_sName", "_sUsername")
            else:
                author = str(submitter or "")

            name = first_value(data, "_sName", default=f"GameBanana Mod {mod_id}")
            mods.append({
                "id": f"gamebanana-{mod_id}",
                "name": name,
                "version": "",
                "author": author,
                "description": strip_html(data.get("_sText", "")),
                "homepage_url": first_value(
                    data,
                    "_sProfileUrl",
                    default=f"https://gamebanana.com/mods/{mod_id}",
                ),
                "file_name": chosen["file_name"],
                "download_url": chosen["download_url"],
                "sha256": "",
                "source": "gamebanana",
                "source_id": mod_id,
            })

    mods.sort(key=lambda m: m["name"].lower())
    payload = {
        "schema": 1,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "source": "GameBanana SpaghettiKart",
        "mods": mods,
    }
    OUTPUT.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\\n", encoding="utf-8")
    print(f"wrote {len(mods)} mods to {OUTPUT}")

if __name__ == "__main__":
    main()
