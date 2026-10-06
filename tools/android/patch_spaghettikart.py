#!/usr/bin/env python3
"""Android-only SpaghettiKart patches applied to pinned upstream during CI.
Abort on upstream changes instead of silently producing an unpatched APK.
"""
from pathlib import Path

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
            float mixed = (static_cast<float>(nas_buffer[i]) +
                           hmas_buffer[i] * 32767.0f) * master_vol;
            if (mixed > 32767.0f) mixed = 32767.0f;
            if (mixed < -32768.0f) mixed = -32768.0f;
            mix_buffer[i] = static_cast<int16_t>(mixed);
        }''')

replace("src/port/ui/PortMenu.cpp",
'''    AddWidget(path, "Renderer API (Needs reload)", WIDGET_VIDEO_BACKEND);

    AddWidget(path, "Internal Resolution: %.0f%%", WIDGET_CVAR_SLIDER_FLOAT)''',
'''    AddWidget(path, "Renderer API (Needs reload)", WIDGET_VIDEO_BACKEND);

#ifdef __ANDROID__
    // The existing 16:9 preset is hidden in the advanced-resolution sidebar.
    // Expose the common modes in the primary graphics settings on phones.
    static const std::unordered_map<int32_t, const char*> androidAspectOptions = {
        { 2, "4:3 (Original)" }, { 3, "16:9 (Widescreen)" }
    };
    AddWidget(path, "Aspect Ratio (4:3 / 16:9)", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_PREFIX_ADVANCED_RESOLUTION ".UIComboItem.AspectRatio")
        .Callback([](WidgetInfo& info) {
            const int mode = CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".UIComboItem.AspectRatio", 2);
            CVarSetInteger(CVAR_LOW_RES_MODE, 0);
            CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", 1);
            CVarSetFloat(CVAR_PREFIX_ADVANCED_RESOLUTION ".AspectRatioX", mode == 3 ? 16.0f : 4.0f);
            CVarSetFloat(CVAR_PREFIX_ADVANCED_RESOLUTION ".AspectRatioY", mode == 3 ? 9.0f : 3.0f);
            CVarSave();
        })
        .Options(ComboboxOptions()
            .ComboMap(androidAspectOptions)
            .DefaultIndex(2)
            .Tooltip("Original 4:3 or true widescreen 16:9 gameplay."));
#endif

    AddWidget(path, "Internal Resolution: %.0f%%", WIDGET_CVAR_SLIDER_FLOAT)''')

print("Android audio + aspect ratio patches complete")
