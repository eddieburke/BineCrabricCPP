#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
namespace net::minecraft::mod {
using HostScreenOpener = void (*)(const std::unordered_map<std::string, std::string>& fields);
void registerHostScreen(std::string_view screenId, HostScreenOpener opener);
bool openHostScreen(std::string_view screenId, const std::unordered_map<std::string, std::string>& fields);
} // namespace net::minecraft::mod
