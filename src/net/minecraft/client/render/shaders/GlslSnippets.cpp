#include "net/minecraft/client/render/shaders/GlslSnippets.hpp"
#include <mutex>
#include <unordered_set>
#include <utility>
#include "net/minecraft/client/ClientLog.hpp"
namespace net::minecraft::client::render {
namespace {
std::mutex gMutex;
std::unordered_map<std::string, std::string> gCache;
std::unordered_map<std::string, std::string> gTestMap;
std::unordered_set<std::string> gMissingWarned;
[[nodiscard]] std::string normalizeLineEndings(std::string text) {
 std::size_t out = 0;
 for(std::size_t in = 0; in < text.size(); ++in) {
  if(text[in] == '\r') {
   if(in + 1 < text.size() && text[in + 1] == '\n') continue;
   text[out++] = '\n';
   continue;
  }
  text[out++] = text[in];
 }
 text.resize(out);
 return text;
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
  const auto& embedded = embeddedGlslSnippets();
  const auto found = embedded.find(key);
  if(found != embedded.end()) {
   text = found->second;
  }
 }
 if(text.empty() && gMissingWarned.insert(key).second) {
  ClientLog::LOGGER.log(::net::minecraft::util::logging::LogLevel::Warning,
                        "engine GLSL snippet '" + key + "' not found (embedded in the executable)");
 }
 return gCache.emplace(key, normalizeLineEndings(std::move(text))).first->second;
}
void GlslSnippets::setSourceMapForTesting(std::unordered_map<std::string, std::string> map) {
 std::lock_guard<std::mutex> lock(gMutex);
 gCache.clear();
 gTestMap = std::move(map);
}
} // namespace net::minecraft::client::render
