#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/resource/language/I18n.hpp"
#endif
#include "net/minecraft/item/BlockItem.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/mod/ModClient.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/mod/lua/LuaModNaming.hpp"
#include "net/minecraft/mod/lua/ModIdRegistry.hpp"
#include "net/minecraft/mod/model/ModModels.hpp"
#include "net/minecraft/registry/ContentRegistries.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/util/math/CoordinateHash.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::mod::lua {
CoordinateVariedTransform coordinateVariedTransform(const BlockRegistrationSpec& spec, int x, int y, int z) {
 namespace hash = net::minecraft::util::math;
 net::minecraft::JavaRandom random = hash::coordinateRandom(x, y, z);
 CoordinateVariedTransform transform;
 transform.offsetX = hash::coordinateOffset(random, spec.boundsOffset);
 transform.offsetY = hash::coordinateOffset(random, spec.boundsOffset);
 transform.offsetZ = hash::coordinateOffset(random, spec.boundsOffset);
 transform.scale = hash::coordinateScale(random, spec.minScale, spec.maxScale);
 return transform;
}
net::minecraft::Box coordinateVariedBlockBounds(const BlockRegistrationSpec& spec, int x, int y, int z) {
 const CoordinateVariedTransform transform = coordinateVariedTransform(spec, x, y, z);
 const float padding = std::clamp(spec.boundsPadding, 0.0f, 0.49f);
 const float half = (0.5f - padding) * transform.scale;
 const auto low = [half](float offset) {
  return std::clamp(0.5f - half + offset, 0.0f, 1.0f);
 };
 const auto high = [half](float offset) {
  return std::clamp(0.5f + half + offset, 0.0f, 1.0f);
 };
 return {low(transform.offsetX),
         low(transform.offsetY),
         low(transform.offsetZ),
         high(transform.offsetX),
         high(transform.offsetY),
         high(transform.offsetZ)};
}
namespace {
using net::minecraft::BlockSoundGroup;
using net::minecraft::BlockView;
using net::minecraft::Box;
using net::minecraft::JavaRandom;
using net::minecraft::World;
using net::minecraft::block::Block;
using net::minecraft::block::material::Material;
BlockSoundGroup kModBlockSound("stone", 1.0f, 1.0f);
Material* materialFromName(const std::string& name) {
 std::string lowered = name;
 std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
  return static_cast<char>(std::tolower(ch));
 });
 static const std::unordered_map<std::string, Material*> kMaterials = {
     {"stone", &Material::STONE},
     {"metal", &Material::METAL},
     {"wood", &Material::WOOD},
     {"glass", &Material::GLASS},
 };
 const auto it = kMaterials.find(lowered);
 return it != kMaterials.end() ? it->second : &Material::STONE;
}
void registerModBlockDisplayName(const BlockRegistrationSpec& spec) {
#ifdef MINECRAFT_NATIVE_EXPORTS
 std::string keyToken = spec.translationKey;
 if(keyToken.empty()) {
  keyToken = "block" + std::to_string(spec.blockId);
 }
 const std::string i18nKey = "tile." + keyToken + ".name";
 std::string label = spec.displayName;
 if(label.empty()) {
  label = humanizeTranslationKey(keyToken);
 }
 if(label.empty()) {
  label = "Block " + std::to_string(spec.blockId);
 }
 client::resource::language::I18n::addTranslation(i18nKey, std::move(label));
#else
 (void)spec;
#endif
}
class LuaModBlock : public Block {
 public:
 LuaModBlock(int id, int textureId, Material& material, const BlockRegistrationSpec& spec)
     : Block(id, textureId, material), spec_(spec) {
#ifdef MINECRAFT_NATIVE_EXPORTS
  // A baked model's bounds are fixed once it is baked, so resolve everything
  // that depends on them here. isFullCube() in particular is asked once per
  // block face while meshing chunks, and the model lookup behind it takes a
  // global mutex — not something to pay on that path.
  if(spec.bakedModel != 0) {
   if(const net::minecraft::mod::model::BakedModel* baked =
          net::minecraft::mod::model::bakedModelForHandle(spec.bakedModel)) {
    if(!baked->bounds.empty) {
     setBoundingBox(baked->bounds.min[0],
                    baked->bounds.min[1],
                    baked->bounds.min[2],
                    baked->bounds.max[0],
                    baked->bounds.max[1],
                    baked->bounds.max[2]);
     fullCube_ = baked->bounds.min[0] <= 0.0f && baked->bounds.min[1] <= 0.0f && baked->bounds.min[2] <= 0.0f &&
                 baked->bounds.max[0] >= 1.0f && baked->bounds.max[1] >= 1.0f && baked->bounds.max[2] >= 1.0f;
    }
   }
  }
#endif
 }
 [[nodiscard]] bool isOpaque() const override {
  return spec_.opaque;
 }
 [[nodiscard]] bool isFullCube() const override {
  return fullCube_;
 }
 [[nodiscard]] bool receivesEntityShadow() const override {
  // Custom-shaped mod blocks are rarely flagged full-cube, but anything
  // solid enough to stand on should still catch entity blob shadows.
  return fullCube_ || material.isSolid();
 }
 [[nodiscard]] int getColorMultiplier(const BlockView* blockView, int x, int y, int z) const override {
  if(spec_.coordinateColor) {
   return net::minecraft::util::math::coordinateColor(x, y, z);
  }
  return Block::getColorMultiplier(blockView, x, y, z);
 }
 [[nodiscard]] int getRenderType() const override {
#ifdef MINECRAFT_NATIVE_EXPORTS
  // register_block requires a baked model (LuaBlockBindings.cpp), so every
  // LuaModBlock instance takes the custom-model render path.
  return 31; // custom type
#else
  return Block::getRenderType();
#endif
 }
 [[nodiscard]] int getRenderLayer() const override {
  return spec_.translucent ? 1 : 0;
 }
 [[nodiscard]] Box getRenderBounds(const BlockView* /*blockView*/, int x, int y, int z) const override {
  if(spec_.coordinateBounds) {
   return coordinateVariedBlockBounds(spec_, x, y, z);
  }
  return Block::getRenderBounds(nullptr, x, y, z);
 }
 void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override {
  setBoundingBox(getRenderBounds(blockView, x, y, z));
 }
 [[nodiscard]] bool isSideVisibleForBounds(
     const BlockView* blockView, int x, int y, int z, int side, const net::minecraft::Box& bounds) const override {
  if(spec_.coordinateBounds) {
   return true;
  }
  return Block::isSideVisibleForBounds(blockView, x, y, z, side, bounds);
 }
 [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const override {
  if(world == nullptr) {
   return Block::canPlaceAt(world, x, y, z);
  }
  // Landing on another copy of this block is decided entirely by stack_on_same:
  // it either supports the new block outright or forbids it outright.
  if(world->getBlockId(x, y - 1, z) == id) {
   return spec_.stackOnSame;
  }
  if(spec_.requiresSolidBelow && !world->getMaterial(x, y - 1, z).isSolid()) {
   return false;
  }
  return Block::canPlaceAt(world, x, y, z);
 }
 [[nodiscard]] std::optional<Box> getCollisionShape(World* /*world*/, int x, int y, int z) const override {
  if(spec_.coordinateBounds) {
   return Box{static_cast<double>(x),
              static_cast<double>(y),
              static_cast<double>(z),
              static_cast<double>(x + 1),
              static_cast<double>(y + 1),
              static_cast<double>(z + 1)};
  }
  if(spec_.collisionHeight <= 1.0f) {
   return Block::getCollisionShape(nullptr, x, y, z);
  }
  return Box{static_cast<double>(x),
             static_cast<double>(y),
             static_cast<double>(z),
             static_cast<double>(x + 1),
             static_cast<double>(y) + spec_.collisionHeight,
             static_cast<double>(z + 1)};
 }
 void setItemOverrideTexture(int textureId) {
  itemOverrideTextureId_ = textureId;
 }
 // With an explicit item texture the inventory/dropped sprite is that flat
 // texture, not a miniature of the block model.
 [[nodiscard]] bool hasItemOverride() const override {
  return itemOverrideTextureId_ >= 0;
 }
 void registerBlockItem() override {
  Block::registerBlockItem();
  if(itemOverrideTextureId_ >= 0) {
   Item* item = Item::ITEMS[static_cast<std::size_t>(id)];
   if(item != nullptr) {
    item->setTextureId(itemOverrideTextureId_);
   }
  }
 }
 private:
 BlockRegistrationSpec spec_;
 // Declared shape, overridden in the constructor when a baked model turns out
 // to fill the whole cube after all.
 bool fullCube_ = spec_.fullCube;
 int itemOverrideTextureId_ = -1;
};
void registerBlockClass(const BlockRegistrationSpec& spec) {
 const int textureId = registry::TextureRegistry::getOrRegisterTexture(spec.texturePath);
 Material* material = materialFromName(spec.material);
 std::string translationKey = spec.translationKey;
 if(translationKey.empty()) {
  translationKey = "block" + std::to_string(spec.blockId);
 }
 registerModBlockDisplayName(spec);
 Block* block = (new LuaModBlock(spec.blockId, textureId, *material, spec))
                    ->setHardness(spec.hardness)
                    ->setResistance(spec.resistance)
                    ->setSoundGroup(&kModBlockSound)
                    ->setTranslationKey(translationKey.c_str());
 if(spec.luminance > 0.0f) {
  block->setLuminance(std::clamp(spec.luminance, 0.0f, 1.0f));
 }
 if(!spec.opaque) {
  block->ignoreMetaUpdates();
 }
 // Block() ctor records BLOCKS_OPAQUE via base isOpaque() before LuaModBlock
 // overrides apply — refresh after the derived object exists.
 Block::BLOCKS_OPAQUE[static_cast<std::size_t>(spec.blockId)] = block->isOpaque();
 if(!block->isOpaque()) {
  Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(spec.blockId)] = 0;
 }
 if(spec.bakedModel != 0) {
#ifdef MINECRAFT_NATIVE_EXPORTS
  registerDraw(spec.blockId, model::drawLuaBlockWorld, model::drawLuaBlockInventory);
#endif
 }
 if(!spec.itemTexturePath.empty()) {
  const int itemTextureId = registry::TextureRegistry::getOrRegisterTexture(spec.itemTexturePath);
  static_cast<LuaModBlock*>(block)->setItemOverrideTexture(itemTextureId);
 }
}
struct BlockTraits {
 using Spec = BlockRegistrationSpec;
 static constexpr const char* kKind = "block";
 static constexpr mod::LifecyclePhase kPhase = mod::LifecyclePhase::Init;
 static void instantiate(const Spec& spec) {
  registerBlockClass(spec);
 }
};
using BlockRegistry = ModIdRegistry<BlockTraits>;
} // namespace
bool registerBlockSpec(const BlockRegistrationSpec& spec, std::string& error) {
 if(spec.blockId <= 0 || spec.blockId >= Block::BLOCK_COUNT) {
  error = "register_block id must be between 1 and " + std::to_string(Block::BLOCK_COUNT - 1);
  return false;
 }
 if(spec.texturePath.empty()) {
  error = "register_block requires texture (a mod resource path)";
  return false;
 }
 if(mod::ModLifecycle::currentPhase() == mod::LifecyclePhase::PostInit ||
    mod::ModLifecycle::currentPhase() == mod::LifecyclePhase::Ready) {
  error = "register_block must run while Lua mod scripts load at startup";
  return false;
 }
 if(BlockRegistry::instance().contains(spec.blockId)) {
  error = "register_block duplicate id: " + std::to_string(spec.blockId);
  return false;
 }
 if(!registry::Registry::tryReserveBlockId(spec.blockId)) {
  error = "register_block id is already reserved: " + std::to_string(spec.blockId);
  return false;
 }
 BlockRegistry::instance().add(spec.blockId, spec);
 return true;
}
int modBlockIdFromName(const char* name) {
 return BlockRegistry::instance().idFromName(name);
}
std::string modBlockWireName(int blockId) {
 return BlockRegistry::instance().wireName(blockId);
}
const BlockRegistrationSpec* blockRegistrationSpecForId(int blockId) noexcept {
 return BlockRegistry::instance().specForId(blockId);
}
} // namespace net::minecraft::mod::lua
