#include "hyprland_binds.hpp"
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <climits>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <cstring>
#include <chrono>

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

// ponytail: Hyprland's own IPC socket speaks the same "eval" command as hyprctl.
// Talking to it directly avoids fork+exec (and the /bin/sh wrapper) on every
// bind/unbind, which otherwise floods the process table with short-lived
// "hyprctl eval hl.bind(...)" and "sh -c ..." processes.
static std::string hyprland_socket_path() {
    const char *runtime = std::getenv("XDG_RUNTIME_DIR");
    const char *sig = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (!runtime || !sig) return "";
    return std::string(runtime) + "/hypr/" + sig + "/.socket.sock";
}

bool HyprlandManager::run_hyprctl(const std::string &lua_expr, std::string *error_out) {
    const std::string path = hyprland_socket_path();
    if (!path.empty()) {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0) {
            struct sockaddr_un addr;
            std::memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

            if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                // Bound the whole exchange so a stalled compositor can never
                // strand this call (and pile up waiting processes/threads).
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
                auto ms_left = [&deadline]() -> int {
                    auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
                        deadline - std::chrono::steady_clock::now()).count();
                    return left > 0 ? (int)left : 0;
                };

                std::string request = "eval " + lua_expr;
                size_t sent = 0;
                while (sent < request.size()) {
                    struct pollfd pfd{fd, POLLOUT, 0};
                    if (poll(&pfd, 1, ms_left()) <= 0 || !(pfd.revents & POLLOUT)) break;
                    ssize_t n = send(fd, request.data() + sent, request.size() - sent, MSG_NOSIGNAL);
                    if (n <= 0) break;
                    sent += (size_t)n;
                }
                shutdown(fd, SHUT_WR);

                std::string out;
                char buf[512];
                while (true) {
                    struct pollfd pfd{fd, POLLIN, 0};
                    int pr = poll(&pfd, 1, ms_left());
                    if (pr <= 0 || !(pfd.revents & POLLIN)) break;
                    ssize_t n = recv(fd, buf, sizeof(buf), 0);
                    if (n <= 0) break;
                    out.append(buf, (size_t)n);
                }
                close(fd);

                if (out.find("error") != std::string::npos) {
                    if (error_out) *error_out = out;
                    std::cerr << "[HyprlandManager] Hyprland error: " << out << "\n";
                    return false;
                }
                return !out.empty();
            }
            close(fd);
        }
    }

    // Fallback for environments without the socket (socket closed / no SNI).
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
