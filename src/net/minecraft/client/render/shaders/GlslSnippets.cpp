#include "net/minecraft/client/render/shaders/GlslSnippets.hpp"
#include <cstdint>
#include <fstream>
#include <iterator>
#include <mutex>
#include <unordered_set>
#include <utility>
#include <vector>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/resource/ResourceRoot.hpp"
namespace net::minecraft::client::render {
namespace {
std::mutex gMutex;
std::unordered_map<std::string, std::string> gCache;
std::unordered_map<std::string, std::string> gTestMap;
std::unordered_set<std::string> gMissingWarned;

// Same fallback chain as TextureManager::loadRasterForResource: the selected
// texture pack first, then the game resources directory.
std::string loadFromDisk(const std::string& name) {
 const std::string relative = "glsl/" + name + ".glsl";
 const auto* minecraft = net::minecraft::client::Minecraft::INSTANCE;
 if(minecraft != nullptr && minecraft->texturePacks != nullptr &&
    minecraft->texturePacks->selected != nullptr) {
  const std::vector<std::uint8_t> bytes = minecraft->texturePacks->selected->getResource(relative);
  if(!bytes.empty()) {
   return std::string(bytes.begin(), bytes.end());
  }
 }
 std::ifstream input(net::minecraft::client::resource::resolveResource(relative), std::ios::binary);
 if(!input) {
  return {};
 }
 return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}
} // namespace
std::string GlslSnippets::get(std::string_view name) {
 const std::string key(name);
 std::lock_guard<std::mutex> lock(gMutex);
 const auto cached = gCache.find(key);
 if(cached != gCache.end()) {
  return cached->second;
 }
 std::string text;
 if(!gTestMap.empty()) {
  const auto found = gTestMap.find(key);
  if(found != gTestMap.end()) {
   text = found->second;
  }
 } else {
  text = loadFromDisk(key);
 }
 if(text.empty() && gMissingWarned.insert(key).second) {
  ClientLog::LOGGER.log(::net::minecraft::util::logging::LogLevel::Warning,
                        "engine GLSL snippet '" + key + "' not found (resources/glsl/" + key + ".glsl)");
 }
 return gCache.emplace(key, std::move(text)).first->second;
}
void GlslSnippets::setSourceMapForTesting(std::unordered_map<std::string, std::string> map) {
 std::lock_guard<std::mutex> lock(gMutex);
 gCache.clear();
 gTestMap = std::move(map);
}
} // namespace net::minecraft::client::render
