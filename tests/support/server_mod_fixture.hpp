#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include "net/minecraft/mod/runtime/ModHost.hpp"
namespace net::minecraft::test {
inline void writeServerScriptMod(const std::filesystem::path& runDirectory,
                                 const std::string& modId,
                                 const std::string& side = "both",
                                 const std::string& downloadUrl = {}) {
 const std::filesystem::path modRoot = runDirectory / "mods" / modId;
 std::filesystem::create_directories(modRoot / "scripts");
 std::ofstream manifest(modRoot / "mod.json", std::ios::binary | std::ios::trunc);
 manifest << "{\"id\":\"" << modId << "\",\"name\":\"" << modId
          << "\",\"version\":\"1.0.0\",\"enabled\":true,\"side\":\"" << side << "\",";
 if(!downloadUrl.empty()) {
  manifest << "\"download_url\":\"" << downloadUrl << "\",";
 }
 manifest << "\"entry\":\"scripts/main.lua\"}\n";
 std::ofstream(modRoot / "scripts" / "main.lua", std::ios::binary | std::ios::trunc)
     << "minecraft.log(\"info\",\"" << modId << "\")\n";
}
} // namespace net::minecraft::test
