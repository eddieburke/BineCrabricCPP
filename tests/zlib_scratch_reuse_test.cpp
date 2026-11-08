#include <gtest/gtest.h>
#include <atomic>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <vector>
#include "net/minecraft/nbt/Compression.hpp"
namespace net::minecraft::test {
namespace {
std::vector<std::uint8_t> pattern(std::size_t size, std::uint32_t seed) {
 std::vector<std::uint8_t> bytes(size);
 std::uint32_t state = seed;
 for(std::size_t i = 0; i < size; ++i) {
  state = state * 1664525u + 1013904223u;
  bytes[i] = static_cast<std::uint8_t>((i / 7) ^ (state >> 28));
 }
 return bytes;
}
} // namespace
TEST(ZlibScratch, ReusedStreamsRoundTripBothFormatsInterleaved) {
 for(std::uint32_t round = 0; round < 4; ++round) {
  const std::vector<std::uint8_t> chunk = pattern(82'000 + round * 1'000, round + 1);
  const std::vector<std::uint8_t> level = pattern(3'000, round + 11);
  const std::vector<std::uint8_t> zlib = zlibCompress(chunk);
  const std::vector<std::uint8_t> gzip = gzipCompress(level);
  EXPECT_EQ(zlibDecompress(zlib), chunk);
  EXPECT_EQ(gzipDecompress(gzip), level);
 }
}
TEST(ZlibScratch, FailedInflateLeavesTheThreadStreamReusable) {
 const std::vector<std::uint8_t> data = pattern(40'000, 7);
 const std::vector<std::uint8_t> compressed = zlibCompress(data);
 EXPECT_THROW((void)zlibDecompress(compressed, 1'024), std::runtime_error);
 const std::vector<std::uint8_t> truncated(compressed.begin(),
                                           compressed.begin() + static_cast<std::ptrdiff_t>(compressed.size() / 2));
 EXPECT_THROW((void)zlibDecompress(truncated), std::runtime_error);
 EXPECT_EQ(zlibDecompress(compressed), data);
}
TEST(ZlibScratch, OversizedScratchIsDroppedAndSmallBuffersStillRoundTrip) {
 const std::vector<std::uint8_t> big = pattern(3u * 1024u * 1024u, 3);
 EXPECT_EQ(zlibDecompress(zlibCompress(big)), big);
 const std::vector<std::uint8_t> small = pattern(500, 4);
 const std::vector<std::uint8_t> packed = zlibCompress(small);
 EXPECT_EQ(zlibDecompress(packed), small);
 EXPECT_EQ(zlibDecompress(packed, small.size() + 16), small);
}
TEST(ZlibScratch, ConcurrentThreadsKeepIndependentStreams) {
 std::atomic<int> failures{0};
 std::vector<std::thread> threads;
 for(std::uint32_t thread = 0; thread < 8; ++thread) {
  threads.emplace_back([thread, &failures] {
   for(std::uint32_t i = 0; i < 16; ++i) {
    const std::vector<std::uint8_t> data = pattern(20'000 + thread * 100 + i, thread * 31 + i);
    if(zlibDecompress(zlibCompress(data)) != data || gzipDecompress(gzipCompress(data)) != data) {
     failures.fetch_add(1, std::memory_order_relaxed);
    }
   }
  });
 }
 for(std::thread& worker : threads) worker.join();
 EXPECT_EQ(failures.load(), 0);
}
} // namespace net::minecraft::test
