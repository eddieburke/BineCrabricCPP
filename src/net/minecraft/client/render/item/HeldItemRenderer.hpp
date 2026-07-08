#pragma once
#include "net/minecraft/client/render/MapRenderer.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::render::item {
class HeldItemRenderer {
   public:
    explicit HeldItemRenderer(net::minecraft::client::Minecraft* minecraft = nullptr);
    void renderItem(const net::minecraft::entity::LivingEntity& entity, const ItemStack& stack);
    void render(float tickDelta);
    void renderScreenOverlays(float tickDelta);
    void tick();
    void place();
    void use();
    net::minecraft::client::Minecraft* minecraft = nullptr;
    ItemStack stack{};
    float height = 0.0f;
    float prevHeight = 0.0f;
    block::BlockRenderManager blockRenderManager{};
    net::minecraft::client::render::MapRenderer mapRenderer{};
    int slot = -1;
    ItemStack* stackSource_ = nullptr;
};
}  // namespace net::minecraft::client::render::item
