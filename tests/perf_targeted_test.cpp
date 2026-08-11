#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <thread>
#include <vector>
#include "perf_support.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/client/render/chunk/TerrainRegion.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightingEngine.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
namespace net::minecraft::test {
namespace {
using Clock = std::chrono::high_resolution_clock;
struct Phase {
 std::string name;
 double totalMs = 0.0;
 int calls = 0;
};
class Trace {
 public:
 explicit Trace(std::string name) : name_(std::move(name)) {
 }
 template <typename Fn>
 void measure(const std::string& phase, int iterations, Fn&& body) {
  const auto start = Clock::now();
  for(int i = 0; i < iterations; ++i) {
   body(i);
  }
  const double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
  phases_.push_back({phase, ms, iterations});
 }
 [[nodiscard]] double totalMs() const {
  double total = 0.0;
  for(const Phase& phase : phases_) total += phase.totalMs;
  return total;
 }
 void report() const {
  const double total = totalMs();
  std::printf("\n[PERF_TRACE] %s -- total %.3f ms\n", name_.c_str(), total);
  std::printf("[PERF_TRACE]   %-34s %10s %8s %10s %7s\n", "phase", "total ms", "calls", "us/call", "share");
  std::vector<Phase> sorted = phases_;
  std::sort(sorted.begin(), sorted.end(),
            [](const Phase& a, const Phase& b) { return a.totalMs > b.totalMs; });
  for(const Phase& phase : sorted) {
   std::printf("[PERF_TRACE]   %-34s %10.3f %8d %10.3f %6.1f%%\n", phase.name.c_str(), phase.totalMs,
               phase.calls, (phase.totalMs * 1000.0) / std::max(1, phase.calls),
               total > 0.0 ? (phase.totalMs / total) * 100.0 : 0.0);
  }
  std::fflush(stdout);
 }

 private:
 std::string name_;
 std::vector<Phase> phases_;
};
} // namespace
TEST(PerfTraceTargeted, SpinlockContentionShortHold) {
 Trace trace("spinlock contention (short hold)");
 auto chunk = std::make_unique<Chunk>(nullptr, 0, 0);
 // cyclesPerNs() calibrates on first use by busy-looping for 2 ms. Paying that
 // inside the first measureStall would consume the holder's entire hold before
 // the contender even tried to acquire.
 (void)cyclesPerNs();
 trace.measure("lock/unlock uncontended", 100000, [&](int) {
  chunk->lockRenderWrite();
  chunk->unlockRenderWrite();
 });
 // Both threads are spawned and parked on the gate before the holder takes the
 // lock. The previous version started the holder first and had it sleep for
 // 100-500 us, which is under std::thread spawn latency: the lock was already
 // free by the time the contender ran, so it timed an uncontended acquire and
 // reported 0.0 ms of contention forever.
 for(int trial = 0; trial < 4; ++trial) {
  const int holdMs = 2 * (trial + 1);
  StartGate gate;
  std::atomic<bool> held{false};
  StallProfile contenderProfile;
  auto holder = std::thread([&] {
   gate.arrive();
   chunk->lockRenderWrite();
   held.store(true, std::memory_order_release);
   const auto until = Clock::now() + std::chrono::milliseconds(holdMs);
   while(Clock::now() < until) {
   }
   chunk->unlockRenderWrite();
  });
  auto contender = std::thread([&] {
   gate.arrive();
   while(!held.load(std::memory_order_acquire)) {
   }
   contenderProfile = measureStall([&] {
    chunk->lockRenderWrite();
    chunk->unlockRenderWrite();
   });
  });
  gate.waitFor(2);
  gate.open();
  holder.join();
  contender.join();
  std::printf("[PERF_TRACE]   (hold %d ms -> contender wall %.2f ms, busy %.2f ms, blocked %.2f ms)\n",
              holdMs, contenderProfile.wallMs, contenderProfile.busyMs, contenderProfile.blockedMs);
  // The point of the test: the contender must actually have waited. A build
  // where this reads ~0 has stopped exercising contention and the numbers
  // below it mean nothing.
  EXPECT_GT(contenderProfile.wallMs, holdMs * 0.5)
      << "no contention was generated for a " << holdMs << " ms hold";
 }
 trace.report();
}
TEST(PerfTraceTargeted, LightingDrainWithSectionInvalidation) {
 Trace trace("lighting drain + section invalidation cascade");
 world::light::UnifiedLightRegistry registry;
 LightingEngine engine(registry);
 constexpr int kChunkRadius = 10;
 constexpr int kColumnCount = (2 * kChunkRadius + 1) * (2 * kChunkRadius + 1);
 std::vector<std::unique_ptr<Chunk>> chunks;
 chunks.reserve(static_cast<std::size_t>(kColumnCount));
 for(int cz = -kChunkRadius; cz <= kChunkRadius; ++cz) {
  for(int cx = -kChunkRadius; cx <= kChunkRadius; ++cx) {
   auto chunk = std::make_unique<Chunk>(nullptr, cx, cz);
   for(int y = 0; y < 64; ++y) {
    for(int z = 0; z < 16; ++z) {
     for(int x = 0; x < 16; ++x) {
      chunk->blocks[static_cast<std::size_t>((x << 11) | (z << 7) | y)] = (y < 32 ? 1 : 0);
     }
    }
   }
   engine.registerChunk(chunk.get());
   chunks.push_back(std::move(chunk));
  }
 }
 for(int cz = -kChunkRadius; cz <= kChunkRadius; ++cz) {
  for(int cx = -kChunkRadius; cx <= kChunkRadius; ++cx) {
   engine.push(LightType::Sky, cx * 16, 0, cz * 16, cx * 16 + 15, 63, cz * 16 + 15, false);
  }
 }
 // Mirrors what World::doLightingUpdates does each tick: hand the staged boxes
 // to the workers, then give them time to propagate before draining.
 engine.flushStaging();
 std::this_thread::sleep_for(std::chrono::milliseconds(300));
 struct FakeSection {
  int x, y, z;
  bool dirty = false;
  int version = 0;
  void invalidate() {
   dirty = true;
   ++version;
  }
 };
 constexpr int kSectionsPerColumn = 4;
 constexpr int kTotalSections = kColumnCount * kSectionsPerColumn;
 std::vector<FakeSection> sections(static_cast<std::size_t>(kTotalSections));
 int secIdx = 0;
 for(int cz = -kChunkRadius; cz <= kChunkRadius; ++cz) {
  for(int cx = -kChunkRadius; cx <= kChunkRadius; ++cx) {
   for(int sy = 0; sy < kSectionsPerColumn; ++sy) {
    sections[static_cast<std::size_t>(secIdx++)] = {cx * 16, sy * 16, cz * 16};
   }
  }
 }
 int drainedRegions = 0;
 int invalidatedSections = 0;
 trace.measure("drain + invalidate sections", 1, [&](int) {
  int drained = 0;
  int invalidated = 0;
  const auto deadline = Clock::now() + std::chrono::seconds(10);
  while((engine.hasDirtyRegions() || engine.busy()) && Clock::now() < deadline) {
   auto regions = engine.drainDirtyRegions(16);
   if(regions.empty()) {
    // Workers are still propagating; an empty poll is not the end of the
    // cascade. Breaking here is what made this test drain nothing.
    std::this_thread::yield();
    continue;
   }
   for(const auto& region : regions) {
    int minCX = region.minX >> 4;
    int maxCX = region.maxX >> 4;
    int minCZ = region.minZ >> 4;
    int maxCZ = region.maxZ >> 4;
    for(int cx = minCX; cx <= maxCX; ++cx) {
     for(int cz = minCZ; cz <= maxCZ; ++cz) {
      for(int sy = 0; sy < kSectionsPerColumn; ++sy) {
       int idx = ((cz + kChunkRadius) * (2 * kChunkRadius + 1) + (cx + kChunkRadius)) * kSectionsPerColumn + sy;
       if(idx >= 0 && idx < kTotalSections) {
        sections[static_cast<std::size_t>(idx)].invalidate();
        ++invalidated;
       }
      }
     }
    }
   }
   drained += static_cast<int>(regions.size());
   if(drained > 50000) break;
  }
  std::printf("[PERF_TRACE]   (drained %d regions, invalidated %d sections)\n", drained, invalidated);
  drainedRegions = drained;
  invalidatedSections = invalidated;
 });
 trace.report();
 engine.stop();
 for(const auto& chunk : chunks) {
  engine.unregisterChunk(chunk.get());
 }
 // Draining nothing used to be a pass. If the engine produces no dirty regions
 // for 441 fully-lit columns, the cascade this test exists to measure did not
 // run and its timing is meaningless.
 EXPECT_GT(drainedRegions, 0) << "lighting produced no dirty regions to drain";
 EXPECT_GT(invalidatedSections, 0) << "no sections were invalidated by the drain";
}
// TerrainRegion::upload is entirely GL calls, so without a current context it
// returns false at the first null function pointer and never reaches
// ensureCapacity. This used to print 0.000 ms at every size and pass against a
// 5000 ms ceiling -- a green result for a measurement that never happened.
// Skipping is the honest outcome; the arena grow cost is measured in-game via
// the ArenaGrows / ArenaGrowVertices counters.
TEST(PerfTraceTargeted, ChunkArenaGrowDirectMeasurement) {
 if(!client::gl::GLCore::vboSupported || client::gl::GLCore::bufferData == nullptr) {
  GTEST_SKIP() << "no current GL context: arena grow cannot be measured headless";
 }
 Trace trace("chunk arena grow (direct ensureCapacity)");
 std::size_t capacities[] = {4096, 16384, 65536, 262144, 1048576};
 int measured = 0;
 for(int trial = 0; trial < 5; ++trial) {
  client::render::chunk::TerrainRegion region(0, 0, 0);
  client::render::chunk::TerrainAllocation alloc{};
  std::vector<client::render::TessellatorVertex> smallBatch(4096);
  for(auto& v : smallBatch) {
   v.x = 1.0f;
   v.y = 2.0f;
   v.z = 3.0f;
   v.color = 0xFF00FF00U;
   v.light = 0x00F000F0;
  }
  region.upload(0, alloc, std::span<const client::render::TessellatorVertex>(smallBatch));
  std::vector<client::render::TessellatorVertex> bigBatch(capacities[static_cast<std::size_t>(trial)]);
  for(auto& v : bigBatch) {
   v.x = 1.0f;
   v.y = 2.0f;
   v.z = 3.0f;
   v.color = 0xFF00FF00U;
   v.light = 0x00F000F0;
  }
  client::render::chunk::TerrainAllocation bigAlloc{};
  bool ok = false;
  const StallProfile grow = measureStall([&] {
   ok = region.upload(0, bigAlloc, std::span<const client::render::TessellatorVertex>(bigBatch));
  });
  if(ok) {
   ++measured;
  }
  std::printf("[PERF_TRACE]   (grow to %zu vertices: %s wall %.3f ms, busy %.3f ms, blocked %.3f ms)\n",
              capacities[static_cast<std::size_t>(trial)], ok ? "ok" : "FAILED", grow.wallMs, grow.busyMs,
              grow.blockedMs);
 }
 trace.report();
 EXPECT_EQ(measured, 5) << "uploads failed, so no grow was timed";
}
TEST(PerfTraceTargeted, RegionSnapshotCaptureScalability) {
 Trace trace("RegionSnapshot capture scalability");
 for(int radius = 1; radius <= 3; ++radius) {
  int colCount = (2 * radius + 1) * (2 * radius + 1);
  std::vector<std::unique_ptr<Chunk>> chunks;
  std::vector<client::render::chunk::RegionSnapshot::SourceChunk> sources;
  chunks.reserve(static_cast<std::size_t>(colCount));
  sources.reserve(static_cast<std::size_t>(colCount));
  for(int cz = -radius; cz <= radius; ++cz) {
   for(int cx = -radius; cx <= radius; ++cx) {
    auto chunk = std::make_unique<Chunk>(nullptr, cx, cz);
    for(int y = 0; y < 64; ++y) {
     for(int z = 0; z < 16; ++z) {
      for(int x = 0; x < 16; ++x) {
       chunk->blocks[static_cast<std::size_t>((x << 11) | (z << 7) | y)] = 1;
      }
     }
    }
    sources.push_back({cx, cz, chunk.get()});
    chunks.push_back(std::move(chunk));
   }
  }
  std::array<float, 16> lightTable{};
  for(std::size_t i = 0; i < 16; ++i) {
   lightTable[i] = static_cast<float>(i) / 15.0f;
  }
  int iterations = 200;
  auto body = [&](int) {
   client::render::chunk::RegionSnapshot snapshot(sources, 0, lightTable, nullptr,
                                                  -16 * radius, 0, -16 * radius,
                                                  16 * radius + 15, 63, 16 * radius + 15);
   volatile int id = snapshot.getBlockId(0, 10, 0);
   (void)id;
  };
  std::string phaseName = "radius " + std::to_string(radius) + " (" + std::to_string(colCount) + " chunks)";
  trace.measure(phaseName, iterations, body);
 }
 trace.report();
 EXPECT_LT(trace.totalMs(), 10000.0);
}
TEST(PerfTraceTargeted, SpinlockContentionWithMultipleSpinners) {
 Trace trace("spinlock contention (multiple spinners)");
 auto chunk = std::make_unique<Chunk>(nullptr, 0, 0);
 // Same defect as the short-hold case: a 200 us hold expired before the
 // spinner threads existed, so every reading was an uncontended acquire.
 constexpr int kHoldMs = 4;
 double multiWaitTimes[4] = {};
 for(int spinnerCount = 1; spinnerCount <= 4; ++spinnerCount) {
  StartGate gate;
  std::atomic<bool> held{false};
  std::atomic<std::int64_t> maxAcquireTimeNs{0};
  auto holder = std::thread([&] {
   gate.arrive();
   chunk->lockRenderWrite();
   held.store(true, std::memory_order_release);
   const auto until = Clock::now() + std::chrono::milliseconds(kHoldMs);
   while(Clock::now() < until) {
   }
   chunk->unlockRenderWrite();
  });
  std::vector<std::thread> spinners;
  for(int s = 0; s < spinnerCount; ++s) {
   spinners.emplace_back([&] {
    gate.arrive();
    while(!held.load(std::memory_order_acquire)) {
    }
    const auto start = Clock::now();
    chunk->lockRenderWrite();
    const auto end = Clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::int64_t current = maxAcquireTimeNs.load(std::memory_order_relaxed);
    while(elapsed > current &&
          !maxAcquireTimeNs.compare_exchange_weak(current, elapsed, std::memory_order_relaxed)) {
    }
    chunk->unlockRenderWrite();
   });
  }
  gate.waitFor(spinnerCount + 1);
  gate.open();
  holder.join();
  for(auto& t : spinners) {
   t.join();
  }
  multiWaitTimes[static_cast<std::size_t>(spinnerCount - 1)] = maxAcquireTimeNs.load() / 1.0e6;
 }
 std::printf("[PERF_TRACE]   (max wait with 1/2/3/4 spinners for a %d ms hold: %.2f/%.2f/%.2f/%.2f ms)\n",
             kHoldMs, multiWaitTimes[0], multiWaitTimes[1], multiWaitTimes[2], multiWaitTimes[3]);
 trace.report();
 for(int i = 0; i < 4; ++i) {
  EXPECT_GT(multiWaitTimes[static_cast<std::size_t>(i)], kHoldMs * 0.5)
      << "no contention generated with " << (i + 1) << " spinner(s)";
 }
}
} // namespace net::minecraft::test
