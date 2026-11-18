#include <cassert>
#include <iostream>
#include "types.hpp"
#include "storage/config_store.hpp"
#include "audio/pipewire_manager.hpp"

int main() {
    setenv("XDG_CONFIG_HOME", "/tmp/antigravity/voxboard_test", 1);
    // 1. Verify default config has audio settings defaults
    AppConfig cfg = Storage::get_default_config();
    assert(cfg.virtual_mic_enabled == true);
    assert(cfg.physical_mic_device.empty());
    assert(cfg.soundboard_output_device.empty());
    assert(cfg.desktop_capture_source.empty());
    assert(cfg.mic_muted == false);
    assert(cfg.monitor_muted == false);
    assert(cfg.virtual_mic_muted == false);
    assert(cfg.stop_all_keybind == "Escape");
    assert(cfg.toggle_media_keybind == "SUPER + ALT + M");

    // 2. Modify audio settings and test round-trip serialization
    cfg.physical_mic_device = "alsa_input.test-mic";
    cfg.soundboard_output_device = "alsa_output.test-headphones";
    cfg.desktop_capture_source = "alsa_output.test-monitor";
    cfg.virtual_mic_enabled = false;
    cfg.mic_muted = true;
    cfg.monitor_muted = true;
    cfg.virtual_mic_muted = true;
    cfg.passthrough_volume = 1.45f;
    cfg.virtual_mic_volume = 0.85f;
    cfg.stop_all_keybind = "SUPER + CTRL + Escape";
    cfg.toggle_media_keybind = "SUPER + SHIFT + M";

    assert(Storage::save_config(cfg));
    AppConfig loaded = Storage::load_config();
    assert(loaded.physical_mic_device == "alsa_input.test-mic");
    assert(loaded.soundboard_output_device == "alsa_output.test-headphones");
    assert(loaded.desktop_capture_source == "alsa_output.test-monitor");
    assert(loaded.virtual_mic_enabled == false);
    assert(loaded.mic_muted == true);
    assert(loaded.monitor_muted == true);
    assert(loaded.virtual_mic_muted == true);
    assert(std::abs(loaded.passthrough_volume - 1.45f) < 0.01f);
    assert(std::abs(loaded.virtual_mic_volume - 0.85f) < 0.01f);
    assert(loaded.stop_all_keybind == "SUPER + CTRL + Escape");
    assert(loaded.toggle_media_keybind == "SUPER + SHIFT + M");

    // 3. Test PipeWire device enumeration
    auto sinks = Audio::PipeWireManager::instance().get_sinks(true);
    auto sources = Audio::PipeWireManager::instance().get_sources(true);
    assert(!sinks.empty());
    assert(!sources.empty());

    // 4. Test virtual mic health checking logic
    bool sink_ok = false, source_ok = false;
    // When virtual mic is disabled in config, health check returns true
    assert(Audio::PipeWireManager::instance().set_virtual_mic_enabled(false, loaded));
    assert(Audio::PipeWireManager::instance().check_health(sink_ok, source_ok));

    // 5. Test stale module cleanup
    Audio::PipeWireManager::clean_stale_modules();

    std::cout << "[Test] Audio settings & device routing test passed.\n";
    return 0;
}
