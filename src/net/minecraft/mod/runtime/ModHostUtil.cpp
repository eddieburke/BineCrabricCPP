#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#endif
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
thread_local World* gModContextWorld = nullptr;
#ifdef MINECRAFT_NATIVE_EXPORTS
thread_local bool gModContextIsClient = true;
#else
thread_local bool gModContextIsClient = false;
#endif
thread_local entity::player::PlayerEntity* gModContextPlayer = nullptr;
} // namespace
World* contextOrClientWorld() {
  if(gModContextWorld != nullptr) {
    return gModContextWorld;
  }
#ifdef MINECRAFT_NATIVE_EXPORTS
  if(client::Minecraft::INSTANCE != nullptr) {
    return client::Minecraft::INSTANCE->world;
  }
#endif
  return nullptr;
}
ModContextScope::ModContextScope(World* world, entity::player::PlayerEntity* player)
    : previousWorld_(gModContextWorld),
      previousIsClient_(gModContextIsClient),
      previousPlayer_(gModContextPlayer) {
  gModContextWorld = world;
  gModContextIsClient = world != nullptr && world->isRemote();
  gModContextPlayer = player;
}
ModContextScope::~ModContextScope() {
  gModContextWorld = previousWorld_;
  gModContextIsClient = previousIsClient_;
  gModContextPlayer = previousPlayer_;
}
void setModContext(World* world, bool isClient, entity::player::PlayerEntity* player) {
  gModContextWorld = world;
  gModContextIsClient = isClient;
  gModContextPlayer = player;
}
void clearModContext() {
  gModContextWorld = nullptr;
#ifdef MINECRAFT_NATIVE_EXPORTS
  gModContextIsClient = true;
#else
  gModContextIsClient = false;
#endif
  gModContextPlayer = nullptr;
}
bool modContextIsClient() {
  return gModContextIsClient;
}
entity::player::PlayerEntity* activeModPlayer() {
  return gModContextPlayer;
}
std::string trimCopy(std::string value) {
  while(!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
    value.erase(value.begin());
  }
  while(!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
    value.pop_back();
  }
  return value;
}
std::string toLowerCopy(std::string value) {
  for(char& ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}
std::string normalizeRelativePath(std::string_view value) {
  std::string normalized(value);
  std::replace(normalized.begin(), normalized.end(), '\\', '/');
  while(!normalized.empty() && normalized.front() == '/') {
    normalized.erase(normalized.begin());
  }
  return normalized;
}
bool isSafeRelativePath(std::string_view value) {
  const std::string normalized = normalizeRelativePath(value);
  if(normalized.empty() || normalized.find(':') != std::string::npos) {
    return false;
  }
  std::stringstream stream(normalized);
  std::string part;
  while(std::getline(stream, part, '/')) {
    if(part.empty() || part == ".") {
      continue;
    }
    if(part == "..") {
      return false;
    }
  }
  return true;
}
bool isSafeModId(std::string_view value) {
  return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char ch) {
    return std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.';
  });
}
bool isDirectoryZipPath(std::string_view value) {
  const std::string normalized = normalizeRelativePath(value);
  return !normalized.empty() && normalized.back() == '/';
}
std::string sanitizeName(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for(char ch : value) {
    if(std::isalnum(static_cast<unsigned char>(ch))) {
      out.push_back(ch);
    } else {
      out.push_back('_');
    }
  }
  while(!out.empty() && out.back() == '_') {
    out.pop_back();
  }
  return out.empty() ? "mod" : out;
}
std::vector<std::uint8_t> readFileBytes(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if(!input.is_open()) {
    return {};
  }
  input.seekg(0, std::ios::end);
  const std::streamsize size = input.tellg();
  input.seekg(0, std::ios::beg);
  if(size <= 0) {
    return {};
  }
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
  if(!input.read(reinterpret_cast<char*>(bytes.data()), size)) {
    return {};
  }
  return bytes;
}
std::string readFileText(const std::filesystem::path& path) {
  const std::vector<std::uint8_t> bytes = readFileBytes(path);
  return std::string(bytes.begin(), bytes.end());
}
bool writeFileBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if(!output.is_open()) {
    return false;
  }
  if(!bytes.empty()) {
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }
  return output.good();
}
bool writeFileText(const std::filesystem::path& path, const std::string& text) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if(!output.is_open()) {
    return false;
  }
  output << text;
  return output.good();
}
} // namespace net::minecraft::mod::runtime
