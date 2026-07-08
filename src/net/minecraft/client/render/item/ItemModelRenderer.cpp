#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"

#include <cstddef>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/mod/lua/LuaItemModel.hpp"
#include "net/minecraft/mod/lua/LuaItemRegistry.hpp"

namespace net::minecraft::client::render::item {
namespace ItemModelRenderer {
net::minecraft::block::Block* blockOf(const ItemStack& stack) {
    if (stack.itemId < 0 || stack.itemId >= net::minecraft::block::Block::BLOCK_COUNT) {
        return nullptr;
    }
    return net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(stack.itemId)];
}

bool rendersAsBlockModel(const ItemStack& stack) {
    net::minecraft::block::Block* block = blockOf(stack);
    if (block != nullptr && block->hasItemOverride()) {
        return false;
    }
    return block != nullptr && block::BlockRenderManager::isSideLit(block->getRenderType());
}

bool hasCustomModel(const ItemStack& stack) {
    if (net::minecraft::mod::lua::itemModelHandOverrideActive() && !rendersAsBlockModel(stack)) {
        return true;
    }
    const auto* spec = net::minecraft::mod::lua::itemRegistrationSpecForId(stack.itemId);
    return spec != nullptr && spec->model.type != net::minecraft::mod::lua::LuaItemModelSpec::Type::Flat;
}

bool usesModTexture(const ItemStack& stack) {
    return net::minecraft::mod::isMod(stack.getTextureId());
}

net::minecraft::block::TerrainAtlasUv spriteUv(const ItemStack& stack) {
    if (usesModTexture(stack)) {
        return {0.0, 1.0, 0.0, 1.0};
    }
    const int sprite = stack.getTextureId();
    return {static_cast<double>((sprite % 16) * 16) / 256.0,
            static_cast<double>((sprite % 16) * 16 + 16) / 256.0,
            static_cast<double>((sprite / 16) * 16) / 256.0,
            static_cast<double>((sprite / 16) * 16 + 16) / 256.0};
}

const char* spriteAtlasPath(const ItemStack& stack) {
    return stack.itemId < net::minecraft::block::Block::BLOCK_COUNT ? "/terrain.png" : "/gui/items.png";
}

ItemTint tintColor(const ItemStack& stack) {
    if (stack.getItem() == nullptr) {
        return ItemTint{};
    }
    const int packed = stack.getItem()->getColorMultiplier(stack.getDamage());
    return ItemTint{
        static_cast<float>((packed >> 16) & 0xFF) / 255.0f,
        static_cast<float>((packed >> 8) & 0xFF) / 255.0f,
        static_cast<float>(packed & 0xFF) / 255.0f,
    };
}
}  // namespace ItemModelRenderer
}  // namespace net::minecraft::client::render::item
