#pragma once

#include "types.hpp"
#include "net/myinstants.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <functional>
#include <future>
#include <chrono>
#include <unordered_map>

struct GLFWwindow;

namespace UI {

namespace Theme {
    void apply();

    // Semantic palette for consistent status and telemetry indicators
    constexpr ImVec4 TextHeader   = ImVec4(0.70f, 0.68f, 0.92f, 1.00f); // Muted violet accent
    constexpr ImVec4 TextSubhead  = ImVec4(0.85f, 0.72f, 0.45f, 1.00f); // Warm gold accent
    constexpr ImVec4 Success      = ImVec4(0.35f, 0.80f, 0.52f, 1.00f); // Balanced emerald
    constexpr ImVec4 Warning      = ImVec4(0.88f, 0.68f, 0.28f, 1.00f); // Amber
    constexpr ImVec4 Danger       = ImVec4(0.88f, 0.38f, 0.38f, 1.00f); // Coral red
    constexpr ImVec4 KeyCapture   = ImVec4(0.82f, 0.54f, 0.16f, 1.00f); // Amber listening state
    constexpr ImVec4 GroupActive  = ImVec4(0.38f, 0.35f, 0.55f, 1.00f); // Active sound group violet
    constexpr ImVec4 SurfaceDeep  = ImVec4(0.09f, 0.09f, 0.12f, 1.00f); // Frame child container
    constexpr ImVec4 MeterMuted   = ImVec4(0.30f, 0.30f, 0.35f, 1.00f); // Muted audio meter
    constexpr ImVec4 SoundMeter   = ImVec4(0.56f, 0.52f, 0.82f, 0.85f); // Soundboard per-sound visualizer
}

struct UIEvents {
    std::function<void(const std::string&, float)> on_play_sound;
    std::function<void(const std::string&, float)> on_play_preview;
    std::function<void()> on_stop_all;
    std::function<void(bool)> on_toggle_media;
    std::function<void(bool)> on_toggle_mic;
    std::function<void(float)> on_monitor_volume;
    std::function<void(float)> on_mic_volume;
    std::function<void(const std::string&)> on_select_group;
    std::function<void()> on_config_modified;

    // Audio Settings & Virtual Mic Callbacks
    std::function<void(const std::string&)> on_change_mic_device;
    std::function<void(const std::string&)> on_change_output_device;
    std::function<void(const std::string&)> on_change_desktop_source;
    std::function<void(bool)> on_toggle_virtual_mic;
    std::function<void(bool)> on_test_virtual_mic;
    std::function<void(bool)> on_toggle_monitor_mute;
    std::function<void(bool)> on_toggle_real_mic_mute;
    std::function<void(bool)> on_toggle_virtual_mic_mute;
    std::function<void(float)> on_passthrough_volume;
    std::function<void(float)> on_virtual_mic_volume;
    std::function<void()> on_repair_pipewire;

    // Global Keybind Callbacks
    std::function<void(const std::string&)> on_change_stop_all_bind;
    std::function<void(const std::string&)> on_change_toggle_media_bind;
};

class UIManager {
public:
    static UIManager& instance();

    UIManager();
    ~UIManager();

    bool init(GLFWwindow *window, const UIEvents &events);
    void shutdown();

    void render(AppConfig &config);

    void set_listening_sound(const std::string &id) {
        listen_target_ = ListenTarget::Sound;
        listening_sound_id_ = id;
    }
    const std::string& get_listening_sound() const { return listening_sound_id_; }

    enum class ListenTarget {
        None,
        Sound,
        StopAll,
        ToggleMedia
    };

private:
    void render_header(AppConfig &config);
    void render_soundboard(AppConfig &config);
    void render_myinstants(AppConfig &config);
    void render_audio_settings(AppConfig &config);
    void render_virtual_mic_setup(AppConfig &config);
    void render_diagnostics(AppConfig &config);
    void render_global_keybinds(AppConfig &config);

    void render_level_meter(const char *label, float level, bool muted, float volume = 1.0f);
    void render_routing_graph(const AppConfig &config);
    void handle_key_capture(AppConfig &config);

    GLFWwindow *window_ = nullptr;
    UIEvents events_;

    ListenTarget listen_target_ = ListenTarget::None;
    std::string listening_sound_id_;
    std::string conflict_warning_;

    // MyInstants search state
    char search_buf_[256] = {0};
    int current_page_ = 1;
    bool searching_ = false;
    Net::SearchResult search_results_;
    std::string status_message_;
    std::string preview_playing_slug_;

    // New group / sound dialog state
    char new_group_buf_[128] = {0};
    char new_sound_name_[128] = {0};
    char new_sound_path_[512] = {0};
    bool show_add_sound_modal_ = false;
    bool initialized_ = false;

    // Device refresh timer & testing state
    std::chrono::steady_clock::time_point last_device_refresh_;
    std::chrono::steady_clock::time_point test_mic_start_time_;
    bool is_testing_mic_ = false;
};

} // namespace UI
