# VoxBoard

Lightweight native Linux soundboard built for Wayland and Hyprland. Routes sound clips and microphone audio through PipeWire virtual devices into Discord, games, and voice apps.

---

## Features

- **Dual Audio Routing**: Route sounds to local headphones, virtual mic, or both simultaneously.
- **PipeWire / PulseAudio Integration**: Automatic virtual source creation (`VoxBoard_Mic`) and virtual sink (`VoxBoard_Sink`) with passthrough management.
- **Hyprland Global Keybinds**: Direct Hyprland IPC evaluation (`hyprctl`) for global shortcuts without grab conflicts. Group-based hotkey profiles.
- **In-App Hotkey Capture**: Single-key (`Escape`, `1`) and multi-modifier (`Super+Shift+Q`, `Ctrl+Alt+S`) binding interface with conflict detection.
- **System Tray (SNI)**: AppIndicator integration for Quickshell, Waybar, and desktop tray bars. Left-click toggle, quick group switcher, volume toggles.
- **UNIX IPC / CLI**: Control running instance via command-line flags (`--play`, `--stop`, `--show`, `--hide`, `--media`, `--mic`, `--group`).
- **MyInstants Browser**: Search, preview, and download sounds directly into sound groups.
- **Fast & Low Overhead**: Dear ImGui + OpenGL3 interface, `yyjson` config serialization, no Electron bloat.

---

## Architecture

```
                 +-----------------------+
                 |    Audio Files / UI   |
                 +-----------+-----------+
                             |
                      [Audio Player]
                       /          \
                      v            v
             [Physical Sink]   [VoxBoard_Sink]
             (Local Headset)          |
                                      v
 [Physical Mic] -------------> [VoxBoard_Mic] ------> Voice Apps / Discord
                (Passthrough)  (Virtual Source)
```

---

## Dependencies

### Arch Linux / EndeavourOS
```bash
sudo pacman -S --needed \
    base-devel \
    cmake \
    git \
    glfw-wayland \
    mesa \
    libpulse \
    libsndfile \
    libsamplerate \
    curl \
    yyjson \
    libayatana-appindicator \
    gtk3
```

---

## Build & Install

```bash
# Clone with submodules (Dear ImGui)
git clone --recurse-submodules https://github.com/twenty2xtone7/VoxBoard.git
cd VoxBoard

# Configure and compile
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run tests
ctest --test-dir build --output-on-failure

# Launch
./build/voxboard
```

---

## CLI & IPC Usage

If VoxBoard is running in the background or system tray, use CLI arguments to control it:

| Command | Description |
|---|---|
| `voxboard --show` | Restore and focus main window |
| `voxboard --hide` | Minimize window to system tray |
| `voxboard --play <SOUND_ID>` | Trigger sound by ID |
| `voxboard --stop` | Stop all playing sounds |
| `voxboard --media <on\|off\|toggle>` | Toggle soundboard routing to virtual mic |
| `voxboard --mic <on\|off>` | Toggle physical microphone passthrough |
| `voxboard --group <GROUP_ID>` | Switch active keybind profile |
| `voxboard --status` | Print playback and virtual device state |

---

## Configuration

Configuration saved automatically to:
```
~/.config/voxboard/config.json
```

Stores sound groups, key combinations, volumes, device selections, and active profiles.

---

## License

MIT
