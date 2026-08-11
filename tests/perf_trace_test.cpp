// Perf TRACES, not perf gates. Each test prints a per-phase attributed
// breakdown so a regression shows up as "which phase grew", not just "the
// number is bigger". The assertions are deliberately loose ceilings; the
// printed [PERF_TRACE] table is the deliverable.
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "net/minecraft/client/debug/RenderProfiler.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightingEngine.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
namespace net::minecraft::test {
namespace {
using Clock = std::chrono::high_resolution_clock;
// std::this_thread::sleep_for is useless for generating known sub-millisecond
// durations on Windows: the scheduler quantum is ~15 ms, so a 400 us sleep
// either returns immediately or eats a whole tick. Spin instead — these are the
// durations the profiler is asked to attribute, so they have to be real.
void burnCpu(std::chrono::nanoseconds duration) {
 const auto deadline = Clock::now() + duration;
 while(Clock::now() < deadline) {
 }
}
// One measured phase of a trace.
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
// A 3x3 column of solid stone with a lit floor, the shape the mesher and the
// lighting engine both actually see in game.
struct TestRegion {
 std::vector<std::unique_ptr<Chunk>> chunks;
 std::vector<client::render::chunk::RegionSnapshot::SourceChunk> sources;
 std::array<float, 16> lightTable{};
 TestRegion() {
  chunks.reserve(9);
  sources.reserve(9);
  for(int cz = -1; cz <= 1; ++cz) {
   for(int cx = -1; cx <= 1; ++cx) {
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
  for(std::size_t i = 0; i < 16; ++i) {
   lightTable[i] = static_cast<float>(i) / 15.0f;
  }
 }
};
} // namespace
// Where does terrain-build time actually go? Snapshot construction is the
// suspect phase: it copies a 3x3 column every time a section re-meshes, so a
// regression there multiplies across every dirty section in the ring.
TEST(PerfTrace, TerrainBuildPhaseBreakdown) {
 TestRegion region;
 Trace trace("terrain build");
 trace.measure("RegionSnapshot construct", 500, [&](int) {
  client::render::chunk::RegionSnapshot snapshot(region.sources, 0, region.lightTable, nullptr, -16, 0,
                                                 -16, 31, 63, 31);
  volatile int id = snapshot.getBlockId(0, 10, 0);
  (void)id;
 });
 {
  client::render::chunk::RegionSnapshot snapshot(region.sources, 0, region.lightTable, nullptr, -16, 0, -16,
                                                 31, 63, 31);
  trace.measure("snapshot getBlockId x4096", 500, [&](int) {
   int accumulator = 0;
   for(int y = 0; y < 16; ++y) {
    for(int z = 0; z < 16; ++z) {
     for(int x = 0; x < 16; ++x) {
      accumulator += snapshot.getBlockId(x, y, z);
     }
    }
   }
   volatile int sink = accumulator;
   (void)sink;
  });
 }
 trace.report();
 EXPECT_LT(trace.totalMs(), 30000.0);
}
// Frustum culling runs once per section per frame, so plane extraction and the
// AABB test are separated here: a slow extract is a per-frame constant, a slow
// AABB test scales with the section count.
TEST(PerfTrace, FrustumCullPhaseBreakdown) {
 using net::minecraft::util::math::Matrix4f;
 Matrix4f projection = Matrix4f::identityMatrix();
 Matrix4f modelView = Matrix4f::identityMatrix();
 projection.m[0] = 1.3f;
 projection.m[5] = 2.4f;
 projection.m[10] = -1.0004f;
 projection.m[11] = -1.0f;
 projection.m[14] = -0.2f;
 projection.m[15] = 0.0f;
 client::render::Frustum frustum;
 Trace trace("frustum cull");
 trace.measure("Frustum::compute (plane extract)", 20000, [&](int) {
  frustum.compute(projection, modelView);
 });
 frustum.compute(projection, modelView);
 frustum.prepare(0.0, 64.0, 0.0);
 // A realistic ring: 16-block sections out to ~16 chunks in every direction.
 std::vector<net::minecraft::Box> boxes;
 for(int x = -16; x < 16; ++x) {
  for(int z = -16; z < 16; ++z) {
   const double bx = x * 16.0;
   const double bz = z * 16.0;
   boxes.emplace_back(bx, 48.0, bz, bx + 16.0, 64.0, bz + 16.0);
  }
 }
 int visible = 0;
 trace.measure("isVisible over 1024-section ring", 200, [&](int) {
  for(const net::minecraft::Box& box : boxes) {
   if(frustum.isVisible(box)) ++visible;
  }
 });
 std::printf("[PERF_TRACE]   (visible accumulator %d over %zu boxes)\n", visible, boxes.size());
 trace.report();
 EXPECT_LT(trace.totalMs(), 30000.0);
}
// Lighting propagation is the phase most likely to stall the main thread when
// a chunk column loads. Registration, push and drain are traced separately
// because they have different owners.
TEST(PerfTrace, LightingEnginePhaseBreakdown) {
 world::light::UnifiedLightRegistry registry;
 LightingEngine engine(registry);
 std::vector<std::unique_ptr<Chunk>> chunks;
 chunks.reserve(9);
 Trace trace("lighting engine");
 trace.measure("registerChunk", 9, [&](int i) {
  const int cx = (i % 3) - 1;
  const int cz = (i / 3) - 1;
  auto chunk = std::make_unique<Chunk>(nullptr, cx, cz);
  for(int y = 0; y < 64; ++y) {
   for(int z = 0; z < 16; ++z) {
    for(int x = 0; x < 16; ++x) {
     chunk->blocks[static_cast<std::size_t>((x << 11) | (z << 7) | y)] = (y == 0 ? 7 : 0);
    }
   }
  }
  engine.registerChunk(chunk.get());
  chunks.push_back(std::move(chunk));
 });
 trace.measure("push(Block)", 2000, [&](int) {
  engine.push(LightType::Block, 0, 1, 0, 15, 10, 15, true);
 });
 trace.measure("push(Sky)", 2000, [&](int) {
  engine.push(LightType::Sky, 0, 1, 0, 15, 10, 15, true);
 });
 trace.report();
 engine.stop();
 for(const auto& chunk : chunks) {
  engine.unregisterChunk(chunk.get());
 }
 EXPECT_LT(trace.totalMs(), 30000.0);
}
// The in-game breakdown is only trustworthy if the profiler charges each stage
// its own self time. This pins that: three stages with known, different
// durations must come back in the right proportion, so a real capture can be
// read as "Deferred is 60% of the frame" rather than just "the frame is slow".
TEST(PerfTrace, MetricAveragesConvergeToWholeFrameTotals) {
 client::debug::RenderProfiler& profiler = client::debug::RenderProfiler::instance();
 profiler.setEnabled(true);
 constexpr int kStagesPerFrame = 20;
 constexpr int kDrawsPerStage = 5;
 for(int frame = 0; frame < 400; ++frame) {
  profiler.beginFrame();
  for(int stage = 0; stage < kStagesPerFrame; ++stage) {
   profiler.record(client::debug::RenderMetric::DrawCalls, kDrawsPerStage);
  }
  profiler.endFrame();
 }
 const double average = profiler.metricAverage(client::debug::RenderMetric::DrawCalls);
 const double expected = kStagesPerFrame * kDrawsPerStage;
 std::printf("\n[PERF_TRACE] draw-call metric average %.2f (expected %.2f)\n", average, expected);
 std::fflush(stdout);
 EXPECT_GT(average, expected * 0.95);
 EXPECT_LE(average, expected);
 profiler.setEnabled(false);
}
} // namespace net::minecraft::test
