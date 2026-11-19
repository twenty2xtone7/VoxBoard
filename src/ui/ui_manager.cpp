#include "ui_manager.hpp"
#include "audio/audio_player.hpp"
#include "audio/pipewire_manager.hpp"
#include "storage/config_store.hpp"
#include "keybinds/hyprland_binds.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <iostream>
#include <thread>
#include <cmath>
#include <algorithm>

namespace fs = std::filesystem;

namespace UI {

namespace Theme {

void apply() {
    ImGuiStyle &style = ImGui::GetStyle();

    // Subtle modern rounding
    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 4.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 4.0f;

    // Borders
    style.WindowBorderSize   = 1.0f;
    style.ChildBorderSize    = 1.0f;
    style.PopupBorderSize    = 1.0f;
    style.FrameBorderSize    = 1.0f;
    style.TabBorderSize      = 0.0f;
    style.TabBarBorderSize   = 1.0f;
    style.TabBarOverlineSize = 2.0f;

    // Dimensions
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 10.0f;

    ImVec4 *c = style.Colors;

    // Background surfaces (deep charcoal palette)
    c[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_ChildBg]               = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    c[ImGuiCol_PopupBg]               = ImVec4(0.13f, 0.13f, 0.16f, 0.98f);
    c[ImGuiCol_Border]                = ImVec4(0.22f, 0.22f, 0.27f, 0.70f);
    c[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Form controls & input surfaces
    c[ImGuiCol_FrameBg]               = ImVec4(0.15f, 0.15f, 0.19f, 1.00f);
    c[ImGuiCol_FrameBgHovered]        = ImVec4(0.20f, 0.20f, 0.26f, 1.00f);
    c[ImGuiCol_FrameBgActive]         = ImVec4(0.25f, 0.24f, 0.33f, 1.00f);

    // Title / Menu bars
    c[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_TitleBgActive]         = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.09f, 0.09f, 0.11f, 1.00f);
    c[ImGuiCol_MenuBarBg]             = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);

    // Scrollbars
    c[ImGuiCol_ScrollbarBg]           = ImVec4(0.10f, 0.10f, 0.13f, 0.40f);
    c[ImGuiCol_ScrollbarGrab]         = ImVec4(0.24f, 0.23f, 0.31f, 0.80f);
    c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.33f, 0.31f, 0.42f, 0.90f);
    c[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.42f, 0.40f, 0.55f, 1.00f);

    // Checkmarks & Sliders (muted indigo/violet)
    c[ImGuiCol_CheckMark]             = ImVec4(0.68f, 0.64f, 0.90f, 1.00f);
    c[ImGuiCol_CheckboxSelectedBg]    = ImVec4(0.22f, 0.21f, 0.32f, 1.00f);
    c[ImGuiCol_SliderGrab]            = ImVec4(0.46f, 0.43f, 0.64f, 1.00f);
    c[ImGuiCol_SliderGrabActive]      = ImVec4(0.58f, 0.54f, 0.78f, 1.00f);

    // Buttons
    c[ImGuiCol_Button]                = ImVec4(0.22f, 0.21f, 0.30f, 0.90f);
    c[ImGuiCol_ButtonHovered]         = ImVec4(0.32f, 0.30f, 0.44f, 1.00f);
    c[ImGuiCol_ButtonActive]          = ImVec4(0.42f, 0.39f, 0.58f, 1.00f);

    // Headers & selections
    c[ImGuiCol_Header]                = ImVec4(0.22f, 0.21f, 0.32f, 0.85f);
    c[ImGuiCol_HeaderHovered]         = ImVec4(0.32f, 0.30f, 0.46f, 0.90f);
    c[ImGuiCol_HeaderActive]          = ImVec4(0.40f, 0.38f, 0.56f, 1.00f);

    // Separators
    c[ImGuiCol_Separator]             = ImVec4(0.20f, 0.20f, 0.26f, 0.80f);
    c[ImGuiCol_SeparatorHovered]      = ImVec4(0.38f, 0.35f, 0.52f, 0.80f);
    c[ImGuiCol_SeparatorActive]       = ImVec4(0.48f, 0.44f, 0.66f, 1.00f);

    // Resize grip
    c[ImGuiCol_ResizeGrip]            = ImVec4(0.26f, 0.25f, 0.36f, 0.30f);
    c[ImGuiCol_ResizeGripHovered]     = ImVec4(0.38f, 0.36f, 0.52f, 0.65f);
    c[ImGuiCol_ResizeGripActive]      = ImVec4(0.50f, 0.47f, 0.68f, 0.90f);

    // Tabs
    c[ImGuiCol_Tab]                   = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    c[ImGuiCol_TabHovered]            = ImVec4(0.30f, 0.28f, 0.44f, 0.85f);
    c[ImGuiCol_TabSelected]           = ImVec4(0.22f, 0.21f, 0.31f, 1.00f);
    c[ImGuiCol_TabSelectedOverline]   = ImVec4(0.68f, 0.64f, 0.90f, 1.00f);
    c[ImGuiCol_TabDimmed]             = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    c[ImGuiCol_TabDimmedSelected]     = ImVec4(0.18f, 0.17f, 0.24f, 1.00f);
    c[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.42f, 0.40f, 0.56f, 1.00f);

    // Plots & meters
    c[ImGuiCol_PlotLines]             = ImVec4(0.55f, 0.52f, 0.74f, 1.00f);
    c[ImGuiCol_PlotLinesHovered]      = ImVec4(0.68f, 0.64f, 0.90f, 1.00f);
    c[ImGuiCol_PlotHistogram]         = ImVec4(0.44f, 0.41f, 0.62f, 1.00f);
    c[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.58f, 0.54f, 0.80f, 1.00f);

    // Tables
    c[ImGuiCol_TableHeaderBg]         = ImVec4(0.15f, 0.15f, 0.19f, 1.00f);
    c[ImGuiCol_TableBorderStrong]     = ImVec4(0.24f, 0.24f, 0.30f, 0.85f);
    c[ImGuiCol_TableBorderLight]      = ImVec4(0.18f, 0.18f, 0.23f, 0.60f);
    c[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_TableRowBgAlt]         = ImVec4(0.14f, 0.14f, 0.18f, 0.45f);

    // Text & typography contrast
    c[ImGuiCol_Text]                  = ImVec4(0.92f, 0.93f, 0.95f, 1.00f);
    c[ImGuiCol_TextDisabled]          = ImVec4(0.48f, 0.49f, 0.56f, 1.00f);
    c[ImGuiCol_TextSelectedBg]        = ImVec4(0.35f, 0.32f, 0.50f, 0.55f);
    c[ImGuiCol_TextLink]              = ImVec4(0.68f, 0.65f, 0.90f, 1.00f);

    // Navigation & Modals
    c[ImGuiCol_NavCursor]             = ImVec4(0.55f, 0.51f, 0.76f, 0.80f);
    c[ImGuiCol_NavWindowingHighlight] = ImVec4(0.60f, 0.56f, 0.82f, 0.70f);
    c[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.10f, 0.10f, 0.13f, 0.60f);
    c[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.08f, 0.08f, 0.10f, 0.65f);
    c[ImGuiCol_InputTextCursor]       = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
}

} // namespace Theme

UIManager& UIManager::instance() {
    static UIManager inst;
    return inst;
}

UIManager::UIManager() = default;

UIManager::~UIManager() {
    shutdown();
}

bool UIManager::init(GLFWwindow *window, const UIEvents &events) {
    if (initialized_) return true;
    window_ = window;
    events_ = events;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Load clean readable system UI font if present, fallback to Dear ImGui default
    ImFontConfig font_cfg;
    font_cfg.OversampleH = 2;
    font_cfg.OversampleV = 2;
    font_cfg.PixelSnapH = true;
    const char *font_paths[] = {
        "/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf",
        "/usr/share/fonts/noto/NotoSans-Regular.ttf"
    };
    bool font_loaded = false;
    for (const char *path : font_paths) {
        if (fs::exists(path)) {
            io.Fonts->AddFontFromFileTTF(path, 15.0f, &font_cfg);
            font_loaded = true;
            break;
        }
    }
    if (!font_loaded) {
        io.Fonts->AddFontDefault();
    }

    // Apply centralized custom theme
    Theme::apply();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    initialized_ = true;
    return true;
}

void UIManager::shutdown() {
    if (!initialized_) return;
    initialized_ = false;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::render(AppConfig &config) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    handle_key_capture(config);

    int w = 0, h = 0;
    glfwGetWindowSize(window_, &w, &h);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));

    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("VoxBoardMain", nullptr, win_flags);

    render_header(config);

    ImGui::Separator();

    if (ImGui::BeginTabBar("MainTabBar")) {
        if (ImGui::BeginTabItem("Soundboard")) {
            render_soundboard(config);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("MyInstants Browser")) {
            render_myinstants(config);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Audio Settings")) {
            render_audio_settings(config);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Virtual Mic Setup")) {
            render_virtual_mic_setup(config);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Global Keybinds & Diagnostics")) {
            render_diagnostics(config);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::render_header(AppConfig &config) {
    ImGui::Text("VoxBoard Linux Soundboard");
    ImGui::SameLine();

    bool sink_ok = false, source_ok = false;
    bool healthy = Audio::PipeWireManager::instance().check_health(sink_ok, source_ok);

    if (!config.virtual_mic_enabled) {
        ImGui::TextDisabled("| Virtual Mic: Disabled [Local Playback Only]");
    } else if (!healthy) {
        ImGui::TextColored(Theme::Danger, "| Virtual Mic: Missing/Error [Check Virtual Mic Setup]");
    }

    ImGui::Spacing();

    // Stop All button: always displays the registered shortcut (e.g. Escape or custom), matching what is actually registered and executed
    std::string stop_shortcut = config.stop_all_keybind.empty() ? "None" : config.stop_all_keybind;
    std::string stop_btn_label = "Stop All (" + stop_shortcut + ")###StopAllBtn";
    if (ImGui::Button(stop_btn_label.c_str(), ImVec2(0, 26))) {
        if (events_.on_stop_all) events_.on_stop_all();
    }
    ImGui::SameLine();
    bool is_stop_listening_hdr = (listen_target_ == ListenTarget::StopAll);
    if (is_stop_listening_hdr) ImGui::PushStyleColor(ImGuiCol_Button, Theme::KeyCapture);
    if (ImGui::Button(is_stop_listening_hdr ? "[Press Key...]##hdr_stop" : "Edit Bind##hdr_stop", ImVec2(0, 26))) {
        if (is_stop_listening_hdr) {
            listen_target_ = ListenTarget::None;
        } else {
            listen_target_ = ListenTarget::StopAll;
            conflict_warning_.clear();
        }
    }
    if (is_stop_listening_hdr) ImGui::PopStyleColor();

    ImGui::SameLine();
    bool mon_mute = config.monitor_muted;
    if (ImGui::Checkbox("Mute Headphones", &mon_mute)) {
        config.monitor_muted = mon_mute;
        if (events_.on_toggle_monitor_mute) events_.on_toggle_monitor_mute(mon_mute);
        if (events_.on_config_modified) events_.on_config_modified();
    }

    ImGui::SameLine();
    bool mic_mute = config.virtual_mic_muted;
    if (ImGui::Checkbox("Mute Virtual Mic", &mic_mute)) {
        config.virtual_mic_muted = mic_mute;
        if (events_.on_toggle_virtual_mic_mute) events_.on_toggle_virtual_mic_mute(mic_mute);
        if (events_.on_config_modified) events_.on_config_modified();
    }

    ImGui::SameLine();
    float mon_vol = config.monitor_volume;
    ImGui::SetNextItemWidth(130);
    if (ImGui::SliderFloat("Playback Vol", &mon_vol, 0.0f, 2.0f, "%.2fx")) {
        config.monitor_volume = mon_vol;
        if (events_.on_monitor_volume) events_.on_monitor_volume(mon_vol);
        if (events_.on_config_modified) events_.on_config_modified();
    }

    ImGui::SameLine();
    float vmic_vol = config.virtual_mic_volume;
    ImGui::SetNextItemWidth(130);
    if (ImGui::SliderFloat("Virtual Mic Vol", &vmic_vol, 0.0f, 2.0f, "%.2fx")) {
        config.virtual_mic_volume = vmic_vol;
        if (events_.on_virtual_mic_volume) events_.on_virtual_mic_volume(vmic_vol);
        if (events_.on_config_modified) events_.on_config_modified();
    }
}

void UIManager::render_soundboard(AppConfig &config) {
    ImGui::Spacing();

    // Helper to find currently active group safely
    auto get_selected_group = [&config]() -> SoundGroup* {
        for (auto &grp : config.groups) {
            if (grp.id == config.active_group_id) {
                return &grp;
            }
        }
        if (!config.groups.empty()) {
            config.active_group_id = config.groups[0].id;
            return &config.groups[0];
        }
        return nullptr;
    };

    get_selected_group();

    // Sound Group Bar
    ImGui::Text("Sound Group: ");
    ImGui::SameLine();

    for (size_t i = 0; i < config.groups.size(); ++i) {
        auto &grp = config.groups[i];
        bool is_active = (grp.id == config.active_group_id);

        std::string tab_label = grp.name + "##grp_" + grp.id;
        if (is_active) {
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::GroupActive);
        }

        if (ImGui::Button(tab_label.c_str())) {
            config.active_group_id = grp.id;
            if (events_.on_select_group) events_.on_select_group(grp.id);
            if (events_.on_config_modified) events_.on_config_modified();
        }

        if (is_active) {
            ImGui::PopStyleColor();
        }
        ImGui::SameLine();
    }

    // New Group button
    if (ImGui::Button("+ New Group")) {
        ImGui::OpenPopup("NewGroupModal");
        new_group_buf_[0] = '\0';
    }

    if (ImGui::BeginPopupModal("NewGroupModal", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter new sound group name:");
        ImGui::InputText("##NewGroupName", new_group_buf_, sizeof(new_group_buf_));
        if (ImGui::Button("Create", ImVec2(100, 0))) {
            if (new_group_buf_[0] != '\0') {
                SoundGroup g;
                g.id = Storage::generate_id();
                g.name = new_group_buf_;
                config.groups.push_back(g);
                config.active_group_id = g.id;
                if (events_.on_select_group) events_.on_select_group(g.id);
                if (events_.on_config_modified) events_.on_config_modified();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Re-resolve active group pointer after potential group list changes
    // ponytail: linear scan of small groups vector; add index map if groups > 100
    SoundGroup *selected_group = get_selected_group();
    if (!selected_group) {
        ImGui::Text("No sound group selected.");
        return;
    }

    ImGui::Text("Active Group: %s", selected_group->name.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Add Sound File")) {
        show_add_sound_modal_ = true;
        new_sound_name_[0] = '\0';
        new_sound_path_[0] = '\0';
    }

    if (config.groups.size() > 1) {
        ImGui::SameLine();
        if (ImGui::Button("Delete This Group")) {
            std::string del_id = selected_group->id;
            for (auto it = config.groups.begin(); it != config.groups.end(); ++it) {
                if (it->id == del_id) {
                    config.groups.erase(it);
                    break;
                }
            }
            if (!config.groups.empty()) {
                config.active_group_id = config.groups[0].id;
                if (events_.on_select_group) events_.on_select_group(config.active_group_id);
            }
            if (events_.on_config_modified) events_.on_config_modified();
            return;
        }
    }

    // Modal: Add Local Sound File
    if (show_add_sound_modal_) {
        ImGui::OpenPopup("AddSoundModal");
    }
    if (ImGui::BeginPopupModal("AddSoundModal", &show_add_sound_modal_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Add Audio File (MP3, WAV, OGG, FLAC):");
        ImGui::InputText("Sound Name", new_sound_name_, sizeof(new_sound_name_));
        ImGui::InputText("File Absolute Path", new_sound_path_, sizeof(new_sound_path_));

        if (ImGui::Button("Add", ImVec2(100, 0))) {
            if (new_sound_path_[0] != '\0' && fs::exists(new_sound_path_)) {
                Sound s;
                s.id = Storage::generate_id();
                s.name = (new_sound_name_[0] != '\0') ? new_sound_name_ : fs::path(new_sound_path_).stem().string();
                s.path = new_sound_path_;
                s.volume = 1.0f;
                selected_group->sounds.push_back(s);
                if (events_.on_config_modified) events_.on_config_modified();
                show_add_sound_modal_ = false;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            show_add_sound_modal_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Global Shortcuts Bar
    render_global_keybinds(config);
    ImGui::Spacing();

    // Sounds Table
    if (ImGui::BeginTable("SoundsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Play", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Sound Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("Keybind", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("Clear", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Delete", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        std::string sound_to_delete;
        // Pin the row height to its natural content height (tallest widget is
        // a 24px button + cell padding) so TableGetCellBgRect() already reports
        // the full cell rectangle when the Sound Name column is submitted.
        const float row_min_height = 24.0f + ImGui::GetStyle().CellPadding.y * 2.0f;

        for (size_t i = 0; i < selected_group->sounds.size(); ++i) {
            auto &snd = selected_group->sounds[i];
            ImGui::TableNextRow(0, row_min_height);

            // Column 0: Play
            ImGui::TableNextColumn();
            std::string play_label = "▶##" + snd.id;
            if (ImGui::Button(play_label.c_str(), ImVec2(40, 24))) {
                if (events_.on_play_sound) events_.on_play_sound(snd.path, snd.volume);
            }

            // Column 1: Name — level fill covers the whole cell, behind the text
            ImGui::TableNextColumn();
            render_sound_name_cell_fill(snd.id, snd.path);
            ImGui::Text("%s", snd.name.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", snd.path.c_str());
            }

            // Column 2: Volume Slider
            ImGui::TableNextColumn();
            std::string vol_id = "##vol_" + snd.id;
            ImGui::SetNextItemWidth(130);
            if (ImGui::SliderFloat(vol_id.c_str(), &snd.volume, 0.0f, 2.0f, "%.2f")) {
                if (events_.on_config_modified) events_.on_config_modified();
            }

            // Column 3: Keybind assignment button
            ImGui::TableNextColumn();
            bool is_listening = (listen_target_ == ListenTarget::Sound && listening_sound_id_ == snd.id);
            std::string btn_label = is_listening ? "[Press Key...]" : (snd.keybind.empty() ? "[Bind Key]" : snd.keybind);
            std::string btn_id = btn_label + "##bind_" + snd.id;

            if (is_listening) {
                ImGui::PushStyleColor(ImGuiCol_Button, Theme::KeyCapture);
            }

            if (ImGui::Button(btn_id.c_str(), ImVec2(170, 24))) {
                if (is_listening) {
                    listen_target_ = ListenTarget::None;
                    listening_sound_id_.clear();
                } else {
                    listen_target_ = ListenTarget::Sound;
                    listening_sound_id_ = snd.id;
                    conflict_warning_.clear();
                }
            }

            if (is_listening) {
                ImGui::PopStyleColor();
            }

            // Column 4: Clear bind
            ImGui::TableNextColumn();
            std::string clr_label = "X##clr_" + snd.id;
            if (ImGui::Button(clr_label.c_str(), ImVec2(30, 24))) {
                if (!snd.keybind.empty()) {
                    Keybinds::HyprlandManager::instance().unbind_sound(snd.keybind);
                    snd.keybind.clear();
                    if (events_.on_config_modified) events_.on_config_modified();
                }
            }

            // Column 5: Delete sound
            ImGui::TableNextColumn();
            std::string del_label = "Del##" + snd.id;
            if (ImGui::Button(del_label.c_str(), ImVec2(45, 24))) {
                sound_to_delete = snd.id;
            }
        }

        if (!sound_to_delete.empty()) {
            for (auto it = selected_group->sounds.begin(); it != selected_group->sounds.end(); ++it) {
                if (it->id == sound_to_delete) {
                    if (!it->keybind.empty()) {
                        Keybinds::HyprlandManager::instance().unbind_sound(it->keybind);
                    }
                    selected_group->sounds.erase(it);
                    break;
                }
            }
            if (events_.on_config_modified) events_.on_config_modified();
        }

        ImGui::EndTable();
    }
}

void UIManager::render_global_keybinds(AppConfig &config) {
    ImGui::PushID("GlobalBinds");

    if (!conflict_warning_.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(Theme::Danger, "%s", conflict_warning_.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Dismiss##conflict")) {
            conflict_warning_.clear();
        }
        ImGui::Spacing();
    }

    ImGui::TextColored(Theme::TextHeader, "Global Keybindings (Hyprland / Wayland - Cross-Group):");

    if (ImGui::BeginTable("GlobalActionsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Shortcut", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Clear", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Default", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        // Row 1: Stop All Sounds
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Stop All Sounds");

        ImGui::TableNextColumn();
        bool is_stop_listening = (listen_target_ == ListenTarget::StopAll);
        std::string stop_btn_label = is_stop_listening ? "[Press Key...]" : (config.stop_all_keybind.empty() ? "[Unassigned]" : config.stop_all_keybind);
        if (is_stop_listening) ImGui::PushStyleColor(ImGuiCol_Button, Theme::KeyCapture);
        if (ImGui::Button((stop_btn_label + "##stop_all").c_str(), ImVec2(190, 24))) {
            if (is_stop_listening) {
                listen_target_ = ListenTarget::None;
            } else {
                listen_target_ = ListenTarget::StopAll;
                conflict_warning_.clear();
            }
        }
        if (is_stop_listening) ImGui::PopStyleColor();

        ImGui::TableNextColumn();
        if (ImGui::Button("Clear##stop", ImVec2(50, 24))) {
            if (!config.stop_all_keybind.empty()) {
                Keybinds::HyprlandManager::instance().unbind_global_action(config.stop_all_keybind);
                config.stop_all_keybind.clear();
                if (events_.on_config_modified) events_.on_config_modified();
            }
        }

        ImGui::TableNextColumn();
        if (ImGui::Button("Default##stop", ImVec2(70, 24))) {
            if (!config.stop_all_keybind.empty()) {
                Keybinds::HyprlandManager::instance().unbind_global_action(config.stop_all_keybind);
            }
            config.stop_all_keybind = "Escape";
            Keybinds::HyprlandManager::instance().bind_global_action(config.stop_all_keybind, "--stop");
            if (events_.on_config_modified) events_.on_config_modified();
        }

        // Row 2: Toggle Media-to-Mic
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Toggle Media-to-Mic");

        ImGui::TableNextColumn();
        bool is_media_listening = (listen_target_ == ListenTarget::ToggleMedia);
        std::string media_btn_label = is_media_listening ? "[Press Key...]" : (config.toggle_media_keybind.empty() ? "[Unassigned]" : config.toggle_media_keybind);
        if (is_media_listening) ImGui::PushStyleColor(ImGuiCol_Button, Theme::KeyCapture);
        if (ImGui::Button((media_btn_label + "##toggle_media").c_str(), ImVec2(190, 24))) {
            if (is_media_listening) {
                listen_target_ = ListenTarget::None;
            } else {
                listen_target_ = ListenTarget::ToggleMedia;
                conflict_warning_.clear();
            }
        }
        if (is_media_listening) ImGui::PopStyleColor();

        ImGui::TableNextColumn();
        if (ImGui::Button("Clear##media", ImVec2(50, 24))) {
            if (!config.toggle_media_keybind.empty()) {
                Keybinds::HyprlandManager::instance().unbind_global_action(config.toggle_media_keybind);
                config.toggle_media_keybind.clear();
                if (events_.on_config_modified) events_.on_config_modified();
            }
        }

        ImGui::TableNextColumn();
        if (ImGui::Button("Default##media", ImVec2(70, 24))) {
            if (!config.toggle_media_keybind.empty()) {
                Keybinds::HyprlandManager::instance().unbind_global_action(config.toggle_media_keybind);
            }
            config.toggle_media_keybind = "SUPER + ALT + M";
            Keybinds::HyprlandManager::instance().bind_global_action(config.toggle_media_keybind, "--media toggle");
            if (events_.on_config_modified) events_.on_config_modified();
        }

        ImGui::EndTable();
    }

    ImGui::PopID();
}

void UIManager::render_myinstants(AppConfig &config) {
    ImGui::Spacing();
    ImGui::Text("Browse and search MyInstants sounds:");

    ImGui::SetNextItemWidth(350);
    bool enter_pressed = ImGui::InputTextWithHint("##MyInstantsQuery", "Search query (e.g. anime, bruh, punch)...", search_buf_, sizeof(search_buf_), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();

    if (ImGui::Button("Search") || enter_pressed) {
        current_page_ = 1;
        searching_ = true;
        std::thread([this]() {
            search_results_ = Net::MyInstantsClient::instance().search(search_buf_, current_page_);
            searching_ = false;
        }).detach();
    }

    ImGui::SameLine();
    if (ImGui::Button("Popular / Trending")) {
        search_buf_[0] = '\0';
        current_page_ = 1;
        searching_ = true;
        std::thread([this]() {
            search_results_ = Net::MyInstantsClient::instance().search("", current_page_);
            searching_ = false;
        }).detach();
    }

    if (searching_) {
        ImGui::SameLine();
        ImGui::Text("Loading results...");
    }

    if (!status_message_.empty()) {
        ImGui::TextColored(Theme::Success, "%s", status_message_.c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Results table
    if (ImGui::BeginTable("MyInstantsResults", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Sound Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Import", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableHeadersRow();

        for (const auto &item : search_results_.items) {
            ImGui::TableNextRow();

            // Preview
            ImGui::TableNextColumn();
            bool is_this_playing = (preview_playing_slug_ == item.slug && Audio::AudioPlayer::instance().is_playing());
            std::string pbtn = is_this_playing ? "Stop##" + item.slug : "▶ Play##" + item.slug;

            if (ImGui::Button(pbtn.c_str(), ImVec2(70, 24))) {
                if (is_this_playing) {
                    Audio::AudioPlayer::instance().stop_all();
                    preview_playing_slug_.clear();
                } else {
                    std::string preview_file = "/tmp/antigravity/preview_" + item.slug + ".mp3";
                    std::string url = item.sound_url;
                    std::string slug = item.slug;
                    std::thread([this, preview_file, url, slug]() {
                        if (Net::MyInstantsClient::instance().download_sound(url, preview_file)) {
                            preview_playing_slug_ = slug;
                            if (events_.on_play_preview) events_.on_play_preview(preview_file, 1.0f);
                        }
                    }).detach();
                }
            }

            // Name
            ImGui::TableNextColumn();
            ImGui::Text("%s", item.name.c_str());

            // Import
            ImGui::TableNextColumn();
            std::string ibtn = "+ Import to Group##" + item.slug;
            if (ImGui::Button(ibtn.c_str(), ImVec2(170, 24))) {
                std::string dest_path = Storage::get_sounds_dir() + "/" + item.slug + ".mp3";
                std::string url = item.sound_url;
                std::string name = item.name;

                std::thread([this, &config, dest_path, url, name]() {
                    if (Net::MyInstantsClient::instance().download_sound(url, dest_path)) {
                        for (auto &grp : config.groups) {
                            if (grp.id == config.active_group_id) {
                                Sound s;
                                s.id = Storage::generate_id();
                                s.name = name;
                                s.path = dest_path;
                                s.volume = 1.0f;
                                grp.sounds.push_back(s);
                                break;
                            }
                        }
                        if (events_.on_config_modified) events_.on_config_modified();
                        status_message_ = "Imported '" + name + "' successfully!";
                    }
                }).detach();
            }
        }

        ImGui::EndTable();
    }

    // Pagination
    ImGui::Spacing();
    if (current_page_ > 1) {
        if (ImGui::Button("< Previous Page")) {
            --current_page_;
            searching_ = true;
            std::thread([this]() {
                search_results_ = Net::MyInstantsClient::instance().search(search_buf_, current_page_);
                searching_ = false;
            }).detach();
        }
        ImGui::SameLine();
    }
    ImGui::Text("Page %d (Total: %d)", current_page_, search_results_.count);
    if (!search_results_.next_url.empty()) {
        ImGui::SameLine();
        if (ImGui::Button("Next Page >")) {
            ++current_page_;
            searching_ = true;
            std::thread([this]() {
                search_results_ = Net::MyInstantsClient::instance().search(search_buf_, current_page_);
                searching_ = false;
            }).detach();
        }
    }
}

float UIManager::smooth_level(const std::string &key, float target) {
    // Framerate-independent smoothing with asymmetric ballistics:
    // fast attack so transients register, slow release so the bar glides down.
    auto &state = meter_smoothing_[key];
    const auto now = std::chrono::steady_clock::now();
    float dt = 0.0f;
    if (state.has_last) {
        dt = std::chrono::duration<float>(now - state.last_update).count();
    }
    state.last_update = now;
    state.has_last = true;

    target = std::clamp(target, 0.0f, 1.0f);
    if (dt > 0.0f) {
        // Clamp dt to avoid a large jump after the window was hidden / stalled.
        dt = std::min(dt, 0.25f);
        // Slow, ease-in attack and a long fall. The source envelope is already
        // smoothed, so a single gentle curve avoids tau-flip jitter.
        const float tau = 0.26f;
        const float alpha = 1.0f - std::exp(-dt / tau);
        state.value += alpha * (target - state.value);
    } else {
        state.value = target;
    }
    return state.value;
}

void UIManager::render_level_meter(const char *label, float level, bool muted, float volume) {
    const float display = smooth_level(label, muted ? 0.0f : level);

    ImGui::Text("%s:", label);
    ImGui::SameLine(180);
    if (muted) {
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Theme::MeterMuted);
        ImGui::ProgressBar(0.0f, ImVec2(240, 16), "MUTED");
        ImGui::PopStyleColor();
        return;
    }

    ImVec4 col = Theme::Success;
    if (display > 0.85f) col = Theme::Danger;
    else if (display > 0.65f) col = Theme::Warning;

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
    char buf[32];
    snprintf(buf, sizeof(buf), "%.0f%%", display * 100.0f);
    ImGui::ProgressBar(display, ImVec2(240, 16), buf);
    ImGui::PopStyleColor();
}

void UIManager::render_sound_name_cell_fill(const std::string &id, const std::string &path) {
    const float raw = Audio::AudioPlayer::instance().get_sound_level(path);
    const float display = smooth_level("sound##" + id, raw);

    // Use the real Sound Name cell rectangle (column 1) as the visualizer region.
    ImGuiTable *table = ImGui::GetCurrentTable();
    if (!table || display <= 0.0f) return;
    const ImRect cell = ImGui::TableGetCellBgRect(table, 1);
    if (cell.Min.x >= cell.Max.x || cell.Min.y >= cell.Max.y) return;

    ImDrawList *dl = ImGui::GetWindowDrawList();
    // Clip strictly to the cell so the fill can never spill into adjacent
    // columns or outside the row (also intersects with ImGui's column clip).
    dl->PushClipRect(cell.Min, cell.Max, true);
    const float fill_w = (cell.Max.x - cell.Min.x) * display;
    const ImU32 solid = ImGui::ColorConvertFloat4ToU32(Theme::SoundMeter);
    const ImU32 faded = ImGui::ColorConvertFloat4ToU32(ImVec4(Theme::SoundMeter.x, Theme::SoundMeter.y, Theme::SoundMeter.z, 0.0f));

    // Soft (blurred) leading edge: solid body + a short horizontal alpha fade.
    const float blur_w = ImMin(22.0f, fill_w);
    const float fade_start = cell.Min.x + fill_w - blur_w;
    if (fill_w > blur_w) {
        dl->AddRectFilled(cell.Min, ImVec2(fade_start, cell.Max.y), solid);
    }
    if (blur_w > 0.0f) {
        dl->AddRectFilledMultiColor(
            ImVec2(fade_start, cell.Min.y),
            ImVec2(cell.Min.x + fill_w, cell.Max.y),
            solid, faded, faded, solid);
    }
    dl->PopClipRect();
}

void UIManager::render_routing_graph(const AppConfig &config) {
    std::string mic_str = config.physical_mic_device.empty() ? "System Default Mic" : config.physical_mic_device;
    std::string out_str = config.soundboard_output_device.empty() ? "System Default Output" : config.soundboard_output_device;
    std::string desktop_str = config.desktop_capture_source.empty() ? "System Output Monitor" : config.desktop_capture_source;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::SurfaceDeep);
    ImGui::BeginChild("RoutingGraphFrame", ImVec2(0, 140), true);

    ImGui::Text("  [Physical Mic: %s] --(Loopback Passthrough: %s)----+",
                mic_str.c_str(), config.mic_passthrough ? "ON" : "OFF");
    ImGui::Text("                                                                               |");
    ImGui::Text("  [Soundboard PCM Engine] -----(Mixer Bus)--------------------> [VoxBoard_Sink] --> [VoxBoard_Mic] --> [Discord / Games]");
    ImGui::Text("          |                                                                    ^");
    ImGui::Text("          v                                                                    |");
    ImGui::Text("  [Headphones/Monitor: %s]                                                     |", out_str.c_str());
    ImGui::Text("  [Desktop Media: %s] ---------(Media-to-Mic Loopback: %s)-----+",
                desktop_str.c_str(), config.media_to_mic ? "ON" : "OFF");

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void UIManager::render_audio_settings(AppConfig &config) {
    ImGui::Spacing();

    ImGui::Text("PipeWire Audio Devices & Level Monitoring");
    ImGui::SameLine();
    if (ImGui::Button("Refresh Devices")) {
        Audio::PipeWireManager::instance().refresh_devices();
    }

    ImGui::Separator();
    ImGui::Spacing();

    auto sinks = Audio::PipeWireManager::instance().get_sinks();
    auto sources = Audio::PipeWireManager::instance().get_sources();

    std::vector<AudioDevice> mic_sources;
    for (const auto &s : sources) {
        if (!s.is_monitor && s.name != "VoxBoard_Mic") {
            mic_sources.push_back(s);
        }
    }

    std::vector<AudioDevice> output_sinks;
    for (const auto &s : sinks) {
        if (s.name != "VoxBoard_Sink") {
            output_sinks.push_back(s);
        }
    }

    std::vector<AudioDevice> monitor_sources;
    for (const auto &s : sources) {
        if (s.is_monitor && s.name != "VoxBoard_Sink.monitor") {
            monitor_sources.push_back(s);
        }
    }

    // 1. Physical Microphone
    ImGui::TextColored(Theme::TextHeader, "1. Physical Microphone Input Device:");
    std::string current_mic_label = "Default Microphone (@DEFAULT_SOURCE@)";
    for (const auto &m : mic_sources) {
        if (m.name == config.physical_mic_device) {
            current_mic_label = m.description + (m.is_default ? " [Default]" : "");
            break;
        }
    }

    ImGui::SetNextItemWidth(450);
    if (ImGui::BeginCombo("##MicDeviceCombo", current_mic_label.c_str())) {
        bool is_sys_def = config.physical_mic_device.empty();
        if (ImGui::Selectable("System Default Microphone (@DEFAULT_SOURCE@)", is_sys_def)) {
            config.physical_mic_device.clear();
            if (events_.on_change_mic_device) events_.on_change_mic_device("");
            if (events_.on_config_modified) events_.on_config_modified();
        }
        for (const auto &m : mic_sources) {
            bool selected = (m.name == config.physical_mic_device);
            std::string label = m.description + " (" + m.state + ")" + (m.is_default ? " [Default]" : "");
            if (ImGui::Selectable(label.c_str(), selected)) {
                config.physical_mic_device = m.name;
                if (events_.on_change_mic_device) events_.on_change_mic_device(m.name);
                if (events_.on_config_modified) events_.on_config_modified();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    bool mic_mute = config.mic_muted;
    if (ImGui::Checkbox("Mute Real Mic", &mic_mute)) {
        config.mic_muted = mic_mute;
        if (events_.on_toggle_real_mic_mute) events_.on_toggle_real_mic_mute(mic_mute);
        if (events_.on_config_modified) events_.on_config_modified();
    }

    float pass_vol = config.passthrough_volume;
    ImGui::SetNextItemWidth(250);
    if (ImGui::SliderFloat("Mic Input Gain##Slider", &pass_vol, 0.0f, 2.0f, "%.2fx")) {
        config.passthrough_volume = pass_vol;
        if (events_.on_passthrough_volume) events_.on_passthrough_volume(pass_vol);
        if (events_.on_config_modified) events_.on_config_modified();
    }

    render_level_meter("Mic Input Level", Audio::AudioPlayer::instance().get_input_mic_level(), config.mic_muted);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 2. Soundboard Playback Output
    ImGui::TextColored(Theme::TextHeader, "2. Soundboard Playback Output Device (Headphones / Speakers):");
    std::string current_out_label = "System Default Output (@DEFAULT_SINK@)";
    for (const auto &o : output_sinks) {
        if (o.name == config.soundboard_output_device) {
            current_out_label = o.description + (o.is_default ? " [Default]" : "");
            break;
        }
    }

    ImGui::SetNextItemWidth(450);
    if (ImGui::BeginCombo("##OutputDeviceCombo", current_out_label.c_str())) {
        bool is_sys_def = config.soundboard_output_device.empty();
        if (ImGui::Selectable("System Default Output (@DEFAULT_SINK@)", is_sys_def)) {
            config.soundboard_output_device.clear();
            if (events_.on_change_output_device) events_.on_change_output_device("");
            if (events_.on_config_modified) events_.on_config_modified();
        }
        for (const auto &o : output_sinks) {
            bool selected = (o.name == config.soundboard_output_device);
            std::string label = o.description + " (" + o.state + ")" + (o.is_default ? " [Default]" : "");
            if (ImGui::Selectable(label.c_str(), selected)) {
                config.soundboard_output_device = o.name;
                if (events_.on_change_output_device) events_.on_change_output_device(o.name);
                if (events_.on_config_modified) events_.on_config_modified();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    bool mon_mute = config.monitor_muted;
    if (ImGui::Checkbox("Mute Playback", &mon_mute)) {
        config.monitor_muted = mon_mute;
        if (events_.on_toggle_monitor_mute) events_.on_toggle_monitor_mute(mon_mute);
        if (events_.on_config_modified) events_.on_config_modified();
    }

    float mon_vol = config.monitor_volume;
    ImGui::SetNextItemWidth(250);
    if (ImGui::SliderFloat("Headphones Volume##Slider", &mon_vol, 0.0f, 2.0f, "%.2fx")) {
        config.monitor_volume = mon_vol;
        if (events_.on_monitor_volume) events_.on_monitor_volume(mon_vol);
        if (events_.on_config_modified) events_.on_config_modified();
    }

    render_level_meter("Monitor Level", Audio::AudioPlayer::instance().get_monitor_level(), config.monitor_muted);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 3. Desktop Audio Capture
    ImGui::TextColored(Theme::TextHeader, "3. Desktop Audio Capture Source (Media-to-Mic Capture):");
    std::string current_desktop_label = "Default Output Monitor (@DEFAULT_SINK@.monitor)";
    for (const auto &m : monitor_sources) {
        if (m.name == config.desktop_capture_source) {
            current_desktop_label = m.description;
            break;
        }
    }

    ImGui::SetNextItemWidth(450);
    if (ImGui::BeginCombo("##DesktopSourceCombo", current_desktop_label.c_str())) {
        bool is_def = config.desktop_capture_source.empty();
        if (ImGui::Selectable("Default Output Monitor (@DEFAULT_SINK@.monitor)", is_def)) {
            config.desktop_capture_source.clear();
            if (events_.on_change_desktop_source) events_.on_change_desktop_source("");
            if (events_.on_config_modified) events_.on_config_modified();
        }
        for (const auto &m : monitor_sources) {
            bool selected = (m.name == config.desktop_capture_source);
            if (ImGui::Selectable(m.description.c_str(), selected)) {
                config.desktop_capture_source = m.name;
                if (events_.on_change_desktop_source) events_.on_change_desktop_source(m.name);
                if (events_.on_config_modified) events_.on_config_modified();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    bool media_active = config.media_to_mic;
    if (ImGui::Checkbox("Media-to-Mic Active", &media_active)) {
        config.media_to_mic = media_active;
        if (events_.on_toggle_media) events_.on_toggle_media(media_active);
        if (events_.on_config_modified) events_.on_config_modified();
    }

    ImGui::SameLine();
    bool is_media_listening_aud = (listen_target_ == ListenTarget::ToggleMedia);
    std::string media_btn_label = is_media_listening_aud ? "[Press Key...]" : (config.toggle_media_keybind.empty() ? "[Bind Media Key]" : ("Bind: " + config.toggle_media_keybind));
    if (is_media_listening_aud) ImGui::PushStyleColor(ImGuiCol_Button, Theme::KeyCapture);
    if (ImGui::Button((media_btn_label + "##audio_media_bind").c_str(), ImVec2(0, 24))) {
        if (is_media_listening_aud) {
            listen_target_ = ListenTarget::None;
        } else {
            listen_target_ = ListenTarget::ToggleMedia;
            conflict_warning_.clear();
        }
    }
    if (is_media_listening_aud) ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 4. Virtual Microphone Device Info
    ImGui::TextColored(Theme::TextHeader, "4. Virtual Microphone Output Device:");
    ImGui::BulletText("Device Node: VoxBoard_Mic [Audio/Source]");
    ImGui::BulletText("Mixer Bus: VoxBoard_Sink (Null-sink stereo 48000Hz)");
    ImGui::BulletText("Setup in Apps: Set Input Device to 'VoxBoard_Virtual_Microphone' in Discord or game settings.");

    bool vmic_mute = config.virtual_mic_muted;
    if (ImGui::Checkbox("Mute Virtual Mic##Output", &vmic_mute)) {
        config.virtual_mic_muted = vmic_mute;
        if (events_.on_toggle_virtual_mic_mute) events_.on_toggle_virtual_mic_mute(vmic_mute);
        if (events_.on_config_modified) events_.on_config_modified();
    }

    float vmic_vol = config.virtual_mic_volume;
    ImGui::SetNextItemWidth(250);
    if (ImGui::SliderFloat("Virtual Mic Volume##OutputSlider", &vmic_vol, 0.0f, 2.0f, "%.2fx")) {
        config.virtual_mic_volume = vmic_vol;
        if (events_.on_virtual_mic_volume) events_.on_virtual_mic_volume(vmic_vol);
        if (events_.on_config_modified) events_.on_config_modified();
    }

    render_level_meter("Virtual Mic Level", Audio::AudioPlayer::instance().get_mic_level(), config.virtual_mic_muted);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    render_global_keybinds(config);
}

void UIManager::render_virtual_mic_setup(AppConfig &config) {
    ImGui::Spacing();
    ImGui::Text("PipeWire Virtual Microphone Routing & Subsystem Setup");
    ImGui::Separator();
    ImGui::Spacing();

    // Master Toggle
    bool v_enabled = config.virtual_mic_enabled;
    if (ImGui::Checkbox("Enable Virtual Microphone Subsystem", &v_enabled)) {
        config.virtual_mic_enabled = v_enabled;
        if (events_.on_toggle_virtual_mic) events_.on_toggle_virtual_mic(v_enabled);
        if (events_.on_config_modified) events_.on_config_modified();
    }
    ImGui::SameLine();
    if (config.virtual_mic_enabled) {
        ImGui::TextColored(Theme::Success, "[ENABLED - Online]");
    } else {
        ImGui::TextDisabled("[DISABLED - Local Audio Only]");
    }

    ImGui::Spacing();

    // Channel Routing Controls
    ImGui::TextColored(Theme::TextSubhead, "Channels Feeding Into Virtual Microphone:");

    bool passthru = config.mic_passthrough;
    if (ImGui::Checkbox("Physical Microphone Passthrough", &passthru)) {
        config.mic_passthrough = passthru;
        if (events_.on_toggle_mic) events_.on_toggle_mic(passthru);
        if (events_.on_config_modified) events_.on_config_modified();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(Routes physical microphone to virtual microphone)");

    ImGui::BulletText("Soundboard Audio Mixing: ALWAYS ACTIVE");

    bool media = config.media_to_mic;
    if (ImGui::Checkbox("Desktop Media Audio (Media-to-Mic)", &media)) {
        config.media_to_mic = media;
        if (events_.on_toggle_media) events_.on_toggle_media(media);
        if (events_.on_config_modified) events_.on_config_modified();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(Streams desktop media without creating feedback loops)");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Routing Graph
    ImGui::TextColored(Theme::TextHeader, "Active Audio Routing Topology:");
    render_routing_graph(config);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Test Virtual Microphone
    ImGui::TextColored(Theme::TextSubhead, "Test Virtual Microphone (Hear What Others Hear):");
    ImGui::Text("Temporarily routes VoxBoard_Mic directly to your headphones so you can verify your soundboard and mic:");

    if (!is_testing_mic_) {
        if (ImGui::Button("[>] Start 4-Second Test", ImVec2(200, 30))) {
            is_testing_mic_ = true;
            test_mic_start_time_ = std::chrono::steady_clock::now();
            if (events_.on_test_virtual_mic) events_.on_test_virtual_mic(true);
        }
    } else {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - test_mic_start_time_).count();
        if (elapsed >= 4000) {
            is_testing_mic_ = false;
            if (events_.on_test_virtual_mic) events_.on_test_virtual_mic(false);
        } else {
            float progress = (float)elapsed / 4000.0f;
            char buf[64];
            snprintf(buf, sizeof(buf), "Testing... (%.1fs remaining)", (4000.0f - elapsed) / 1000.0f);
            ImGui::ProgressBar(1.0f - progress, ImVec2(200, 30), buf);
            ImGui::SameLine();
            if (ImGui::Button("Stop Test")) {
                is_testing_mic_ = false;
                if (events_.on_test_virtual_mic) events_.on_test_virtual_mic(false);
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Health Check & Repair
    ImGui::TextColored(Theme::TextHeader, "Virtual Device Diagnostics & Auto-Repair:");
    bool sink_ok = false, source_ok = false;
    bool healthy = Audio::PipeWireManager::instance().check_health(sink_ok, source_ok);

    if (healthy) {
        ImGui::TextColored(Theme::Success, "[OK] Virtual sink (VoxBoard_Sink): ONLINE");
        ImGui::TextColored(Theme::Success, "[OK] Virtual source (VoxBoard_Mic): ONLINE");
    } else {
        if (!sink_ok) ImGui::TextColored(Theme::Danger, "[ERR] Virtual sink (VoxBoard_Sink): MISSING");
        if (!source_ok) ImGui::TextColored(Theme::Danger, "[ERR] Virtual source (VoxBoard_Mic): MISSING");
        ImGui::TextColored(Theme::Warning, "PipeWire service may have restarted or modules were unloaded.");
        if (ImGui::Button("Repair Virtual Audio Devices")) {
            if (events_.on_repair_pipewire) events_.on_repair_pipewire();
        }
    }
}

void UIManager::render_diagnostics(AppConfig &config) {
    ImGui::Spacing();
    render_global_keybinds(config);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("PipeWire Audio Routing Diagnostics:");
    ImGui::BulletText("Sink: VoxBoard_Sink (Null-sink stereo 48000Hz)");
    ImGui::BulletText("Source: VoxBoard_Mic (Remap-source Audio/Source for Discord & games)");
    ImGui::BulletText("Physical Mic Passthrough: %s", Audio::PipeWireManager::instance().is_mic_passthrough() ? "ACTIVE" : "INACTIVE");
    ImGui::BulletText("Media-to-Mic Loopback: %s", Audio::PipeWireManager::instance().is_media_to_mic() ? "ACTIVE" : "INACTIVE");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Hyprland 0.57 Keybind Diagnostics:");
    ImGui::BulletText("Global Action: Stop All Sounds -> '%s'", config.stop_all_keybind.empty() ? "(none)" : config.stop_all_keybind.c_str());
    ImGui::BulletText("Global Action: Toggle Media-to-Mic -> '%s'", config.toggle_media_keybind.empty() ? "(none)" : config.toggle_media_keybind.c_str());
    ImGui::BulletText("Active Sound Group ID: %s", config.active_group_id.c_str());

    for (const auto &grp : config.groups) {
        if (grp.id == config.active_group_id) {
            int bind_count = 0;
            for (const auto &snd : grp.sounds) {
                if (!snd.keybind.empty()) {
                    ImGui::BulletText("Bind: %s -> Play '%s'", snd.keybind.c_str(), snd.name.c_str());
                    ++bind_count;
                }
            }
            if (bind_count == 0) {
                ImGui::BulletText("No keybinds assigned in this group.");
            }
            break;
        }
    }

    ImGui::Spacing();
    if (ImGui::Button("Re-apply Hyprland Keybinds")) {
        Keybinds::HyprlandManager::instance().bind_global_actions(config);
        for (const auto &grp : config.groups) {
            if (grp.id == config.active_group_id) {
                Keybinds::HyprlandManager::instance().bind_group(grp);
                break;
            }
        }
    }
}

static std::string normalize_key_to_xkb(const std::string &raw) {
    if (raw.empty()) return "";

    // Digits 0-9
    if (raw.size() == 1 && raw[0] >= '0' && raw[0] <= '9') return raw;

    // Alphabet A-Z (Hyprland accepts uppercase)
    if (raw.size() == 1 && raw[0] >= 'A' && raw[0] <= 'Z') return raw;
    if (raw.size() == 1 && raw[0] >= 'a' && raw[0] <= 'z') {
        return std::string(1, (char)std::toupper(raw[0]));
    }

    // Function keys F1-F24
    if (raw.size() >= 2 && raw[0] == 'F' && std::isdigit(raw[1])) return raw;

    // Whitespace / Control
    if (raw == "Tab") return "Tab";
    if (raw == "Enter" || raw == "Return") return "Return";
    if (raw == "Escape" || raw == "Esc") return "Escape";
    if (raw == "Space" || raw == "space") return "space";
    if (raw == "Backspace") return "BackSpace";

    // Navigation / Cursor
    if (raw == "LeftArrow") return "Left";
    if (raw == "RightArrow") return "Right";
    if (raw == "UpArrow") return "Up";
    if (raw == "DownArrow") return "Down";
    if (raw == "PageUp") return "Prior";
    if (raw == "PageDown") return "Next";
    if (raw == "Home") return "Home";
    if (raw == "End") return "End";
    if (raw == "Insert") return "Insert";
    if (raw == "Delete") return "Delete";

    // Punctuation / Symbols
    if (raw == "Apostrophe" || raw == "'") return "apostrophe";
    if (raw == "Comma" || raw == ",") return "comma";
    if (raw == "Minus" || raw == "-") return "minus";
    if (raw == "Period" || raw == ".") return "period";
    if (raw == "Slash" || raw == "/") return "slash";
    if (raw == "Semicolon" || raw == ";") return "semicolon";
    if (raw == "Equal" || raw == "=") return "equal";
    if (raw == "LeftBracket" || raw == "[") return "bracketleft";
    if (raw == "RightBracket" || raw == "]") return "bracketright";
    if (raw == "Backslash" || raw == "\\") return "backslash";
    if (raw == "GraveAccent" || raw == "`") return "grave";

    // Locks & system
    if (raw == "CapsLock") return "Caps_Lock";
    if (raw == "ScrollLock") return "Scroll_Lock";
    if (raw == "NumLock") return "Num_Lock";
    if (raw == "PrintScreen") return "Print";
    if (raw == "Pause") return "Pause";

    // Keypad (XKB keysyms KP_0 .. KP_9, KP_Decimal, etc.)
    if (raw == "Keypad0") return "KP_0";
    if (raw == "Keypad1") return "KP_1";
    if (raw == "Keypad2") return "KP_2";
    if (raw == "Keypad3") return "KP_3";
    if (raw == "Keypad4") return "KP_4";
    if (raw == "Keypad5") return "KP_5";
    if (raw == "Keypad6") return "KP_6";
    if (raw == "Keypad7") return "KP_7";
    if (raw == "Keypad8") return "KP_8";
    if (raw == "Keypad9") return "KP_9";
    if (raw == "KeypadDecimal") return "KP_Decimal";
    if (raw == "KeypadDivide") return "KP_Divide";
    if (raw == "KeypadMultiply") return "KP_Multiply";
    if (raw == "KeypadSubtract") return "KP_Subtract";
    if (raw == "KeypadAdd") return "KP_Add";
    if (raw == "KeypadEnter") return "KP_Enter";
    if (raw == "KeypadEqual") return "KP_Equal";

    return raw;
}

void UIManager::handle_key_capture(AppConfig &config) {
    ImGuiIO &io = ImGui::GetIO();

    if (listen_target_ == ListenTarget::None) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) && !io.WantTextInput) {
            if (events_.on_stop_all) events_.on_stop_all();
        }
        return;
    }

    // Independent modifier state detection
    bool super = io.KeySuper || ImGui::IsKeyDown(ImGuiKey_LeftSuper) || ImGui::IsKeyDown(ImGuiKey_RightSuper);
    bool ctrl  = io.KeyCtrl  || ImGui::IsKeyDown(ImGuiKey_LeftCtrl)  || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool alt   = io.KeyAlt   || ImGui::IsKeyDown(ImGuiKey_LeftAlt)   || ImGui::IsKeyDown(ImGuiKey_RightAlt);
    bool shift = io.KeyShift || ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);

    for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k) {
        ImGuiKey key = (ImGuiKey)k;

        // Skip modifiers and modifier pseudo-keys
        if (key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl ||
            key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift ||
            key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt ||
            key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper ||
            key == ImGuiKey_Menu) {
            continue;
        }

        // Exclude reserved ImGui modifier ranges and aliases
        if (key == ImGuiKey_ReservedForModCtrl ||
            key == ImGuiKey_ReservedForModShift ||
            key == ImGuiKey_ReservedForModAlt ||
            key == ImGuiKey_ReservedForModSuper) {
            continue;
        }

        // Exclude mouse and gamepad buttons
        if (key >= ImGuiKey_GamepadStart && key <= ImGuiKey_GamepadRStickDown) continue;
        if (key >= ImGuiKey_MouseLeft && key <= ImGuiKey_MouseWheelY) continue;

        if (ImGui::IsKeyPressed(key)) {
            const char *raw_name = ImGui::GetKeyName(key);
            if (!raw_name || raw_name[0] == '\0') continue;

            std::string kname(raw_name);
            if (kname.find("Mod") != std::string::npos ||
                kname.find("Mouse") != std::string::npos ||
                kname.find("Gamepad") != std::string::npos ||
                kname == "None" || kname == "Unknown") {
                continue;
            }

            std::string xkb_key = normalize_key_to_xkb(kname);
            if (xkb_key.empty()) continue;

            // Build canonical combo string: SUPER + CTRL + ALT + SHIFT + KEY
            std::string combo;
            if (super) combo += "SUPER + ";
            if (ctrl)  combo += "CTRL + ";
            if (alt)   combo += "ALT + ";
            if (shift) combo += "SHIFT + ";
            combo += xkb_key;

            conflict_warning_.clear();

            // Conflict detection & Resolution
            if (listen_target_ == ListenTarget::Sound) {
                // Check if combo conflicts with global actions
                if (combo == config.stop_all_keybind) {
                    conflict_warning_ = "Conflict: " + combo + " is bound to Global 'Stop All Sounds'!";
                } else if (combo == config.toggle_media_keybind) {
                    conflict_warning_ = "Conflict: " + combo + " is bound to Global 'Toggle Media-to-Mic'!";
                }

                // Check other sounds in groups
                for (auto &grp : config.groups) {
                    for (auto &snd : grp.sounds) {
                        if (snd.id != listening_sound_id_ && snd.keybind == combo) {
                            conflict_warning_ = "Reassigned " + combo + " from '" + snd.name + "' to this sound.";
                            snd.keybind.clear();
                        }
                    }
                }

                // Assign to target sound
                for (auto &grp : config.groups) {
                    for (auto &snd : grp.sounds) {
                        if (snd.id == listening_sound_id_) {
                            if (grp.id == config.active_group_id) {
                                if (!snd.keybind.empty()) {
                                    Keybinds::HyprlandManager::instance().unbind_sound(snd.keybind);
                                }
                                std::string err;
                                if (!Keybinds::HyprlandManager::instance().bind_sound(combo, snd.id, &err)) {
                                    conflict_warning_ = "Hyprland registration failed: " + err;
                                }
                            }
                            snd.keybind = combo;
                            break;
                        }
                    }
                }
            } else if (listen_target_ == ListenTarget::StopAll) {
                if (combo == config.toggle_media_keybind) {
                    conflict_warning_ = "Reassigned " + combo + " from Toggle Media-to-Mic to Stop All Sounds.";
                    Keybinds::HyprlandManager::instance().unbind_global_action(config.toggle_media_keybind);
                    config.toggle_media_keybind.clear();
                }

                // Check sounds
                for (auto &grp : config.groups) {
                    for (auto &snd : grp.sounds) {
                        if (snd.keybind == combo) {
                            conflict_warning_ = "Reassigned " + combo + " from sound '" + snd.name + "' to Stop All Sounds.";
                            if (grp.id == config.active_group_id) {
                                Keybinds::HyprlandManager::instance().unbind_sound(snd.keybind);
                            }
                            snd.keybind.clear();
                        }
                    }
                }

                if (!config.stop_all_keybind.empty()) {
                    Keybinds::HyprlandManager::instance().unbind_global_action(config.stop_all_keybind);
                }
                config.stop_all_keybind = combo;
                std::string err;
                if (!Keybinds::HyprlandManager::instance().bind_global_action(combo, "--stop", &err)) {
                    conflict_warning_ = "Hyprland registration failed: " + err;
                }
                if (events_.on_change_stop_all_bind) events_.on_change_stop_all_bind(combo);
            } else if (listen_target_ == ListenTarget::ToggleMedia) {
                if (combo == config.stop_all_keybind) {
                    conflict_warning_ = "Reassigned " + combo + " from Stop All Sounds to Toggle Media-to-Mic.";
                    Keybinds::HyprlandManager::instance().unbind_global_action(config.stop_all_keybind);
                    config.stop_all_keybind.clear();
                }

                // Check sounds
                for (auto &grp : config.groups) {
                    for (auto &snd : grp.sounds) {
                        if (snd.keybind == combo) {
                            conflict_warning_ = "Reassigned " + combo + " from sound '" + snd.name + "' to Toggle Media-to-Mic.";
                            if (grp.id == config.active_group_id) {
                                Keybinds::HyprlandManager::instance().unbind_sound(snd.keybind);
                            }
                            snd.keybind.clear();
                        }
                    }
                }

                if (!config.toggle_media_keybind.empty()) {
                    Keybinds::HyprlandManager::instance().unbind_global_action(config.toggle_media_keybind);
                }
                config.toggle_media_keybind = combo;
                std::string err;
                if (!Keybinds::HyprlandManager::instance().bind_global_action(combo, "--media toggle", &err)) {
                    conflict_warning_ = "Hyprland registration failed: " + err;
                }
                if (events_.on_change_toggle_media_bind) events_.on_change_toggle_media_bind(combo);
            }

            if (events_.on_config_modified) events_.on_config_modified();
            listen_target_ = ListenTarget::None;
            listening_sound_id_.clear();
            break;
        }
    }
}

} // namespace UI
