#pragma once
#include "net/minecraft/item/ItemStack.hpp"
namespace net::minecraft::block {
class Block;
}
namespace net::minecraft::client::render::item {
// RGB tint applied to an item/block icon (each channel 0..1).
struct ItemTint {
  float red = 1.0f;
  float green = 1.0f;
  float blue = 1.0f;
};
// Shared item-rendering primitives used by every place an ItemStack is drawn:
// the GUI slot (ItemRenderer), the dropped item in the world (ItemEntityRenderer),
// and the item held in hand (HeldItemRenderer). Before this existed each of those
// re-derived the block-vs-sprite branch, the atlas selection, and the tint
// extraction independently.
namespace ItemModelRenderer {
// The block backing a block-item stack (id < 256), or nullptr for true items.
[[nodiscard]] net::minecraft::block::Block* blockOf(const ItemStack& stack);
// True when the stack should be drawn as a lit 3D block model rather than a flat
// sprite (a full cube, slab, fence, cactus or piston).
[[nodiscard]] bool rendersAsBlockModel(const ItemStack& stack);
// Atlas texture path for this stack's flat sprite: the terrain atlas for block
// items, the item atlas otherwise.
[[nodiscard]] const char* spriteAtlasPath(const ItemStack& stack);
// Per-item display tint (e.g. dyed leather, foliage). White when the stack has
// no Item backing it.
[[nodiscard]] ItemTint tintColor(const ItemStack& stack);
} // namespace ItemModelRenderer
} // namespace net::minecraft::client::render::item
