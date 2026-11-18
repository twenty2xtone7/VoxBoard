#include "hyprland_binds.hpp"
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <climits>
#include <unistd.h>

namespace Keybinds {

HyprlandManager& HyprlandManager::instance() {
    static HyprlandManager inst;
    return inst;
}

HyprlandManager::HyprlandManager() = default;

HyprlandManager::~HyprlandManager() {
    unbind_all();
}

std::string HyprlandManager::get_executable_path() {
    char buf[PATH_MAX] = {0};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        return std::string(buf);
    }
    return "voxboard";
}

bool HyprlandManager::run_hyprctl(const std::string &lua_expr, std::string *error_out) {
    std::string cmd = "hyprctl eval '" + lua_expr + "' 2>&1";
    FILE *fp = popen(cmd.c_str(), "r");
    if (!fp) return false;

    char buf[512];
    std::string out;
    while (fgets(buf, sizeof(buf), fp)) {
        out += buf;
    }
    int ret = pclose(fp);

    if (out.find("error:") != std::string::npos) {
        if (error_out) *error_out = out;
        std::cerr << "[HyprlandManager] Hyprland error: " << out << "\n";
        return false;
    }
    return ret == 0;
}

bool HyprlandManager::bind_sound(const std::string &key_combo, const std::string &sound_id, std::string *error_out) {
    if (key_combo.empty() || sound_id.empty()) return false;
    std::string exe = get_executable_path();
    std::string lua = "hl.bind(\"" + key_combo + "\", hl.dsp.exec_cmd(\"" + exe + " --play " + sound_id + "\"))";
    return run_hyprctl(lua, error_out);
}

bool HyprlandManager::unbind_sound(const std::string &key_combo) {
    if (key_combo.empty()) return false;
    std::string lua = "hl.unbind(\"" + key_combo + "\")";
    return run_hyprctl(lua);
}

bool HyprlandManager::bind_global_action(const std::string &key_combo, const std::string &action_args, std::string *error_out) {
    if (key_combo.empty() || action_args.empty()) return false;
    std::string exe = get_executable_path();
    std::string lua = "hl.bind(\"" + key_combo + "\", hl.dsp.exec_cmd(\"" + exe + " " + action_args + "\"))";
    return run_hyprctl(lua, error_out);
}

bool HyprlandManager::unbind_global_action(const std::string &key_combo) {
    if (key_combo.empty()) return false;
    std::string lua = "hl.unbind(\"" + key_combo + "\")";
    return run_hyprctl(lua);
}

bool HyprlandManager::focus_window_in_hyprland() {
    std::string hypr_focus_lua =
        "local wins = hl.get_windows(); "
        "local ws = hl.get_active_workspace(); "
        "local my_pid = " + std::to_string(getpid()) + "; "
        "for i = 1, #wins do "
        "  local w = wins[i]; "
        "  if w.pid == my_pid then "
        "    pcall(function() hl.dispatch(hl.dsp.window.move({ window = w, workspace = ws })) end); "
        "    pcall(function() hl.dispatch(hl.dsp.window.bring_to_top({ window = w })) end); "
        "    pcall(function() hl.dispatch(hl.dsp.focus({ window = w })) end); "
        "    return true; "
        "  end "
        "end; return false";
    return run_hyprctl(hypr_focus_lua);
}

void HyprlandManager::bind_group(const SoundGroup &group) {
    unbind_sound_binds();
    for (const auto &snd : group.sounds) {
        if (!snd.keybind.empty()) {
            if (bind_sound(snd.keybind, snd.id)) {
                active_sound_binds_.push_back(snd.keybind);
            }
        }
    }
}

void HyprlandManager::bind_global_actions(const AppConfig &config) {
    for (const auto &combo : active_global_binds_) {
        unbind_global_action(combo);
    }
    active_global_binds_.clear();

    if (!config.stop_all_keybind.empty()) {
        if (bind_global_action(config.stop_all_keybind, "--stop")) {
            active_global_binds_.push_back(config.stop_all_keybind);
        }
    }

    if (!config.toggle_media_keybind.empty()) {
        if (bind_global_action(config.toggle_media_keybind, "--media toggle")) {
            active_global_binds_.push_back(config.toggle_media_keybind);
        }
    }
}

void HyprlandManager::unbind_sound_binds() {
    for (const auto &combo : active_sound_binds_) {
        unbind_sound(combo);
    }
    active_sound_binds_.clear();
}

void HyprlandManager::unbind_all() {
    unbind_sound_binds();
    for (const auto &combo : active_global_binds_) {
        unbind_global_action(combo);
    }
    active_global_binds_.clear();
}

} // namespace Keybinds
