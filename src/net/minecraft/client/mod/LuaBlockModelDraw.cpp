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
bool drawLuaBlockWorld(BlockRenderManager& manager, Block& block, int x, int y, int z) {
  const BlockRegistrationSpec* spec = blockRegistrationSpecForId(block.id);
  if(spec == nullptr) {
    return false;
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
void drawLuaBlockInventory(BlockRenderManager& manager, Block& block, int /*metadata*/, float /*brightness*/) {
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
