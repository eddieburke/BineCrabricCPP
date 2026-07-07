#include "net/minecraft/mod/lua/LuaBlockModel.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::mod::lua {
namespace {
using net::minecraft::BlockView;
using net::minecraft::block::Block;
using net::minecraft::client::gl::GL11;
using net::minecraft::client::render::Tessellator;
using net::minecraft::client::render::block::BlockRenderManager;
bool matchesConnectionRule(const BlockView* world, const Block& block, int nx, int ny, int nz, ConnectionRule rule) {
  if(world == nullptr) {
    return false;
  }
  const int neighborId = world->getBlockId(nx, ny, nz);
  Block* neighbor = Block::BLOCKS[static_cast<std::size_t>(neighborId)];
  if(neighbor == nullptr) {
    return false;
  }
  switch(rule) {
  case ConnectionRule::Same:
    return neighbor->id == block.id;
  case ConnectionRule::Opaque:
    return neighbor->isOpaque();
  case ConnectionRule::Glass:
    return Block::GLASS != nullptr && neighborId == Block::GLASS->id;
  case ConnectionRule::Fence:
    return Block::FENCE != nullptr && neighborId == Block::FENCE->id;
  }
  return false;
}
bool canConnect(const BlockView* world, const Block& block, int nx, int ny, int nz,
                const std::vector<ConnectionRule>& rules) {
  for(ConnectionRule rule : rules) {
    if(matchesConnectionRule(world, block, nx, ny, nz, rule)) {
      return true;
    }
  }
  return false;
}
void drawBounds(BlockRenderManager& manager, Block& block, int x, int y, int z, const ModelBox& box) {
  manager.ctx.setRenderBounds(box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ);
  manager.renderStandardBlock(block, x, y, z);
}
struct ActiveManualBlockDraw {
  BlockRenderManager* manager = nullptr;
  Block* block = nullptr;
  int x = 0;
  int y = 0;
  int z = 0;
  bool inventory = false;
  float brightness = 1.0f;
};
thread_local ActiveManualBlockDraw* gManualBlockDraw = nullptr;
class ScopedManualBlockDraw {
public:
  explicit ScopedManualBlockDraw(ActiveManualBlockDraw& context) : previous_(gManualBlockDraw) {
    gManualBlockDraw = &context;
  }
  ~ScopedManualBlockDraw() {
    gManualBlockDraw = previous_;
  }
  ScopedManualBlockDraw(const ScopedManualBlockDraw&) = delete;
  ScopedManualBlockDraw& operator=(const ScopedManualBlockDraw&) = delete;

private:
  ActiveManualBlockDraw* previous_;
};
net::minecraft::block::TerrainAtlasUv uvAtPixels(int textureId, double u, double v) {
  const net::minecraft::mod::TileScale tile = net::minecraft::mod::tileScale(textureId);
  return {(static_cast<double>(tile.u) + u) * tile.inv, 0.0, (static_cast<double>(tile.v) + v) * tile.inv, 0.0};
}
bool shouldDrawBox(const BlockView* world, const Block& block, const ModelBox& box, int x, int y, int z,
                   const std::vector<ConnectionRule>& rules) {
  if(box.alwaysDraw) {
    return true;
  }
  if(box.connectNorth != 0 && canConnect(world, block, x, y, z - 1, rules)) {
    return true;
  }
  if(box.connectSouth != 0 && canConnect(world, block, x, y, z + 1, rules)) {
    return true;
  }
  if(box.connectWest != 0 && canConnect(world, block, x - 1, y, z, rules)) {
    return true;
  }
  if(box.connectEast != 0 && canConnect(world, block, x + 1, y, z, rules)) {
    return true;
  }
  return false;
}
} // namespace
bool emitManualBlockModelQuad(const ManualBlockVertex* vertices, int textureId, float red, float green, float blue,
                              float alpha) {
  if(gManualBlockDraw == nullptr || gManualBlockDraw->manager == nullptr || gManualBlockDraw->block == nullptr ||
     vertices == nullptr) {
    return false;
  }
  if(textureId < 0) {
    textureId = gManualBlockDraw->block->textureId;
  }
  BlockRenderManager& manager = *gManualBlockDraw->manager;
  if(!gManualBlockDraw->inventory) {
    manager.ctx.bindTextureFor(textureId);
  }
  Tessellator& t = gManualBlockDraw->inventory ? *manager.ctx.tess : manager.ctx.activeTess(textureId);
  const double baseX = gManualBlockDraw->inventory ? 0.0 : static_cast<double>(gManualBlockDraw->x);
  const double baseY = gManualBlockDraw->inventory ? 0.0 : static_cast<double>(gManualBlockDraw->y);
  const double baseZ = gManualBlockDraw->inventory ? 0.0 : static_cast<double>(gManualBlockDraw->z);
  t.startQuads();
  t.color(red * gManualBlockDraw->brightness, green * gManualBlockDraw->brightness,
          blue * gManualBlockDraw->brightness, alpha);
  for(int i = 0; i < 4; ++i) {
    const auto uv = uvAtPixels(textureId, vertices[i].u, vertices[i].v);
    t.vertex(baseX + vertices[i].x, baseY + vertices[i].y, baseZ + vertices[i].z, uv.uMin, uv.vMin);
  }
  t.draw();
  return true;
}
bool drawLuaBlockWorld(BlockRenderManager& manager, Block& block, int x, int y, int z) {
  const BlockRegistrationSpec* spec = blockRegistrationSpecForId(block.id);
  if(spec == nullptr) {
    return false;
  }
  if(spec->model.type == LuaBlockModelSpec::Type::Manual) {
    ActiveManualBlockDraw context{&manager, &block, x, y, z, false, block.getLuminance(manager.ctx.blockView, x, y, z)};
    const ScopedManualBlockDraw scope(context);
    return invokeManualBlockModelDraw(*spec, false, x, y, z, context.brightness);
  }
  const BlockView* world = manager.ctx.blockView;
  const std::vector<ConnectionRule>& rules = spec->model.connectRules;
  for(const ModelBox& box : spec->model.boxes) {
    if(!shouldDrawBox(world, block, box, x, y, z, rules)) {
      continue;
    }
    drawBounds(manager, block, x, y, z, box);
  }
  manager.ctx.setRenderBounds(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
  return true;
}
void drawLuaBlockInventory(BlockRenderManager& manager, Block& block, int /*metadata*/, float brightness) {
  const BlockRegistrationSpec* spec = blockRegistrationSpecForId(block.id);
  if(spec == nullptr) {
    return;
  }
  const net::minecraft::block::TerrainAtlasUv uv = tileUv(block.textureId);
  GL11::glPushMatrix();
  GL11::glTranslatef(-0.5f, -0.5f, -0.5f);
  GL11::glEnable(GL11::GL_BLEND);
  GL11::glBlendFunc(GL11::GL_SRC_ALPHA, GL11::GL_ONE_MINUS_SRC_ALPHA);
  GL11::glDisable(GL11::GL_LIGHTING);
  GL11::glDisable(GL11::GL_CULL_FACE);
  Tessellator& t = *manager.ctx.tess;
  if(spec->model.type == LuaBlockModelSpec::Type::Manual) {
    ActiveManualBlockDraw context{&manager, &block, 0, 0, 0, true, brightness};
    const ScopedManualBlockDraw scope(context);
    invokeManualBlockModelDraw(*spec, true, 0, 0, 0, brightness);
    GL11::glEnable(GL11::GL_LIGHTING);
    GL11::glEnable(GL11::GL_CULL_FACE);
    GL11::glDisable(GL11::GL_BLEND);
    GL11::glPopMatrix();
    return;
  }
  for(const ModelBox& box : spec->model.boxes) {
    if(!box.alwaysDraw) {
      continue;
    }
    t.startQuads();
    t.normal(1.0f, 0.0f, 0.0f);
    t.vertex(box.maxX, box.maxY, box.minZ, uv.uMax, uv.vMin);
    t.vertex(box.maxX, box.minY, box.minZ, uv.uMax, uv.vMax);
    t.vertex(box.maxX, box.minY, box.maxZ, uv.uMin, uv.vMax);
    t.vertex(box.maxX, box.maxY, box.maxZ, uv.uMin, uv.vMin);
    t.normal(-1.0f, 0.0f, 0.0f);
    t.vertex(box.minX, box.maxY, box.maxZ, uv.uMax, uv.vMin);
    t.vertex(box.minX, box.minY, box.maxZ, uv.uMax, uv.vMax);
    t.vertex(box.minX, box.minY, box.minZ, uv.uMin, uv.vMax);
    t.vertex(box.minX, box.maxY, box.minZ, uv.uMin, uv.vMin);
    t.normal(0.0f, 0.0f, 1.0f);
    t.vertex(box.maxX, box.maxY, box.maxZ, uv.uMax, uv.vMin);
    t.vertex(box.maxX, box.minY, box.maxZ, uv.uMax, uv.vMax);
    t.vertex(box.minX, box.minY, box.maxZ, uv.uMin, uv.vMax);
    t.vertex(box.minX, box.maxY, box.maxZ, uv.uMin, uv.vMin);
    t.normal(0.0f, 0.0f, -1.0f);
    t.vertex(box.minX, box.maxY, box.minZ, uv.uMax, uv.vMin);
    t.vertex(box.minX, box.minY, box.minZ, uv.uMax, uv.vMax);
    t.vertex(box.maxX, box.minY, box.minZ, uv.uMin, uv.vMax);
    t.vertex(box.maxX, box.maxY, box.minZ, uv.uMin, uv.vMin);
    t.normal(0.0f, 1.0f, 0.0f);
    t.vertex(box.minX, box.maxY, box.minZ, uv.uMin, uv.vMin);
    t.vertex(box.minX, box.maxY, box.maxZ, uv.uMin, uv.vMax);
    t.vertex(box.maxX, box.maxY, box.maxZ, uv.uMax, uv.vMax);
    t.vertex(box.maxX, box.maxY, box.minZ, uv.uMax, uv.vMin);
    t.normal(0.0f, -1.0f, 0.0f);
    t.vertex(box.minX, box.minY, box.maxZ, uv.uMin, uv.vMin);
    t.vertex(box.minX, box.minY, box.minZ, uv.uMin, uv.vMax);
    t.vertex(box.maxX, box.minY, box.minZ, uv.uMax, uv.vMax);
    t.vertex(box.maxX, box.minY, box.maxZ, uv.uMax, uv.vMin);
    t.draw();
  }
  GL11::glEnable(GL11::GL_LIGHTING);
  GL11::glEnable(GL11::GL_CULL_FACE);
  GL11::glDisable(GL11::GL_BLEND);
  GL11::glPopMatrix();
}
} // namespace net::minecraft::mod::lua
