#pragma once

#include <string>
#include <vector>

struct Sound {
    std::string id;
    std::string name;
    std::string path;
    float volume = 1.0f;
    std::string keybind; // Format: "SUPER + ALT + 1" or empty
};

struct SoundGroup {
    std::string id;
    std::string name;
    std::vector<Sound> sounds;
};

struct AudioDevice {
    std::string name;        // PipeWire/Pulse name e.g. "alsa_input..."
    std::string description; // Human readable description
    bool is_sink = false;    // true = output sink, false = input source
    bool is_default = false;
    bool is_monitor = false;
    std::string state;       // "RUNNING", "IDLE", "SUSPENDED"
    float volume = 1.0f;
    bool mute = false;
};

struct AppConfig {
    std::string active_group_id;
    bool media_to_mic = false;
    bool mic_passthrough = true;
    float monitor_volume = 1.0f;
    float mic_volume = 1.0f;
    std::string physical_mic_device;     // Empty = default input source
    std::string soundboard_output_device; // Empty = default output sink
    std::string desktop_capture_source;  // Empty = default sink monitor
    bool virtual_mic_enabled = true;
    bool mic_muted = false;
    bool monitor_muted = false;
    bool virtual_mic_muted = false;
    float passthrough_volume = 1.0f;
    float virtual_mic_volume = 1.0f;
    std::string stop_all_keybind = "Escape";
    std::string toggle_media_keybind = "SUPER + ALT + M";
    std::vector<SoundGroup> groups;
};
