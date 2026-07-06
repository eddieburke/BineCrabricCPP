#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/item/Item.hpp"
#include <cstddef>
namespace net::minecraft::client::render::item {
namespace ItemModelRenderer {
net::minecraft::block::Block* blockOf(const ItemStack& stack) {
  if(stack.itemId < 0 || stack.itemId >= net::minecraft::block::Block::BLOCK_COUNT) {
    return nullptr;
  }
  return net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(stack.itemId)];
}
bool rendersAsBlockModel(const ItemStack& stack) {
  net::minecraft::block::Block* block = blockOf(stack);
  return block != nullptr && block::BlockRenderManager::isSideLit(block->getRenderType());
}
const char* spriteAtlasPath(const ItemStack& stack) {
  return stack.itemId < net::minecraft::block::Block::BLOCK_COUNT ? "/terrain.png" : "/gui/items.png";
}
ItemTint tintColor(const ItemStack& stack) {
  if(stack.getItem() == nullptr) {
    return ItemTint{};
  }
  const int packed = stack.getItem()->getColorMultiplier(stack.getDamage());
  return ItemTint{
      static_cast<float>((packed >> 16) & 0xFF) / 255.0f,
      static_cast<float>((packed >> 8) & 0xFF) / 255.0f,
      static_cast<float>(packed & 0xFF) / 255.0f,
  };
}
}
} // namespace net::minecraft::client::render::item::ItemModelRenderer
