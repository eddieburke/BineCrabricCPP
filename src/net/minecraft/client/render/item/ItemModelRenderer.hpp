#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/ItemStack.hpp"
namespace net::minecraft::block {
class Block;
}
namespace net::minecraft::client::render::item {
struct ItemTint {
 float red = 1.0f;
 float green = 1.0f;
 float blue = 1.0f;
};
namespace ItemModelRenderer {
[[nodiscard]] net::minecraft::block::Block* blockOf(const ItemStack& stack);
[[nodiscard]] bool rendersAsBlockModel(const ItemStack& stack);
[[nodiscard]] bool hasCustomModel(const ItemStack& stack);
[[nodiscard]] bool usesModTexture(const ItemStack& stack);
[[nodiscard]] net::minecraft::block::TerrainAtlasUv spriteUv(const ItemStack& stack);
[[nodiscard]] const char* spriteAtlasPath(const ItemStack& stack);
[[nodiscard]] ItemTint tintColor(const ItemStack& stack);
} // namespace ItemModelRenderer
} // namespace net::minecraft::client::render::item
