#include "net/minecraft/client/render/block/BlockFaceLighting.hpp"
#include <algorithm>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::client::render::block {
namespace {
// Per face: the axis the face is offset along (0=x, 1=y, 2=z) and its two in-plane
// axes u and v. Faces are the usual 0..5 (down, up, east, west, north, south).
struct FaceAxes {
 int uAxis;
 int vAxis;
};
constexpr FaceAxes kFaceAxes[6] = {
    {0, 2}, // down
    {0, 2}, // up
    {0, 1}, // east  (-Z)
    {0, 1}, // west  (+Z)
    {2, 1}, // north (-X)
    {2, 1}, // south (+X)
};
// Which geometric corner each face renderer emits as its 1st..4th vertex, as
// {uHigh, vHigh}. Transcribed from the six sampling blocks CubeBlockRenderer used
// to carry inline; that is the only reason this table is per-face rather than
// derived, and getting a row wrong reshades every block of that facing.
constexpr int kWindingOrder[6][4][2] = {
    {{0, 1}, {0, 0}, {1, 0}, {1, 1}}, // down
    {{1, 1}, {1, 0}, {0, 0}, {0, 1}}, // up
    {{0, 1}, {1, 1}, {1, 0}, {0, 0}}, // east
    {{0, 1}, {0, 0}, {1, 0}, {1, 1}}, // west
    {{1, 1}, {0, 1}, {0, 0}, {1, 0}}, // north
    {{1, 0}, {0, 0}, {0, 1}, {1, 1}}, // south
};
// The face's own 3x3 neighbourhood, in the face's plane, one sample per cell.
//
// Every corner of a face averages four cells -- the face cell, the two edge cells
// beside it and the diagonal -- and the four corners overlap heavily. Sampling per
// corner read these same nine cells 24 times for block ids and 17 times for light,
// for values that do not depend on which corner asked. Reading each once is where
// most of the AO cost went.
struct FaceNeighbourhood {
 struct Cell {
  int blockLight = 15;
  int skyLight = 15;
  float occlusion = 1.0f;
  // Vanilla's inner-corner test: an edge stops closing the diagonal off only when
  // its light opacity is zero. NOT material.blocksVision().
  bool transmitsLight = true;
 };
 // Indexed [1 + du][1 + dv] along the face's own u and v axes.
 Cell cells[3][3];
 [[nodiscard]] const Cell& at(int du, int dv) const noexcept {
  return cells[1 + du][1 + dv];
 }
};
FaceNeighbourhood sampleFaceNeighbourhood(BlockRenderContext& ctx, const FaceAxes& axes, const int base[3]) {
 using net::minecraft::block::Block;
 FaceNeighbourhood out;
 for(int du = -1; du <= 1; ++du) {
  for(int dv = -1; dv <= 1; ++dv) {
   int at[3] = {base[0], base[1], base[2]};
   at[axes.uAxis] += du;
   at[axes.vAxis] += dv;
   FaceNeighbourhood::Cell& cell = out.cells[1 + du][1 + dv];
   const int blockId = ctx.blockIdAt(at[0], at[1], at[2]);
   ctx.sampleFaceLight(at[0], at[1], at[2], blockId);
   cell.blockLight = ctx.faceBlockLight;
   cell.skyLight = ctx.faceSkyLight;
   const bool known = blockId > 0 && blockId < Block::BLOCK_COUNT;
   cell.occlusion = known && Block::BLOCKS_OPAQUE[static_cast<std::size_t>(blockId)] ? 0.2f : 1.0f;
   cell.transmitsLight = !known || Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(blockId)] == 0;
  }
 }
 return out;
}
// Vanilla's corner average: a zero channel means "no light recorded there", which
// reads as the face cell's own value rather than as darkness.
float averageCornerChannel(int diagonal, int side, int other, int center) {
 if(diagonal == 0) diagonal = center;
 if(side == 0) side = center;
 if(other == 0) other = center;
 return static_cast<float>(diagonal + side + other + center) / 4.0f;
}
} // namespace
FaceCornerSamples sampleCubeFaceCorners(BlockRenderContext& ctx, int face, int faceX, int faceY, int faceZ) {
 FaceCornerSamples out;
 if(face < 0 || face > 5) {
  return out;
 }
 out.face = face;
 const int base[3] = {faceX, faceY, faceZ};
 const FaceNeighbourhood neighbourhood = sampleFaceNeighbourhood(ctx, kFaceAxes[face], base);
 const FaceNeighbourhood::Cell& center = neighbourhood.at(0, 0);
 out.faceBlockLight = center.blockLight;
 out.faceSkyLight = center.skyLight;
 for(int uHigh = 0; uHigh < 2; ++uHigh) {
  for(int vHigh = 0; vHigh < 2; ++vHigh) {
   const int du = uHigh == 0 ? -1 : 1;
   const int dv = vHigh == 0 ? -1 : 1;
   const FaceNeighbourhood::Cell& side = neighbourhood.at(du, 0);
   const FaceNeighbourhood::Cell& other = neighbourhood.at(0, dv);
   // The diagonal is unreachable behind a closed inner corner, so vanilla reads
   // the edge in its place -- for the occlusion average exactly as for the light.
   const bool closed = !(side.transmitsLight || other.transmitsLight);
   const FaceNeighbourhood::Cell& diagonal = closed ? side : neighbourhood.at(du, dv);
   CornerSample& corner = out.corners[uHigh][vHigh];
   corner.blockLight =
       averageCornerChannel(diagonal.blockLight, side.blockLight, other.blockLight, center.blockLight);
   corner.skyLight = averageCornerChannel(diagonal.skyLight, side.skyLight, other.skyLight, center.skyLight);
   corner.occlusion = (diagonal.occlusion + side.occlusion + other.occlusion + center.occlusion) / 4.0f;
  }
 }
 // The face renderers read ctx.faceBlockLight/faceSkyLight out of the context when
 // they open their tessellator -- activeTess re-issues blockData with them -- and
 // what they have to see is this face's own cell. The old sampler left it there by
 // accident, its last corner average happening to read the centre last. Set it.
 ctx.faceBlockLight = center.blockLight;
 ctx.faceSkyLight = center.skyLight;
 return out;
}
void FaceCornerSamples::toWinding(CornerSample (&out)[4]) const {
 for(int i = 0; i < 4; ++i) {
  out[i] = corners[kWindingOrder[face][i][0]][kWindingOrder[face][i][1]];
 }
}
CornerSample FaceCornerSamples::blend(double lx, double ly, double lz) const {
 const FaceAxes axes = kFaceAxes[face];
 const double local[3] = {lx, ly, lz};
 const double u = std::clamp(local[axes.uAxis], 0.0, 1.0);
 const double v = std::clamp(local[axes.vAxis], 0.0, 1.0);
 const auto mix = [](float lo, float hi, double t) {
  return static_cast<float>(static_cast<double>(lo) + (static_cast<double>(hi) - static_cast<double>(lo)) * t);
 };
 CornerSample out;
 out.blockLight = mix(mix(corners[0][0].blockLight, corners[0][1].blockLight, v),
                      mix(corners[1][0].blockLight, corners[1][1].blockLight, v), u);
 out.skyLight = mix(mix(corners[0][0].skyLight, corners[0][1].skyLight, v),
                    mix(corners[1][0].skyLight, corners[1][1].skyLight, v), u);
 out.occlusion = mix(mix(corners[0][0].occlusion, corners[0][1].occlusion, v),
                     mix(corners[1][0].occlusion, corners[1][1].occlusion, v), u);
 return out;
}
} // namespace net::minecraft::client::render::block
