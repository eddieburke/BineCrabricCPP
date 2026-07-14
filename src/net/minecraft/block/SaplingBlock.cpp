#include "net/minecraft/block/SaplingBlock.hpp"
#include "net/minecraft/item/SaplingBlockItem.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace {
net::minecraft::BlockSoundGroup kDirtSound("grass", 1.0f, 1.0f);
}
namespace net::minecraft::block {
void SaplingBlock::registerClass() {
  Block::SAPLING = (new SaplingBlock(kBlockId, 15))
                       ->setHardness(0.0f)
                       ->setSoundGroup(&kDirtSound)
                       ->setTranslationKey("sapling")
                       ->ignoreMetaUpdates();
}
void SaplingBlock::registerBlockItems() {
  (new item::SaplingBlockItem(6 - 256))->setTranslationKey("sapling")->setFuelTime(100);
}
MC_REGISTER_BLOCK(SaplingBlock)
} // namespace net::minecraft::block
