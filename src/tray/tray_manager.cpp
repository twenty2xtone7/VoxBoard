#include "tray_manager.hpp"
#include <libayatana-appindicator/app-indicator.h>
#include <gtk/gtk.h>
#include <iostream>

namespace Tray {

TrayManager& TrayManager::instance() {
    static TrayManager inst;
    return inst;
}

TrayManager::TrayManager() = default;

TrayManager::~TrayManager() {
    if (indicator_) {
        app_indicator_set_status(indicator_, APP_INDICATOR_STATUS_PASSIVE);
        indicator_ = nullptr;
    }
}

static void on_open_clicked(GtkWidget*, gpointer user_data) {
    auto *tm = static_cast<TrayManager*>(user_data);
    if (tm) tm->pump_events();
    auto &cbs = TrayManager::instance();
}

bool TrayManager::init(const TrayCallbacks &cbs) {
    cbs_ = cbs;

    int argc = 0;
    char **argv = nullptr;
    if (!gtk_init_check(&argc, &argv)) {
        std::cerr << "[Tray] Failed to init GTK\n";
        return false;
    }

    indicator_ = app_indicator_new(
        "voxboard",
        "audio-volume-high",
        APP_INDICATOR_CATEGORY_APPLICATION_STATUS
    );

    if (!indicator_) {
        std::cerr << "[Tray] Failed to create AppIndicator\n";
        return false;
    }

    app_indicator_set_status(indicator_, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_title(indicator_, "VoxBoard");

    // Connect primary click (SNI Activate) from bar / tray icon (e.g. Quickshell / Caelestia)
    g_signal_connect(indicator_, "activate", G_CALLBACK(+[](AppIndicator*, gint, gint, gpointer) {
        if (TrayManager::instance().cbs_.on_show_window) {
            TrayManager::instance().cbs_.on_show_window();
        }
    }), nullptr);

    rebuild_menu();
    return true;
}

void TrayManager::rebuild_menu() {
    if (!indicator_) return;
    updating_menu_ = true;

    if (menu_) {
        app_indicator_set_secondary_activate_target(indicator_, nullptr);
        gtk_widget_destroy(menu_);
        menu_ = nullptr;
    }

    menu_ = gtk_menu_new();

    // 1. Open VoxBoard
    GtkWidget *open_item = gtk_menu_item_new_with_label("Open VoxBoard");
    g_signal_connect(open_item, "activate", G_CALLBACK(+[](GtkMenuItem*, gpointer) {
        std::cout << "[Tray] Open VoxBoard activated via GTK signal\n" << std::flush;
        if (TrayManager::instance().cbs_.on_show_window) {
            TrayManager::instance().cbs_.on_show_window();
        }
    }), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_), open_item);

    // 2. Hide to Tray
    GtkWidget *hide_item = gtk_menu_item_new_with_label("Hide to Tray");
    g_signal_connect_swapped(hide_item, "activate", G_CALLBACK(+[](void*) {
        if (TrayManager::instance().cbs_.on_hide_window) {
            TrayManager::instance().cbs_.on_hide_window();
        }
    }), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_), hide_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_), gtk_separator_menu_item_new());

    // 3. Media-to-Mic Checkbox
    media_item_ = gtk_check_menu_item_new_with_label("Media-to-Mic");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(media_item_), current_media_);
    g_signal_connect(media_item_, "toggled", G_CALLBACK(+[](GtkCheckMenuItem *item, gpointer) {
        if (TrayManager::instance().updating_menu_ || TrayManager::instance().updating_toggles_) return;
        gboolean active = gtk_check_menu_item_get_active(item);
        if (TrayManager::instance().cbs_.on_toggle_media) {
            TrayManager::instance().cbs_.on_toggle_media(active);
        }
    }), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_), media_item_);

    // 4. Mic Passthrough Checkbox
    mic_item_ = gtk_check_menu_item_new_with_label("Mic Passthrough");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mic_item_), current_mic_);
    g_signal_connect(mic_item_, "toggled", G_CALLBACK(+[](GtkCheckMenuItem *item, gpointer) {
        if (TrayManager::instance().updating_menu_ || TrayManager::instance().updating_toggles_) return;
        gboolean active = gtk_check_menu_item_get_active(item);
        if (TrayManager::instance().cbs_.on_toggle_mic) {
            TrayManager::instance().cbs_.on_toggle_mic(active);
        }
    }), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_), mic_item_);

    // 5. Active Group Submenu
    GtkWidget *groups_parent = gtk_menu_item_new_with_label("Active Group");
    group_submenu_ = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(groups_parent), group_submenu_);

    GSList *group_radio = nullptr;
    for (const auto &grp : current_groups_) {
        std::string label = grp.name;
        GtkWidget *ritem = gtk_radio_menu_item_new_with_label(group_radio, label.c_str());
        group_radio = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(ritem));

        if (grp.id == current_active_id_) {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ritem), TRUE);
        }

        char *gid = g_strdup(grp.id.c_str());
        g_object_set_data_full(G_OBJECT(ritem), "group_id", gid, g_free);

        g_signal_connect(ritem, "toggled", G_CALLBACK(+[](GtkCheckMenuItem *item, gpointer) {
            if (TrayManager::instance().updating_menu_) return;
            if (!gtk_check_menu_item_get_active(item)) return;
            const char *id = static_cast<const char*>(g_object_get_data(G_OBJECT(item), "group_id"));
            if (id && TrayManager::instance().cbs_.on_select_group) {
                TrayManager::instance().cbs_.on_select_group(std::string(id));
            }
        }), nullptr);

        gtk_menu_shell_append(GTK_MENU_SHELL(group_submenu_), ritem);
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_), groups_parent);

    // 6. Stop All Sounds
    GtkWidget *stop_item = gtk_menu_item_new_with_label("Stop All Sounds");
    g_signal_connect_swapped(stop_item, "activate", G_CALLBACK(+[](void*) {
        if (TrayManager::instance().cbs_.on_stop_all) {
            TrayManager::instance().cbs_.on_stop_all();
        }
    }), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_), stop_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_), gtk_separator_menu_item_new());

    // 7. Quit
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit VoxBoard");
    g_signal_connect_swapped(quit_item, "activate", G_CALLBACK(+[](void*) {
        if (TrayManager::instance().cbs_.on_quit) {
            TrayManager::instance().cbs_.on_quit();
        }
    }), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_), quit_item);

    gtk_widget_show_all(menu_);
    app_indicator_set_menu(indicator_, GTK_MENU(menu_));
    app_indicator_set_secondary_activate_target(indicator_, open_item);
    updating_menu_ = false;
}

void TrayManager::update_groups(const std::vector<SoundGroup> &groups, const std::string &active_id) {
    if (updating_menu_) return;
    current_groups_ = groups;
    current_active_id_ = active_id;
    rebuild_menu();
}

void TrayManager::update_toggles(bool media_to_mic, bool mic_passthrough) {
    current_media_ = media_to_mic;
    current_mic_ = mic_passthrough;
    updating_toggles_ = true;
    if (media_item_) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(media_item_), media_to_mic);
    }
    if (mic_item_) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mic_item_), mic_passthrough);
    }
    updating_toggles_ = false;
}

void TrayManager::pump_events() {
    while (g_main_context_iteration(NULL, FALSE)) {
        // Pump GTK/DBus events
    }
}

} // namespace Tray
