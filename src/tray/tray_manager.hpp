#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <functional>

typedef struct _AppIndicator AppIndicator;
typedef struct _GtkWidget GtkWidget;

namespace Tray {

struct TrayCallbacks {
    std::function<void()> on_show_window;
    std::function<void()> on_hide_window;
    std::function<void(bool)> on_toggle_media;
    std::function<void(bool)> on_toggle_mic;
    std::function<void(const std::string&)> on_select_group;
    std::function<void()> on_stop_all;
    std::function<void()> on_quit;
};

class TrayManager {
public:
    static TrayManager& instance();

    TrayManager();
    ~TrayManager();

    bool init(const TrayCallbacks &cbs);
    void update_groups(const std::vector<SoundGroup> &groups, const std::string &active_id);
    void update_toggles(bool media_to_mic, bool mic_passthrough);
    void pump_events();

private:
    void rebuild_menu();

    AppIndicator *indicator_ = nullptr;
    GtkWidget *menu_ = nullptr;
    GtkWidget *media_item_ = nullptr;
    GtkWidget *mic_item_ = nullptr;
    GtkWidget *group_submenu_ = nullptr;

    TrayCallbacks cbs_;
    std::vector<SoundGroup> current_groups_;
    std::string current_active_id_;
    bool current_media_ = false;
    bool current_mic_ = true;
    bool updating_toggles_ = false;
    bool updating_menu_ = false;
};

} // namespace Tray
