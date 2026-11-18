#pragma once

#include "types.hpp"
#include "net/myinstants.hpp"
#include <string>
#include <vector>
#include <functional>
#include <future>
#include <chrono>

struct GLFWwindow;

namespace UI {

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
