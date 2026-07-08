#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include "net/minecraft/mod/lua/LuaBlockModel.hpp"
#include "net/minecraft/mod/lua/ModIdRegistry.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include <string>
namespace net::minecraft::mod::lua {
namespace {
using net::minecraft::block::Block;
struct BlockTraits {
  using Spec = BlockRegistrationSpec;
  static constexpr const char* kKind = "block";
  static constexpr mod::LifecyclePhase kPhase = mod::LifecyclePhase::BlockRegistration;
  static void instantiate(const Spec& spec) {
    instantiateLuaModBlock(spec);
  }
};
using BlockRegistry = ModIdRegistry<BlockTraits>;
} // namespace
bool registerBlockSpec(const BlockRegistrationSpec& spec, std::string& error) {
  if(spec.blockId <= 0 || spec.blockId >= Block::BLOCK_COUNT) {
    error = "register_block id must be between 1 and " + std::to_string(Block::BLOCK_COUNT - 1);
    return false;
  }
  if(spec.texturePath.empty() && spec.terrainTextureId < 0) {
    error = "register_block requires texture or texture_id";
    return false;
  }
  if(spec.terrainTextureId > 255) {
    error = "register_block texture_id must be a vanilla terrain-atlas index from 0 to 255";
    return false;
  }
  if(registry::Registry::isBootstrapped()) {
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
