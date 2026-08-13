#include "net/minecraft/client/render/pipeline/Resources.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/PbrTextures.hpp"
#include "net/minecraft/client/render/shaderpack/PackTexture.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlResource.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>
namespace net::minecraft::client::render {
namespace {
std::string textureKey(const CustomTexture& texture) {
 return texture.stage.empty() ? texture.name : texture.stage + "." + texture.name;
}
std::uint64_t vramBytes() {
 static const bool hasNvxMemoryInfo = []() {
  const std::vector<std::string> extensions = supportedGlExtensions();
  return std::find(extensions.begin(), extensions.end(), "GL_NVX_gpu_memory_info") != extensions.end();
 }();
 if(hasNvxMemoryInfo) {
  int available = 0;
  ::glGetIntegerv(0x9049, &available);
  if(available > 0) return static_cast<std::uint64_t>(available) * 1024ull;
 }
 return 4294967296ull;
}
} // namespace
bool ensurePackResources(PackInstance& pack, int width, int height, const gl::GlTexture& lightmapTexture,
                            const std::function<std::string(const PackInstance&, const std::string&)>& readText) {
  if(!hasGlContext()) return false;
  if(!pack.definition.images.empty() && gl::GLCore::bindImageTexture == nullptr) return false;
  bool samplerBindingsChanged = pack.worldTextures.empty() && pack.worldVolumeTextures.empty();
 const bool customNoise = std::any_of(pack.definition.customTextures.begin(), pack.definition.customTextures.end(),
                                      [](const CustomTexture& texture) { return texture.name == "noisetex"; });
  // The pack's value is stored verbatim like Java's; the allocation is what has to be sane,
  // the same way GameRenderer.cpp clamps shadowMapResolution at the shadow texture.
  const int noiseResolution = std::clamp(pack.definition.noiseTextureResolution, 1, 4096);
  if(!customNoise && (!pack.noiseTexture || pack.noiseResolution != noiseResolution)) {
   samplerBindingsChanged = true;
  pack.noiseTexture = gl::GlTexture(core::genTexture());
  pack.noiseResolution = noiseResolution;
  if(!pack.noiseTexture) return false;
  const std::size_t count = static_cast<std::size_t>(pack.noiseResolution) * pack.noiseResolution * 3;
  std::vector<std::uint8_t> noise(count);
  std::uint32_t state = 0x9E3779B9u;
  for(std::uint8_t& value : noise) {
   state ^= state << 13;
   state ^= state >> 17;
   state ^= state << 5;
   value = static_cast<std::uint8_t>(state);
  }
  core::bindTexture(static_cast<int>(pack.noiseTexture.handle()));
  ::glTexImage2D(gl::cap::Texture2D, 0, 0x8051, pack.noiseResolution, pack.noiseResolution, 0, 0x1907, 0x1401,
                 noise.data());
  ::glTexParameteri(gl::cap::Texture2D, 0x2801, 0x2601);
  ::glTexParameteri(gl::cap::Texture2D, 0x2800, 0x2601);
  ::glTexParameteri(gl::cap::Texture2D, 0x2802, 0x2901);
  ::glTexParameteri(gl::cap::Texture2D, 0x2803, 0x2901);
 }
 const std::uint64_t vram = vramBytes();
 for(const BufferObject& declaration : pack.definition.bufferObjects) {
  if(declaration.index < 0 || declaration.index >= kMaxShaderStorageBuffers || !gl::GLCore::ssboSupported)
   return false;
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/gl/buffer/ShaderStorageBufferHolder.java
  if(declaration.byteSize > vram) return false;
  if(declaration.index > gl::GLCore::maxShaderStorageUnits) return false;
  std::size_t bytes = declaration.byteSize;
  if(declaration.relative) {
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/gl/buffer/ShaderStorageBuffer.java
   const std::size_t scaledWidth =
       static_cast<std::size_t>(std::max(0.0, static_cast<double>(width) * declaration.scaleX));
   const std::size_t scaledHeight =
       static_cast<std::size_t>(std::max(0.0, static_cast<double>(height) * declaration.scaleY));
   if(scaledWidth != 0 && bytes > std::numeric_limits<std::size_t>::max() / scaledWidth) return false;
   bytes *= scaledWidth;
   if(scaledHeight != 0 && bytes > std::numeric_limits<std::size_t>::max() / scaledHeight) return false;
   bytes *= scaledHeight;
  }
  gl::GlBuffer& buffer = pack.bufferObjects[declaration.index];
  if(!buffer || pack.bufferBytes[declaration.index] != bytes) {
   std::vector<std::uint8_t> initData;
   if(!declaration.initPath.empty()) {
    std::string path = declaration.initPath;
    while(!path.empty() && path.front() == '/') path.erase(path.begin());
    // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/ShaderPack.java
    const std::string data = readText(pack, path);
    if(!data.empty()) {
     if(data.size() > declaration.byteSize) return false;
     initData.assign(data.begin(), data.end());
    }
   }
   unsigned int handle = 0;
   gl::GLCore::genBuffers(1, &handle);
   if(handle == 0) return false;
   buffer = gl::GlBuffer(handle);
   gl::GLCore::bindBuffer(0x90D2, handle);
   const int zero = 0;
   if(declaration.relative) {
    gl::GLCore::bufferStorage(0x90D2, static_cast<intptr_t>(bytes), nullptr, 0);
    gl::GLCore::clearBufferSubData(0x90D2, 0x8229, 0, static_cast<intptr_t>(bytes), 0x1903, 0x1400, &zero);
   } else {
    gl::GLCore::bufferStorage(0x90D2, static_cast<intptr_t>(bytes), nullptr, initData.empty() ? 0 : 0x0100);
    if(!initData.empty()) {
     gl::GLCore::bufferSubData(0x90D2, 0, static_cast<intptr_t>(initData.size()), initData.data());
    } else {
     gl::GLCore::clearBufferSubData(0x90D2, 0x8229, 0, static_cast<intptr_t>(bytes), 0x1903, 0x1400, &zero);
    }
   }
   pack.bufferBytes[declaration.index] = bytes;
  }
  gl::GLCore::bindBufferBase(0x90D2, static_cast<unsigned int>(declaration.index), buffer.handle());
 }
 gl::GLCore::bindBuffer(0x90D2, 0);
  for(const CustomTexture& declaration : pack.definition.customTextures) {
   const std::string key = textureKey(declaration);
   if(pack.customTextures.contains(key)) continue;
   samplerBindingsChanged = true;
  std::string path = declaration.path;
  if(path.rfind("minecraft:", 0) == 0) {
   if(path == "minecraft:dynamic/lightmap_1" || path == "minecraft:dynamic/light_map_1") {
    if(!lightmapTexture) return false;
    pack.customTextures[key] = lightmapTexture.handle();
    continue;
   }
   net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
   if(minecraft == nullptr) return false;
   std::string resource = path.substr(10);
   static constexpr std::pair<const char*, const char*> kAtlasRemap[] = {
       {"textures/atlas/blocks.png", "/terrain.png"},
       {"textures/atlas/blocks_n.png", "/terrain_n.png"},
       {"textures/atlas/blocks_s.png", "/terrain_s.png"},
       {"textures/atlas/items.png", "/gui/items.png"},
       {"textures/atlas/items_n.png", "/gui/items_n.png"},
       {"textures/atlas/items_s.png", "/gui/items_s.png"},
   };
   bool remapped = false;
   for(const auto& [from, to] : kAtlasRemap) {
    if(resource == from) {
     resource = to;
     remapped = true;
     break;
    }
   }
   if(!remapped) resource = "/" + resource;
   const int texture = minecraft->textureManager.getTextureId(resource);
   if(texture < 0) return false;
    // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/CustomTextureManager.java
    if(const std::size_t extension = resource.find_last_of('.'); extension != std::string::npos &&
                                                                 extension > 2) {
     const std::string base = resource.substr(0, extension);
     const bool normal = base.ends_with("_n");
     const bool specular = base.ends_with("_s");
     if(normal || specular) {
      const std::string basePath = base.substr(0, base.size() - 2) + resource.substr(extension);
      const int baseId = minecraft->textureManager.getTextureId(basePath);
      if(baseId > 0) {
       // ColorWheel: _n/_s companion textures in the lab-pbr format.
       const render::PbrTextures::Holder holder =
          render::PbrTextures::getOrLoad(baseId, minecraft->textureManager,
                                         pack.definition.labPbr);
      const int pbrId = normal ? holder.normal : holder.specular;
      if(pbrId > 0) {
       pack.customTextures[key] = static_cast<unsigned int>(pbrId);
       continue;
      }
     }
    }
   }
   pack.customTextures[key] = static_cast<unsigned int>(texture);
   continue;
  }
  while(!path.empty() && path.front() == '/') path.erase(path.begin());
  std::string data = readText(pack, "shaders/" + path);
  if(data.empty()) data = readText(pack, path);
  if(data.empty()) return false;
  DecodedTexture decoded;
  std::vector<int> textureDimensions = declaration.dimensions;
  const void* pixels = data.data();
  if(declaration.encoded) {
   decoded = decodeTexture(data);
   if(decoded.rgba.empty()) return false;
   textureDimensions = {decoded.width, decoded.height};
   pixels = decoded.rgba.data();
  }
  bool blur = !declaration.encoded;
  bool clamp = !declaration.encoded;
  const std::string metadata = readText(pack, "shaders/" + path + ".mcmeta");
  auto metadataFlag = [&metadata](std::string_view name, bool fallback) {
   const std::size_t key = metadata.find(name);
   if(key == std::string::npos) return fallback;
   const std::size_t value = metadata.find_first_not_of(" \t\r\n:", key + name.size());
   return value == std::string::npos ? fallback : metadata.compare(value, 4, "true") == 0;
  };
  blur = metadataFlag("\"blur\"", blur);
  clamp = metadataFlag("\"clamp\"", clamp);
  const unsigned int target = textureTarget(declaration.type, textureDimensions.size());
  if((target == kTexture3D && textureDimensions.size() < 3) ||
     (target == gl::cap::Texture2D && textureDimensions.size() < 2)) return false;
  const unsigned int texture = core::genTexture();
  if(texture == 0) return false;
  if(target == gl::cap::Texture2D) {
   core::bindTexture(gl::cap::Texture2D, static_cast<int>(texture));
  } else {
   ::glBindTexture(target, texture);
   core::invalidateTextureBindCache();
  }
  ::glPixelStorei(0x0CF5, 1);
  const unsigned int internal = internalFormat(declaration.internalFormat);
  const unsigned int format = pixelFormat(declaration.pixelFormat);
  const unsigned int type = pixelType(declaration.pixelType);
  if(target == kTexture3D && gl::GLCore::texImage3D != nullptr) {
   gl::GLCore::texImage3D(target, 0, static_cast<int>(internal), textureDimensions[0], textureDimensions[1],
                          textureDimensions[2], 0, format, type, pixels);
  } else if(target == 0x0DE0) {
   ::glTexImage1D(target, 0, static_cast<int>(internal), textureDimensions[0], 0, format, type, pixels);
  } else {
   ::glTexImage2D(target, 0, static_cast<int>(internal), textureDimensions[0], textureDimensions[1], 0,
                  format, type, pixels);
  }
  ::glTexParameteri(target, 0x2801, blur ? 0x2601 : 0x2600);
  ::glTexParameteri(target, 0x2800, blur ? 0x2601 : 0x2600);
  ::glTexParameteri(target, 0x2802, clamp ? 0x812F : 0x2901);
  if(target != 0x0DE0) ::glTexParameteri(target, 0x2803, clamp ? 0x812F : 0x2901);
  if(target == kTexture3D) ::glTexParameteri(target, 0x8072, clamp ? 0x812F : 0x2901);
  pack.customTextures[key] = texture;
  pack.ownedCustomTextures.insert(texture);
 }
  for(const CustomImage& declaration : pack.definition.images) {
  const int imageWidth = declaration.relative ? std::max(1, static_cast<int>(std::ceil(width * declaration.width)))
                                              : std::max(1, static_cast<int>(declaration.width));
  const int imageHeight = declaration.relative ? std::max(1, static_cast<int>(std::ceil(height * declaration.height)))
                                               : std::max(1, static_cast<int>(declaration.height));
   PackInstance::ImageTarget& image = pack.images[declaration.name];
   if(image.texture && image.width == imageWidth && image.height == imageHeight && image.depth == declaration.depth) continue;
   samplerBindingsChanged = true;
  image.texture = gl::GlTexture(core::genTexture());
  image.width = imageWidth;
  image.height = imageHeight;
  image.depth = declaration.depth;
  if(!image.texture) return false;
  const unsigned int target = image.depth > 1 ? kTexture3D : gl::cap::Texture2D;
  if(target == gl::cap::Texture2D) {
   core::bindTexture(static_cast<int>(image.texture.handle()));
  } else {
   ::glBindTexture(target, image.texture.handle());
   core::invalidateTextureBindCache();
  }
  if(target == kTexture3D && gl::GLCore::texImage3D != nullptr) {
   gl::GLCore::texImage3D(target, 0, static_cast<int>(internalFormat(declaration.internalFormat)), image.width,
                          image.height, image.depth, 0, pixelFormat(declaration.format),
                          pixelType(declaration.pixelType), nullptr);
  } else {
   ::glTexImage2D(target, 0, static_cast<int>(internalFormat(declaration.internalFormat)), image.width, image.height,
                  0, pixelFormat(declaration.format), pixelType(declaration.pixelType), nullptr);
  }
  ::glTexParameteri(target, 0x2801, 0x2601);
  ::glTexParameteri(target, 0x2800, 0x2601);
  ::glTexParameteri(target, 0x2802, 0x812F);
  ::glTexParameteri(target, 0x2803, 0x812F);
  if(target == kTexture3D) ::glTexParameteri(target, 0x8072, 0x812F);
 }
  if(samplerBindingsChanged) {
   pack.worldTextures.clear();
   pack.worldVolumeTextures.clear();
   addPackTextures(pack, "gbuffers", pack.worldTextures, pack.worldVolumeTextures);
  }
  return true;
}
void bindPackResources(PackInstance& pack, gl::ShaderProgram& program, unsigned int imageUnitStart) {
 for(const BufferObject& declaration : pack.definition.bufferObjects) {
  if(pack.bufferObjects[declaration.index]) {
   gl::GLCore::bindBufferBase(0x90D2, static_cast<unsigned int>(declaration.index), pack.bufferObjects[declaration.index].handle());
  }
 }
 unsigned int imageUnit = imageUnitStart;
 for(const CustomImage& declaration : pack.definition.images) {
  const auto found = pack.images.find(declaration.name);
  if(found == pack.images.end() || !found->second.texture || imageUnit >= 16) continue;
  program.set1i(declaration.name, static_cast<int>(imageUnit));
  gl::GLCore::bindImageTexture(imageUnit, found->second.texture.handle(), 0, found->second.depth > 1 ? 1 : 0, 0, 0x88BA,
                               internalFormat(declaration.internalFormat));
  ++imageUnit;
 }
}
void addPackTextures(PackInstance& pack,
                                const std::string& stage,
                                std::unordered_map<std::string, int>& textures,
                                std::unordered_map<std::string, int>& volumes) {
 if(pack.noiseTexture) textures["noisetex"] = static_cast<int>(pack.noiseTexture.handle());
 for(const CustomTexture& declaration : pack.definition.customTextures) {
  if(!declaration.stage.empty() && declaration.stage != stage) continue;
  const auto found = pack.customTextures.find(textureKey(declaration));
  if(found != pack.customTextures.end())
   (declaration.dimensions.size() == 3 ? volumes : textures)[declaration.name] =
       static_cast<int>(found->second);
 }
 for(const CustomImage& declaration : pack.definition.images) {
  const auto found = pack.images.find(declaration.name);
  if(found != pack.images.end())
   (found->second.depth > 1 ? volumes : textures)[declaration.sampler] =
       static_cast<int>(found->second.texture.handle());
 }
}
} // namespace net::minecraft::client::render
