#pragma once
#include <filesystem>
#include <string>
namespace net::minecraft::client::util {
/// Resolves the Minecraft run directory and OS-specific application data paths.
/// Anchor: Minecraft.cpp L384–421.
class MinecraftDirectories {
public:
  inline static std::filesystem::path runDirectoryCache{};
  [[nodiscard]] static std::filesystem::path getRunDirectory();
  [[nodiscard]] static std::filesystem::path getApplicationDirectory(const std::string& name);
};
} // namespace net::minecraft::client::util
