#include "pipewire_manager.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <yyjson.h>

namespace Audio {

PipeWireManager& PipeWireManager::instance() {
    static PipeWireManager inst;
    return inst;
}

PipeWireManager::PipeWireManager() = default;

PipeWireManager::~PipeWireManager() {
    shutdown();
}

std::string PipeWireManager::exec_cmd(const std::string &cmd) {
    FILE *fp = popen(cmd.c_str(), "r");
    if (!fp) return "";
    char buf[512];
    std::string out;
    while (fgets(buf, sizeof(buf), fp)) {
        out += buf;
    }
    pclose(fp);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) {
        out.pop_back();
    }
    return out;
}

uint32_t PipeWireManager::exec_load(const std::string &args) {
    std::string cmd = "pactl load-module " + args + " 2>/dev/null";
    FILE *fp = popen(cmd.c_str(), "r");
    if (!fp) return 0;

    char buf[64] = {0};
    uint32_t id = 0;
    if (fgets(buf, sizeof(buf), fp)) {
        id = (uint32_t)std::strtoul(buf, nullptr, 10);
    }
    pclose(fp);
    return id;
}

void PipeWireManager::exec_unload(uint32_t mod_id) {
    if (mod_id == 0) return;
    std::string cmd = "pactl unload-module " + std::to_string(mod_id) + " 2>/dev/null";
    system(cmd.c_str());
}

void PipeWireManager::clean_stale_modules() {
    FILE *fp = popen("pactl list modules short 2>/dev/null", "r");
    if (!fp) return;

    char line[512];
    std::vector<uint32_t> to_unload;
    while (fgets(line, sizeof(line), fp)) {
        std::string s(line);
        if (s.find("VoxBoard") != std::string::npos || s.find("voxboard") != std::string::npos) {
            std::stringstream ss(s);
            uint32_t id = 0;
            if (ss >> id) {
                to_unload.push_back(id);
            }
        }
    }
    pclose(fp);

    std::reverse(to_unload.begin(), to_unload.end());
    for (uint32_t id : to_unload) {
        exec_unload(id);
    }
}

std::string PipeWireManager::get_default_sink_name() {
    return exec_cmd("pactl get-default-sink 2>/dev/null");
}

std::string PipeWireManager::get_default_source_name() {
    return exec_cmd("pactl get-default-source 2>/dev/null");
}

static std::vector<AudioDevice> parse_pactl_devices(const std::string &json_str, bool sinks, const std::string &def_name) {
    std::vector<AudioDevice> list;
    if (json_str.empty()) return list;

    yyjson_doc *doc = yyjson_read(json_str.c_str(), json_str.length(), 0);
    if (!doc) return list;

    yyjson_val *root = yyjson_doc_get_root(doc);
    if (yyjson_is_arr(root)) {
        size_t idx, max;
        yyjson_val *item;
        yyjson_arr_foreach(root, idx, max, item) {
            AudioDevice dev;
            dev.is_sink = sinks;

            yyjson_val *v = yyjson_obj_get(item, "name");
            if (v && yyjson_is_str(v)) dev.name = yyjson_get_str(v);

            v = yyjson_obj_get(item, "description");
            if (v && yyjson_is_str(v)) dev.description = yyjson_get_str(v);

            v = yyjson_obj_get(item, "state");
            if (v && yyjson_is_str(v)) dev.state = yyjson_get_str(v);

            v = yyjson_obj_get(item, "mute");
            if (v && yyjson_is_bool(v)) dev.mute = yyjson_get_bool(v);

            if (dev.name.find(".monitor") != std::string::npos) {
                dev.is_monitor = true;
            }
            if (dev.name == def_name) {
                dev.is_default = true;
            }
            list.push_back(dev);
        }
    }
    yyjson_doc_free(doc);
    return list;
}

void PipeWireManager::refresh_devices() {
    std::lock_guard<std::mutex> lock(dev_mutex_);
    std::string def_sink = get_default_sink_name();
    std::string def_source = get_default_source_name();

    std::string sinks_json = exec_cmd("pactl -f json list sinks 2>/dev/null");
    cached_sinks_ = parse_pactl_devices(sinks_json, true, def_sink);

    std::string sources_json = exec_cmd("pactl -f json list sources 2>/dev/null");
    cached_sources_ = parse_pactl_devices(sources_json, false, def_source);
}

std::vector<AudioDevice> PipeWireManager::get_sinks(bool force_refresh) {
    if (force_refresh || cached_sinks_.empty()) {
        refresh_devices();
    }
    std::lock_guard<std::mutex> lock(dev_mutex_);
    return cached_sinks_;
}

std::vector<AudioDevice> PipeWireManager::get_sources(bool force_refresh) {
    if (force_refresh || cached_sources_.empty()) {
        refresh_devices();
    }
    std::lock_guard<std::mutex> lock(dev_mutex_);
    return cached_sources_;
}

bool PipeWireManager::init(const AppConfig &cfg) {
    virtual_mic_enabled_ = cfg.virtual_mic_enabled;
    current_mic_device_ = cfg.physical_mic_device;
    current_desktop_source_ = cfg.desktop_capture_source;

    clean_stale_modules();

    if (!virtual_mic_enabled_) {
        std::cout << "[Audio] Virtual microphone is disabled in configuration.\n";
        refresh_devices();
        return true;
    }

    // 1. Create null-sink mixer
    sink_module_id_ = exec_load("module-null-sink sink_name=VoxBoard_Sink sink_properties=device.description=\"VoxBoard_Virtual_Sink\"");
    if (sink_module_id_ == 0) {
        std::cerr << "[Audio] Failed to create VoxBoard_Sink null-sink\n";
        return false;
    }

    // 2. Create remap-source (visible to Discord/games as regular microphone)
    source_module_id_ = exec_load("module-remap-source master=VoxBoard_Sink.monitor source_name=VoxBoard_Mic source_properties=device.description=\"VoxBoard_Virtual_Microphone\"");
    if (source_module_id_ == 0) {
        std::cerr << "[Audio] Failed to create VoxBoard_Mic remap-source\n";
        shutdown();
        return false;
    }

    // 3. Physical mic passthrough
    if (cfg.mic_passthrough) {
        set_physical_mic(cfg.physical_mic_device, true);
        set_passthrough_volume(cfg.passthrough_volume);
        set_passthrough_mute(cfg.mic_muted);
    }

    // 4. Media-to-Mic loopback
    if (cfg.media_to_mic) {
        set_desktop_source(cfg.desktop_capture_source, true);
    }

    // 5. Set virtual microphone master gain & mute
    set_virtual_mic_volume(cfg.virtual_mic_volume);
    set_virtual_mic_mute(cfg.virtual_mic_muted);

    refresh_devices();
    std::cout << "[Audio] PipeWire virtual devices initialized successfully\n";
    return true;
}

void PipeWireManager::shutdown() {
    if (test_loopback_id_ != 0) {
        exec_unload(test_loopback_id_);
        test_loopback_id_ = 0;
    }
    if (media_loopback_id_ != 0) {
        exec_unload(media_loopback_id_);
        media_loopback_id_ = 0;
    }
    if (mic_loopback_id_ != 0) {
        exec_unload(mic_loopback_id_);
        mic_loopback_id_ = 0;
    }
    if (source_module_id_ != 0) {
        exec_unload(source_module_id_);
        source_module_id_ = 0;
    }
    if (sink_module_id_ != 0) {
        exec_unload(sink_module_id_);
        sink_module_id_ = 0;
    }
}

bool PipeWireManager::set_virtual_mic_enabled(bool enable, const AppConfig &cfg) {
    if (virtual_mic_enabled_ == enable) return true;
    virtual_mic_enabled_ = enable;

    if (!enable) {
        shutdown();
        refresh_devices();
        return true;
    } else {
        return init(cfg);
    }
}

bool PipeWireManager::set_physical_mic(const std::string &device_name, bool enable) {
    current_mic_device_ = device_name;
    if (mic_loopback_id_ != 0) {
        exec_unload(mic_loopback_id_);
        mic_loopback_id_ = 0;
    }
    if (!enable || !virtual_mic_enabled_) return true;

    std::string src = device_name.empty() ? "@DEFAULT_SOURCE@" : device_name;
    mic_loopback_id_ = exec_load("module-loopback source=" + src + " sink=VoxBoard_Sink latency_msec=10");
    return mic_loopback_id_ != 0;
}

bool PipeWireManager::set_desktop_source(const std::string &source_name, bool enable) {
    current_desktop_source_ = source_name;
    if (media_loopback_id_ != 0) {
        exec_unload(media_loopback_id_);
        media_loopback_id_ = 0;
    }
    if (!enable || !virtual_mic_enabled_) return true;

    std::string src = source_name.empty() ? "@DEFAULT_SINK@.monitor" : source_name;
    media_loopback_id_ = exec_load("module-loopback source=" + src + " sink=VoxBoard_Sink latency_msec=10");
    return media_loopback_id_ != 0;
}

bool PipeWireManager::set_mic_passthrough(bool enable) {
    return set_physical_mic(current_mic_device_, enable);
}

bool PipeWireManager::set_media_to_mic(bool enable) {
    return set_desktop_source(current_desktop_source_, enable);
}

bool PipeWireManager::test_virtual_mic(bool enable, const std::string &target_sink) {
    if (test_loopback_id_ != 0) {
        exec_unload(test_loopback_id_);
        test_loopback_id_ = 0;
    }
    if (!enable || !virtual_mic_enabled_) return true;

    std::string sink = target_sink.empty() ? "@DEFAULT_SINK@" : target_sink;
    test_loopback_id_ = exec_load("module-loopback source=VoxBoard_Sink.monitor sink=" + sink + " latency_msec=20");
    return test_loopback_id_ != 0;
}

bool PipeWireManager::check_health(bool &sink_ok, bool &source_ok) {
    if (!virtual_mic_enabled_) {
        sink_ok = true;
        source_ok = true;
        return true;
    }
    static auto last_check = std::chrono::steady_clock::now() - std::chrono::seconds(10);
    static bool cached_sink = false;
    static bool cached_source = false;
    static bool cached_res = false;

    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_check).count() < 2) {
        sink_ok = cached_sink;
        source_ok = cached_source;
        return cached_res;
    }
    last_check = now;

    std::string sinks = exec_cmd("pactl list sinks short 2>/dev/null");
    std::string sources = exec_cmd("pactl list sources short 2>/dev/null");
    cached_sink = (sinks.find("VoxBoard_Sink") != std::string::npos);
    cached_source = (sources.find("VoxBoard_Mic") != std::string::npos);
    cached_res = cached_sink && cached_source;
    sink_ok = cached_sink;
    source_ok = cached_source;
    return cached_res;
}

bool PipeWireManager::repair(const AppConfig &cfg) {
    std::cout << "[Audio] Repairing PipeWire virtual devices...\n";
    shutdown();
    clean_stale_modules();
    return init(cfg);
}

void PipeWireManager::set_device_volume(const std::string &dev_name, bool is_sink, float volume) {
    int pct = std::clamp((int)(volume * 100.0f), 0, 150);
    std::string target = dev_name.empty() ? (is_sink ? "@DEFAULT_SINK@" : "@DEFAULT_SOURCE@") : dev_name;
    std::string cmd = is_sink ? ("pactl set-sink-volume " + target + " " + std::to_string(pct) + "% 2>/dev/null")
                              : ("pactl set-source-volume " + target + " " + std::to_string(pct) + "% 2>/dev/null");
    system(cmd.c_str());
}

void PipeWireManager::set_device_mute(const std::string &dev_name, bool is_sink, bool mute) {
    std::string target = dev_name.empty() ? (is_sink ? "@DEFAULT_SINK@" : "@DEFAULT_SOURCE@") : dev_name;
    std::string cmd = is_sink ? ("pactl set-sink-mute " + target + " " + (mute ? "1" : "0") + " 2>/dev/null")
                              : ("pactl set-source-mute " + target + " " + (mute ? "1" : "0") + " 2>/dev/null");
    system(cmd.c_str());
}

uint32_t PipeWireManager::find_sink_input_for_module(uint32_t mod_id) {
    if (mod_id == 0) return 0;
    std::string json_str = exec_cmd("pactl -f json list sink-inputs 2>/dev/null");
    if (json_str.empty()) return 0;

    yyjson_doc *doc = yyjson_read(json_str.c_str(), json_str.length(), 0);
    if (!doc) return 0;

    uint32_t found_idx = 0;
    std::string mod_str = std::to_string(mod_id);

    yyjson_val *root = yyjson_doc_get_root(doc);
    if (yyjson_is_arr(root)) {
        size_t idx, max;
        yyjson_val *item;
        yyjson_arr_foreach(root, idx, max, item) {
            yyjson_val *props = yyjson_obj_get(item, "properties");
            if (!props) continue;

            yyjson_val *mod_val = yyjson_obj_get(props, "pulse.module.id");
            if (mod_val && yyjson_is_str(mod_val)) {
                if (mod_str == yyjson_get_str(mod_val)) {
                    yyjson_val *index_val = yyjson_obj_get(item, "index");
                    if (index_val && yyjson_is_num(index_val)) {
                        found_idx = (uint32_t)yyjson_get_uint(index_val);
                        break;
                    }
                }
            }
        }
    }
    yyjson_doc_free(doc);
    return found_idx;
}

bool PipeWireManager::set_passthrough_volume(float volume) {
    uint32_t idx = find_sink_input_for_module(mic_loopback_id_);
    if (idx == 0) return false;
    int pct = std::clamp((int)(volume * 100.0f), 0, 200);
    std::string cmd = "pactl set-sink-input-volume " + std::to_string(idx) + " " + std::to_string(pct) + "% 2>/dev/null";
    system(cmd.c_str());
    return true;
}

bool PipeWireManager::set_passthrough_mute(bool mute) {
    uint32_t idx = find_sink_input_for_module(mic_loopback_id_);
    if (idx == 0) return false;
    std::string cmd = "pactl set-sink-input-mute " + std::to_string(idx) + " " + (mute ? "1" : "0") + " 2>/dev/null";
    system(cmd.c_str());
    return true;
}

bool PipeWireManager::set_virtual_mic_volume(float volume) {
    int pct = std::clamp((int)(volume * 100.0f), 0, 200);
    std::string cmd1 = "pactl set-source-volume VoxBoard_Mic " + std::to_string(pct) + "% 2>/dev/null";
    std::string cmd2 = "pactl set-sink-volume VoxBoard_Sink " + std::to_string(pct) + "% 2>/dev/null";
    system(cmd1.c_str());
    system(cmd2.c_str());
    return true;
}

bool PipeWireManager::set_virtual_mic_mute(bool mute) {
    std::string cmd1 = "pactl set-source-mute VoxBoard_Mic " + std::string(mute ? "1" : "0") + " 2>/dev/null";
    std::string cmd2 = "pactl set-sink-mute VoxBoard_Sink " + std::string(mute ? "1" : "0") + " 2>/dev/null";
    system(cmd1.c_str());
    system(cmd2.c_str());
    return true;
}

} // namespace Audio
