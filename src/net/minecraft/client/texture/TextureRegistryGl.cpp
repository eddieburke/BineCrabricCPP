#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/registry/TextureRegistryInternal.hpp"
namespace net::minecraft::registry {
int TextureRegistry::resolveGlId(int textureId, net::minecraft::client::texture::TextureManager& textureManager) {
 if(textureId < kCustomTextureBase) {
  return -1;
 }
 std::string path;
 {
  std::lock_guard<std::mutex> lock(detail::registryMutex());
  const int index = textureId - kCustomTextureBase;
  auto& entries = detail::registryEntries();
  if(index < 0 || index >= static_cast<int>(entries.size())) {
   if(detail::warnedInvalidIds().insert(textureId).second) {
    net::minecraft::client::ClientLog::LOGGER.log(
        net::minecraft::util::logging::LogLevel::Warning,
        "Unregistered custom texture id, using missing fallback: " + std::to_string(textureId));
   }
   return textureManager.getTextureId(std::string());
  }
  Entry& entry = entries[static_cast<std::size_t>(index)];
  if(entry.glId >= 0) {
   return entry.glId;
  }
  path = entry.path;
 }
 const int glId = textureManager.getTextureId(path);
 int width = 0;
 int height = 0;
 static_cast<void>(textureManager.getTextureDimensions(path, width, height));
 {
  std::lock_guard<std::mutex> lock(detail::registryMutex());
  const int index = textureId - kCustomTextureBase;
  auto& entries = detail::registryEntries();
  if(index >= 0 && index < static_cast<int>(entries.size())) {
   Entry& entry = entries[static_cast<std::size_t>(index)];
   entry.glId = glId;
   entry.width = width;
   entry.height = height;
  }
 }
 return glId;
}
} // namespace net::minecraft::registry
