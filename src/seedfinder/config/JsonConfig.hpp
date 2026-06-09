#pragma once

#include "seedfinder/config/ConfigSchema.hpp"

#include <string>

namespace seedfinder::config {

struct LoadResult {
    bool ok = false;
    std::string error;
    SearchConfig config;
};

[[nodiscard]] LoadResult loadConfigFromFile(const std::string& path);
[[nodiscard]] LoadResult loadConfigFromString(const std::string& json);

} // namespace seedfinder::config
