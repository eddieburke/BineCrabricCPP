#include <gtest/gtest.h>
#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
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
void setupChunks(int radius, std::vector<std::unique_ptr<Chunk>>& chunks,
                 std::vector<client::render::chunk::RegionSnapshot::SourceChunk>& sources) {
 chunks.clear();
 sources.clear();
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
}
} // namespace
TEST(PerfTraceDiagnose, RegionSnapshotAllocationOnly) {
 Trace trace("RegionSnapshot allocation scaling");
 std::vector<client::render::chunk::RegionSnapshot::SourceChunk> sources;
 std::vector<std::unique_ptr<Chunk>> chunks;
 for(int radius = 1; radius <= 4; ++radius) {
  setupChunks(radius, chunks, sources);
  int colCount = (2 * radius + 1) * (2 * radius + 1);
  std::size_t perChunkBytes = static_cast<std::size_t>(16 * 16 * 64) + 3 * static_cast<std::size_t>(16 * 16 * 32);
  trace.measure("per-chunk alloc (r=" + std::to_string(radius) + ", " + std::to_string(colCount) + " chunks)", 200, [&](int) {
   std::vector<std::vector<std::uint8_t>> copies(static_cast<std::size_t>(colCount));
   for(auto& copy : copies) {
    copy.resize(perChunkBytes);
   }
   volatile std::size_t sink = copies.size();
   (void)sink;
  });
  trace.measure("single buffer (r=" + std::to_string(radius) + ", " + std::to_string(colCount) + " chunks)", 200, [&](int) {
   std::vector<std::uint8_t> buffer(perChunkBytes * static_cast<std::size_t>(colCount));
   volatile std::size_t sink = buffer.size();
   (void)sink;
  });
 }
 trace.report();
 EXPECT_LT(trace.totalMs(), 10000.0);
}
TEST(PerfTraceDiagnose, RegionSnapshotMemcpyOnly) {
 Trace trace("RegionSnapshot memcpy scaling");
 std::vector<client::render::chunk::RegionSnapshot::SourceChunk> sources;
 std::vector<std::unique_ptr<Chunk>> chunks;
 for(int radius = 1; radius <= 4; ++radius) {
  setupChunks(radius, chunks, sources);
  int colCount = (2 * radius + 1) * (2 * radius + 1);
  std::size_t perChunkBytes = static_cast<std::size_t>(16 * 16 * 64) + 3 * static_cast<std::size_t>(16 * 16 * 32);
  std::vector<std::uint8_t> buffer(perChunkBytes * static_cast<std::size_t>(colCount));
  trace.measure("memcpy into single buffer (r=" + std::to_string(radius) + ", " + std::to_string(colCount) + " chunks)", 200, [&](int) {
   std::size_t offset = 0;
   for(const auto& source : sources) {
    const Chunk& chunk = *source.chunk;
    std::memcpy(buffer.data() + offset, chunk.blocks.data(), static_cast<std::size_t>(16 * 16 * 64));
    offset += perChunkBytes;
   }
   volatile std::size_t sink = buffer.size();
   (void)sink;
  });
 }
 trace.report();
 EXPECT_LT(trace.totalMs(), 10000.0);
}
TEST(PerfTraceDiagnose, RegionSnapshotAnyOfOnly) {
 Trace trace("RegionSnapshot any_of scan scaling");
 std::vector<client::render::chunk::RegionSnapshot::SourceChunk> sources;
 std::vector<std::unique_ptr<Chunk>> chunks;
 for(int radius = 1; radius <= 4; ++radius) {
  setupChunks(radius, chunks, sources);
  int colCount = (2 * radius + 1) * (2 * radius + 1);
  std::size_t perChunkBytes = static_cast<std::size_t>(16 * 16 * 64);
  std::vector<std::vector<std::uint8_t>> buffers(static_cast<std::size_t>(colCount),
                                                 std::vector<std::uint8_t>(perChunkBytes, 1));
  trace.measure("any_of scan (r=" + std::to_string(radius) + ", " + std::to_string(colCount) + " chunks)", 200, [&](int) {
   int found = 0;
   for(const auto& buf : buffers) {
    bool any = std::any_of(buf.begin(), buf.end(), [](std::uint8_t b) { return b != 0; });
    if(any) ++found;
   }
   volatile int sink = found;
   (void)sink;
  });
 }
 trace.report();
 EXPECT_LT(trace.totalMs(), 10000.0);
}
TEST(PerfTraceDiagnose, RegionSnapshotFullConstruct) {
 Trace trace("RegionSnapshot full construct");
 std::vector<client::render::chunk::RegionSnapshot::SourceChunk> sources;
 std::vector<std::unique_ptr<Chunk>> chunks;
 for(int radius = 1; radius <= 4; ++radius) {
  setupChunks(radius, chunks, sources);
  int colCount = (2 * radius + 1) * (2 * radius + 1);
  std::array<float, 16> lightTable{};
  for(std::size_t i = 0; i < 16; ++i) {
   lightTable[i] = static_cast<float>(i) / 15.0f;
  }
  trace.measure("full construct (r=" + std::to_string(radius) + ", " + std::to_string(colCount) + " chunks)", 200, [&](int) {
   client::render::chunk::RegionSnapshot snapshot(sources, 0, lightTable, nullptr,
                                                  -16 * radius, 0, -16 * radius,
                                                  16 * radius + 15, 63, 16 * radius + 15);
   volatile int id = snapshot.getBlockId(0, 10, 0);
   (void)id;
  });
 }
 trace.report();
 EXPECT_LT(trace.totalMs(), 30000.0);
}
TEST(PerfTraceDiagnose, RegionSnapshotDeallocation) {
 Trace trace("RegionSnapshot deallocation scaling");
 std::vector<client::render::chunk::RegionSnapshot::SourceChunk> sources;
 std::vector<std::unique_ptr<Chunk>> chunks;
 for(int radius = 1; radius <= 4; ++radius) {
  setupChunks(radius, chunks, sources);
  int colCount = (2 * radius + 1) * (2 * radius + 1);
  std::size_t perChunkBytes = static_cast<std::size_t>(16 * 16 * 64) + 3 * static_cast<std::size_t>(16 * 16 * 32);
  trace.measure("alloc+free (r=" + std::to_string(radius) + ", " + std::to_string(colCount) + " chunks)", 200, [&](int) {
   std::vector<std::vector<std::uint8_t>> buffers;
   buffers.reserve(static_cast<std::size_t>(colCount));
   for(int i = 0; i < colCount; ++i) {
    buffers.emplace_back(perChunkBytes);
   }
   volatile std::size_t sink = buffers.size();
   (void)sink;
  });
 }
 trace.report();
 EXPECT_LT(trace.totalMs(), 10000.0);
}
} // namespace net::minecraft::test
