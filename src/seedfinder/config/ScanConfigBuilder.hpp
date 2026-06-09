#pragma once

#include "seedfinder/config/ConfigSchema.hpp"
#include "seedfinder/config/ScanConfig.hpp"

namespace seedfinder::config {

// Converts the legacy flat SearchConfig (JSON keys + GUI form fields) into the new
// ScanConfig + node graph representation used by SearchEngine.
[[nodiscard]] ScanConfig toScanConfig(const SearchConfig& legacy);

} // namespace seedfinder::config
