#include <cstdint>
#include <string>
#include <unordered_map>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaBindings.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
int luaTextureBind(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 int textureId = 0;
 int unit = 0;
 if(!args.integer(1, textureId) || (args.count() >= 2 && !args.integer(2, unit))) {
  return args.fail("minecraft.texture.bind expects (texture_id, unit?)");
 }
 if(textureId < 0 || unit < 0 || unit > 31) {
  return args.fail("minecraft.texture.bind received an invalid texture id or unit");
 }
 client::gl::GLCore::ensureLoaded();
 if(unit != 0 && client::gl::GLCore::activeTexture == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
 client::render::RenderSystem::activeTexture(client::gl::tex::Texture0 + unit);
 client::render::RenderSystem::bindTexture(textureId);
 client::render::RenderSystem::activeTexture(client::gl::tex::Texture0);
 api.pushboolean(state, 1);
 return 1;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
std::unordered_map<std::string, client::texture::RasterImage>& rasterCache() {
 static std::unordered_map<std::string, client::texture::RasterImage> cache;
 return cache;
}
constexpr std::size_t kRasterCacheMax = 64;
const client::texture::RasterImage* cachedRaster(const std::string& path) {
 auto& cache = rasterCache();
 auto it = cache.find(path);
 if(it != cache.end()) {
  return &it->second;
 }
 if(cache.size() >= kRasterCacheMax) {
  cache.clear();
 }
 client::texture::RasterImage image = client::Minecraft::INSTANCE != nullptr
                                          ? client::Minecraft::INSTANCE->textureManager.loadRasterForResource(path)
                                          : client::texture::RasterImage{};
 auto inserted = cache.emplace(path, std::move(image));
 return &inserted.first->second;
}
int luaTextureSize(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 std::string path;
 if(!args.string(1, path)) {
  return args.fail("minecraft.texture.size expects (path)");
 }
 const auto* img = cachedRaster(path);
 api.createtable(state, 0, 2);
 setField(state, "width", img->width);
 setField(state, "height", img->height);
 return 1;
}
int luaTexturePixel(lua_State* state) {
 LuaApi& api = luaApi();
 LuaArgs args(state);
 std::string path;
 int x = 0;
 int y = 0;
 if(!args.string(1, path) || !args.integer(2, x) || !args.integer(3, y)) {
  return args.fail("minecraft.texture.pixel expects (path, x, y)");
 }
 const auto* img = cachedRaster(path);
 if(x < 0 || y < 0 || x >= img->width || y >= img->height) {
  return args.fail("minecraft.texture.pixel coordinates are outside the image");
 }
 const std::uint32_t pixel = img->argb[static_cast<std::size_t>(y) * img->width + x];
 api.createtable(state, 0, 4);
 setField(state, "a", static_cast<int>((pixel >> 24) & 0xFF));
 setField(state, "r", static_cast<int>((pixel >> 16) & 0xFF));
 setField(state, "g", static_cast<int>((pixel >> 8) & 0xFF));
 setField(state, "b", static_cast<int>(pixel & 0xFF));
 return 1;
}
#else
int luaTextureSize(lua_State* state) {
 luaApi().createtable(state, 0, 2);
 return 1;
}
int luaTexturePixel(lua_State* state) {
 luaApi().createtable(state, 0, 4);
 return 1;
}
#endif
} // namespace
void installTextureApi(lua_State* state) {
 LuaApi& api = luaApi();
 pushFunctionTable(state,
                   {
                       {"size", luaTextureSize},
                       {"pixel", luaTexturePixel},
                       {"bind", luaTextureBind},
                   });
 api.setfield(state, -2, "texture");
}
} // namespace net::minecraft::mod::runtime
