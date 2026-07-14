#include "net/minecraft/mod/HostScreenRegistry.hpp"
#include <unordered_map>
namespace net::minecraft::mod {
namespace {
std::unordered_map<std::string, HostScreenOpener>& hostScreenOpeners() {
  static std::unordered_map<std::string, HostScreenOpener> openers;
  return openers;
}
} // namespace
void registerHostScreen(std::string_view screenId, HostScreenOpener opener) {
  if(screenId.empty() || opener == nullptr) {
    return;
  }
  hostScreenOpeners()[std::string(screenId)] = opener;
}
bool openHostScreen(std::string_view screenId, const std::unordered_map<std::string, std::string>& fields) {
  if(screenId.empty()) {
    return false;
  }
  const auto found = hostScreenOpeners().find(std::string(screenId));
  if(found == hostScreenOpeners().end()) {
    return false;
  }
  found->second(fields);
  return true;
}
} // namespace net::minecraft::mod
