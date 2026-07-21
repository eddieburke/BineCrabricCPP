#include "net/minecraft/mod/runtime/LuaTextureBindings.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
int luaTextureBind(lua_State* state) {
 LuaApi& api = luaApi();
 if(api.gettop(state) < 1) {
  api.pushboolean(state, 0);
  return 1;
 }
 const int textureId = static_cast<int>(api.tointegerx(state, 1, nullptr));
 const int unit = api.gettop(state) >= 2 ? static_cast<int>(api.tointegerx(state, 2, nullptr)) : 0;
 if(textureId < 0 || unit < 0 || unit > 31) {
  api.pushboolean(state, 0);
  return 1;
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
 const std::string path =
     api.gettop(state) >= 1 && api.type(state, 1) == kLuaTString ? luaString(state, 1, "") : std::string();
 const auto* img = cachedRaster(path);
 api.createtable(state, 0, 2);
 setField(state, "width", img != nullptr ? img->width : 0);
 setField(state, "height", img != nullptr ? img->height : 0);
 return 1;
}
int luaTexturePixel(lua_State* state) {
 LuaApi& api = luaApi();
 int isNumber = 0;
 const std::string path =
     api.gettop(state) >= 1 && api.type(state, 1) == kLuaTString ? luaString(state, 1, "") : std::string();
 const int x = static_cast<int>(api.tonumberx(state, 2, &isNumber));
 const int y = static_cast<int>(api.tonumberx(state, 3, &isNumber));
 const auto* img = cachedRaster(path);
 int a = 0;
 int r = 0;
 int g = 0;
 int b = 0;
 if(img != nullptr && x >= 0 && y >= 0 && x < img->width && y < img->height) {
  const std::uint32_t pixel = img->argb[static_cast<std::size_t>(y) * img->width + x];
  b = static_cast<int>(pixel & 0xFF);
  g = static_cast<int>((pixel >> 8) & 0xFF);
  r = static_cast<int>((pixel >> 16) & 0xFF);
  a = static_cast<int>((pixel >> 24) & 0xFF);
 }
 api.createtable(state, 0, 4);
 setField(state, "a", a);
 setField(state, "r", r);
 setField(state, "g", g);
 setField(state, "b", b);
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
