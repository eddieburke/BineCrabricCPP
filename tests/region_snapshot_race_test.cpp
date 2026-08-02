// Concurrency coverage for the RegionSnapshot copy path (WI-3).
//
// RegionSnapshot::copyChunkBand memcpys the live chunk's blocks/meta/light
// arrays onto the mesh job. A lighting worker mutating the light nibbles at
// the same time used to be a plain data race (torn snapshots were
// nondeterministic). The per-chunk render-write guard (Chunk::lockRenderWrite)
// now serializes copyChunkBand against every writer, so a snapshot can never
// observe a partially-updated array. These tests run one snapshotting thread
// against a lighting-style writer and assert the copied values stay coherent.
// No GL context is required (the Tessellator is not used).
#include <gtest/gtest.h>
#include <array>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightType.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::client::render::chunk::RegionSnapshot;
std::vector<RegionSnapshot::SourceChunk> sourceFor(const Chunk& chunk) {
 return {RegionSnapshot::SourceChunk{0, 0, &chunk}};
}
std::array<float, 16> flatLuminance() {
 std::array<float, 16> table{};
 table.fill(0.0f);
 return table;
}
} // namespace
// A lighting-style writer hammers one blockLight nibble between 0x5 and 0xA
// while the reader takes RegionSnapshots. With the render-write guard the
// copied nibble is always exactly one of the two written values, never a torn
// mix (1000 iterations).
TEST(RegionSnapshotRace, ConcurrentCopySeesNoTornNibble) {
 auto chunk = std::make_unique<Chunk>(nullptr, 0, 0);
 const int x = 7;
 const int y = 40;
 const int z = 9;
 chunk->setLight(LightType::Block, x, y, z, 0x5);
 std::atomic<bool> stop{false};
 std::thread writer([&] {
  int high = 0;
  while(!stop.load(std::memory_order_relaxed)) {
   chunk->setLight(LightType::Block, x, y, z, high ? 0xA : 0x5);
   high = 1 - high;
  }
 });
 const std::vector<RegionSnapshot::SourceChunk> sources = sourceFor(*chunk);
 const std::array<float, 16> luminance = flatLuminance();
 for(int i = 0; i < 1000; ++i) {
  RegionSnapshot snapshot(sources, /*ambientDarkness=*/0, luminance, nullptr, -1, -1, -1, 17, 60, 17);
  const int copied = snapshot.getBlockLight(x, y, z);
  EXPECT_TRUE(copied == 0x5 || copied == 0xA) << "torn blockLight nibble: " << copied;
 }
 stop.store(true, std::memory_order_relaxed);
 writer.join();
}
// The guard must also make the WHOLE copy atomic: a writer that updates two
// correlated nibbles inside one lockRenderWrite section can never be split by
// copyChunkBand, so the snapshot always sees both writes together.
TEST(RegionSnapshotRace, WriteLockSerializesSnapshotCopy) {
 auto chunk = std::make_unique<Chunk>(nullptr, 0, 0);
 const int x1 = 1;
 const int y1 = 20;
 const int z1 = 2;
 const int x2 = 5;
 const int y2 = 21;
 const int z2 = 6;
 std::atomic<bool> stop{false};
 std::thread writer([&] {
  int high = 0;
  while(!stop.load(std::memory_order_relaxed)) {
   const int value = high ? 0xC : 0x3;
   chunk->lockRenderWrite();
   chunk->blockLight.set(x1, y1, z1, value);
   chunk->blockLight.set(x2, y2, z2, value);
   chunk->unlockRenderWrite();
   high = 1 - high;
  }
 });
 const std::vector<RegionSnapshot::SourceChunk> sources = sourceFor(*chunk);
 const std::array<float, 16> luminance = flatLuminance();
 for(int i = 0; i < 1000; ++i) {
  RegionSnapshot snapshot(sources, /*ambientDarkness=*/0, luminance, nullptr, -1, -1, -1, 17, 40, 17);
  const int first = snapshot.getBlockLight(x1, y1, z1);
  const int second = snapshot.getBlockLight(x2, y2, z2);
  EXPECT_EQ(first, second) << "paired write split by snapshot copy: " << first << " vs " << second;
 }
 stop.store(true, std::memory_order_relaxed);
 writer.join();
}
} // namespace net::minecraft::test
