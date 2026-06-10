#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/BlockItem.hpp"
#include "net/minecraft/item/Item.hpp"

#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::block {

struct GenericBlockItemPass {
    static void registerClass()
    {
        for (int blockId = 0; blockId < 256; ++blockId) {
            if (Block::BLOCKS[static_cast<std::size_t>(blockId)] == nullptr
                || Item::ITEMS[static_cast<std::size_t>(blockId)] != nullptr) {
                continue;
            }
            Item::ITEMS[static_cast<std::size_t>(blockId)] = new item::BlockItem(blockId - 256);
            Block::BLOCKS[static_cast<std::size_t>(blockId)]->init();
        }
    }
};

static registry::RegisterCustom<GenericBlockItemPass> s_reg(
    registry::kGenericBlockItemRegistrarPriority);

} // namespace net::minecraft::block
