#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/mod/lua/LuaModEntity.hpp"
namespace net::minecraft::client::render::entity {
namespace {
// Mod entities draw their visuals from Lua (world_render stage + model.draw),
// so the native renderer emits no geometry; it exists so postRender gives mod
// entities the same blob shadow vanilla entities get. Mods can override the
// width-derived radius by putting a "shadow_radius" float in the entity data
// (0 disables the shadow).
class LuaModEntityRenderer : public EntityRenderer {
 public:
 void render(const net::minecraft::Entity& entity, double, double, double, float, float) override {
  shadowRadius = entity.width * 0.5f;
  const auto* modEntity = dynamic_cast<const net::minecraft::mod::lua::LuaModEntity*>(&entity);
  if(modEntity != nullptr && modEntity->data().contains("shadow_radius")) {
   shadowRadius = modEntity->data().getFloat("shadow_radius");
  }
 }
};
}
} // namespace net::minecraft::client::render::entity
namespace net::minecraft::mod::lua {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> LuaModEntity::ClientRenderer::create() {
 return std::make_unique<::net::minecraft::client::render::entity::LuaModEntityRenderer>();
}
} // namespace net::minecraft::mod::lua
namespace {
const ::net::minecraft::registry::RegisterEntityRenderer<::net::minecraft::mod::lua::LuaModEntity>
    registerLuaModEntityRenderer;
} // namespace
