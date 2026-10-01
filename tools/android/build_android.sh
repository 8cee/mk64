#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SDL_VERSION="2.30.8"
SDL_JAVA_DIR="$ROOT/android/app/src/main/java/org/libsdl/app"
if [[ ! -f "$SDL_JAVA_DIR/SDLActivity.java" ]]; then
  echo "[android] staging SDL ${SDL_VERSION} Java sources"
  TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
  curl -fsSL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-${SDL_VERSION}.tar.gz" -o "$TMP/sdl.tar.gz"
  tar -xzf "$TMP/sdl.tar.gz" -C "$TMP"
  mkdir -p "$SDL_JAVA_DIR"
  cp "$TMP/SDL2-${SDL_VERSION}/android-project/app/src/main/java/org/libsdl/app/"*.java "$SDL_JAVA_DIR/"
fi
# asset alias generator validated
python3 "$ROOT/tools/android/gen_asset_symbols.py"
python3 "$ROOT/tools/android/gen_course_metadata.py"
RECIPE_TMP="$(mktemp -d)"
curl -fsSL "https://github.com/beckerd/psp_mk64_portable/releases/download/v1.7/MK64Portable-1.7.zip" -o "$RECIPE_TMP/portable.zip"
python3 "$ROOT/tools/android/extract_psp_recipe.py" "$RECIPE_TMP/portable.zip" -o "$RECIPE_TMP/recipes.bin"
python3 "$ROOT/tools/android/analyze_external_relocs.py" "$RECIPE_TMP/recipes.bin"
python3 "$ROOT/tools/android/embed_recipe_blob.py" "$RECIPE_TMP/recipes.bin" "$ROOT/android/app/src/main/cpp/android_recipe_blob.c"
rm -rf "$RECIPE_TMP"
python3 "$ROOT/tools/android/gen_seg_tables.py"
gradle -p "$ROOT/android" --no-daemon :app:assembleDebug
APK="$ROOT/android/app/build/outputs/apk/debug/app-debug.apk"
test -f "$APK"
echo "[android] built $APK"
