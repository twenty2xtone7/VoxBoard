cmake_minimum_required(VERSION 3.20)
project(voxboard LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(DEPS REQUIRED
    glfw3
    gl
    sndfile
    samplerate
    libcurl
    yyjson
    ayatana-appindicator3-0.1
    gtk+-3.0
    libpulse-simple
    libpulse
)

include_directories(
    src
    vendor/imgui
    vendor/imgui/backends
    ${DEPS_INCLUDE_DIRS}
)

add_compile_options(-Wno-deprecated-declarations)

set(SOURCES
    src/main.cpp
    src/storage/config_store.cpp
    src/audio/pipewire_manager.cpp
    src/audio/audio_player.cpp
    src/keybinds/hyprland_binds.cpp
    src/ipc/ipc_server.cpp
    src/net/myinstants.cpp
    src/tray/tray_manager.cpp
    src/ui/ui_manager.cpp
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
)

add_executable(voxboard ${SOURCES})

target_link_libraries(voxboard PRIVATE
    ${DEPS_LIBRARIES}
    pthread
)

enable_testing()

add_executable(test_group_ops
    tests/test_group_ops.cpp
    src/storage/config_store.cpp
)
target_link_libraries(test_group_ops PRIVATE ${DEPS_LIBRARIES} pthread)
add_test(NAME test_group_ops COMMAND test_group_ops)

add_executable(test_audio_settings
    tests/test_audio_settings.cpp
    src/storage/config_store.cpp
    src/audio/pipewire_manager.cpp
)
target_link_libraries(test_audio_settings PRIVATE ${DEPS_LIBRARIES} pthread)
add_test(NAME test_audio_settings COMMAND test_audio_settings)

add_executable(test_keybind_logic
    tests/test_keybind_logic.cpp
    src/storage/config_store.cpp
    src/keybinds/hyprland_binds.cpp
)
target_link_libraries(test_keybind_logic PRIVATE ${DEPS_LIBRARIES} pthread)
add_test(NAME test_keybind_logic COMMAND test_keybind_logic)
