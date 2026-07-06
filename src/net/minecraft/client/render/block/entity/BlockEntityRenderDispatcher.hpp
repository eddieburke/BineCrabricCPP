#pragma once
#include "net/minecraft/block/entity/MobSpawnerBlockEntity.hpp"
#include "net/minecraft/block/entity/PistonBlockEntity.hpp"
#include "net/minecraft/block/entity/SignBlockEntity.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderer.hpp"
#include "net/minecraft/client/render/block/entity/MobSpawnerBlockEntityRenderer.hpp"
#include "net/minecraft/client/render/block/entity/PistonBlockEntityRenderer.hpp"
#include "net/minecraft/client/render/block/entity/SignBlockEntityRenderer.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
namespace net::minecraft::client::font {
class TextRenderer;
}
namespace net::minecraft::client::texture {
class TextureManager;
}
namespace net::minecraft::client::render::block::entity {
class BlockEntityRenderDispatcher {
public:
  BlockEntityRenderDispatcher();
  static BlockEntityRenderDispatcher& instance();
  template <typename BlockEntityType>
  void registerRenderer(std::unique_ptr<BlockEntityRenderer> renderer) {
    registerRenderer(std::type_index(typeid(BlockEntityType)), std::move(renderer));
  }
  void registerRenderer(std::type_index key, std::unique_ptr<BlockEntityRenderer> renderer) {
    renderer->setDispatcher(this);
    renderers[key] = std::move(renderer);
  }
  [[nodiscard]] BlockEntityRenderer* getRenderer(const std::type_info& type) {
    const auto it = renderers.find(std::type_index(type));
    if(it != renderers.end()) {
      return it->second.get();
    }
    return nullptr;
  }
  [[nodiscard]] BlockEntityRenderer* getRenderer(const net::minecraft::block::entity::BlockEntity& blockEntity) {
    return getRenderer(typeid(blockEntity));
  }
  [[nodiscard]] bool hasRenderer(const net::minecraft::block::entity::BlockEntity& blockEntity) const {
    const auto it = renderers.find(std::type_index(typeid(blockEntity)));
    return it != renderers.end() && it->second != nullptr;
  }
  void prepare(net::minecraft::World* worldIn, ::net::minecraft::client::texture::TextureManager* textureManagerIn,
               font::TextRenderer* textRendererIn, const net::minecraft::Entity* cameraIn, float tickDelta);
  void render(const net::minecraft::block::entity::BlockEntity& blockEntity, float tickDelta);
  void render(const net::minecraft::block::entity::BlockEntity& blockEntity, double x, double y, double z,
              float tickDelta) {
    if(auto* renderer = getRenderer(blockEntity); renderer != nullptr) {
      renderer->render(blockEntity, x, y, z, tickDelta);
    }
  }
  void setWorld(net::minecraft::World* worldIn) {
    world = worldIn;
    for(auto& [_, renderer] : renderers) {
      if(renderer != nullptr) {
        renderer->setWorld(worldIn);
      }
    }
  }
  [[nodiscard]] font::TextRenderer* getTextRenderer() const noexcept {
    return textRenderer;
  }
  std::unordered_map<std::type_index, std::unique_ptr<BlockEntityRenderer>> renderers{};
  font::TextRenderer* textRenderer = nullptr;
  ::net::minecraft::client::texture::TextureManager* textureManager = nullptr;
  net::minecraft::World* world = nullptr;
  const net::minecraft::Entity* camera = nullptr;
  float cameraYaw = 0.0f;
  float cameraPitch = 0.0f;
  double cameraX = 0.0;
  double cameraY = 0.0;
  double cameraZ = 0.0;
  inline static double offsetX = 0.0;
  inline static double offsetY = 0.0;
  inline static double offsetZ = 0.0;
};
} // namespace net::minecraft::client::render::block::entity
