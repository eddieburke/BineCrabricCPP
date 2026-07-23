#include "net/minecraft/world/gen/GenerationApi.hpp"
#include <algorithm>
#include <array>
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::world::gen {
namespace {
std::array<int, 4>& stageListenerCounts() {
 static std::array<int, 4> counts{};
 return counts;
}
std::size_t stageIndex(ChunkStage stage) {
 return static_cast<std::size_t>(stage);
}
} // namespace
void subscribeChunkStage(ChunkStage stage, int priority, Listener listener) {
 ++stageListenerCounts()[stageIndex(stage)];
 net::minecraft::mod::runtime::registerChunkStageListener(stage, priority, std::move(listener));
}
void publishChunkStage(ChunkGenerationEvent& event) {
 net::minecraft::mod::runtime::fireChunkGeneration(event);
}
bool hasChunkStageListeners(ChunkStage stage) {
 return stageListenerCounts()[stageIndex(stage)] > 0;
}
bool hasAnyChunkGenerationListeners() {
 if(net::minecraft::mod::runtime::hasLuaHook(net::minecraft::mod::runtime::LuaEventId::ChunkGeneration)) {
  return true;
 }
 return std::any_of(stageListenerCounts().begin(), stageListenerCounts().end(), [](int count) { return count > 0; });
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
