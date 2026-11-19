#pragma once
#include <array>
#include <cstdint>
namespace net::minecraft::test {
struct HistoricalTerrainSample {
 std::int64_t seed;
 int chunkX;
 int chunkZ;
 std::uint64_t fnv64;
};
inline constexpr std::array<HistoricalTerrainSample, 16> kAlpha11201Samples{{
 {-7LL, -7, 13, 0x24bd459e259618b8ULL},
 {-7LL, 0, 0, 0x4bb53cf0df1dc62bULL},
 {-7LL, 1, -1, 0x9e1bf9461620dd40ULL},
 {-7LL, 40, -22, 0x8e990c06d2ca92c0ULL},
 {0LL, -7, 13, 0x0312ab4db8a5bea6ULL},
 {0LL, 0, 0, 0x233db58ec328674cULL},
 {0LL, 1, -1, 0x1c4165ad695c3476ULL},
 {0LL, 40, -22, 0x648b85347e1d5d7cULL},
 {42LL, -7, 13, 0x9b4a443875648339ULL},
 {42LL, 0, 0, 0xf2ab9db09f55acdbULL},
 {42LL, 1, -1, 0xc9ffc7eb727a65b4ULL},
 {42LL, 40, -22, 0x4228174d1a2af689ULL},
 {81985529216486895LL, -7, 13, 0xec3c1aeda3b844faULL},
 {81985529216486895LL, 0, 0, 0x1e7786d24c96b2d5ULL},
 {81985529216486895LL, 1, -1, 0xc48a5465f1ffe2abULL},
 {81985529216486895LL, 40, -22, 0xa71dcae026345e55ULL},
}};
}
