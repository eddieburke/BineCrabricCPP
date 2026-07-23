#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/registry/TextureRegistryInternal.hpp"
namespace net::minecraft::registry {
namespace {
std::string normalizePath(const std::string& path) {
 std::string out = path;
 while(!out.empty() && (out.front() == '/' || out.front() == '\\')) {
  out.erase(out.begin());
 }
 return out;
}
} // namespace
int TextureRegistry::getOrRegisterTexture(const std::string& path) {
 const std::string normalized = normalizePath(path);
 if(normalized.empty()) {
  return 0;
 }
 std::lock_guard<std::mutex> lock(detail::registryMutex());
 auto& index = detail::registryIndex();
 const auto found = index.find(normalized);
 if(found != index.end()) {
  return found->second;
 }
 auto& entries = detail::registryEntries();
 entries.push_back(Entry{normalized, -1, 0, 0});
 const int id = kCustomTextureBase + static_cast<int>(entries.size()) - 1;
 index.emplace(normalized, id);
 return id;
}
bool TextureRegistry::isCustomTexture(int textureId) noexcept {
 if(textureId < kCustomTextureBase) {
  return false;
 }
 std::lock_guard<std::mutex> lock(detail::registryMutex());
 const int index = textureId - kCustomTextureBase;
 return index >= 0 && index < static_cast<int>(detail::registryEntries().size());
}
const std::string& TextureRegistry::getTexturePath(int textureId) {
 static const std::string kEmpty;
 std::lock_guard<std::mutex> lock(detail::registryMutex());
 const int index = textureId - kCustomTextureBase;
 auto& entries = detail::registryEntries();
 if(textureId < kCustomTextureBase || index < 0 || index >= static_cast<int>(entries.size())) {
  return kEmpty;
 }
 return entries[static_cast<std::size_t>(index)].path;
}
const TextureRegistry::Entry* TextureRegistry::getEntry(int textureId) {
 std::lock_guard<std::mutex> lock(detail::registryMutex());
 const int index = textureId - kCustomTextureBase;
 auto& entries = detail::registryEntries();
 if(textureId < kCustomTextureBase || index < 0 || index >= static_cast<int>(entries.size())) {
  return nullptr;
 }
 return &entries[static_cast<std::size_t>(index)];
}
void TextureRegistry::seedResolvedTexture(int textureId, int glId, int width, int height) {
 std::lock_guard<std::mutex> lock(detail::registryMutex());
 const int index = textureId - kCustomTextureBase;
 auto& entries = detail::registryEntries();
 if(index < 0 || index >= static_cast<int>(entries.size())) {
  return;
 }
 Entry& entry = entries[static_cast<std::size_t>(index)];
 entry.glId = glId;
 entry.width = width;
 entry.height = height;
}
void TextureRegistry::invalidateGlIds() {
 std::lock_guard<std::mutex> lock(detail::registryMutex());
 for(auto& entry : detail::registryEntries()) {
  entry.glId = -1;
 }
}
} // namespace net::minecraft::registry
