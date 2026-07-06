#include "net/minecraft/world/gen/Generator.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
namespace net::minecraft {
void Generator::place(ChunkSource* /*source*/, World* world, std::uint64_t worldSeed, int chunkX, int chunkZ,
                      Chunk& chunk) {
  const int n = range;
  random.setSeed(worldSeed);
  const std::int64_t l = random.nextLong() / 2LL * 2LL + 1LL;
  const std::int64_t l2 = random.nextLong() / 2LL * 2LL + 1LL;
  for(int i = chunkX - n; i <= chunkX + n; ++i) {
    for(int j = chunkZ - n; j <= chunkZ + n; ++j) {
      const std::uint64_t seed =
          (static_cast<std::uint64_t>(static_cast<std::int64_t>(i)) * static_cast<std::uint64_t>(l) +
           static_cast<std::uint64_t>(static_cast<std::int64_t>(j)) * static_cast<std::uint64_t>(l2)) ^
          worldSeed;
      random.setSeed(seed);
      place(world, i, j, chunkX, chunkZ, chunk);
    }
  }
}
} // namespace net::minecraft
