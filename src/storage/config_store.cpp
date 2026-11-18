#include "config_store.hpp"
#include <yyjson.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <cstdlib>

namespace fs = std::filesystem;

namespace Storage {

static std::string get_home() {
    const char *h = std::getenv("HOME");
    return h ? std::string(h) : "/home/twenty2xtone";
}

std::string get_config_dir() {
    const char *xdg = std::getenv("XDG_CONFIG_HOME");
    std::string path;
    if (xdg && *xdg) {
        path = std::string(xdg) + "/voxboard";
    } else {
        path = get_home() + "/.config/voxboard";
    }
    fs::create_directories(path);
    return path;
}

std::string get_sounds_dir() {
    const char *xdg = std::getenv("XDG_DATA_HOME");
    std::string path;
    if (xdg && *xdg) {
        path = std::string(xdg) + "/voxboard/sounds";
    } else {
        path = get_home() + "/.local/share/voxboard/sounds";
    }
    fs::create_directories(path);
    return path;
}

std::string generate_id() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    std::stringstream ss;
    ss << std::hex << dis(gen);
    return ss.str();
}

AppConfig get_default_config() {
    AppConfig cfg;
    cfg.media_to_mic = false;
    cfg.mic_passthrough = true;
    cfg.monitor_volume = 1.0f;
    cfg.mic_volume = 1.0f;
    cfg.passthrough_volume = 1.0f;
    cfg.virtual_mic_volume = 1.0f;
    cfg.stop_all_keybind = "Escape";
    cfg.toggle_media_keybind = "SUPER + ALT + M";

    SoundGroup def_group;
    def_group.id = generate_id();
    def_group.name = "Default Group";
    cfg.groups.push_back(def_group);
    cfg.active_group_id = def_group.id;
    return cfg;
}

AppConfig load_config() {
    std::string filepath = get_config_dir() + "/config.json";
    if (!fs::exists(filepath)) {
        AppConfig cfg = get_default_config();
        save_config(cfg);
        return cfg;
    }

    yyjson_read_err err;
    yyjson_doc *doc = yyjson_read_file(filepath.c_str(), 0, NULL, &err);
    if (!doc) {
        AppConfig cfg = get_default_config();
        return cfg;
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) {
        yyjson_doc_free(doc);
        return get_default_config();
    }

    AppConfig cfg;
    yyjson_val *v;

    v = yyjson_obj_get(root, "active_group_id");
    if (v && yyjson_is_str(v)) cfg.active_group_id = yyjson_get_str(v);

    v = yyjson_obj_get(root, "media_to_mic");
    if (v && yyjson_is_bool(v)) cfg.media_to_mic = yyjson_get_bool(v);

    v = yyjson_obj_get(root, "mic_passthrough");
    if (v && yyjson_is_bool(v)) cfg.mic_passthrough = yyjson_get_bool(v);

    v = yyjson_obj_get(root, "monitor_volume");
    if (v && yyjson_is_num(v)) cfg.monitor_volume = (float)yyjson_get_num(v);

    v = yyjson_obj_get(root, "mic_volume");
    if (v && yyjson_is_num(v)) cfg.mic_volume = (float)yyjson_get_num(v);

    v = yyjson_obj_get(root, "physical_mic_device");
    if (v && yyjson_is_str(v)) cfg.physical_mic_device = yyjson_get_str(v);

    v = yyjson_obj_get(root, "soundboard_output_device");
    if (v && yyjson_is_str(v)) cfg.soundboard_output_device = yyjson_get_str(v);

    v = yyjson_obj_get(root, "desktop_capture_source");
    if (v && yyjson_is_str(v)) cfg.desktop_capture_source = yyjson_get_str(v);

    v = yyjson_obj_get(root, "virtual_mic_enabled");
    if (v && yyjson_is_bool(v)) cfg.virtual_mic_enabled = yyjson_get_bool(v);

    v = yyjson_obj_get(root, "mic_muted");
    if (v && yyjson_is_bool(v)) cfg.mic_muted = yyjson_get_bool(v);

    v = yyjson_obj_get(root, "monitor_muted");
    if (v && yyjson_is_bool(v)) cfg.monitor_muted = yyjson_get_bool(v);

    v = yyjson_obj_get(root, "virtual_mic_muted");
    if (v && yyjson_is_bool(v)) cfg.virtual_mic_muted = yyjson_get_bool(v);

    v = yyjson_obj_get(root, "passthrough_volume");
    if (v && yyjson_is_num(v)) cfg.passthrough_volume = (float)yyjson_get_num(v);

    v = yyjson_obj_get(root, "virtual_mic_volume");
    if (v && yyjson_is_num(v)) cfg.virtual_mic_volume = (float)yyjson_get_num(v);

    v = yyjson_obj_get(root, "stop_all_keybind");
    if (v && yyjson_is_str(v)) cfg.stop_all_keybind = yyjson_get_str(v);

    v = yyjson_obj_get(root, "toggle_media_keybind");
    if (v && yyjson_is_str(v)) cfg.toggle_media_keybind = yyjson_get_str(v);

    yyjson_val *groups_arr = yyjson_obj_get(root, "groups");
    if (groups_arr && yyjson_is_arr(groups_arr)) {
        size_t idx, max;
        yyjson_val *grp_val;
        yyjson_arr_foreach(groups_arr, idx, max, grp_val) {
            if (!yyjson_is_obj(grp_val)) continue;
            SoundGroup group;
            yyjson_val *gv;

            gv = yyjson_obj_get(grp_val, "id");
            if (gv && yyjson_is_str(gv)) group.id = yyjson_get_str(gv);

            gv = yyjson_obj_get(grp_val, "name");
            if (gv && yyjson_is_str(gv)) group.name = yyjson_get_str(gv);

            yyjson_val *sounds_arr = yyjson_obj_get(grp_val, "sounds");
            if (sounds_arr && yyjson_is_arr(sounds_arr)) {
                size_t sidx, smax;
                yyjson_val *snd_val;
                yyjson_arr_foreach(sounds_arr, sidx, smax, snd_val) {
                    if (!yyjson_is_obj(snd_val)) continue;
                    Sound snd;
                    yyjson_val *sv;

                    sv = yyjson_obj_get(snd_val, "id");
                    if (sv && yyjson_is_str(sv)) snd.id = yyjson_get_str(sv);

                    sv = yyjson_obj_get(snd_val, "name");
                    if (sv && yyjson_is_str(sv)) snd.name = yyjson_get_str(sv);

                    sv = yyjson_obj_get(snd_val, "path");
                    if (sv && yyjson_is_str(sv)) snd.path = yyjson_get_str(sv);

                    sv = yyjson_obj_get(snd_val, "volume");
                    if (sv && yyjson_is_num(sv)) snd.volume = (float)yyjson_get_num(sv);

                    sv = yyjson_obj_get(snd_val, "keybind");
                    if (sv && yyjson_is_str(sv)) snd.keybind = yyjson_get_str(sv);

                    group.sounds.push_back(snd);
                }
            }
            cfg.groups.push_back(group);
        }
    }

    yyjson_doc_free(doc);

    if (cfg.groups.empty()) {
        SoundGroup def_group;
        def_group.id = generate_id();
        def_group.name = "Default Group";
        cfg.groups.push_back(def_group);
        cfg.active_group_id = def_group.id;
    } else if (cfg.active_group_id.empty()) {
        cfg.active_group_id = cfg.groups[0].id;
    }

    return cfg;
}

bool save_config(const AppConfig &cfg) {
    std::string filepath = get_config_dir() + "/config.json";

    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_str(doc, root, "active_group_id", cfg.active_group_id.c_str());
    yyjson_mut_obj_add_bool(doc, root, "media_to_mic", cfg.media_to_mic);
    yyjson_mut_obj_add_bool(doc, root, "mic_passthrough", cfg.mic_passthrough);
    yyjson_mut_obj_add_real(doc, root, "monitor_volume", cfg.monitor_volume);
    yyjson_mut_obj_add_real(doc, root, "mic_volume", cfg.mic_volume);
    yyjson_mut_obj_add_str(doc, root, "physical_mic_device", cfg.physical_mic_device.c_str());
    yyjson_mut_obj_add_str(doc, root, "soundboard_output_device", cfg.soundboard_output_device.c_str());
    yyjson_mut_obj_add_str(doc, root, "desktop_capture_source", cfg.desktop_capture_source.c_str());
    yyjson_mut_obj_add_bool(doc, root, "virtual_mic_enabled", cfg.virtual_mic_enabled);
    yyjson_mut_obj_add_bool(doc, root, "mic_muted", cfg.mic_muted);
    yyjson_mut_obj_add_bool(doc, root, "monitor_muted", cfg.monitor_muted);
    yyjson_mut_obj_add_bool(doc, root, "virtual_mic_muted", cfg.virtual_mic_muted);
    yyjson_mut_obj_add_real(doc, root, "passthrough_volume", cfg.passthrough_volume);
    yyjson_mut_obj_add_real(doc, root, "virtual_mic_volume", cfg.virtual_mic_volume);
    yyjson_mut_obj_add_str(doc, root, "stop_all_keybind", cfg.stop_all_keybind.c_str());
    yyjson_mut_obj_add_str(doc, root, "toggle_media_keybind", cfg.toggle_media_keybind.c_str());

    yyjson_mut_val *groups_arr = yyjson_mut_arr(doc);
    yyjson_mut_obj_add_val(doc, root, "groups", groups_arr);

    for (const auto &grp : cfg.groups) {
        yyjson_mut_val *gobj = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_str(doc, gobj, "id", grp.id.c_str());
        yyjson_mut_obj_add_str(doc, gobj, "name", grp.name.c_str());

        yyjson_mut_val *sounds_arr = yyjson_mut_arr(doc);
        yyjson_mut_obj_add_val(doc, gobj, "sounds", sounds_arr);

        for (const auto &snd : grp.sounds) {
            yyjson_mut_val *sobj = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_str(doc, sobj, "id", snd.id.c_str());
            yyjson_mut_obj_add_str(doc, sobj, "name", snd.name.c_str());
            yyjson_mut_obj_add_str(doc, sobj, "path", snd.path.c_str());
            yyjson_mut_obj_add_real(doc, sobj, "volume", snd.volume);
            yyjson_mut_obj_add_str(doc, sobj, "keybind", snd.keybind.c_str());
            yyjson_mut_arr_append(sounds_arr, sobj);
        }

        yyjson_mut_arr_append(groups_arr, gobj);
    }

    size_t len = 0;
    char *json = yyjson_mut_write(doc, YYJSON_WRITE_PRETTY, &len);
    if (!json) {
        yyjson_mut_doc_free(doc);
        return false;
    }

    std::ofstream out(filepath, std::ios::trunc);
    if (!out.is_open()) {
        free(json);
        yyjson_mut_doc_free(doc);
        return false;
    }

    out.write(json, len);
    out.close();

    free(json);
    yyjson_mut_doc_free(doc);
    return true;
}

} // namespace Storage
