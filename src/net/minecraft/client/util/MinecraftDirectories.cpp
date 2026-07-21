#include "net/minecraft/client/util/MinecraftDirectories.hpp"
#include <cstdlib>
#include <filesystem>
#include "net/minecraft/util/OperatingSystem.hpp"
namespace net::minecraft::client::util {
namespace {
net::minecraft::util::OperatingSystem getOperatingSystem() {
 return net::minecraft::util::OperatingSystem::WINDOWS;
}
} // namespace
std::filesystem::path MinecraftDirectories::getRunDirectory() {
 if(runDirectoryCache.empty()) {
  runDirectoryCache = getApplicationDirectory("minecraft");
 }
 return runDirectoryCache;
}
std::filesystem::path MinecraftDirectories::getApplicationDirectory(const std::string& name) {
 std::filesystem::path file;
 const char* home = std::getenv("USERPROFILE");
 if(home == nullptr) {
  home = ".";
 }
 switch(getOperatingSystem()) {
 case net::minecraft::util::OperatingSystem::LINUX:
 case net::minecraft::util::OperatingSystem::SOLARIS:
  file = std::filesystem::path(home) / ("." + name);
  break;
 case net::minecraft::util::OperatingSystem::WINDOWS: {
  const char* appData = std::getenv("APPDATA");
  if(appData != nullptr) {
   file = std::filesystem::path(appData) / ("." + name);
  } else {
   file = std::filesystem::path(home) / ("." + name);
  }
  break;
 }
 case net::minecraft::util::OperatingSystem::MACOS:
  file = std::filesystem::path(home) / "Library/Application Support" / name;
  break;
 default:
  file = std::filesystem::path(home) / name;
  break;
 }
 std::filesystem::create_directories(file);
 return file;
}
} // namespace net::minecraft::client::util
