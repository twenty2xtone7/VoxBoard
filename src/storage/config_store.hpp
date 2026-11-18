#pragma once

#include "types.hpp"
#include <string>

namespace Storage {

std::string get_config_dir();
std::string get_sounds_dir();
std::string generate_id();

AppConfig get_default_config();
AppConfig load_config();
bool save_config(const AppConfig &cfg);

} // namespace Storage
