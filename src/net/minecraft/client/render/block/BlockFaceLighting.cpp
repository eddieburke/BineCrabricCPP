#include "net/minecraft/client/render/block/BlockFaceLighting.hpp"
#include <algorithm>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::client::render::block {
bool edgeTransmitsLight(const net::minecraft::BlockView* blockView, int x, int y, int z) {
 if(blockView == nullptr) {
  return true;
 }
 const int blockId = blockView->getBlockId(x, y, z);
 if(blockId <= 0 || blockId >= net::minecraft::block::Block::BLOCK_COUNT) {
  return true;
 }
 return net::minecraft::block::Block::BLOCKS_LIGHT_OPACITY[static_cast<std::size_t>(blockId)] == 0;
}
float cornerOcclusion(const net::minecraft::BlockView* blockView, int x, int y, int z) {
 if(blockView == nullptr) {
  return 1.0f;
 }
 const int blockId = blockView->getBlockId(x, y, z);
 if(blockId <= 0 || blockId >= net::minecraft::block::Block::BLOCK_COUNT) {
  return 1.0f;
 }
 return net::minecraft::block::Block::BLOCKS_OPAQUE[static_cast<std::size_t>(blockId)] ? 0.2f : 1.0f;
}
void readCornerLight(BlockRenderContext& ctx, int x, int y, int z, int& blockLight, int& skyLight) {
 ctx.sampleFaceLight(x, y, z);
 blockLight = ctx.faceBlockLight;
 skyLight = ctx.faceSkyLight;
}
float averageCornerChannel(int diagonal, int side, int other, int center) {
 if(diagonal == 0) diagonal = center;
 if(side == 0) side = center;
 if(other == 0) other = center;
 return static_cast<float>(diagonal + side + other + center) / 4.0f;
}
void averageCornerLight(BlockRenderContext& ctx,
                        int dx,
                        int dy,
                        int dz,
                        int sx,
                        int sy,
                        int sz,
                        int ox,
                        int oy,
                        int oz,
                        int cx,
                        int cy,
                        int cz,
                        bool closed,
                        CornerSample& corner) {
 int bd = 0;
 int sd = 0;
 int bs = 0;
 int ss = 0;
 int bo = 0;
 int so = 0;
 int bc = 0;
 int sc = 0;
 readCornerLight(ctx, dx, dy, dz, bd, sd);
 readCornerLight(ctx, sx, sy, sz, bs, ss);
 readCornerLight(ctx, ox, oy, oz, bo, so);
 readCornerLight(ctx, cx, cy, cz, bc, sc);
 float od = cornerOcclusion(ctx.blockView, dx, dy, dz);
 const float os = cornerOcclusion(ctx.blockView, sx, sy, sz);
 const float oo = cornerOcclusion(ctx.blockView, ox, oy, oz);
 const float oc = cornerOcclusion(ctx.blockView, cx, cy, cz);
 if(closed) {
  bd = bs;
  sd = ss;
  // The diagonal is unreachable behind a closed inner corner, so vanilla reads
  // the edge in its place — for the occlusion average exactly as for the light.
  od = os;
 }
 corner.blockLight = averageCornerChannel(bd, bs, bo, bc);
 corner.skyLight = averageCornerChannel(sd, ss, so, sc);
 corner.occlusion = (od + os + oo + oc) / 4.0f;
}
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
} // namespace
FaceCornerSamples sampleCubeFaceCorners(BlockRenderContext& ctx, int face, int faceX, int faceY, int faceZ) {
 FaceCornerSamples out;
 if(face < 0 || face > 5) {
  return out;
 }
 out.face = face;
 const FaceAxes axes = kFaceAxes[face];
 const int base[3] = {faceX, faceY, faceZ};
 ctx.sampleFaceLight(faceX, faceY, faceZ);
 out.faceBlockLight = ctx.faceBlockLight;
 out.faceSkyLight = ctx.faceSkyLight;
 for(int uHigh = 0; uHigh < 2; ++uHigh) {
  for(int vHigh = 0; vHigh < 2; ++vHigh) {
   int side[3] = {base[0], base[1], base[2]};
   side[axes.uAxis] += uHigh == 0 ? -1 : 1;
   int other[3] = {base[0], base[1], base[2]};
   other[axes.vAxis] += vHigh == 0 ? -1 : 1;
   const int diagonal[3] = {side[0] + other[0] - base[0], side[1] + other[1] - base[1],
                            side[2] + other[2] - base[2]};
   const bool closed = !(edgeTransmitsLight(ctx.blockView, side[0], side[1], side[2]) ||
                         edgeTransmitsLight(ctx.blockView, other[0], other[1], other[2]));
   averageCornerLight(ctx, diagonal[0], diagonal[1], diagonal[2], side[0], side[1], side[2], other[0],
                      other[1], other[2], base[0], base[1], base[2], closed, out.corners[uHigh][vHigh]);
  }
 }
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
