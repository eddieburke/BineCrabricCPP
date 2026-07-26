#include "net/minecraft/world/gen/GenerationApi.hpp"
#include <algorithm>
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::world::gen {
void subscribeChunkStage(ChunkStage stage, int priority, Listener listener) {
 net::minecraft::mod::runtime::registerChunkStageListener(stage, priority, std::move(listener));
}
void publishChunkStage(ChunkGenerationEvent& event) {
 net::minecraft::mod::runtime::fireChunkGeneration(event);
}
bool hasChunkStageListeners(ChunkStage stage) {
 return net::minecraft::mod::runtime::chunkStageListenerCount(stage) > 0;
}
bool hasAnyChunkGenerationListeners() {
 if(net::minecraft::mod::runtime::hasLuaHook(net::minecraft::mod::runtime::LuaEventId::ChunkGeneration)) {
  return true;
 }
 const auto count = static_cast<std::size_t>(ChunkStage::Count);
 for(std::size_t i = 0; i < count; ++i) {
  if(net::minecraft::mod::runtime::chunkStageListenerCount(static_cast<ChunkStage>(i)) > 0) {
   return true;
  }
 }
 return false;
}
std::uint64_t seedPopulationRandom(JavaRandom& random, std::uint64_t worldSeed, int chunkX, int chunkZ) {
 random.setSeed(worldSeed);
 const std::int64_t a = (static_cast<std::int64_t>(random.nextLong()) / 2LL) * 2LL + 1LL;
 const std::int64_t b = (static_cast<std::int64_t>(random.nextLong()) / 2LL) * 2LL + 1LL;
 const std::uint64_t seed =
     static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkX) * a + static_cast<std::int64_t>(chunkZ) * b) ^
     worldSeed;
 random.setSeed(seed);
 return seed;
}
} // namespace net::minecraft::world::gen
