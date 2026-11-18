#include <cassert>
#include <iostream>
#include "types.hpp"
#include "storage/config_store.hpp"

int main() {
    setenv("XDG_CONFIG_HOME", "/tmp/antigravity/voxboard_test", 1);
    AppConfig config = Storage::get_default_config();
    assert(config.groups.size() == 1);
    assert(config.active_group_id == config.groups[0].id);

    // Simulate multiple group additions causing vector reallocations
    for (int i = 0; i < 20; ++i) {
        SoundGroup g;
        g.id = Storage::generate_id();
        g.name = "Test Group " + std::to_string(i);
        config.groups.push_back(g);
        config.active_group_id = g.id;

        // Verify active group lookup after mutation
        SoundGroup *active = nullptr;
        for (auto &grp : config.groups) {
            if (grp.id == config.active_group_id) {
                active = &grp;
                break;
            }
        }
        assert(active != nullptr);
        assert(active->name == "Test Group " + std::to_string(i));
    }

    // Verify persistence round-trip
    assert(Storage::save_config(config));
    AppConfig loaded = Storage::load_config();
    assert(loaded.groups.size() == 21);
    assert(loaded.active_group_id == config.active_group_id);

    std::cout << "[Test] Group operations passed.\n";
    return 0;
}
