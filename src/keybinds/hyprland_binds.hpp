#pragma once

#include "types.hpp"
#include <string>
#include <vector>

namespace Keybinds {

class HyprlandManager {
public:
    static HyprlandManager& instance();

    HyprlandManager();
    ~HyprlandManager();

    void bind_group(const SoundGroup &group);
    void bind_global_actions(const AppConfig &config);
    void unbind_sound_binds();
    void unbind_all();

    bool bind_sound(const std::string &key_combo, const std::string &sound_id, std::string *error_out = nullptr);
    bool unbind_sound(const std::string &key_combo);

    bool bind_global_action(const std::string &key_combo, const std::string &action_args, std::string *error_out = nullptr);
    bool unbind_global_action(const std::string &key_combo);

    // Legacy aliases
    bool bind_key(const std::string &key_combo, const std::string &sound_id) {
        return bind_sound(key_combo, sound_id);
    }
    bool unbind_key(const std::string &key_combo) {
        return unbind_sound(key_combo);
    }

    static std::string get_executable_path();
    bool focus_window_in_hyprland();

    const std::vector<std::string>& get_active_sound_binds() const { return active_sound_binds_; }
    const std::vector<std::string>& get_active_global_binds() const { return active_global_binds_; }

private:
    std::vector<std::string> active_sound_binds_;
    std::vector<std::string> active_global_binds_;
    bool run_hyprctl(const std::string &lua_expr, std::string *error_out = nullptr);
};

} // namespace Keybinds
