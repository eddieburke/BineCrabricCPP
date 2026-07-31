#include "net/minecraft/client/render/shaderpack/ShaderPackResources.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackGl.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackInstance.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderTexture.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>
namespace net::minecraft::client::render::shaderpack {
namespace {
std::string textureKey(const CustomTexture& texture) {
 return texture.stage.empty() ? texture.name : texture.stage + "." + texture.name;
}
} // namespace
bool ShaderPackResources::ensure(ShaderPackInstance& pack, int width, int height, unsigned int lightmapTexture,
                                 const std::function<std::string(const ShaderPackInstance&, const std::string&)>& readText) {
 if(!glutil::hasGlContext()) return false;
 if(!pack.definition.images.empty() && gl::GLCore::bindImageTexture == nullptr) return false;
 const bool customNoise = std::any_of(pack.definition.customTextures.begin(), pack.definition.customTextures.end(),
                                      [](const CustomTexture& texture) { return texture.name == "noisetex"; });
 if(!customNoise && (pack.noiseTexture == 0 || pack.noiseResolution != pack.definition.noiseTextureResolution)) {
  if(pack.noiseTexture != 0) core::deleteTexture(pack.noiseTexture);
  pack.noiseTexture = core::genTexture();
  pack.noiseResolution = pack.definition.noiseTextureResolution;
  if(pack.noiseTexture == 0) return false;
  const std::size_t count = static_cast<std::size_t>(pack.noiseResolution) * pack.noiseResolution * 3;
  std::vector<std::uint8_t> noise(count);
  std::uint32_t state = 0x9E3779B9u;
  for(std::uint8_t& value : noise) {
   state ^= state << 13;
   state ^= state >> 17;
   state ^= state << 5;
   value = static_cast<std::uint8_t>(state);
  }
  core::bindTexture(static_cast<int>(pack.noiseTexture));
  ::glTexImage2D(glutil::kTexture2D, 0, 0x8051, pack.noiseResolution, pack.noiseResolution, 0, 0x1907, 0x1401,
                 noise.data());
  ::glTexParameteri(glutil::kTexture2D, 0x2801, 0x2601);
  ::glTexParameteri(glutil::kTexture2D, 0x2800, 0x2601);
  ::glTexParameteri(glutil::kTexture2D, 0x2802, 0x2901);
  ::glTexParameteri(glutil::kTexture2D, 0x2803, 0x2901);
 }
 for(const BufferObject& declaration : pack.definition.bufferObjects) {
  if(declaration.index < 0 || declaration.index > 8 || !gl::GLCore::ssboSupported) return false;
  std::size_t bytes = declaration.byteSize;
  if(declaration.relative) {
   const std::size_t scaledWidth = static_cast<std::size_t>(std::max(1, static_cast<int>(std::ceil(width * declaration.scaleX))));
   const std::size_t scaledHeight = static_cast<std::size_t>(std::max(1, static_cast<int>(std::ceil(height * declaration.scaleY))));
   if(scaledWidth > std::numeric_limits<std::size_t>::max() / scaledHeight ||
      scaledWidth * scaledHeight > 134217728ull / declaration.byteSize) return false;
   bytes = scaledWidth * scaledHeight * declaration.byteSize;
  }
  unsigned int& buffer = pack.bufferObjects[declaration.index];
  if(buffer == 0) gl::GLCore::genBuffers(1, &buffer);
  if(buffer == 0) return false;
  gl::GLCore::bindBuffer(0x90D2, buffer);
  if(pack.bufferBytes[declaration.index] != bytes) {
   std::vector<std::uint8_t> initData;
   const void* upload = nullptr;
   if(!declaration.initPath.empty() && !declaration.relative) {
    std::string path = declaration.initPath;
    while(!path.empty() && path.front() == '/') path.erase(path.begin());
    std::string data = readText(pack, "shaders/" + path);
    if(data.empty()) data = readText(pack, path);
    if(!data.empty()) {
     initData.assign(data.begin(), data.end());
     if(initData.size() > bytes) initData.resize(bytes);
     if(initData.size() < bytes) initData.resize(bytes, 0);
     upload = initData.data();
    }
   }
   gl::GLCore::bufferData(0x90D2, static_cast<intptr_t>(bytes), upload, 0x88E8);
   pack.bufferBytes[declaration.index] = bytes;
  }
  gl::GLCore::bindBufferBase(0x90D2, static_cast<unsigned int>(declaration.index), buffer);
 }
 gl::GLCore::bindBuffer(0x90D2, 0);
 for(const CustomTexture& declaration : pack.definition.customTextures) {
  const std::string key = textureKey(declaration);
  if(pack.customTextures.contains(key)) continue;
  std::string path = declaration.path;
  if(path.rfind("minecraft:", 0) == 0) {
   if(path == "minecraft:dynamic/lightmap_1" || path == "minecraft:dynamic/light_map_1") {
    if(lightmapTexture == 0) return false;
    pack.customTextures[key] = lightmapTexture;
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
  const unsigned int target = glutil::textureTarget(declaration.type, textureDimensions.size());
  if((target == glutil::kTexture3D && textureDimensions.size() < 3) ||
     (target == glutil::kTexture2D && textureDimensions.size() < 2)) return false;
  const unsigned int texture = core::genTexture();
  if(texture == 0) return false;
  if(target == glutil::kTexture2D) {
   core::bindTexture(glutil::kTexture2D, static_cast<int>(texture));
  } else {
   ::glBindTexture(target, texture);
   core::invalidateTextureBindCache();
  }
  ::glPixelStorei(0x0CF5, 1);
  const unsigned int internal = glutil::internalFormat(declaration.internalFormat);
  const unsigned int format = glutil::pixelFormat(declaration.pixelFormat);
  const unsigned int type = glutil::pixelType(declaration.pixelType);
  if(target == glutil::kTexture3D && gl::GLCore::texImage3D != nullptr) {
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
  if(target == glutil::kTexture3D) ::glTexParameteri(target, 0x8072, clamp ? 0x812F : 0x2901);
  pack.customTextures[key] = texture;
  pack.ownedCustomTextures.insert(texture);
 }
 for(const CustomImage& declaration : pack.definition.images) {
  const int imageWidth = declaration.relative ? std::max(1, static_cast<int>(std::ceil(width * declaration.width)))
                                              : std::max(1, static_cast<int>(declaration.width));
  const int imageHeight = declaration.relative ? std::max(1, static_cast<int>(std::ceil(height * declaration.height)))
                                               : std::max(1, static_cast<int>(declaration.height));
  ShaderPackInstance::ImageTarget& image = pack.images[declaration.name];
  if(image.texture != 0 && image.width == imageWidth && image.height == imageHeight && image.depth == declaration.depth) continue;
  if(image.texture != 0) core::deleteTexture(image.texture);
  image.texture = core::genTexture();
  image.width = imageWidth;
  image.height = imageHeight;
  image.depth = declaration.depth;
  if(image.texture == 0) return false;
  const unsigned int target = image.depth > 1 ? glutil::kTexture3D : glutil::kTexture2D;
  if(target == glutil::kTexture2D) {
   core::bindTexture(static_cast<int>(image.texture));
  } else {
   ::glBindTexture(target, image.texture);
   core::invalidateTextureBindCache();
  }
  if(target == glutil::kTexture3D && gl::GLCore::texImage3D != nullptr) {
   gl::GLCore::texImage3D(target, 0, static_cast<int>(glutil::internalFormat(declaration.internalFormat)), image.width,
                          image.height, image.depth, 0, glutil::pixelFormat(declaration.format),
                          glutil::pixelType(declaration.pixelType), nullptr);
  } else {
   ::glTexImage2D(target, 0, static_cast<int>(glutil::internalFormat(declaration.internalFormat)), image.width, image.height,
                  0, glutil::pixelFormat(declaration.format), glutil::pixelType(declaration.pixelType), nullptr);
  }
  ::glTexParameteri(target, 0x2801, 0x2601);
  ::glTexParameteri(target, 0x2800, 0x2601);
  ::glTexParameteri(target, 0x2802, 0x812F);
  ::glTexParameteri(target, 0x2803, 0x812F);
  if(target == glutil::kTexture3D) ::glTexParameteri(target, 0x8072, 0x812F);
 }
 int maxDim = std::max(width, height);
 for(const auto& [name, target] : pack.definition.targets) {
  (void)name;
  if(target.absoluteWidth > 0 && target.absoluteHeight > 0) {
   maxDim = std::max(maxDim, std::max(target.absoluteWidth, target.absoluteHeight));
  } else {
   maxDim = std::max(maxDim,
                     std::max(1, static_cast<int>(std::ceil(width * target.scaleX))));
   maxDim = std::max(maxDim,
                     std::max(1, static_cast<int>(std::ceil(height * target.scaleY))));
  }
 }
 int level = 0;
 for(int d = std::max(1, maxDim); d > 1; d >>= 1) ++level;
 pack.definition.mcMipmapLevel = std::max(0, level);
 return true;
}
void ShaderPackResources::bind(ShaderPackInstance& pack, gl::ShaderProgram& program, unsigned int imageUnitStart) {
 for(const BufferObject& declaration : pack.definition.bufferObjects) {
  if(pack.bufferObjects[declaration.index] != 0)
   gl::GLCore::bindBufferBase(0x90D2, static_cast<unsigned int>(declaration.index), pack.bufferObjects[declaration.index]);
 }
 unsigned int imageUnit = imageUnitStart;
 for(const CustomImage& declaration : pack.definition.images) {
  const auto found = pack.images.find(declaration.name);
  if(found == pack.images.end() || found->second.texture == 0 || imageUnit >= 16) continue;
  program.set1i(declaration.name, static_cast<int>(imageUnit));
  gl::GLCore::bindImageTexture(imageUnit, found->second.texture, 0, found->second.depth > 1 ? 1 : 0, 0, 0x88BA,
                               glutil::internalFormat(declaration.internalFormat));
  ++imageUnit;
 }
}
void ShaderPackResources::addTextures(ShaderPackInstance& pack,
                                      const std::string& stage,
                                      std::unordered_map<std::string, int>& textures,
                                      std::unordered_map<std::string, int>& volumes) {
 if(pack.noiseTexture != 0) textures["noisetex"] = static_cast<int>(pack.noiseTexture);
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
       static_cast<int>(found->second.texture);
 }
}
} // namespace net::minecraft::client::render::shaderpack
