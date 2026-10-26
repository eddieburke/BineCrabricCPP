#include "net/minecraft/mod/runtime/LuaSeedBindings.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <map>
#include <memory>
#include <random>
#include <thread>
#include <unordered_map>
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/runtime/LuaBindings.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/BiomeNames.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/gui/screen/Screen.hpp"
#endif
namespace net::minecraft::seedsearch {
namespace {
constexpr int kSandId = 12;
constexpr int kSeaLevelGround = 63;
constexpr std::uint64_t kBatch = 64;
constexpr int kCoarseStride = 4;
struct TerrainView {
 explicit TerrainView(std::uint64_t seed) : generator(nullptr, seed) {
  generator.useLocalBiomeSource(true);
  generator.enableModGeneration(false);
 }
 const std::vector<std::uint8_t>& chunk(int chunkX, int chunkZ) {
  const std::uint64_t key =
      (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX)) << 32U) | static_cast<std::uint32_t>(chunkZ);
  auto found = chunks.find(key);
  if(found == chunks.end()) {
   Chunk generated = generator.loadChunk(&generator, chunkX, chunkZ);
   found = chunks.emplace(key, std::move(generated.blocks)).first;
  }
  return found->second;
 }
 int block(int x, int y, int z) {
  if(y < 0 || y >= Chunk::height) {
   return 0;
  }
  const std::vector<std::uint8_t>& blocks = chunk(x >> 4, z >> 4);
  return blocks[static_cast<std::size_t>(((x & 15) << 11) | ((z & 15) << 7) | y)];
 }
 int groundTop(int x, int z) {
  const std::vector<std::uint8_t>& blocks = chunk(x >> 4, z >> 4);
  const std::size_t column = static_cast<std::size_t>(((x & 15) << 11) | ((z & 15) << 7));
  for(int y = Chunk::height - 1; y >= 0; --y) {
   const int id = blocks[column + static_cast<std::size_t>(y)];
   if(id != 0 && id != 8 && id != 9 && id != 79) {
    return y;
   }
  }
  return -1;
 }
 int spawnBlock(int x, int z) {
  int y = kSeaLevelGround;
  while(block(x, y + 1, z) != 0) {
   ++y;
  }
  return block(x, y, z);
 }
 OverworldChunkGenerator generator;
 std::unordered_map<std::uint64_t, std::vector<std::uint8_t>> chunks;
};
SpawnPoint walkSpawn(TerrainView& terrain, std::uint64_t seed, unsigned maxSteps) {
 JavaRandom random(seed);
 int x = 0;
 int z = 0;
 for(unsigned step = 0; step < maxSteps; ++step) {
  if(terrain.spawnBlock(x, z) == kSandId) {
   return {true, x, z};
  }
  const int dxA = random.nextInt(64);
  const int dxB = random.nextInt(64);
  x += dxA - dxB;
  const int dzA = random.nextInt(64);
  const int dzB = random.nextInt(64);
  z += dzA - dzB;
 }
 return {};
}
std::uint64_t splitMix(std::uint64_t value) {
 value += 0x9E3779B97F4A7C15ULL;
 value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
 value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
 return value ^ (value >> 31U);
}
int maskedCount(const std::array<int, kBiomeCount>& counts, std::uint32_t mask) {
 int sum = 0;
 for(int id = 0; id < kBiomeCount; ++id) {
  if(((mask >> id) & 1U) != 0U) {
   sum += counts[static_cast<std::size_t>(id)];
  }
 }
 return sum;
}
bool scanArea(const Criteria& c, const BiomeSource& source, int& unique, double& preferPercent) {
 const int step = std::max(1, c.areaStep);
 const int side = std::max(1, c.areaRadius * 2 / step + 1);
 const int total = side * side;
 const int startX = c.areaCenterX - c.areaRadius;
 const int startZ = c.areaCenterZ - c.areaRadius;
 std::array<int, kBiomeCount> counts{};
 int visited = 0;
 for(int phase = 0; phase < 2; ++phase) {
  for(int i = 0; i < side; ++i) {
   const bool coarseRow = i % kCoarseStride == 0;
   for(int j = 0; j < side; ++j) {
    const bool coarse = coarseRow && j % kCoarseStride == 0;
    if(coarse != (phase == 0)) {
     continue;
    }
    const int id = biomeIdAt(source, startX + i * step, startZ + j * step);
    if(((c.forbid >> id) & 1U) != 0U) {
     return false;
    }
    ++counts[static_cast<std::size_t>(id)];
    ++visited;
   }
  }
  const int remaining = total - visited;
  for(const CoverageRule& rule : c.coverage) {
   const int matched = maskedCount(counts, rule.mask);
   const int maxCount = static_cast<int>(std::floor(rule.maxPercent * total / 100.0 + 1e-9));
   const int minCount = static_cast<int>(std::ceil(rule.minPercent * total / 100.0 - 1e-9));
   if(matched > maxCount || matched + remaining < minCount) {
    return false;
   }
  }
 }
 std::uint32_t present = 0;
 for(int id = 0; id < kBiomeCount; ++id) {
  if(counts[static_cast<std::size_t>(id)] > 0) {
   present |= 1U << id;
  }
 }
 if((c.require & ~present) != 0U) {
  return false;
 }
 unique = std::popcount(present);
 if(unique < c.minUnique) {
  return false;
 }
 preferPercent = c.preferMask != 0U ? 100.0 * maskedCount(counts, c.preferMask) / total : 0.0;
 return true;
}
bool betterMatch(const Match& a, const Match& b) {
 if(a.score != b.score) {
  return a.score > b.score;
 }
 return static_cast<std::int64_t>(a.seed) < static_cast<std::int64_t>(b.seed);
}
} // namespace
int biomeIdAt(const BiomeSource& source, int x, int z) {
 const BiomeSource::ClimateSample climate = source.sampleClimate(x, z);
 return static_cast<int>(Biome::getBiome(climate.temperature, climate.downfall).id);
}
SpawnPoint findSpawn(std::uint64_t seed, unsigned maxSteps) {
 TerrainView terrain(seed);
 return walkSpawn(terrain, seed, maxSteps);
}
std::uint64_t seedCount(const Criteria& criteria) {
 if(criteria.randomSeeds) {
  return criteria.randomCount;
 }
 const std::uint64_t span = criteria.last - criteria.first;
 return span == UINT64_MAX ? UINT64_MAX : span + 1U;
}
std::uint64_t seedAt(const Criteria& criteria, std::uint64_t index) {
 if(criteria.randomSeeds) {
  return splitMix(criteria.randomBase + index);
 }
 return criteria.first + index;
}
bool evaluateSeed(const Criteria& c, std::uint64_t seed, Match& out) {
 out = Match{};
 out.seed = seed;
 const BiomeSource source(seed);
 for(const PointRule& rule : c.points) {
  const int id = biomeIdAt(source, rule.x, rule.z);
  if((((rule.mask >> id) & 1U) != 0U) != rule.allow) {
   return false;
  }
 }
 double preferPercent = 0.0;
 if(c.area && !scanArea(c, source, out.unique, preferPercent)) {
  return false;
 }
 std::unique_ptr<TerrainView> terrain;
 for(const TerrainRule& rule : c.terrain) {
  if(terrain == nullptr) {
   terrain = std::make_unique<TerrainView>(seed);
  }
  const int ground = terrain->groundTop(rule.x, rule.z);
  const bool land = ground >= kSeaLevelGround;
  if((rule.land == 1 && !land) || (rule.land == 0 && land) || ground < rule.minHeight || ground > rule.maxHeight) {
   return false;
  }
 }
 if(c.spawn) {
  if(terrain == nullptr) {
   terrain = std::make_unique<TerrainView>(seed);
  }
  const SpawnPoint spawn = walkSpawn(*terrain, seed, kMaxSpawnSteps);
  if(!spawn.found) {
   return false;
  }
  const double distance = std::hypot(static_cast<double>(spawn.x), static_cast<double>(spawn.z));
  if(distance < c.spawnMinDistance || (c.spawnMaxDistance >= 0 && distance > c.spawnMaxDistance)) {
   return false;
  }
  const int biome = biomeIdAt(source, spawn.x, spawn.z);
  if(c.spawnBiomes != 0U && ((c.spawnBiomes >> biome) & 1U) == 0U) {
   return false;
  }
  out.hasSpawn = true;
  out.spawnX = spawn.x;
  out.spawnZ = spawn.z;
  out.spawnBiome = biome;
 }
 out.originBiome = biomeIdAt(source, 0, 0);
 out.score = static_cast<double>(out.unique) + c.preferWeight * preferPercent / 10.0;
 return true;
}
SearchJob::SearchJob(Criteria criteria) : criteria_(std::move(criteria)), total_(seedCount(criteria_)) {
}
bool SearchJob::start(const std::shared_ptr<SearchJob>& job) {
 util::concurrent::WorkerPool& pool =
     util::concurrent::ThreadCoordinator::instance().pool(util::concurrent::Domain::Compute);
 unsigned tasks = pool.threadCount();
 if(job->criteria_.threads > 0) {
  tasks = std::min(tasks, job->criteria_.threads);
 }
 tasks = std::max(1U, tasks);
 job->startTime_ = std::chrono::steady_clock::now();
 job->active_.store(tasks);
 unsigned submitted = 0;
 for(unsigned i = 0; i < tasks; ++i) {
  if(pool.submit([job] { job->worker(); }, static_cast<int>(util::concurrent::TaskPriority::Low))) {
   ++submitted;
  }
 }
 if(submitted == tasks) {
  return true;
 }
 job->cancel_.store(true);
 const unsigned missed = tasks - submitted;
 if(job->active_.fetch_sub(missed) == missed) {
  job->endNanos_.store(0);
 }
 return submitted > 0;
}
void SearchJob::worker() {
 std::uint64_t pending = 0;
 Match match;
 while(!cancel_.load(std::memory_order_relaxed) && !stop_.load(std::memory_order_relaxed)) {
  const std::uint64_t base = cursor_.fetch_add(kBatch, std::memory_order_relaxed);
  if(base >= total_) {
   break;
  }
  const std::uint64_t end = std::min(total_, base + kBatch);
  for(std::uint64_t index = base; index < end; ++index) {
   if(cancel_.load(std::memory_order_relaxed) || stop_.load(std::memory_order_relaxed)) {
    break;
   }
   if(evaluateSeed(criteria_, seedAt(criteria_, index), match)) {
    record(match);
   }
   ++pending;
  }
  checked_.fetch_add(pending, std::memory_order_relaxed);
  pending = 0;
 }
 checked_.fetch_add(pending, std::memory_order_relaxed);
 if(active_.fetch_sub(1) == 1) {
  endNanos_.store(std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::steady_clock::now() - startTime_)
                      .count());
 }
}
void SearchJob::record(const Match& match) {
 const std::lock_guard lock(mutex_);
 const std::uint64_t count = matches_.fetch_add(1) + 1U;
 const std::size_t limit = static_cast<std::size_t>(std::max(1, criteria_.maxResults));
 results_.insert(std::upper_bound(results_.begin(), results_.end(), match, betterMatch), match);
 if(results_.size() > limit) {
  results_.pop_back();
 }
 if(criteria_.stopAfter > 0 && count >= criteria_.stopAfter) {
  stop_.store(true);
 }
}
void SearchJob::cancel() {
 cancel_.store(true);
}
bool SearchJob::running() const {
 return active_.load() > 0;
}
bool SearchJob::cancelled() const {
 return cancel_.load();
}
std::uint64_t SearchJob::checked() const {
 return checked_.load();
}
std::uint64_t SearchJob::total() const {
 return total_;
}
std::uint64_t SearchJob::matches() const {
 return matches_.load();
}
double SearchJob::elapsedSeconds() const {
 const std::int64_t ended = endNanos_.load();
 if(ended >= 0) {
  return static_cast<double>(ended) / 1e9;
 }
 return std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime_).count();
}
std::vector<Match> SearchJob::results() const {
 const std::lock_guard lock(mutex_);
 return results_;
}
void SearchJob::wait() {
 while(running()) {
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
 }
}
} // namespace net::minecraft::seedsearch
namespace net::minecraft::mod::runtime {
using namespace net::minecraft::mod::lua;
namespace {
using seedsearch::Criteria;
using seedsearch::SearchJob;
std::mutex gJobsMutex;
std::map<int, std::shared_ptr<SearchJob>> gJobs;
int gNextJobId = 1;
int fail(lua_State* state, const std::string& message) {
 LuaApi& api = luaApi();
 api.pushnil(state);
 api.pushstring(state, message.c_str());
 return 2;
}
bool readInt64(lua_State* state, int index, std::int64_t& value) {
 int isNumber = 0;
 const long long number = luaApi().tointegerx(state, index, &isNumber);
 if(isNumber == 0) {
  return false;
 }
 value = number;
 return true;
}
bool fieldInt64(lua_State* state, int table, const char* key, std::int64_t& value) {
 luaApi().getfield(state, table, key);
 const bool ok = readInt64(state, -1, value);
 luaApi().settop(state, -2);
 return ok;
}
bool fieldTable(lua_State* state, int table, const char* key) {
 luaApi().getfield(state, table, key);
 if(luaApi().type(state, -1) == kLuaTTable) {
  return true;
 }
 luaApi().settop(state, -2);
 return false;
}
bool fieldPresent(lua_State* state, int table, const char* key) {
 luaApi().getfield(state, table, key);
 const bool present = luaApi().type(state, -1) != kLuaTNil;
 luaApi().settop(state, -2);
 return present;
}
bool biomeMask(lua_State* state, int table, const char* key, std::uint32_t& mask, std::string& error) {
 mask = 0;
 if(!fieldTable(state, table, key)) {
  return true;
 }
 LuaApi& api = luaApi();
 const int list = api.gettop(state);
 const std::size_t count = api.rawlen(state, list);
 const std::vector<std::string> names = allBiomeWireNames();
 bool ok = true;
 for(std::size_t i = 1; i <= count && ok; ++i) {
  api.rawgeti(state, list, static_cast<long long>(i));
  const std::string name = luaString(state, -1, "");
  api.settop(state, -2);
  const auto found = std::find(names.begin(), names.end(), name);
  if(found == names.end()) {
   error = "unknown biome '" + name + "' in " + key;
   ok = false;
  } else {
   mask |= 1U << static_cast<unsigned>(found - names.begin());
  }
 }
 api.settop(state, list - 1);
 return ok;
}
bool parseCriteria(lua_State* state, int table, Criteria& c, std::string& error) {
 LuaApi& api = luaApi();
 std::int64_t value = 0;
 const std::string mode = luaStringField(state, table, "mode", "range");
 if(mode == "random") {
  c.randomSeeds = true;
  c.randomCount = fieldInt64(state, table, "samples", value) && value > 0 ? static_cast<std::uint64_t>(value) : 1000000U;
  std::random_device device;
  c.randomBase = (static_cast<std::uint64_t>(device()) << 32U) | device();
 } else {
  std::int64_t first = 0;
  std::int64_t last = 1000000;
  (void)fieldInt64(state, table, "seed_start", first);
  (void)fieldInt64(state, table, "seed_end", last);
  if(first > last) {
   std::swap(first, last);
  }
  c.first = static_cast<std::uint64_t>(first);
  c.last = static_cast<std::uint64_t>(last);
 }
 if(fieldInt64(state, table, "max_results", value)) {
  c.maxResults = static_cast<int>(std::clamp<std::int64_t>(value, 1, 1000));
 }
 if(fieldInt64(state, table, "stop_after", value) && value > 0) {
  c.stopAfter = static_cast<std::uint64_t>(value);
 }
 if(fieldInt64(state, table, "threads", value) && value > 0) {
  c.threads = static_cast<unsigned>(std::min<std::int64_t>(value, 64));
 }
 std::uint32_t origin = 0;
 if(!biomeMask(state, table, "origin_biomes", origin, error)) {
  return false;
 }
 if(origin != 0U) {
  c.points.push_back({0, 0, origin, true});
 }
 if(fieldTable(state, table, "points")) {
  const int list = api.gettop(state);
  const std::size_t count = std::min<std::size_t>(api.rawlen(state, list), 64);
  for(std::size_t i = 1; i <= count; ++i) {
   api.rawgeti(state, list, static_cast<long long>(i));
   const int item = api.gettop(state);
   seedsearch::PointRule rule;
   rule.x = luaIntField(state, item, "x", 0);
   rule.z = luaIntField(state, item, "z", 0);
   rule.allow = !luaBoolField(state, item, "exclude", false);
   const bool ok = biomeMask(state, item, "biomes", rule.mask, error);
   api.settop(state, item - 1);
   if(!ok) {
    api.settop(state, list - 1);
    return false;
   }
   if(rule.mask != 0U) {
    c.points.push_back(rule);
   }
  }
  api.settop(state, list - 1);
 }
 if(fieldTable(state, table, "area")) {
  const int area = api.gettop(state);
  c.area = true;
  c.areaRadius = std::clamp(luaIntField(state, area, "radius", 128), 16, 4096);
  c.areaStep = std::clamp(luaIntField(state, area, "step", 16), 4, 256);
  c.areaCenterX = luaIntField(state, area, "center_x", 0);
  c.areaCenterZ = luaIntField(state, area, "center_z", 0);
  c.minUnique = luaIntField(state, area, "min_unique", 0);
  c.preferWeight = luaDoubleField(state, area, "prefer_weight", 1.0);
  bool ok = biomeMask(state, area, "require", c.require, error) && biomeMask(state, area, "forbid", c.forbid, error) &&
            biomeMask(state, area, "prefer", c.preferMask, error);
  if(ok && fieldTable(state, area, "coverage")) {
   const int list = api.gettop(state);
   const std::size_t count = std::min<std::size_t>(api.rawlen(state, list), 32);
   for(std::size_t i = 1; i <= count && ok; ++i) {
    api.rawgeti(state, list, static_cast<long long>(i));
    const int item = api.gettop(state);
    seedsearch::CoverageRule rule;
    rule.minPercent = luaDoubleField(state, item, "min", 0.0);
    rule.maxPercent = luaDoubleField(state, item, "max", 100.0);
    ok = biomeMask(state, item, "biomes", rule.mask, error);
    api.settop(state, item - 1);
    if(ok && rule.mask != 0U) {
     c.coverage.push_back(rule);
    }
   }
   api.settop(state, list - 1);
  }
  api.settop(state, area - 1);
  if(!ok) {
   return false;
  }
  const int side = c.areaRadius * 2 / c.areaStep + 1;
  if(side * side > 65536) {
   error = "area too large: raise step or lower radius";
   return false;
  }
 }
 if(fieldTable(state, table, "spawn")) {
  const int spawn = api.gettop(state);
  c.spawn = true;
  c.spawnMinDistance = std::max(0, luaIntField(state, spawn, "min_distance", 0));
  c.spawnMaxDistance = luaIntField(state, spawn, "max_distance", -1);
  const bool ok = biomeMask(state, spawn, "biomes", c.spawnBiomes, error);
  api.settop(state, spawn - 1);
  if(!ok) {
   return false;
  }
 }
 if(fieldTable(state, table, "terrain")) {
  const int list = api.gettop(state);
  const std::size_t count = std::min<std::size_t>(api.rawlen(state, list), 16);
  for(std::size_t i = 1; i <= count; ++i) {
   api.rawgeti(state, list, static_cast<long long>(i));
   const int item = api.gettop(state);
   seedsearch::TerrainRule rule;
   rule.x = luaIntField(state, item, "x", 0);
   rule.z = luaIntField(state, item, "z", 0);
   rule.minHeight = luaIntField(state, item, "min_height", -1);
   rule.maxHeight = luaIntField(state, item, "max_height", 255);
   api.getfield(state, item, "land");
   if(api.type(state, -1) == kLuaTBoolean) {
    rule.land = api.toboolean(state, -1) != 0 ? 1 : 0;
   }
   api.settop(state, item - 1);
   c.terrain.push_back(rule);
  }
  api.settop(state, list - 1);
 }
 return true;
}
int luaSeedsSearch(lua_State* state) {
 LuaArgs args(state);
 if(!args.table(1)) {
  return args.fail("minecraft.seeds.search expects (criteria, count?)");
 }
 net::minecraft::Biome::init();
 Criteria criteria;
 std::string error;
 if(!parseCriteria(state, 1, criteria, error)) {
  return fail(state, error);
 }
 int count = 0;
 if(args.count() >= 2 && args.integer(2, count) && count > 0) {
  criteria.maxResults = std::min(count, 1000);
 }
 auto job = std::make_shared<SearchJob>(std::move(criteria));
 if(!SearchJob::start(job)) {
  return fail(state, "compute pool unavailable");
 }
 int id = 0;
 {
  const std::lock_guard lock(gJobsMutex);
  id = gNextJobId++;
  gJobs.emplace(id, std::move(job));
 }
 luaApi().pushinteger(state, id);
 return 1;
}
std::shared_ptr<SearchJob> findJob(lua_State* state) {
 int id = 0;
 if(!LuaArgs(state).integer(1, id)) {
  return nullptr;
 }
 const std::lock_guard lock(gJobsMutex);
 const auto found = gJobs.find(id);
 return found == gJobs.end() ? nullptr : found->second;
}
int luaSeedsPoll(lua_State* state) {
 LuaApi& api = luaApi();
 const std::shared_ptr<SearchJob> job = findJob(state);
 if(job == nullptr) {
  api.pushnil(state);
  return 1;
 }
 const bool running = job->running();
 const double elapsed = job->elapsedSeconds();
 const std::uint64_t checked = job->checked();
 api.createtable(state, 0, 10);
 setField(state, "state", running ? "running" : (job->cancelled() ? "cancelled" : "done"));
 setField(state, "checked", static_cast<std::int64_t>(checked));
 setField(state, "total", static_cast<std::int64_t>(std::min<std::uint64_t>(job->total(), INT64_MAX)));
 setField(state, "matches", static_cast<std::int64_t>(job->matches()));
 setField(state, "elapsed", elapsed);
 setField(state, "rate", elapsed > 0.0 ? static_cast<double>(checked) / elapsed : 0.0);
 const std::vector<seedsearch::Match> results = job->results();
 api.createtable(state, static_cast<int>(results.size()), 0);
 for(std::size_t i = 0; i < results.size(); ++i) {
  const seedsearch::Match& match = results[i];
  api.createtable(state, 0, 8);
  setField(state, "seed", static_cast<std::int64_t>(match.seed));
  setField(state, "score", match.score);
  setField(state, "unique", match.unique);
  setField(state, "origin_biome", biomeWireName(match.originBiome));
  if(match.hasSpawn) {
   setField(state, "spawn_x", match.spawnX);
   setField(state, "spawn_z", match.spawnZ);
   setField(state, "spawn_biome", biomeWireName(match.spawnBiome));
  }
  api.rawseti(state, -2, static_cast<long long>(i + 1));
 }
 api.setfield(state, -2, "results");
 return 1;
}
int luaSeedsCancel(lua_State* state) {
 const std::shared_ptr<SearchJob> job = findJob(state);
 if(job != nullptr) {
  job->cancel();
 }
 luaApi().pushboolean(state, job != nullptr ? 1 : 0);
 return 1;
}
int luaSeedsRelease(lua_State* state) {
 int id = 0;
 bool released = false;
 if(LuaArgs(state).integer(1, id)) {
  const std::lock_guard lock(gJobsMutex);
  const auto found = gJobs.find(id);
  if(found != gJobs.end()) {
   found->second->cancel();
   gJobs.erase(found);
   released = true;
  }
 }
 luaApi().pushboolean(state, released ? 1 : 0);
 return 1;
}
int luaSeedsBiomeAt(lua_State* state) {
 LuaApi& api = luaApi();
 std::int64_t seed = 0;
 int x = 0;
 int z = 0;
 LuaArgs args(state);
 if(!readInt64(state, 1, seed) || !args.integer(2, x) || !args.integer(3, z)) {
  return args.fail("minecraft.seeds.biome_at expects (seed, x, z)");
 }
 net::minecraft::Biome::init();
 const BiomeSource source(static_cast<std::uint64_t>(seed));
 api.pushstring(state, biomeWireName(seedsearch::biomeIdAt(source, x, z)).c_str());
 return 1;
}
int luaSeedsMap(lua_State* state) {
 LuaApi& api = luaApi();
 std::int64_t seed = 0;
 int centerX = 0;
 int centerZ = 0;
 LuaArgs args(state);
 if(!readInt64(state, 1, seed) || !args.integer(2, centerX) || !args.integer(3, centerZ) ||
    (args.count() >= 4 && !args.table(4))) {
  return args.fail("minecraft.seeds.map expects (seed, center_x, center_z, options?)");
 }
 int radius = 256;
 int sideRequest = 128;
 if(args.count() >= 4) {
  radius = luaIntField(state, 4, "radius", radius);
  sideRequest = luaIntField(state, 4, "side", sideRequest);
 }
 radius = std::clamp(radius, 16, 16384);
 const int side = std::clamp(sideRequest, 8, 256);
 const int step = std::max(1, radius * 2 / side);
 const int startX = centerX - radius;
 const int startZ = centerZ - radius;
 net::minecraft::Biome::init();
 const BiomeSource source(static_cast<std::uint64_t>(seed));
 api.createtable(state, 0, 8);
 setField(state, "side", side);
 setField(state, "step", step);
 setField(state, "origin_x", startX);
 setField(state, "origin_z", startZ);
 setField(state, "center_x", centerX);
 setField(state, "center_z", centerZ);
 api.createtable(state, 0, 2);
 api.createtable(state, side * side, 0);
 api.createtable(state, side * side, 0);
 long long index = 1;
 for(int row = 0; row < side; ++row) {
  for(int col = 0; col < side; ++col) {
   const int id = seedsearch::biomeIdAt(source, startX + col * step, startZ + row * step);
   api.pushinteger(state, static_cast<long long>(0xFF000000U | static_cast<std::uint32_t>(
                                                                    Biome::byId(static_cast<BiomeId>(id)).grassColor &
                                                                    0xFFFFFF)));
   api.rawseti(state, -3, index);
   api.pushinteger(state, id);
   api.rawseti(state, -2, index);
   ++index;
  }
 }
 api.setfield(state, -3, "biome_id");
 api.setfield(state, -2, "grass");
 api.setfield(state, -2, "channels");
 return 1;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
int luaSeedsCopy(lua_State* state) {
 std::string text;
 if(!LuaArgs(state).string(1, text)) {
  return LuaArgs(state).fail("minecraft.seeds.copy expects (text)");
 }
 client::gui::screen::Screen::setClipboard(std::move(text));
 luaApi().pushboolean(state, 1);
 return 1;
}
#endif
} // namespace
void installSeedApi(lua_State* state, ModHost::LoadedLuaMod& mod) {
 (void)mod;
 pushFunctionTable(state,
                   {
                       {"search", luaSeedsSearch},
                       {"poll", luaSeedsPoll},
                       {"cancel", luaSeedsCancel},
                       {"release", luaSeedsRelease},
                       {"biome_at", luaSeedsBiomeAt},
                       {"map", luaSeedsMap},
#ifdef MINECRAFT_NATIVE_EXPORTS
                       {"copy", luaSeedsCopy},
#endif
                   });
 luaApi().setfield(state, -2, "seeds");
}
} // namespace net::minecraft::mod::runtime
