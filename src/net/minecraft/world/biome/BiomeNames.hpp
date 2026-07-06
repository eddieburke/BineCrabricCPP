#pragma once
#include <string>
#include <string_view>
#include <vector>
namespace net::minecraft {
[[nodiscard]] std::string biomeWireName(int biomeId);
[[nodiscard]] std::vector<std::string> allBiomeWireNames();
[[nodiscard]] std::vector<std::string> spawnPickerBiomeWireNames();
[[nodiscard]] int biomeIdFromWireName(std::string_view name);
} // namespace net::minecraft
