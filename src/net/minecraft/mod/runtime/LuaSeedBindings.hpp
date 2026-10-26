#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
namespace net::minecraft::seedsearch {
struct PointRule {
 int x = 0;
 int z = 0;
 std::uint32_t mask = 0;
 bool allow = true;
};
struct CoverageRule {
 std::uint32_t mask = 0;
 double minPercent = 0.0;
 double maxPercent = 100.0;
};
struct TerrainRule {
 int x = 0;
 int z = 0;
 int land = -1;
 int minHeight = -1;
 int maxHeight = 255;
};
struct Criteria {
 std::uint64_t first = 0;
 std::uint64_t last = 0;
 bool randomSeeds = false;
 std::uint64_t randomCount = 0;
 std::uint64_t randomBase = 0;
 std::vector<PointRule> points;
 bool area = false;
 int areaCenterX = 0;
 int areaCenterZ = 0;
 int areaRadius = 128;
 int areaStep = 16;
 std::uint32_t require = 0;
 std::uint32_t forbid = 0;
 int minUnique = 0;
 std::vector<CoverageRule> coverage;
 std::uint32_t preferMask = 0;
 double preferWeight = 1.0;
 bool spawn = false;
 std::uint32_t spawnBiomes = 0;
 int spawnMinDistance = 0;
 int spawnMaxDistance = -1;
 std::vector<TerrainRule> terrain;
 int maxResults = 16;
 std::uint64_t stopAfter = 0;
 unsigned threads = 0;
};
struct Match {
 std::uint64_t seed = 0;
 double score = 0.0;
 int originBiome = 0;
 int unique = 0;
 bool hasSpawn = false;
 int spawnX = 0;
 int spawnZ = 0;
 int spawnBiome = 0;
};
struct SpawnPoint {
 bool found = false;
 int x = 0;
 int z = 0;
};
inline constexpr std::uint32_t kAllBiomes = (1U << kBiomeCount) - 1U;
inline constexpr unsigned kMaxSpawnSteps = 4096;
[[nodiscard]] int biomeIdAt(const BiomeSource& source, int x, int z);
[[nodiscard]] bool evaluateSeed(const Criteria& criteria, std::uint64_t seed, Match& out);
[[nodiscard]] SpawnPoint findSpawn(std::uint64_t seed, unsigned maxSteps = kMaxSpawnSteps);
[[nodiscard]] std::uint64_t seedAt(const Criteria& criteria, std::uint64_t index);
[[nodiscard]] std::uint64_t seedCount(const Criteria& criteria);
class SearchJob {
 public:
 explicit SearchJob(Criteria criteria);
 [[nodiscard]] static bool start(const std::shared_ptr<SearchJob>& job);
 void cancel();
 [[nodiscard]] bool running() const;
 [[nodiscard]] bool cancelled() const;
 [[nodiscard]] std::uint64_t checked() const;
 [[nodiscard]] std::uint64_t total() const;
 [[nodiscard]] std::uint64_t matches() const;
 [[nodiscard]] double elapsedSeconds() const;
 [[nodiscard]] std::vector<Match> results() const;
 void wait();

 private:
 void worker();
 void record(const Match& match);
 Criteria criteria_;
 std::uint64_t total_ = 0;
 std::atomic<std::uint64_t> cursor_{0};
 std::atomic<std::uint64_t> checked_{0};
 std::atomic<std::uint64_t> matches_{0};
 std::atomic<unsigned> active_{0};
 std::atomic<bool> cancel_{false};
 std::atomic<bool> stop_{false};
 std::chrono::steady_clock::time_point startTime_{};
 std::atomic<std::int64_t> endNanos_{-1};
 mutable std::mutex mutex_;
 std::vector<Match> results_;
};
} // namespace net::minecraft::seedsearch
