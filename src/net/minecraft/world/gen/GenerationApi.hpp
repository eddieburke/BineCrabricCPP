#pragma once
#include <cstdint>
#include <functional>
#include <utility>
namespace net::minecraft {
class BiomeSource;
class Chunk;
class ChunkSource;
class JavaRandom;
class World;
} // namespace net::minecraft
namespace net::minecraft::world::gen {
enum class ChunkStage {
  Terrain = 0,
  Surface,
  Carver,
  Features,
};
enum class HookMoment {
  Before = 0,
  After,
};
struct ChunkGenerationContext {
  net::minecraft::World* world = nullptr;
  net::minecraft::ChunkSource* source = nullptr;
  net::minecraft::Chunk* chunk = nullptr;
  net::minecraft::BiomeSource* biomeSource = nullptr;
  net::minecraft::JavaRandom* random = nullptr;
  std::uint64_t worldSeed = 0;
  int chunkX = 0;
  int chunkZ = 0;
  bool modGeneration = false;
  bool overworld = false;
};
struct ChunkGenerationEvent {
  ChunkStage stage;
  HookMoment moment;
  ChunkGenerationContext& context;
  bool cancelVanilla = false;
  bool vanillaStageRan = false;
  ChunkGenerationEvent(ChunkStage stageIn, HookMoment momentIn, ChunkGenerationContext& contextIn)
      : stage(stageIn), moment(momentIn), context(contextIn) {
  }
};
using Listener = std::function<void(ChunkGenerationEvent&)>;
void subscribeChunkStage(ChunkStage stage, int priority, Listener listener);
void publishChunkStage(ChunkGenerationEvent& event);
[[nodiscard]] bool hasChunkStageListeners(ChunkStage stage);
[[nodiscard]] bool hasAnyChunkGenerationListeners();
[[nodiscard]] std::uint64_t seedPopulationRandom(JavaRandom& random, std::uint64_t worldSeed, int chunkX, int chunkZ);
template <typename Fn>
bool runVanillaStage(ChunkStage stage, ChunkGenerationContext& context, Fn&& vanillaStage) {
  if(!hasAnyChunkGenerationListeners()) {
    std::forward<Fn>(vanillaStage)();
    return true;
  }
  ChunkGenerationEvent before(stage, HookMoment::Before, context);
  publishChunkStage(before);
  const bool vanillaRan = !before.cancelVanilla;
  if(vanillaRan) {
    std::forward<Fn>(vanillaStage)();
  }
  ChunkGenerationEvent after(stage, HookMoment::After, context);
  after.vanillaStageRan = vanillaRan;
  publishChunkStage(after);
  return vanillaRan;
}
} // namespace net::minecraft::world::gen
