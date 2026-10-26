#include "net/minecraft/client/render/chunk/TerrainLod.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>
namespace net::minecraft::client::render::chunk {
int terrainLodLevel(int distanceChunks, int previousLevel, bool enabled) noexcept {
 if(!enabled) return 0;
 constexpr int thresholds[] = {16, 32, 64};
 int level = 0;
 while(level < 3 && distanceChunks >= thresholds[level]) ++level;
 if(previousLevel >= 0 && previousLevel <= 3) {
  if(level > previousLevel && distanceChunks < thresholds[previousLevel] + 2) return previousLevel;
  if(level < previousLevel && distanceChunks >= thresholds[previousLevel - 1] - 2) return previousLevel;
 }
 return level;
}
bool terrainColumnInRange(int x, int z, int cameraX, int cameraZ, int radius) noexcept {
 const auto dx = static_cast<std::int64_t>(x) - cameraX;
 const auto dz = static_cast<std::int64_t>(z) - cameraZ;
 if(radius < 0 || std::abs(dx) > radius || std::abs(dz) > radius) return false;
 return dx * dx + dz * dz <= static_cast<std::int64_t>(radius) * radius;
}
namespace {
struct Quad {
 std::array<TessellatorVertex, 4> key;
 std::size_t source = 0;
 int u = 0;
 int v = 0;
 int axis = 0;
 int plane = 0;
};
float coordinate(const TessellatorVertex& vertex, int axis) {
 return axis == 0 ? vertex.x : axis == 1 ? vertex.y : vertex.z;
}
void setCoordinate(TessellatorVertex& vertex, int axis, float value) {
 if(axis == 0) vertex.x = value;
 else if(axis == 1) vertex.y = value;
 else vertex.z = value;
}
bool classify(const TessellatorVertex* vertices, Quad& out) {
 std::array<float, 3> low{vertices[0].x, vertices[0].y, vertices[0].z};
 auto high = low;
 for(int i = 0; i < 4; ++i) {
  if(vertices[i].color != vertices[0].color || vertices[i].light != vertices[0].light ||
     vertices[i].normal != vertices[0].normal) return false;
  for(int axis = 0; axis < 3; ++axis) {
   const float value = coordinate(vertices[i], axis);
   if(!std::isfinite(value) || value < 0.0f || value > 128.0f || value != std::floor(value)) return false;
   low[axis] = std::min(low[axis], value);
   high[axis] = std::max(high[axis], value);
  }
 }
 int flat = -1;
 for(int axis = 0; axis < 3; ++axis) {
  if(high[axis] == low[axis]) {
   if(flat >= 0) return false;
   flat = axis;
  } else if(high[axis] - low[axis] != 1.0f) return false;
 }
 if(flat < 0) return false;
 const auto normal = static_cast<std::uint32_t>(vertices[0].normal);
 for(int axis = 0; axis < 3; ++axis) {
  const int n = static_cast<std::int8_t>((normal >> (axis * 8)) & 255);
  if(axis == flat ? std::abs(n) != 127 : n != 0) return false;
 }
 const int uAxis = (flat + 1) % 3;
 const int vAxis = (flat + 2) % 3;
 unsigned corners = 0;
 for(int i = 0; i < 4; ++i) {
  const unsigned corner = (coordinate(vertices[i], uAxis) == high[uAxis] ? 1U : 0U) |
                          (coordinate(vertices[i], vAxis) == high[vAxis] ? 2U : 0U);
  corners |= 1U << corner;
  out.key[i] = vertices[i];
  out.key[i].x = out.key[i].y = out.key[i].z = 0;
  out.key[i].midBlock = static_cast<std::int32_t>(static_cast<std::uint32_t>(vertices[i].midBlock) & 0xff000000U);
 }
 if(corners != 15) return false;
 out.axis = flat;
 out.plane = static_cast<int>(low[flat]);
 out.u = static_cast<int>(low[uAxis]);
 out.v = static_cast<int>(low[vAxis]);
 return true;
}
int compareKey(const Quad& a, const Quad& b) {
 if(a.axis != b.axis) return a.axis < b.axis ? -1 : 1;
 if(a.plane != b.plane) return a.plane < b.plane ? -1 : 1;
 if(a.u / 16 != b.u / 16) return a.u / 16 < b.u / 16 ? -1 : 1;
 if(a.v / 16 != b.v / 16) return a.v / 16 < b.v / 16 ? -1 : 1;
 return std::memcmp(a.key.data(), b.key.data(), sizeof(a.key));
}
}
std::size_t simplifyTerrainMesh(TessellatorMesh& mesh, int level, std::span<const std::uint8_t> eligible) {
 if(level <= 0 || mesh.mode != 7 || mesh.vertices.size() % 4 != 0) return 0;
 const std::size_t count = mesh.vertices.size() / 4;
 if(eligible.size() != count) return 0;
 const int extent = 1 << (std::clamp(level, 1, 3) + 1);
 static thread_local std::vector<Quad> quads;
 static thread_local std::vector<std::size_t> order;
 static thread_local std::vector<std::uint8_t> removed;
 quads.clear();
 removed.assign(count, 0);
 for(std::size_t i = 0; i < count; ++i) {
  if(!eligible[i]) continue;
  Quad quad{};
  quad.source = i;
  if(classify(mesh.vertices.data() + i * 4, quad)) quads.push_back(quad);
 }
 order.resize(quads.size());
 for(std::size_t i = 0; i < order.size(); ++i) order[i] = i;
 std::sort(order.begin(), order.end(), [](std::size_t ai, std::size_t bi) {
  const Quad& a = quads[ai];
  const Quad& b = quads[bi];
  const int key = compareKey(a, b);
  if(key != 0) return key < 0;
  if(a.v != b.v) return a.v < b.v;
  if(a.u != b.u) return a.u < b.u;
  return a.source < b.source;
 });
 for(std::size_t begin = 0; begin < order.size();) {
  std::size_t end = begin + 1;
  while(end < order.size() && compareKey(quads[order[begin]], quads[order[end]]) == 0) ++end;
  std::array<int, 256> grid;
  grid.fill(-1);
  for(std::size_t i = begin; i < end; ++i) {
   const auto index = order[i];
   const int cell = (quads[index].v & 15) * 16 + (quads[index].u & 15);
   if(quads[index].u >= 128 || quads[index].v >= 128) continue;
   grid[cell] = grid[cell] == -1 ? static_cast<int>(index) : -2;
  }
  for(std::size_t i = begin; i < end; ++i) {
   const auto index = order[i];
   const Quad& quad = quads[index];
   const int cellU = quad.u & 15;
   const int cellV = quad.v & 15;
   if(quad.u >= 128 || quad.v >= 128 || grid[cellV * 16 + cellU] != static_cast<int>(index)) continue;
   const int maxWidth = extent - quad.u % extent;
   const int maxHeight = extent - quad.v % extent;
   int width = 1;
   while(width < maxWidth && grid[cellV * 16 + cellU + width] >= 0) ++width;
   int height = 1;
   while(height < maxHeight) {
    bool complete = true;
    for(int u = 0; u < width; ++u) {
     if(grid[(cellV + height) * 16 + cellU + u] < 0) complete = false;
    }
    if(!complete) break;
    ++height;
   }
   for(int v = 0; v < height; ++v) {
    for(int u = 0; u < width; ++u) {
     int& cell = grid[(cellV + v) * 16 + cellU + u];
     if(cell >= 0 && quads[cell].source != quad.source) removed[quads[cell].source] = 1;
     cell = -1;
    }
   }
   auto* vertices = mesh.vertices.data() + quad.source * 4;
   const int uAxis = (quad.axis + 1) % 3;
   const int vAxis = (quad.axis + 2) % 3;
   for(int c = 0; c < 4; ++c) {
    if(coordinate(vertices[c], uAxis) == static_cast<float>(quad.u + 1))
     setCoordinate(vertices[c], uAxis, static_cast<float>(quad.u + width));
    if(coordinate(vertices[c], vAxis) == static_cast<float>(quad.v + 1))
     setCoordinate(vertices[c], vAxis, static_cast<float>(quad.v + height));
   }
  }
  begin = end;
 }
 std::size_t write = 0;
 for(std::size_t i = 0; i < count; ++i) {
  if(removed[i]) continue;
  for(int c = 0; c < 4; ++c) mesh.vertices[write++] = mesh.vertices[i * 4 + c];
 }
 const std::size_t saved = mesh.vertices.size() - write;
 mesh.resizeCpuVertices(write);
 return saved;
}
}
