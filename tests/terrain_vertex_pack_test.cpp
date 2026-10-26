#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/client/render/chunk/TerrainRegion.hpp"
namespace net::minecraft::test {
using client::render::TerrainVertex;
using client::render::TessellatorVertex;
TEST(TerrainVertexPack, PacksEveryVertexInPlaceWithoutClobberingLaterOnes) {
 std::vector<TessellatorVertex> vertices(1000);
 for(std::size_t i = 0; i < vertices.size(); ++i) {
  TessellatorVertex& v = vertices[i];
  v.x = static_cast<float>(i) * 0.5f;
  v.y = static_cast<float>(i) + 0.25f;
  v.z = -static_cast<float>(i);
  v.u = static_cast<float>(i) / 1000.0f;
  v.v = 1.0f - static_cast<float>(i) / 1000.0f;
  v.color = 0xFF000000U | static_cast<std::uint32_t>(i);
  v.normal = static_cast<std::int32_t>(0x007F00U);
  v.midBlock = static_cast<std::int32_t>(i * 7U);
  v.light = static_cast<std::int32_t>(0x00F00000U | (i & 0xFFU));
  v.entity[0] = static_cast<std::int16_t>(i);
  v.entity[1] = static_cast<std::int16_t>(i % 2 == 0 ? 1 : -1);
  v.entity[2] = 9;
  v.midU = 0.5f;
  v.midV = 0.25f;
  v.tangent[0] = 32767;
  v.tangent[1] = 0;
  v.tangent[2] = -32767;
  v.tangent[3] = static_cast<std::int16_t>(i % 2 == 0 ? 32767 : -32767);
 }
 const std::vector<TessellatorVertex> original = vertices;
 const std::span<const std::byte> packed = client::render::chunk::packTerrainVertices(vertices);
 ASSERT_EQ(packed.size(), original.size() * sizeof(TerrainVertex));
 for(std::size_t i = 0; i < original.size(); ++i) {
  TerrainVertex out;
  std::memcpy(&out, packed.data() + i * sizeof(TerrainVertex), sizeof(TerrainVertex));
  const TessellatorVertex& in = original[i];
  EXPECT_EQ(out.x, in.x);
  EXPECT_EQ(out.y, in.y);
  EXPECT_EQ(out.z, in.z);
  EXPECT_EQ(out.u, in.u);
  EXPECT_EQ(out.v, in.v);
  EXPECT_EQ(out.color, in.color);
  EXPECT_EQ(out.normal, in.normal);
  EXPECT_EQ(out.midBlock, in.midBlock);
  EXPECT_EQ(out.light, in.light);
  EXPECT_EQ(out.entity[0], in.entity[0]);
  EXPECT_EQ(out.entity[1], in.entity[1]);
  EXPECT_EQ(out.midTexCoord[0], 32768);
  EXPECT_EQ(out.midTexCoord[1], 16384);
  EXPECT_EQ(out.tangent[0], 127);
  EXPECT_EQ(out.tangent[1], 0);
  EXPECT_EQ(out.tangent[2], -127);
  EXPECT_EQ(out.tangent[3], i % 2 == 0 ? 127 : -127);
 }
}
TEST(TerrainVertexPack, TerrainFormatsStayInsideThePackedStride) {
 for(const auto& format : client::render::vertex_abi::TerrainFormats) {
  EXPECT_LT(format.offset, sizeof(TerrainVertex)) << "location " << format.location;
 }
}
} // namespace net::minecraft::test
