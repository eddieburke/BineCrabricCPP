#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/mod/GameHooks.hpp"
namespace net::minecraft::client::render::block::entity {
BlockEntityRenderDispatcher& BlockEntityRenderDispatcher::instance() {
  static BlockEntityRenderDispatcher dispatcher;
  return dispatcher;
}
BlockEntityRenderDispatcher::BlockEntityRenderDispatcher() {
  registerRenderer<net::minecraft::block::entity::SignBlockEntity>(std::make_unique<SignBlockEntityRenderer>());
  registerRenderer<net::minecraft::block::entity::MobSpawnerBlockEntity>(
      std::make_unique<MobSpawnerBlockEntityRenderer>());
  registerRenderer<net::minecraft::block::entity::PistonBlockEntity>(std::make_unique<PistonBlockEntityRenderer>());
}
void BlockEntityRenderDispatcher::prepare(net::minecraft::World* worldIn,
                                          ::net::minecraft::client::texture::TextureManager* textureManagerIn,
                                          font::TextRenderer* textRendererIn,
                                          const net::minecraft::Entity* cameraIn,
                                          float tickDelta) {
  if(world != worldIn) {
    setWorld(worldIn);
  }
  textureManager = textureManagerIn;
  textRenderer = textRendererIn;
  camera = cameraIn;
  if(camera != nullptr) {
    cameraYaw = camera->prevYaw + (camera->yaw - camera->prevYaw) * tickDelta;
    cameraPitch = camera->prevPitch + (camera->pitch - camera->prevPitch) * tickDelta;
    cameraX = camera->lastTickX + (camera->x - camera->lastTickX) * static_cast<double>(tickDelta);
    cameraY = camera->lastTickY + (camera->y - camera->lastTickY) * static_cast<double>(tickDelta);
    cameraZ = camera->lastTickZ + (camera->z - camera->lastTickZ) * static_cast<double>(tickDelta);
  } else {
    cameraYaw = 0.0f;
    cameraPitch = 0.0f;
    cameraX = cameraY = cameraZ = 0.0;
  }
}
void BlockEntityRenderDispatcher::render(const net::minecraft::block::entity::BlockEntity& blockEntity,
                                         float tickDelta) {
  if(net::minecraft::mod::hooks().hasListeners<net::minecraft::mod::PreTileEntityRenderEvent>()) {
    net::minecraft::mod::PreTileEntityRenderEvent event;
    event.entity = &blockEntity;
    event.x = blockEntity.x;
    event.y = blockEntity.y;
    event.z = blockEntity.z;
    event.id = blockEntity.id();
    event.tickDelta = tickDelta;
    net::minecraft::mod::hooks().publish(event);
    if(event.canceled) {
      return;
    }
  }
  if(blockEntity.distanceFrom(cameraX, cameraY, cameraZ) < 4096.0) {
    const float brightness = world->getLightBrightness(blockEntity.x, blockEntity.y, blockEntity.z);
    gl::color3f(brightness, brightness, brightness);
    render(blockEntity,
           static_cast<double>(blockEntity.x) - offsetX,
           static_cast<double>(blockEntity.y) - offsetY,
           static_cast<double>(blockEntity.z) - offsetZ,
           tickDelta);
  }
}
} // namespace net::minecraft::client::render::block::entity
