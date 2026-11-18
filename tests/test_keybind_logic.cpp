#include <cassert>
#include <iostream>
#include <algorithm>
#include "types.hpp"
#include "storage/config_store.hpp"
#include "keybinds/hyprland_binds.hpp"

// Test harness for normalization logic
static std::string normalize_key_to_xkb(const std::string &raw) {
    if (raw.empty()) return "";
    if (raw.size() == 1 && raw[0] >= '0' && raw[0] <= '9') return raw;
    if (raw.size() == 1 && raw[0] >= 'A' && raw[0] <= 'Z') return raw;
    if (raw.size() == 1 && raw[0] >= 'a' && raw[0] <= 'z') return std::string(1, (char)std::toupper(raw[0]));
    if (raw.size() >= 2 && raw[0] == 'F' && std::isdigit(raw[1])) return raw;

    if (raw == "Tab") return "Tab";
    if (raw == "Enter") return "Return";
    if (raw == "Escape") return "Escape";
    if (raw == "Space") return "space";
    if (raw == "Backspace") return "BackSpace";
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

    if (raw == "CapsLock") return "Caps_Lock";
    if (raw == "ScrollLock") return "Scroll_Lock";
    if (raw == "NumLock") return "Num_Lock";
    if (raw == "PrintScreen") return "Print";
    if (raw == "Pause") return "Pause";

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

static std::string format_combo(bool super, bool ctrl, bool alt, bool shift, const std::string &raw_key) {
    std::string xkb = normalize_key_to_xkb(raw_key);
    if (xkb.empty()) return "";
    std::string combo;
    if (super) combo += "SUPER + ";
    if (ctrl)  combo += "CTRL + ";
    if (alt)   combo += "ALT + ";
    if (shift) combo += "SHIFT + ";
    combo += xkb;
    return combo;
}

int main() {
    setenv("XDG_CONFIG_HOME", "/tmp/antigravity/voxboard_test", 1);

    // 1. Test XKB Keysym mappings
    assert(normalize_key_to_xkb("Tab") == "Tab");
    assert(normalize_key_to_xkb("Enter") == "Return");
    assert(normalize_key_to_xkb("Space") == "space");
    assert(normalize_key_to_xkb("Escape") == "Escape");
    assert(normalize_key_to_xkb("LeftBracket") == "bracketleft");
    assert(normalize_key_to_xkb("RightBracket") == "bracketright");
    assert(normalize_key_to_xkb("Keypad1") == "KP_1");
    assert(normalize_key_to_xkb("KeypadAdd") == "KP_Add");
    assert(normalize_key_to_xkb("PageUp") == "Prior");
    assert(normalize_key_to_xkb("PageDown") == "Next");

    // 2. Test Combo generation regardless of physical input order
    // Super + Shift + Q
    assert(format_combo(true, false, false, true, "Q") == "SUPER + SHIFT + Q");
    // Ctrl + Alt + S
    assert(format_combo(false, true, true, false, "S") == "CTRL + ALT + S");
    // Super + Ctrl + Shift + Q
    assert(format_combo(true, true, false, true, "Q") == "SUPER + CTRL + SHIFT + Q");
    // Super + Alt + Escape
    assert(format_combo(true, false, true, false, "Escape") == "SUPER + ALT + Escape");
    // Super + Alt + M
    assert(format_combo(true, false, true, false, "M") == "SUPER + ALT + M");

    // 3. Test Executable path resolution
    std::string exe = Keybinds::HyprlandManager::get_executable_path();
    assert(!exe.empty());
    assert(exe.find("test_keybind_logic") != std::string::npos || exe.find("voxboard") != std::string::npos);

    // 4. Test live Hyprland global shortcuts binding and unbinding
    auto &hl = Keybinds::HyprlandManager::instance();
    AppConfig cfg = Storage::get_default_config();
    assert(cfg.stop_all_keybind == "Escape");
    assert(cfg.toggle_media_keybind == "SUPER + ALT + M");

    std::string err;
    bool b1 = hl.bind_global_action(cfg.stop_all_keybind, "--stop", &err);
    assert(b1);

    bool b2 = hl.bind_global_action(cfg.toggle_media_keybind, "--media toggle", &err);
    assert(b2);

    // Also verify multi-key combo registration
    bool b3 = hl.bind_global_action("SUPER + ALT + Escape", "--stop", &err);
    assert(b3);

    hl.unbind_all();

    std::cout << "[Test] Keybind normalization & Hyprland registration tests passed.\n";
    return 0;
}
