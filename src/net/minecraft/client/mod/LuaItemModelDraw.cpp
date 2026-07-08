#include "net/minecraft/mod/lua/LuaItemModel.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/client/mod/LuaModelBoxDraw.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
namespace net::minecraft::mod::lua {
namespace {
using net::minecraft::client::render::Tessellator;
struct ActiveManualItemDraw {
  float brightness = 1.0f;
};
thread_local ActiveManualItemDraw* gManualItemDraw = nullptr;
class ScopedManualItemDraw {
public:
  explicit ScopedManualItemDraw(ActiveManualItemDraw& context) : previous_(gManualItemDraw) {
    gManualItemDraw = &context;
  }
  ~ScopedManualItemDraw() {
    gManualItemDraw = previous_;
  }

private:
  ActiveManualItemDraw* previous_;
};
net::minecraft::block::TerrainAtlasUv uvAtPixels(int textureId, double u, double v) {
  const net::minecraft::mod::TileScale tile = net::minecraft::mod::tileScale(textureId);
  return {(static_cast<double>(tile.u) + u) * tile.inv, 0.0, (static_cast<double>(tile.v) + v) * tile.inv, 0.0};
}
} // namespace
bool emitManualItemModelQuad(const ManualBlockVertex* vertices, int textureId, float red, float green, float blue,
                             float alpha) {
  if(gManualItemDraw == nullptr || vertices == nullptr) {
    return false;
  }
  Tessellator& t = Tessellator::INSTANCE;
  t.startQuads();
  t.color(red * gManualItemDraw->brightness, green * gManualItemDraw->brightness, blue * gManualItemDraw->brightness,
          alpha);
  for(int i = 0; i < 4; ++i) {
    const auto uv = uvAtPixels(textureId, vertices[i].u, vertices[i].v);
    t.vertex(vertices[i].x, vertices[i].y, vertices[i].z, uv.uMin, uv.vMin);
  }
  t.draw();
  return true;
}
bool drawLuaItemModel(Tessellator& tessellator, const ItemStack& stack, float brightness) {
  const ItemRegistrationSpec* spec = itemRegistrationSpecForId(stack.itemId);
  if(spec == nullptr || spec->model.type == LuaItemModelSpec::Type::Flat) {
    return false;
  }
  const gl::preset::ModItemDraw itemDraw;
  if(spec->model.type == LuaItemModelSpec::Type::Manual) {
    ActiveManualItemDraw context{brightness};
    const ScopedManualItemDraw scope(context);
    return invokeManualItemModelDraw(*spec, brightness);
  }
  const auto uv = tileUv(stack.getTextureId());
  for(const ModelBox& box : spec->model.boxes) {
    client::mod::emitModelBoxGeometry(tessellator, box, uv);
  }
  return true;
}
} // namespace net::minecraft::mod::lua
