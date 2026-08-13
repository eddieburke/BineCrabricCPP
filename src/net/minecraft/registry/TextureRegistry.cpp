#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/registry/TextureRegistryInternal.hpp"
#include "net/minecraft/util/PathUtil.hpp"
namespace net::minecraft::registry {
int TextureRegistry::getOrRegisterTexture(const std::string& path) {
 const std::string normalized = util::normalizePath(path);
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
 const auto& entries = detail::registryEntries();
 return index >= 0 && index < static_cast<int>(entries.size()) &&
        !entries[static_cast<std::size_t>(index)].path.empty();
}
std::optional<TextureRegistry::Entry> TextureRegistry::getEntry(int textureId) {
 std::lock_guard<std::mutex> lock(detail::registryMutex());
 const int index = textureId - kCustomTextureBase;
 auto& entries = detail::registryEntries();
 if(textureId < kCustomTextureBase || index < 0 || index >= static_cast<int>(entries.size()) ||
    entries[static_cast<std::size_t>(index)].path.empty()) {
  return std::nullopt;
 }
 return entries[static_cast<std::size_t>(index)];
}
void TextureRegistry::seedResolvedTexture(int textureId, int glId, int width, int height) {
 std::lock_guard<std::mutex> lock(detail::registryMutex());
 const int index = textureId - kCustomTextureBase;
 auto& entries = detail::registryEntries();
 if(index < 0 || index >= static_cast<int>(entries.size()) ||
    entries[static_cast<std::size_t>(index)].path.empty()) {
  return;
 }
 Entry& entry = entries[static_cast<std::size_t>(index)];
 entry.glId = glId;
 entry.width = width;
 entry.height = height;
}
int TextureRegistry::releaseTexture(int textureId) {
 std::lock_guard<std::mutex> lock(detail::registryMutex());
 const int index = textureId - kCustomTextureBase;
 auto& entries = detail::registryEntries();
 if(index < 0 || index >= static_cast<int>(entries.size())) return -1;
 Entry& entry = entries[static_cast<std::size_t>(index)];
 if(entry.path.empty()) return -1;
 const int glId = entry.glId;
 auto& registryIndex = detail::registryIndex();
 const auto found = registryIndex.find(entry.path);
 if(found != registryIndex.end() && found->second == textureId) registryIndex.erase(found);
 entry = Entry{};
 return glId;
}
void TextureRegistry::invalidateGlIds() {
 std::lock_guard<std::mutex> lock(detail::registryMutex());
 for(auto& entry : detail::registryEntries()) {
  if(!entry.path.starts_with("mod://")) entry.glId = -1;
 }
}
} // namespace net::minecraft::registry
