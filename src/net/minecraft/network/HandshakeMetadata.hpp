#pragma once
#include <string>
#include <unordered_map>
#include <vector>
namespace net::minecraft::network {
struct HandshakeMetadata {
  std::string serverId;
  bool hasOmegaMetadata = false;
  bool nativeCppMods = false;
  bool luaModsEnabled = false;
  std::vector<std::string> requiredMods;
  std::unordered_map<std::string, std::string> downloadUrls;
};
[[nodiscard]] std::string appendHandshakeMetadata(const std::string& serverId, bool nativeCppMods, bool luaModsEnabled,
                                                  const std::vector<std::string>& requiredMods,
                                                  const std::unordered_map<std::string, std::string>& downloadUrls);
[[nodiscard]] HandshakeMetadata parseHandshakeMetadata(const std::string& raw);
} // namespace net::minecraft::network
