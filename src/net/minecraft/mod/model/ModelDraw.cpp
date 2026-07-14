#include "net/minecraft/mod/model/ModelDraw.hpp"
#include <string>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"
#include "net/minecraft/mod/model/BakedModel.hpp"
#include "net/minecraft/mod/model/ModelRegistry.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/util/math/CoordinateHash.hpp"
#include "net/minecraft/world/BlockView.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
#endif
namespace net::minecraft::mod::model {
using namespace net::minecraft::mod::lua;
using namespace net::minecraft::client::render::item;
namespace {
using net::minecraft::BlockView;
using net::minecraft::block::Block;
using net::minecraft::client::gl::MatrixGuard;
using net::minecraft::client::render::Tessellator;
using net::minecraft::client::render::block::BlockRenderManager;
net::minecraft::block::TerrainAtlasUv uvAtPixels(int textureId, double u, double v) {
  const net::minecraft::mod::TileScale tile = net::minecraft::mod::tileScale(textureId);
  return {(static_cast<double>(tile.u) + u) * tile.inv, 0.0, (static_cast<double>(tile.v) + v) * tile.inv, 0.0};
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
template <typename BuildFields>
bool invokeModelRender(const std::string& ownerModId, int luaModelRef, BuildFields buildFields) {
  LuaApi& api = luaApi();
  if(!api.ready() || luaModelRef == kLuaNoRef || ownerModId.empty()) {
    return false;
  }
  for(const std::shared_ptr<runtime::ModHost::LoadedLuaMod>& mod : runtime::host().loadedMods()) {
    if(mod == nullptr || mod->modId != ownerModId) {
      continue;
    }
    const std::lock_guard<std::recursive_mutex> lock(mod->stateMutex);
    if(!mod->active || mod->state == nullptr) {
      return false;
    }
    auto* state = static_cast<lua_State*>(mod->state);
    const int top = api.gettop(state);
    if(api.checkstack(state, 32) == 0) {
      runtimeLog(ownerModId, "error", "model render skipped: Lua stack exhausted");
      return false;
    }
    api.rawgeti(state, kLuaRegistryIndex, luaModelRef);
    if(api.type(state, -1) != kLuaTFunction) {
      api.settop(state, top);
      return false;
    }
    api.createtable(state, 0, 9);
    buildFields(state);
    api.getfield(state, -10002, "minecraft");
    if(api.type(state, -1) == kLuaTTable) {
      api.getfield(state, -1, "tessellator");
      if(api.type(state, -1) == kLuaTTable) {
        api.setfield(state, -3, "tessellator");
      } else {
        pop(state, 1);
      }
    }
    pop(state, 1);
    const int status = api.pcallk(state, 1, 0, 0, 0, nullptr);
    api.settop(state, top);
    return status == kLuaOk;
  }
  return false;
}
} // namespace
bool parseModelCallback(lua_State* state, int index, int& ref, std::string& error) {
  LuaApi& api = luaApi();
  if(api.type(state, index) == kLuaTFunction) {
    api.pushvalue(state, index);
    ref = api.ref(state, kLuaRegistryIndex);
    return true;
  }
  error = "model must be a function";
  return false;
}
bool emitManualBlockModelQuad(
    const ManualBlockVertex* vertices, int textureId, float red, float green, float blue, float alpha) {
  if(gManualBlockDraw == nullptr || gManualBlockDraw->manager == nullptr || gManualBlockDraw->block == nullptr ||
     vertices == nullptr) {
    return false;
  }
  if(textureId < 0) {
    textureId = gManualBlockDraw->block->textureId;
  }
  BlockRenderManager& manager = *gManualBlockDraw->manager;
  if(!gManualBlockDraw->inventory && manager.ctx.textureOverride >= 0) {
    textureId = manager.ctx.textureOverride;
  }
  if(!gManualBlockDraw->inventory) {
    manager.ctx.bindTextureFor(textureId);
  }
  Tessellator& t = gManualBlockDraw->inventory ? *manager.ctx.tess : manager.ctx.activeTess(textureId);
  const bool capturing = !gManualBlockDraw->inventory && manager.ctx.modMeshes != nullptr;
  const double baseX = gManualBlockDraw->inventory ? 0.0 : static_cast<double>(gManualBlockDraw->x);
  const double baseY = gManualBlockDraw->inventory ? 0.0 : static_cast<double>(gManualBlockDraw->y);
  const double baseZ = gManualBlockDraw->inventory ? 0.0 : static_cast<double>(gManualBlockDraw->z);
  if(!capturing) {
    t.startQuads();
  }
  t.color(red * gManualBlockDraw->brightness,
          green * gManualBlockDraw->brightness,
          blue * gManualBlockDraw->brightness,
          alpha);
  for(int i = 0; i < 4; ++i) {
    const auto uv = uvAtPixels(textureId, vertices[i].u, vertices[i].v);
    t.vertex(baseX + vertices[i].x, baseY + vertices[i].y, baseZ + vertices[i].z, uv.uMin, uv.vMin);
  }
  if(!capturing) {
    t.draw();
  }
  return true;
}
bool emitManualItemModelQuad(
    const ManualBlockVertex* vertices, int textureId, float red, float green, float blue, float alpha) {
  if(gManualItemDraw == nullptr || vertices == nullptr) {
    return false;
  }
  Tessellator& t = Tessellator::INSTANCE;
  t.startQuads();
  t.color(red * gManualItemDraw->brightness,
          green * gManualItemDraw->brightness,
          blue * gManualItemDraw->brightness,
          alpha);
  for(int i = 0; i < 4; ++i) {
    const auto uv = uvAtPixels(textureId, vertices[i].u, vertices[i].v);
    t.vertex(vertices[i].x, vertices[i].y, vertices[i].z, uv.uMin, uv.vMin);
  }
  t.draw();
  return true;
}
bool drawBakedModelQuads(int handle) {
  return drawBakedModelQuads(handle, BakedQuadTransform{});
}
bool drawBakedModelQuads(int handle, const BakedQuadTransform& transform) {
  const BakedModel* baked = bakedModelForHandle(handle);
  if(baked == nullptr) {
    return false;
  }
  const ActiveManualBlockDraw* draw = gManualBlockDraw;
  const BlockView* blockView =
      (draw != nullptr && draw->manager != nullptr) ? draw->manager->ctx.blockView : nullptr;
  const bool cullFaces = draw != nullptr && !draw->inventory && draw->block != nullptr && blockView != nullptr;
  bool emitted = false;
  for(const BakedTextureBatch& batch : baked->batches) {
    const int textureId = texture(batch.texturePath.c_str());
    for(const BakedQuad& quad : batch.quads) {
      if(cullFaces) {
        const int side = static_cast<int>(quad.face);
        int nx = draw->x;
        int ny = draw->y;
        int nz = draw->z;
        switch(side) {
        case 0:
          --ny;
          break;
        case 1:
          ++ny;
          break;
        case 2:
          --nz;
          break;
        case 3:
          ++nz;
          break;
        case 4:
          --nx;
          break;
        case 5:
          ++nx;
          break;
        default:
          break;
        }
        if(!draw->block->isSideVisible(blockView, nx, ny, nz, side)) {
          continue;
        }
      }
      ManualBlockVertex vertices[4];
      for(int i = 0; i < 4; ++i) {
        vertices[i].x = (quad.vertices[i].x - 0.5) * transform.scale + 0.5 + transform.offsetX;
        vertices[i].y = (quad.vertices[i].y - 0.5) * transform.scale + 0.5 + transform.offsetY;
        vertices[i].z = (quad.vertices[i].z - 0.5) * transform.scale + 0.5 + transform.offsetZ;
        vertices[i].u = quad.vertices[i].u * 16.0;
        vertices[i].v = quad.vertices[i].v * 16.0;
      }
      const float red = quad.red * quad.shade * transform.colorR;
      const float green = quad.green * quad.shade * transform.colorG;
      const float blue = quad.blue * quad.shade * transform.colorB;
      emitted = emitManualBlockModelQuad(vertices, textureId, red, green, blue, quad.alpha) ||
                emitManualItemModelQuad(vertices, textureId, red, green, blue, quad.alpha) || emitted;
    }
  }
  return emitted;
}
bool invokeManualBlockModelDraw(
    const BlockRegistrationSpec& spec, bool inventory, int x, int y, int z, float brightness) {
  LuaApi& api = luaApi();
  return invokeModelRender(spec.ownerModId, spec.modelRef, [&](lua_State* state) {
    api.pushstring(state, inventory ? "inventory" : "world");
    api.setfield(state, -2, "type");
    api.pushinteger(state, x);
    api.setfield(state, -2, "x");
    api.pushinteger(state, y);
    api.setfield(state, -2, "y");
    api.pushinteger(state, z);
    api.setfield(state, -2, "z");
    api.pushboolean(state, inventory ? 1 : 0);
    api.setfield(state, -2, "inventory");
    api.pushnumber(state, brightness);
    api.setfield(state, -2, "brightness");
    api.pushinteger(state, spec.blockId);
    api.setfield(state, -2, "block_id");
    api.pushstring(state, spec.texturePath.c_str());
    api.setfield(state, -2, "texture");
    api.pushinteger(state, spec.terrainTextureId);
    api.setfield(state, -2, "texture_id");
  });
}
// Baked-model equivalent of LuaModBlock::getRenderBounds/getColorMultiplier:
// register_block's coordinate_bounds/coordinate_color only affect the vanilla
// cube-shaped render path unless applied here too, since a block with a
// model takes the drawBakedModelQuads path instead.
static BakedQuadTransform coordinateQuadTransform(const BlockRegistrationSpec& spec, int x, int y, int z) {
  BakedQuadTransform transform;
  if(spec.coordinateBounds) {
    const lua::CoordinateVariedTransform varied = lua::coordinateVariedTransform(spec, x, y, z);
    transform.scale = varied.scale;
    transform.offsetX = varied.offsetX;
    transform.offsetY = varied.offsetY;
    transform.offsetZ = varied.offsetZ;
  }
  if(spec.coordinateColor) {
    const int color = net::minecraft::util::math::coordinateColor(x, y, z);
    transform.colorR = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    transform.colorG = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
    transform.colorB = static_cast<float>(color & 0xFF) / 255.0f;
  }
  return transform;
}
bool drawLuaBlockWorld(BlockRenderManager& manager, Block& block, int x, int y, int z) {
  const BlockRegistrationSpec* spec = blockRegistrationSpecForId(block.id);
  if(spec == nullptr || (spec->modelRef == kLuaNoRef && spec->bakedModel == 0)) {
    return false;
  }
  ActiveManualBlockDraw context{
      &manager, &block, x, y, z, false, block.getLuminance(manager.ctx.blockView, x, y, z)};
  const ScopedManualBlockDraw scope(context);
  gl::setCap(gl::cap::AlphaTest, true);
  gl::alphaFunc(gl::compare::Greater, 0.1f);
  if(spec->bakedModel != 0) {
    if(spec->coordinateBounds || spec->coordinateColor) {
      return drawBakedModelQuads(spec->bakedModel, coordinateQuadTransform(*spec, x, y, z));
    }
    return drawBakedModelQuads(spec->bakedModel);
  }
  return invokeManualBlockModelDraw(*spec, false, x, y, z, context.brightness);
}
void drawLuaBlockInventory(BlockRenderManager& manager, Block& block, int /*metadata*/, float brightness) {
  const BlockRegistrationSpec* spec = blockRegistrationSpecForId(block.id);
  if(spec == nullptr || (spec->modelRef == kLuaNoRef && spec->bakedModel == 0)) {
    return;
  }
  const gl::preset::ModBlockInventory inventoryDraw(true);
  const MatrixGuard matrix;
  gl::translatef(-0.5f, -0.5f, -0.5f);
  ActiveManualBlockDraw context{&manager, &block, 0, 0, 0, true, brightness};
  const ScopedManualBlockDraw scope(context);
  if(spec->bakedModel != 0) {
    drawBakedModelQuads(spec->bakedModel);
    return;
  }
  invokeManualBlockModelDraw(*spec, true, 0, 0, 0, brightness);
}
bool drawLuaItemModel(Tessellator& tessellator, const ItemStack& stack, float brightness) {
  (void)tessellator;
  const ItemRegistrationSpec* spec = itemRegistrationSpecForId(stack.itemId);
  if(spec == nullptr || (spec->modelRef == kLuaNoRef && spec->bakedModel == 0)) {
    return false;
  }
  LuaApi& api = luaApi();
  ActiveManualItemDraw context{brightness};
  const ScopedManualItemDraw scope(context);
  if(spec->bakedModel != 0) {
    return drawBakedModelQuads(spec->bakedModel);
  }
  return invokeModelRender(spec->ownerModId, spec->modelRef, [&](lua_State* state) {
    api.pushstring(state, "item");
    api.setfield(state, -2, "type");
    api.pushnumber(state, brightness);
    api.setfield(state, -2, "brightness");
    api.pushinteger(state, spec->itemId);
    api.setfield(state, -2, "item_id");
    api.pushstring(state, spec->texturePath.c_str());
    api.setfield(state, -2, "texture");
    api.pushinteger(state, spec->itemsTextureId);
    api.setfield(state, -2, "texture_id");
  });
}
#ifdef MINECRAFT_NATIVE_EXPORTS
bool drawBakedModelWorld(int handle, const WorldModelDraw& options) {
  const BakedModel* baked = bakedModelForHandle(handle);
  if(baked == nullptr || !runtime::ModWorldDrawContext::active() || client::Minecraft::INSTANCE == nullptr) {
    return false;
  }
  const client::render::FrameRenderCamera& camera = client::render::RenderCameraState::instance().frame();
  const bool textured = !baked->batches.empty() && !baked->batches.front().texturePath.empty();
  const client::gl::preset::ModLuaDraw modCaps(textured, options.blend, options.cull, options.depthTest,
                                               options.depthWrite);
  client::render::platform::Lighting::turnOff();
  const client::gl::MatrixGuard matrix;
  client::gl::translatef(static_cast<float>(options.x - camera.x), static_cast<float>(options.y - camera.y),
                         static_cast<float>(options.z - camera.z));
  if(options.yaw != 0.0f) {
    client::gl::rotatef(options.yaw, 0.0f, 1.0f, 0.0f);
  }
  if(options.pitch != 0.0f) {
    client::gl::rotatef(options.pitch, 1.0f, 0.0f, 0.0f);
  }
  if(options.roll != 0.0f) {
    client::gl::rotatef(options.roll, 0.0f, 0.0f, 1.0f);
  }
  if(options.scale != 1.0f) {
    client::gl::scaled(options.scale, options.scale, options.scale);
  }
  client::gl::translatef(-0.5f, -options.pivotY, -0.5f);
  client::texture::TextureManager& textures = client::Minecraft::INSTANCE->textureManager;
  Tessellator& tessellator = Tessellator::INSTANCE;
  for(const BakedTextureBatch& batch : baked->batches) {
    if(!batch.texturePath.empty()) {
      client::gl::bindTexture(client::gl::cap::Texture2D, textures.getTextureId(batch.texturePath));
    }
    tessellator.startQuads();
    for(const BakedQuad& quad : batch.quads) {
      const float light = quad.shade * options.brightness;
      tessellator.color(quad.red * light, quad.green * light, quad.blue * light, quad.alpha * options.alpha);
      for(const BakedVertex& vertex : quad.vertices) {
        tessellator.vertex(vertex.x, vertex.y, vertex.z, vertex.u, vertex.v);
      }
    }
    tessellator.draw();
  }
  return true;
}
bool drawItemStackWorld(const ItemStack& stack, const WorldModelDraw& options) {
  if(!runtime::ModWorldDrawContext::active() || client::Minecraft::INSTANCE == nullptr) {
    return false;
  }
  const bool custom = ItemModelRenderer::hasCustomModel(stack);
  const bool blockModel = !custom && ItemModelRenderer::rendersAsBlockModel(stack);
  if(!custom && !blockModel) {
    return false;
  }
  Block* block = blockModel ? ItemModelRenderer::blockOf(stack) : nullptr;
  if(blockModel && block == nullptr) {
    return false;
  }
  const client::render::FrameRenderCamera& camera = client::render::RenderCameraState::instance().frame();
  const client::gl::preset::ModLuaDraw modCaps(true, options.blend, options.cull, options.depthTest,
                                               options.depthWrite);
  client::render::platform::Lighting::turnOff();
  const client::gl::MatrixGuard matrix;
  client::gl::translatef(static_cast<float>(options.x - camera.x), static_cast<float>(options.y - camera.y),
                         static_cast<float>(options.z - camera.z));
  if(options.yaw != 0.0f) {
    client::gl::rotatef(options.yaw, 0.0f, 1.0f, 0.0f);
  }
  if(options.pitch != 0.0f) {
    client::gl::rotatef(options.pitch, 1.0f, 0.0f, 0.0f);
  }
  if(options.roll != 0.0f) {
    client::gl::rotatef(options.roll, 0.0f, 0.0f, 1.0f);
  }
  if(options.scale != 1.0f) {
    client::gl::scaled(options.scale, options.scale, options.scale);
  }
  client::texture::TextureManager& textures = client::Minecraft::INSTANCE->textureManager;
  if(custom) {
    // Custom item models are baked in 0..1 model space; recentre onto the pivot.
    client::gl::translatef(-0.5f, -options.pivotY, -0.5f);
    if(ItemModelRenderer::usesModTexture(stack)) {
      net::minecraft::mod::bind(textures, stack.getTextureId());
    } else {
      client::gl::bindTexture(client::gl::cap::Texture2D, textures.getTextureId(ItemModelRenderer::spriteAtlasPath(stack)));
    }
    return drawLuaItemModel(Tessellator::INSTANCE, stack, options.brightness);
  }
  // The inventory block renderers (vanilla and drawLuaBlockInventory) emit
  // geometry already centred on the origin, so only the pivot's deviation
  // from centre needs compensating; a second -0.5 shift would put the cube's
  // visual centre half a block off the rotation pivot (lopsided tumbling).
  if(options.pivotY != 0.5f) {
    client::gl::translatef(0.0f, 0.5f - options.pivotY, 0.0f);
  }
  if(net::minecraft::mod::isMod(block->textureId)) {
    net::minecraft::mod::bind(textures, block->textureId);
  } else {
    client::gl::bindTexture(client::gl::cap::Texture2D, textures.getTextureId("/terrain.png"));
  }
  static BlockRenderManager itemDropBlockManager;
  auto* previousTextureManager = itemDropBlockManager.ctx.textureManager;
  const bool previousUseAo = itemDropBlockManager.ctx.faceState.useAo;
  itemDropBlockManager.ctx.textureManager = &textures;
  itemDropBlockManager.ctx.faceState.useAo = false;
  itemDropBlockManager.render(*block, stack.getDamage(), options.brightness);
  itemDropBlockManager.ctx.textureManager = previousTextureManager;
  itemDropBlockManager.ctx.faceState.useAo = previousUseAo;
  return true;
}
#else
bool drawBakedModelWorld(int /*handle*/, const WorldModelDraw& /*options*/) {
  return false;
}
bool drawItemStackWorld(const ItemStack& /*stack*/, const WorldModelDraw& /*options*/) {
  return false;
}
#endif
bool itemStackBounds(const ItemStack& stack, BakedBounds& outBounds) {
  if(ItemModelRenderer::hasCustomModel(stack)) {
    const ItemRegistrationSpec* spec = itemRegistrationSpecForId(stack.itemId);
    if(spec == nullptr || spec->bakedModel == 0) {
      return false;
    }
    const BakedModel* baked = bakedModelForHandle(spec->bakedModel);
    if(baked == nullptr || baked->bounds.empty) {
      return false;
    }
    outBounds = baked->bounds;
    return true;
  }
  if(ItemModelRenderer::rendersAsBlockModel(stack)) {
    Block* block = ItemModelRenderer::blockOf(stack);
    if(block == nullptr) {
      return false;
    }
    const BlockRegistrationSpec* spec = blockRegistrationSpecForId(block->id);
    if(spec != nullptr && spec->bakedModel != 0) {
      const BakedModel* baked = bakedModelForHandle(spec->bakedModel);
      if(baked != nullptr && !baked->bounds.empty) {
        outBounds = baked->bounds;
        return true;
      }
    }
    // Vanilla (and shape-less custom) blocks: approximate with the full unit
    // cube. Non-cube shapes (stairs, fences, ...) are subsets of this box, so
    // it is a safe (if slightly loose) tumble hitbox rather than an exact one.
    outBounds.min[0] = outBounds.min[1] = outBounds.min[2] = 0.0f;
    outBounds.max[0] = outBounds.max[1] = outBounds.max[2] = 1.0f;
    outBounds.empty = false;
    return true;
  }
  return false;
}
} // namespace net::minecraft::mod::model
