#!/usr/bin/env python3
"""Android-only SpaghettiKart patches applied to pinned upstream during CI.
Abort on upstream changes instead of silently producing an unpatched APK.
"""
from pathlib import Path
import shutil

root = Path(__file__).resolve().parents[2] / "runtime"
def replace(path, before, after):
    file = root / path
    data = file.read_text()
    if data.count(before) != 1:
        raise RuntimeError(f"{path}: expected 1 match; found {data.count(before)}")
    file.write_text(data.replace(before, after, 1))
    print("PATCHED", path)

# Keep a deeper output queue on Android under occasional graphics/GC stalls.
# Higher queue threshold avoids dropping a complete audio block when an
# Android frame is delayed, at the cost of some additional audio latency.
replace("libultraship/src/ship/audio/SDLAudioPlayer.cpp",
'''    if (Buffered() < 6000) {
        // Don't fill the audio buffer too much in case this happens
        SDL_QueueAudio(mDevice, buf, len);
    }''',
'''#ifdef __ANDROID__
    constexpr int kMaxQueuedFrames = 12288;
#else
    constexpr int kMaxQueuedFrames = 6000;
#endif
    if (Buffered() < kMaxQueuedFrames) {
        SDL_QueueAudio(mDevice, buf, len);
    }''')

replace("src/port/Engine.cpp",
'''    if (!audio.running) {
        audio.running = true;''',
'''#ifdef __ANDROID__
    // Keep roughly 93ms at 44.1kHz ahead of SDL on loaded mobile GPUs.
    auto player = Ship::Context::GetRawInstance()->GetAudio()->GetAudioPlayer();
    if (player != nullptr) {
        player->SetDesiredBuffered(4096);
    }
#endif
    if (!audio.running) {
        audio.running = true;''')

replace("src/port/Engine.cpp",
'''        for (size_t i = 0; i < SAMPLES_PER_FRAME; i++) {
            mix_buffer[i] = nas_buffer[i] + ((int16_t)(hmas_buffer[i] * 32767.0f) * master_vol);
        }''',
'''        // Saturating mix: never wrap signed 16-bit PCM when game samples
        // and enhanced music peak together. Apply the master gain once
        // to the complete signal rather than only to HMAS.
        const size_t validSamples = num_audio_samples * NUM_AUDIO_CHANNELS * 2;
        for (size_t i = 0; i < validSamples; i++) {
            // Leave real headroom for N64 + enhanced audio summing. The
            // previous saturating-only mix still spent too much time at
            // full-scale on Android and produced audible crackle.
            float mixed = (static_cast<float>(nas_buffer[i]) * 0.72f +
                           hmas_buffer[i] * 32767.0f * 0.62f) * master_vol;
            if (mixed > 32767.0f) mixed = 32767.0f;
            if (mixed < -32768.0f) mixed = -32768.0f;
            mix_buffer[i] = static_cast<int16_t>(mixed);
        }''')

# Android typically has a native 48 kHz output path. The old 26800 Hz
# SDL device rate is resampled by Android, while a fixed 896-frame
# submission cadence leaves no margin for audio scheduling jitter.
# Generate 1600 frames per 30 Hz game tick at 48 kHz instead.
replace("src/port/Engine.h",
'''#define SAMPLES_HIGH 448
#define SAMPLES_LOW 432''',
'''#ifdef __ANDROID__
#define SAMPLES_HIGH 800
#define SAMPLES_LOW 784
#else
#define SAMPLES_HIGH 448
#define SAMPLES_LOW 432
#endif''')

replace("src/port/Engine.cpp",
'''this->context->Init({assets_path}, {}, 3, { 26800, 512, 1100 }, wnd, controlDeck);''',
'''#ifdef __ANDROID__
    this->context->Init({assets_path}, {}, 3, { 48000, 1024, 3200 }, wnd, controlDeck);
#else
    this->context->Init({assets_path}, {}, 3, { 26800, 512, 1100 }, wnd, controlDeck);
#endif''')

replace("libultraship/src/ship/audio/SDLAudioPlayer.cpp",
'''    SDL_PauseAudioDevice(mDevice, 0);
    return true;''',
'''#ifdef __ANDROID__
    // Prime with 64ms silence so the first frame and brief UI stalls do
    // not immediately empty Android's hardware output FIFO.
    const size_t silenceSize = 3072 * mNumChannels * sizeof(int16_t);
    std::vector<uint8_t> silence(silenceSize, 0);
    SDL_QueueAudio(mDevice, silence.data(), static_cast<Uint32>(silenceSize));
#endif
    SDL_PauseAudioDevice(mDevice, 0);
    return true;''')

replace("libultraship/src/ship/audio/SDLAudioPlayer.cpp",
'''#include <spdlog/spdlog.h>''',
'''#include <spdlog/spdlog.h>
#include <vector>''')

# Android graphics: expose and default a real 16:9 render path.
replace("src/port/ui/PortMenu.cpp",
'''    AddWidget(path, "Renderer API (Needs reload)", WIDGET_VIDEO_BACKEND);

    AddWidget(path, "Internal Resolution: %.0f%%", WIDGET_CVAR_SLIDER_FLOAT)''',
'''    AddWidget(path, "Renderer API (Needs reload)", WIDGET_VIDEO_BACKEND);

#ifdef __ANDROID__
    AddWidget(path, "Widescreen 16:9", WIDGET_CVAR_CHECKBOX)
        .CVar("gAndroidWidescreen16x9")
        .Callback([](WidgetInfo& info) {
            const bool enabled = CVarGetInteger("gAndroidWidescreen16x9", 1) != 0;
            CVarSetInteger("gAndroidDisplayMode", enabled ? 3 : 2);
            CVarSetInteger(CVAR_LOW_RES_MODE, 0);
            CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", 1);
            CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".PixelPerfectMode", 0);
            CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalResolutionToggle", 0);
            CVarSetFloat(CVAR_PREFIX_ADVANCED_RESOLUTION ".AspectRatioX", enabled ? 16.0f : 4.0f);
            CVarSetFloat(CVAR_PREFIX_ADVANCED_RESOLUTION ".AspectRatioY", enabled ? 9.0f : 3.0f);
            CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".UIComboItem.AspectRatio", enabled ? 3 : 2);
            CVarSave();
        })
        .Options(CheckboxOptions()
            .DefaultValue(true)
            .Tooltip("Render gameplay at true 16:9. Disable for original 4:3."));
#endif

    AddWidget(path, "Internal Resolution: %.0f%%", WIDGET_CVAR_SLIDER_FLOAT)''')

replace("src/port/Engine.cpp",
'''    this->context->InitConsoleVariables(); // without this line the controldeck constructor failes in
                                           // ShipDeviceIndexMappingManager::UpdateControllerNamesFromConfig()
''',
'''    this->context->InitConsoleVariables(); // without this line the controldeck constructor failes in
                                           // ShipDeviceIndexMappingManager::UpdateControllerNamesFromConfig()
#ifdef __ANDROID__
    // Migrate older Android builds which defaulted to locked 4:3 and
    // ensure aspect settings apply before the first rendered game frame.
    int displayMode = CVarGetInteger("gAndroidDisplayMode", -1);
    if (displayMode != 2 && displayMode != 3) {
        displayMode = 3; // Force true 16:9 widescreen by default on Android.
    }
    CVarSetInteger("gAndroidDisplayMode", displayMode);
    CVarSetInteger("gAndroidWidescreen16x9", displayMode == 3 ? 1 : 0);
    CVarSetInteger(CVAR_LOW_RES_MODE, 0);
    CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", displayMode != 0);
    CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".PixelPerfectMode", 0);
    CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalResolutionToggle", 0);
    if (displayMode != 0) {
        CVarSetFloat(CVAR_PREFIX_ADVANCED_RESOLUTION ".AspectRatioX", displayMode == 3 ? 16.0f : 4.0f);
        CVarSetFloat(CVAR_PREFIX_ADVANCED_RESOLUTION ".AspectRatioY", displayMode == 3 ? 9.0f : 3.0f);
    }
#endif
''')

# An always-visible touch Settings control remains available even when
# virtual gamepad controls are hidden. Do not trap the menu in button_group.
replace("android/app/src/main/res/layout/touchcontrol_overlay.xml",
'''    <!-- Menu Button (Escape) -->''',
'''</androidx.constraintlayout.widget.ConstraintLayout>

    <!-- Settings remains visible when the virtual controls are hidden. -->
    <!-- Menu Button (Escape) -->''')
replace("android/app/src/main/res/layout/touchcontrol_overlay.xml",
'''        android:layout_width="60dp"
        android:layout_height="35dp"
        android:layout_marginTop="20dp"
        android:background="@drawable/ic_rectangular_button"
        android:text="Menu"''',
'''        android:layout_width="84dp"
        android:layout_height="40dp"
        android:layout_marginTop="16dp"
        android:background="@drawable/ic_rectangular_button"
        android:text="Settings"''')
replace("android/app/src/main/res/layout/touchcontrol_overlay.xml",
'''</androidx.constraintlayout.widget.ConstraintLayout>

    <!-- Toggle Button -->''',
'''    <!-- Toggle Button -->''')
replace("src/port/ui/PortMenu.cpp",
'''    AddSidebarEntry("Settings", "Controls", 1);
    AddWidget(path,
''',
'''    AddSidebarEntry("Settings", "Controls", 1);
#ifdef __ANDROID__
    AddWidget(path, "Back to Graphics / Settings", WIDGET_BUTTON)
        .Callback([](WidgetInfo& info) {
            CVarSetString("gSettings.Menu.SettingsSidebarSection", "Graphics");
            CVarSave();
        })
        .Options(ButtonOptions().Tooltip("Return to Graphics to adjust display options."));
#endif
    AddWidget(path,
''')

print("Android audio + aspect ratio patches complete")


# Built-in remote mod browser. Keep the UI client in our integration tree and
# copy it into the pinned SpaghettiKart Android source during CI.
mod_browser_src = Path(__file__).resolve().parent / "mod_browser"
mod_browser_dst = root / "android/app/src/main/java/com/izzy/kart"
for filename in ("RemoteModRepository.kt", "RemoteModsActivity.kt"):
    shutil.copyfile(mod_browser_src / filename, mod_browser_dst / filename)
    print("COPIED", filename)

replace("android/app/src/main/AndroidManifest.xml",
'''<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    android:installLocation="auto">

    <!-- OpenGL ES 3.0 -->''',
'''<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    android:installLocation="auto">

    <uses-permission android:name="android.permission.INTERNET" />

    <!-- OpenGL ES 3.0 -->''')

replace("android/app/src/main/AndroidManifest.xml",
'''        <activity
            android:name=".ModsActivity"''',
'''        <activity
            android:name=".RemoteModsActivity"
            android:exported="false"
            android:label="Online Mods"
            android:screenOrientation="sensorLandscape"
            android:theme="@android:style/Theme.Material.NoActionBar" />

        <activity
            android:name=".ModsActivity"''')

replace("android/app/src/main/java/com/izzy/kart/ModsActivity.kt",
'''        actions.addView(button("Import mod") { importArchives.launch(arrayOf("*/*")) })''',
'''        actions.addView(button("Browse online") {
            startActivity(Intent(this, RemoteModsActivity::class.java))
        })
        actions.addView(button("Import mod") { importArchives.launch(arrayOf("*/*")) })''')

replace("android/app/src/main/java/com/izzy/kart/ModStore.kt",
'''    /** A folder mod leaves as a .zip, which the engine loads just the same. */
    fun exportName(mod: Mod): String = if (mod.isFolder) "${mod.name}.zip" else mod.name

    private const val COPY_BUFFER = 1 shl 16''',
'''    /** A folder mod leaves as a .zip, which the engine loads just the same. */
    fun exportName(mod: Mod): String = if (mod.isFolder) "${mod.name}.zip" else mod.name

    /** Installs an already-downloaded remote archive into the managed load order. */
    fun importDownloadedFile(context: Context, source: File, displayName: String): String? {
        if (!displayName.endsWith(".o2r", true) && !displayName.endsWith(".zip", true)) {
            return "$displayName is not a supported mod archive."
        }
        val target = nextFreeFile(context, displayName)
        return try {
            source.inputStream().use { input ->
                FileOutputStream(target).use { output -> input.copyTo(output, COPY_BUFFER) }
            }
            null
        } catch (error: Exception) {
            target.delete()
            Log.e(TAG, "Remote install of $displayName failed", error)
            "Could not install $displayName: ${error.message ?: error}"
        }
    }

    private const val COPY_BUFFER = 1 shl 16''')

print("Android remote mod browser patches complete")

# N64 controller cleanup: the touch overlay must not expose Xbox-style X/Y
# face buttons. MK64 uses A/B plus the four C buttons.
replace("android/app/src/main/res/layout/touchcontrol_overlay.xml",
'''    <!-- Action Buttons (A, B, X, Y) -->''',
'''    <!-- N64 Action Buttons (A, B) -->''')

replace("android/app/src/main/res/layout/touchcontrol_overlay.xml",
'''    <Button
        android:id="@+id/buttonY"
        android:layout_width="48dp"
        android:layout_height="48dp"
        android:background="@drawable/ic_button"
        android:text="Y"
        app:layout_constraintBottom_toTopOf="@id/buttonB"
        app:layout_constraintEnd_toStartOf="@id/buttonB"
        android:textColor="#25FFFFFF"/>

    <Button
        android:id="@+id/buttonX"
        android:layout_width="48dp"
        android:layout_height="48dp"
        android:background="@drawable/ic_button"
        android:text="X"
        app:layout_constraintBottom_toTopOf="@id/buttonA"
        app:layout_constraintEnd_toStartOf="@id/buttonA"
        android:textColor="#25FFFFFF"/>

''',
'''''')

replace("android/app/src/main/java/com/izzy/kart/MainActivity.kt",
'''            ControllerButtons.BUTTON_A, ControllerButtons.BUTTON_B,
            ControllerButtons.BUTTON_X, ControllerButtons.BUTTON_Y,
            ControllerButtons.BUTTON_LB, ControllerButtons.BUTTON_RB,''',
'''            ControllerButtons.BUTTON_A, ControllerButtons.BUTTON_B,
            ControllerButtons.BUTTON_LB, ControllerButtons.BUTTON_RB,''')

replace("android/app/src/main/java/com/izzy/kart/MainActivity.kt",
'''        overlay.bindButton(R.id.buttonA, ControllerButtons.BUTTON_A)
        overlay.bindButton(R.id.buttonB, ControllerButtons.BUTTON_B)
        overlay.bindButton(R.id.buttonX, ControllerButtons.BUTTON_X)
        overlay.bindButton(R.id.buttonY, ControllerButtons.BUTTON_Y)
        overlay.bindButton(R.id.buttonLB, ControllerButtons.BUTTON_LB)''',
'''        overlay.bindButton(R.id.buttonA, ControllerButtons.BUTTON_A)
        overlay.bindButton(R.id.buttonB, ControllerButtons.BUTTON_B)
        overlay.bindButton(R.id.buttonLB, ControllerButtons.BUTTON_LB)''')

print("N64 touch controller cleanup complete")

