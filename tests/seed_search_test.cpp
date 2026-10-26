#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

#include "net/minecraft/mod/runtime/LuaSeedBindings.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"

namespace net::minecraft::test {
namespace {

using seedsearch::Criteria;
using seedsearch::Match;
using seedsearch::SearchJob;

std::uint32_t maskOf(BiomeId id) {
  return 1U << static_cast<unsigned>(id);
}

Criteria rangeCriteria(std::int64_t first, std::int64_t last) {
  Criteria c;
  c.first = static_cast<std::uint64_t>(first);
  c.last = static_cast<std::uint64_t>(last);
  c.maxResults = 1000;
  return c;
}

std::vector<Match> runJob(const Criteria& criteria, std::uint64_t* checked = nullptr, std::uint64_t* matches = nullptr) {
  Biome::init();
  auto job = std::make_shared<SearchJob>(criteria);
  EXPECT_TRUE(SearchJob::start(job));
  job->wait();
  if(checked != nullptr) {
    *checked = job->checked();
  }
  if(matches != nullptr) {
    *matches = job->matches();
  }
  return job->results();
}

}  // namespace

TEST(SeedSearch, PointBiomeMatchesBiomeSourceLookup) {
  Biome::init();
  const int points[][2] = {{0, 0}, {137, -422}, {-3000, 2500}, {16, 16}, {-1, -1}, {50000, -48000}};
  for(const std::int64_t seed : {0LL, 1LL, 12345LL, -987654321LL, 8675309123456789LL}) {
    BiomeSource reference(static_cast<std::uint64_t>(seed));
    const BiomeSource fast(static_cast<std::uint64_t>(seed));
    for(const auto& p : points) {
      EXPECT_EQ(seedsearch::biomeIdAt(fast, p[0], p[1]), static_cast<int>(reference.getBiome(p[0], p[1]).id))
          << "seed " << seed << " at " << p[0] << "," << p[1];
    }
  }
}

TEST(SeedSearch, WorldSpawnStepsKeepJavaLeftToRightOrder) {
  JavaRandom a(4242);
  JavaRandom b(4242);
  for(int i = 0; i < 64; ++i) {
    const int first = b.nextInt(64);
    const int second = b.nextInt(64);
    EXPECT_EQ(a.nextInt(64) - a.nextInt(64), first - second);
  }
}

TEST(SeedSearch, SpawnReplaysTheVanillaSandWalk) {
  Biome::init();
  for(const std::uint64_t seed : {12345ULL, 1ULL, 777ULL}) {
    const seedsearch::SpawnPoint spawn = seedsearch::findSpawn(seed);
    ASSERT_TRUE(spawn.found) << "seed " << seed;

    OverworldChunkGenerator generator(nullptr, seed);
    generator.useLocalBiomeSource(true);
    generator.enableModGeneration(false);
    std::unordered_map<std::int64_t, Chunk> chunks;
    const auto blockAt = [&](int x, int y, int z) {
      const int cx = x >> 4;
      const int cz = z >> 4;
      const std::int64_t key = (static_cast<std::int64_t>(cx) << 32) ^ static_cast<std::int64_t>(static_cast<std::uint32_t>(cz));
      auto found = chunks.find(key);
      if(found == chunks.end()) {
        found = chunks.emplace(key, generator.loadChunk(&generator, cx, cz)).first;
      }
      return y >= Chunk::height ? 0 : found->second.getBlockId(x & 15, y, z & 15);
    };
    const auto spawnBlock = [&](int x, int z) {
      int y = 63;
      while(blockAt(x, y + 1, z) != 0) {
        ++y;
      }
      return blockAt(x, y, z);
    };

    JavaRandom random(seed);
    int x = 0;
    int z = 0;
    int steps = 0;
    while(spawnBlock(x, z) != 12) {
      const int a = random.nextInt(64);
      const int b = random.nextInt(64);
      x += a - b;
      const int c = random.nextInt(64);
      const int d = random.nextInt(64);
      z += c - d;
      ASSERT_LT(++steps, 4096);
    }
    EXPECT_EQ(spawn.x, x) << "seed " << seed;
    EXPECT_EQ(spawn.z, z) << "seed " << seed;
  }
}

TEST(SeedSearch, OriginBiomeSearchMatchesBruteForce) {
  Criteria c = rangeCriteria(0, 3999);
  c.points.push_back({0, 0, maskOf(BiomeId::Desert), true});
  std::uint64_t checked = 0;
  std::uint64_t matches = 0;
  const std::vector<Match> results = runJob(c, &checked, &matches);

  std::uint64_t expected = 0;
  for(std::uint64_t seed = 0; seed < 4000; ++seed) {
    BiomeSource source(seed);
    if(source.getBiome(0, 0).id == BiomeId::Desert) {
      ++expected;
    }
  }
  EXPECT_EQ(checked, 4000U);
  EXPECT_EQ(matches, expected);
  EXPECT_GT(expected, 0U);
  EXPECT_EQ(results.size(), expected);
  for(const Match& m : results) {
    BiomeSource source(m.seed);
    EXPECT_EQ(source.getBiome(0, 0).id, BiomeId::Desert);
    EXPECT_EQ(m.originBiome, static_cast<int>(BiomeId::Desert));
  }
}

TEST(SeedSearch, NegativeSeedRangesAreScanned) {
  Criteria c = rangeCriteria(-500, 499);
  c.points.push_back({0, 0, seedsearch::kAllBiomes, true});
  std::uint64_t checked = 0;
  const std::vector<Match> results = runJob(c, &checked);
  EXPECT_EQ(checked, 1000U);
  ASSERT_FALSE(results.empty());
  EXPECT_LT(static_cast<std::int64_t>(std::min_element(results.begin(), results.end(), [](const Match& a, const Match& b) {
                                         return static_cast<std::int64_t>(a.seed) < static_cast<std::int64_t>(b.seed);
                                       })->seed),
            0);
}

TEST(SeedSearch, AreaRulesMatchBruteForceGrid) {
  Criteria c = rangeCriteria(0, 799);
  c.area = true;
  c.areaRadius = 96;
  c.areaStep = 16;
  c.require = maskOf(BiomeId::Forest);
  c.forbid = maskOf(BiomeId::Desert);
  c.minUnique = 2;
  std::uint64_t matches = 0;
  const std::vector<Match> results = runJob(c, nullptr, &matches);

  std::uint64_t expected = 0;
  for(std::uint64_t seed = 0; seed < 800; ++seed) {
    BiomeSource source(seed);
    std::uint32_t present = 0;
    for(int i = 0; i < 13; ++i) {
      for(int j = 0; j < 13; ++j) {
        present |= maskOf(source.getBiome(-96 + i * 16, -96 + j * 16).id);
      }
    }
    int unique = 0;
    for(unsigned bit = 0; bit < 13; ++bit) {
      unique += (present >> bit) & 1U;
    }
    if((present & maskOf(BiomeId::Forest)) != 0U && (present & maskOf(BiomeId::Desert)) == 0U && unique >= 2) {
      ++expected;
    }
  }
  EXPECT_EQ(matches, expected);
  EXPECT_GT(expected, 0U);
  for(const Match& m : results) {
    EXPECT_GE(m.unique, 2);
  }
}

TEST(SeedSearch, CoverageBoundsMatchBruteForceGrid) {
  Criteria c = rangeCriteria(0, 599);
  c.area = true;
  c.areaRadius = 64;
  c.areaStep = 8;
  c.coverage.push_back({maskOf(BiomeId::Plains) | maskOf(BiomeId::Forest), 40.0, 90.0});
  std::uint64_t matches = 0;
  (void)runJob(c, nullptr, &matches);

  std::uint64_t expected = 0;
  constexpr int side = 64 * 2 / 8 + 1;
  for(std::uint64_t seed = 0; seed < 600; ++seed) {
    BiomeSource source(seed);
    int hit = 0;
    for(int i = 0; i < side; ++i) {
      for(int j = 0; j < side; ++j) {
        const BiomeId id = source.getBiome(-64 + i * 8, -64 + j * 8).id;
        hit += (id == BiomeId::Plains || id == BiomeId::Forest) ? 1 : 0;
      }
    }
    const double percent = 100.0 * hit / (side * side);
    if(percent >= 40.0 - 1e-9 && percent <= 90.0 + 1e-9) {
      ++expected;
    }
  }
  EXPECT_EQ(matches, expected);
}

TEST(SeedSearch, ThreadCountDoesNotChangeResults) {
  Criteria c = rangeCriteria(0, 2999);
  c.area = true;
  c.areaRadius = 64;
  c.areaStep = 16;
  c.minUnique = 3;
  c.preferMask = maskOf(BiomeId::Taiga);
  c.maxResults = 25;
  Criteria single = c;
  single.threads = 1;
  const std::vector<Match> a = runJob(single);
  const std::vector<Match> b = runJob(c);
  ASSERT_EQ(a.size(), b.size());
  for(std::size_t i = 0; i < a.size(); ++i) {
    EXPECT_EQ(a[i].seed, b[i].seed);
    EXPECT_DOUBLE_EQ(a[i].score, b[i].score);
  }
  for(std::size_t i = 1; i < a.size(); ++i) {
    EXPECT_GE(a[i - 1].score, a[i].score);
  }
}

TEST(SeedSearch, StopAfterEndsEarlyAndCancelStopsAHugeRange) {
  Criteria c = rangeCriteria(0, 1'000'000'000);
  c.points.push_back({0, 0, maskOf(BiomeId::Rainforest), true});
  c.stopAfter = 3;
  std::uint64_t checked = 0;
  std::uint64_t matches = 0;
  (void)runJob(c, &checked, &matches);
  EXPECT_GE(matches, 3U);
  EXPECT_LT(checked, 1'000'000U);

  Criteria huge = rangeCriteria(0, 4'000'000'000'000LL);
  huge.points.push_back({0, 0, seedsearch::kAllBiomes, true});
  Biome::init();
  auto job = std::make_shared<SearchJob>(huge);
  ASSERT_TRUE(SearchJob::start(job));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  job->cancel();
  job->wait();
  EXPECT_TRUE(job->cancelled());
  EXPECT_GT(job->checked(), 0U);
  EXPECT_LT(job->checked(), job->total());
}

TEST(SeedSearch, RandomModeSamplesTheRequestedCount) {
  Criteria c;
  c.randomSeeds = true;
  c.randomCount = 2000;
  c.randomBase = 99;
  c.points.push_back({0, 0, seedsearch::kAllBiomes, true});
  c.maxResults = 5;
  std::uint64_t checked = 0;
  const std::vector<Match> results = runJob(c, &checked);
  EXPECT_EQ(checked, 2000U);
  EXPECT_EQ(results.size(), 5U);
}

TEST(SeedSearch, SpawnAndTerrainRulesAreConsistent) {
  Criteria c = rangeCriteria(0, 11);
  c.spawn = true;
  c.spawnMaxDistance = 4000;
  c.terrain.push_back({0, 0, -1, -1, 255});
  std::uint64_t checked = 0;
  const std::vector<Match> results = runJob(c, &checked);
  EXPECT_EQ(checked, 12U);
  ASSERT_FALSE(results.empty());
  Biome::init();
  for(const Match& m : results) {
    ASSERT_TRUE(m.hasSpawn);
    const seedsearch::SpawnPoint spawn = seedsearch::findSpawn(m.seed);
    ASSERT_TRUE(spawn.found);
    EXPECT_EQ(spawn.x, m.spawnX);
    EXPECT_EQ(spawn.z, m.spawnZ);
    const BiomeSource source(m.seed);
    EXPECT_EQ(seedsearch::biomeIdAt(source, m.spawnX, m.spawnZ), m.spawnBiome);
  }
}

TEST(SeedSearch, ReportsSeedsPerSecond) {
  Criteria cheap = rangeCriteria(0, 199999);
  cheap.points.push_back({0, 0, maskOf(BiomeId::Desert), true});
  const auto start = std::chrono::steady_clock::now();
  std::uint64_t checked = 0;
  (void)runJob(cheap, &checked);
  const double cheapSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

  Criteria area = rangeCriteria(0, 19999);
  area.area = true;
  area.areaRadius = 128;
  area.areaStep = 16;
  area.require = maskOf(BiomeId::Desert) | maskOf(BiomeId::Taiga);
  const auto areaStart = std::chrono::steady_clock::now();
  std::uint64_t areaChecked = 0;
  (void)runJob(area, &areaChecked);
  const double areaSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - areaStart).count();

  std::printf("[seedsearch] origin-biome rule: %.0f seeds/s, area rule (17x17 grid): %.0f seeds/s\n",
              static_cast<double>(checked) / cheapSeconds,
              static_cast<double>(areaChecked) / areaSeconds);
  EXPECT_GT(static_cast<double>(checked) / cheapSeconds, 5000.0);
}

}  // namespace net::minecraft::test
