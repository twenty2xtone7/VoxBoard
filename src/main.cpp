#include "types.hpp"
#include "storage/config_store.hpp"
#include "audio/pipewire_manager.hpp"
#include "audio/audio_player.hpp"
#include "keybinds/hyprland_binds.hpp"
#include "ipc/ipc_server.hpp"
#include "tray/tray_manager.hpp"
#include "ui/ui_manager.hpp"

#include <GLFW/glfw3.h>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <atomic>
#include <thread>
#include <chrono>

#if defined(__linux__) && defined(__GLIBC__)
#include <malloc.h>
#endif

static std::atomic<bool> g_running{true};
static std::atomic<int> g_window_action{0}; // 1: show, 2: hide
static GLFWwindow *g_window = nullptr;

static void clean_exit() {
    static std::atomic<bool> cleaned{false};
    if (cleaned.exchange(true)) return;

    std::cout << "[VoxBoard] Cleaning up resources...\n";
    Keybinds::HyprlandManager::instance().unbind_all();
    Audio::AudioPlayer::instance().shutdown();
    Audio::PipeWireManager::instance().shutdown();
    IPC::Server::instance().stop();
}

static void sig_handler(int) {
    g_running = false;
    if (g_window) {
        glfwSetWindowShouldClose(g_window, GLFW_TRUE);
    }
}

int main(int argc, char **argv) {
    // 1. CLI Commands mode
    if (argc >= 2) {
        std::string arg = argv[1];
        if (arg == "--play" && argc >= 3) {
            std::string sound_id = argv[2];
            if (IPC::Server::send_command("PLAY " + sound_id)) return 0;
            std::cerr << "[VoxBoard] Error: VoxBoard server is not running.\n";
            return 1;
        } else if (arg == "--stop") {
            if (IPC::Server::send_command("STOP")) return 0;
            return 1;
        } else if (arg == "--show") {
            if (IPC::Server::send_command("SHOW")) return 0;
            return 1;
        } else if (arg == "--hide") {
            if (IPC::Server::send_command("HIDE")) return 0;
            return 1;
        } else if (arg == "--media") {
            std::string val = (argc >= 3) ? argv[2] : "toggle";
            if (IPC::Server::send_command("MEDIA " + val)) return 0;
            return 1;
        } else if (arg == "--mic" && argc >= 3) {
            std::string val = argv[2];
            if (IPC::Server::send_command("MIC " + val)) return 0;
            return 1;
        } else if (arg == "--group" && argc >= 3) {
            std::string val = argv[2];
            if (IPC::Server::send_command("GROUP " + val)) return 0;
            return 1;
        } else if (arg == "--status") {
            if (IPC::Server::send_command("STATUS")) return 0;
            return 1;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: voxboard [OPTIONS]\n"
                      << "  --play <ID>       Play sound by ID in running VoxBoard instance\n"
                      << "  --stop            Stop all active sounds\n"
                      << "  --show            Restore and focus VoxBoard window\n"
                      << "  --hide            Minimize VoxBoard to system tray\n"
                      << "  --media <on|off>  Toggle or set Media-to-Mic routing\n"
                      << "  --mic <on|off>    Toggle or set Physical Mic passthrough\n"
                      << "  --group <ID>      Switch active keybind group\n"
                      << "  --status          Print active VoxBoard status\n"
                      << "  --help            Show this help message\n";
            return 0;
        }
    }

    // 2. Check if already running; if so, bring to front
    if (IPC::Server::send_command("SHOW")) {
        std::cout << "[VoxBoard] VoxBoard is already running. Brought to front.\n";
        return 0;
    }

    // 3. Register signal handlers
    std::signal(SIGINT, sig_handler);
    std::signal(SIGTERM, sig_handler);
    std::atexit(clean_exit);

    // 4. Load configuration
    AppConfig config = Storage::load_config();

    // 5. Initialize PipeWire virtual audio engine
    if (!Audio::PipeWireManager::instance().init(config)) {
        std::cerr << "[VoxBoard] Warning: PipeWire initialization had errors. Check pipewire status.\n";
    }

    // 6. Initialize Audio Player
    Audio::AudioPlayer::instance().init(config.soundboard_output_device, config.physical_mic_device);
    Audio::AudioPlayer::instance().set_monitor_volume(config.monitor_volume);
    Audio::AudioPlayer::instance().set_mic_volume(config.mic_volume);
    Audio::AudioPlayer::instance().set_media_to_mic_active(config.media_to_mic);
    Audio::AudioPlayer::instance().set_monitor_muted(config.monitor_muted);
    Audio::AudioPlayer::instance().set_mic_muted(config.virtual_mic_muted);

    // 7. Bind active sound group and global shortcuts in Hyprland
    Keybinds::HyprlandManager::instance().bind_global_actions(config);
    for (const auto &grp : config.groups) {
        if (grp.id == config.active_group_id) {
            Keybinds::HyprlandManager::instance().bind_group(grp);
            break;
        }
    }

    // 8. Initialize GLFW & OpenGL
    if (!glfwInit()) {
        std::cerr << "[VoxBoard] Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    g_window = glfwCreateWindow(860, 620, "VoxBoard", nullptr, nullptr);
    if (!g_window) {
        std::cerr << "[VoxBoard] Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1); // Enable vsync

    // Intercept window close to minimize to tray instead of exiting
    glfwSetWindowCloseCallback(g_window, [](GLFWwindow *w) {
        glfwHideWindow(w);
        glfwSetWindowShouldClose(w, GLFW_FALSE);
    });

    // 9. Setup IPC Server
    IPC::Server::instance().start([&config](const std::string &cmd) {
        if (cmd.rfind("PLAY ", 0) == 0) {
            std::string id = cmd.substr(5);
            for (const auto &grp : config.groups) {
                for (const auto &snd : grp.sounds) {
                    if (snd.id == id) {
                        Audio::AudioPlayer::instance().play(snd.path, snd.volume);
                        return;
                    }
                }
            }
        } else if (cmd == "STOP") {
            Audio::AudioPlayer::instance().stop_all();
        } else if (cmd == "SHOW") {
            std::cout << "[IPC] Received SHOW\n" << std::flush;
            g_window_action.store(1);
        } else if (cmd == "HIDE") {
            std::cout << "[IPC] Received HIDE\n" << std::flush;
            g_window_action.store(2);
        } else if (cmd.rfind("MEDIA ", 0) == 0) {
            std::string val = cmd.substr(6);
            bool enable = false;
            if (val == "toggle") {
                enable = !config.media_to_mic;
            } else {
                enable = (val == "1" || val == "on" || val == "true");
            }
            config.media_to_mic = enable;
            Audio::PipeWireManager::instance().set_media_to_mic(enable);
            Audio::AudioPlayer::instance().set_media_to_mic_active(enable);
            Tray::TrayManager::instance().update_toggles(config.media_to_mic, config.mic_passthrough);
            Storage::save_config(config);
        } else if (cmd.rfind("MIC ", 0) == 0) {
            std::string val = cmd.substr(4);
            bool enable = (val == "1" || val == "on" || val == "true");
            config.mic_passthrough = enable;
            Audio::PipeWireManager::instance().set_mic_passthrough(enable);
            Tray::TrayManager::instance().update_toggles(config.media_to_mic, config.mic_passthrough);
            Storage::save_config(config);
        } else if (cmd.rfind("GROUP ", 0) == 0) {
            std::string gid = cmd.substr(6);
            for (const auto &grp : config.groups) {
                if (grp.id == gid) {
                    config.active_group_id = gid;
                    Keybinds::HyprlandManager::instance().bind_group(grp);
                    Tray::TrayManager::instance().update_groups(config.groups, config.active_group_id);
                    Storage::save_config(config);
                    break;
                }
            }
        }
    });

    // 10. Setup System Tray
    Tray::TrayCallbacks tray_cbs;
    tray_cbs.on_show_window = []() {
        std::cout << "[Main] on_show_window called\n" << std::flush;
        g_window_action.store(1);
    };
    tray_cbs.on_hide_window = []() {
        std::cout << "[Main] on_hide_window called\n" << std::flush;
        g_window_action.store(2);
    };
    tray_cbs.on_toggle_media = [&config](bool enable) {
        config.media_to_mic = enable;
        Audio::PipeWireManager::instance().set_media_to_mic(enable);
        Audio::AudioPlayer::instance().set_media_to_mic_active(enable);
        Storage::save_config(config);
    };
    tray_cbs.on_toggle_mic = [&config](bool enable) {
        config.mic_passthrough = enable;
        Audio::PipeWireManager::instance().set_mic_passthrough(enable);
        Storage::save_config(config);
    };
    tray_cbs.on_select_group = [&config](const std::string &group_id) {
        config.active_group_id = group_id;
        for (const auto &grp : config.groups) {
            if (grp.id == group_id) {
                Keybinds::HyprlandManager::instance().bind_group(grp);
                break;
            }
        }
        Storage::save_config(config);
        Tray::TrayManager::instance().update_groups(config.groups, config.active_group_id);
    };
    tray_cbs.on_stop_all = []() {
        Audio::AudioPlayer::instance().stop_all();
    };
    tray_cbs.on_quit = []() {
        g_running = false;
        if (g_window) {
            glfwSetWindowShouldClose(g_window, GLFW_TRUE);
        }
    };

    Tray::TrayManager::instance().init(tray_cbs);
    Tray::TrayManager::instance().update_groups(config.groups, config.active_group_id);
    Tray::TrayManager::instance().update_toggles(config.media_to_mic, config.mic_passthrough);

    // 11. Setup UI
    UI::UIEvents ui_events;
    ui_events.on_play_sound = [](const std::string &path, float vol) {
        Audio::AudioPlayer::instance().play(path, vol);
    };
    ui_events.on_play_preview = [](const std::string &path, float vol) {
        Audio::AudioPlayer::instance().play_preview(path, vol);
    };
    ui_events.on_stop_all = []() {
        Audio::AudioPlayer::instance().stop_all();
    };
    ui_events.on_toggle_media = [&config](bool enable) {
        Audio::PipeWireManager::instance().set_media_to_mic(enable);
        Audio::AudioPlayer::instance().set_media_to_mic_active(enable);
        Tray::TrayManager::instance().update_toggles(config.media_to_mic, config.mic_passthrough);
    };
    ui_events.on_toggle_mic = [&config](bool enable) {
        Audio::PipeWireManager::instance().set_mic_passthrough(enable);
        Tray::TrayManager::instance().update_toggles(config.media_to_mic, config.mic_passthrough);
    };
    ui_events.on_monitor_volume = [](float vol) {
        Audio::AudioPlayer::instance().set_monitor_volume(vol);
    };
    ui_events.on_mic_volume = [](float vol) {
        Audio::AudioPlayer::instance().set_mic_volume(vol);
    };
    ui_events.on_select_group = [&config](const std::string &group_id) {
        for (const auto &grp : config.groups) {
            if (grp.id == group_id) {
                Keybinds::HyprlandManager::instance().bind_group(grp);
                break;
            }
        }
        Tray::TrayManager::instance().update_groups(config.groups, config.active_group_id);
    };
    ui_events.on_config_modified = [&config]() {
        Storage::save_config(config);
        Tray::TrayManager::instance().update_groups(config.groups, config.active_group_id);
        Tray::TrayManager::instance().update_toggles(config.media_to_mic, config.mic_passthrough);
    };
    ui_events.on_change_mic_device = [&config](const std::string &dev) {
        config.physical_mic_device = dev;
        Audio::PipeWireManager::instance().set_physical_mic(dev, config.mic_passthrough);
        Audio::AudioPlayer::instance().set_meter_input_device(dev);
    };
    ui_events.on_change_output_device = [&config](const std::string &sink) {
        config.soundboard_output_device = sink;
        Audio::AudioPlayer::instance().set_output_device(sink);
    };
    ui_events.on_change_desktop_source = [&config](const std::string &src) {
        config.desktop_capture_source = src;
        Audio::PipeWireManager::instance().set_desktop_source(src, config.media_to_mic);
    };
    ui_events.on_toggle_virtual_mic = [&config](bool enable) {
        config.virtual_mic_enabled = enable;
        Audio::PipeWireManager::instance().set_virtual_mic_enabled(enable, config);
        Tray::TrayManager::instance().update_toggles(config.media_to_mic, config.mic_passthrough);
    };
    ui_events.on_test_virtual_mic = [&config](bool enable) {
        Audio::PipeWireManager::instance().test_virtual_mic(enable, config.soundboard_output_device);
    };
    ui_events.on_toggle_monitor_mute = [&config](bool mute) {
        config.monitor_muted = mute;
        Audio::AudioPlayer::instance().set_monitor_muted(mute);
    };
    ui_events.on_toggle_real_mic_mute = [&config](bool mute) {
        config.mic_muted = mute;
        Audio::PipeWireManager::instance().set_passthrough_mute(mute);
    };
    ui_events.on_toggle_virtual_mic_mute = [&config](bool mute) {
        config.virtual_mic_muted = mute;
        Audio::AudioPlayer::instance().set_mic_muted(mute);
        Audio::PipeWireManager::instance().set_virtual_mic_mute(mute);
    };
    ui_events.on_passthrough_volume = [&config](float vol) {
        config.passthrough_volume = vol;
        Audio::PipeWireManager::instance().set_passthrough_volume(vol);
    };
    ui_events.on_virtual_mic_volume = [&config](float vol) {
        config.virtual_mic_volume = vol;
        Audio::PipeWireManager::instance().set_virtual_mic_volume(vol);
    };
    ui_events.on_change_stop_all_bind = [&config](const std::string &combo) {
        config.stop_all_keybind = combo;
        Keybinds::HyprlandManager::instance().bind_global_actions(config);
    };
    ui_events.on_change_toggle_media_bind = [&config](const std::string &combo) {
        config.toggle_media_keybind = combo;
        Keybinds::HyprlandManager::instance().bind_global_actions(config);
    };
    ui_events.on_repair_pipewire = [&config]() {
        Audio::PipeWireManager::instance().repair(config);
    };

    UI::UIManager::instance().init(g_window, ui_events);

    // 12. Main Application Loop
    while (g_running && !glfwWindowShouldClose(g_window)) {
        glfwPollEvents();
        Tray::TrayManager::instance().pump_events();

        int act = g_window_action.exchange(0);
        if (act == 1 && g_window) {
            std::cout << "[Main] Executing glfwShowWindow & focus\n" << std::flush;
            glfwShowWindow(g_window);
            glfwFocusWindow(g_window);
        } else if (act == 2 && g_window) {
            std::cout << "[Main] Executing glfwHideWindow\n" << std::flush;
            glfwHideWindow(g_window);
        }

        bool is_vis = glfwGetWindowAttrib(g_window, GLFW_VISIBLE);
        if (is_vis) {
            int display_w, display_h;
            glfwGetFramebufferSize(g_window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            UI::UIManager::instance().render(config);

            glfwSwapBuffers(g_window);
        } else {
            // Window is minimized to tray: sleep briefly to idle CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
    }

    // 13. Teardown
    UI::UIManager::instance().shutdown();
    glfwDestroyWindow(g_window);
    glfwTerminate();

    Storage::save_config(config);
    clean_exit();

    std::cout << "[VoxBoard] Exited cleanly.\n";
    return 0;
}
