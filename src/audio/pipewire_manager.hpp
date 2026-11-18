#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

namespace Audio {

class PipeWireManager {
public:
    static PipeWireManager& instance();

    PipeWireManager();
    ~PipeWireManager();

    bool init(const AppConfig &cfg);
    void shutdown();

    // Device enumeration
    void refresh_devices();
    std::vector<AudioDevice> get_sinks(bool force_refresh = false);
    std::vector<AudioDevice> get_sources(bool force_refresh = false);
    std::string get_default_sink_name();
    std::string get_default_source_name();

    // Virtual audio management
    bool set_virtual_mic_enabled(bool enable, const AppConfig &cfg);
    bool is_virtual_mic_enabled() const { return virtual_mic_enabled_; }

    bool set_physical_mic(const std::string &device_name, bool enable);
    bool set_desktop_source(const std::string &source_name, bool enable);
    bool set_mic_passthrough(bool enable);
    bool set_media_to_mic(bool enable);

    bool is_mic_passthrough() const { return mic_loopback_id_ != 0; }
    bool is_media_to_mic() const { return media_loopback_id_ != 0; }

    // Test virtual mic (loopback to user's headphones)
    bool test_virtual_mic(bool enable, const std::string &target_sink = "");
    bool is_virtual_mic_testing() const { return test_loopback_id_ != 0; }

    // Health, diagnostics & auto-repair
    bool check_health(bool &sink_ok, bool &source_ok);
    bool repair(const AppConfig &cfg);

    // Native device volume & mute
    void set_device_volume(const std::string &dev_name, bool is_sink, float volume);
    void set_device_mute(const std::string &dev_name, bool is_sink, bool mute);

    // Virtual pipeline gain & mute controls
    bool set_passthrough_volume(float volume);
    bool set_passthrough_mute(bool mute);
    bool set_virtual_mic_volume(float volume);
    bool set_virtual_mic_mute(bool mute);
    uint32_t find_sink_input_for_module(uint32_t mod_id);

    static void clean_stale_modules();

private:
    uint32_t sink_module_id_ = 0;
    uint32_t source_module_id_ = 0;
    uint32_t mic_loopback_id_ = 0;
    uint32_t media_loopback_id_ = 0;
    uint32_t test_loopback_id_ = 0;

    bool virtual_mic_enabled_ = true;
    std::string current_mic_device_;
    std::string current_desktop_source_;

    std::vector<AudioDevice> cached_sinks_;
    std::vector<AudioDevice> cached_sources_;
    std::mutex dev_mutex_;

    static uint32_t exec_load(const std::string &args);
    static void exec_unload(uint32_t mod_id);
    static std::string exec_cmd(const std::string &cmd);
};

} // namespace Audio
