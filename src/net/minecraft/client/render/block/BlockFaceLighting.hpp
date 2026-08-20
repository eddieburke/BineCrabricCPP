#pragma once
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"
namespace net::minecraft::client::render::block {
struct CornerSample {
 float blockLight = 15.0f;
 float skyLight = 15.0f;
 float occlusion = 1.0f;
};
bool edgeTransmitsLight(const net::minecraft::BlockView* blockView, int x, int y, int z);
float cornerOcclusion(const net::minecraft::BlockView* blockView, int x, int y, int z);
void readCornerLight(BlockRenderContext& ctx, int x, int y, int z, int& blockLight, int& skyLight);
float averageCornerChannel(int diagonal, int side, int other, int center);
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
                        CornerSample& corner);
// The four corners of one cube face, indexed GEOMETRICALLY as [uHigh][vHigh] in
// the face's own plane. Both consumers read the same samples: CubeBlockRenderer
// reorders them to its face renderers' winding, baked model quads interpolate
// between them because their vertex order comes from the model JSON.
// `face` is the usual 0..5 (down, up, east, west, north, south) and is carried in
// the result so no consumer has to hand it back to look up the same axes again.
struct FaceCornerSamples {
 CornerSample corners[2][2];
 int face = 0;
 int faceBlockLight = 15;
 int faceSkyLight = 15;
 // Bilinear blend at a point given in block-local 0..1 coordinates.
 [[nodiscard]] CornerSample blend(double lx, double ly, double lz) const;
 // The same four corners in the order the face renderer for `face` emits its
 // vertices, which is what assignAoCorners indexes.
 void toWinding(CornerSample (&out)[4]) const;
};
// (faceX, faceY, faceZ) is the cell the face samples its light from — the caller
// resolves it, because a partial block's face sits on its own cell rather than
// the neighbour's (renderBounds), and only the caller knows its bounds.
FaceCornerSamples sampleCubeFaceCorners(BlockRenderContext& ctx, int face, int faceX, int faceY, int faceZ);
} // namespace net::minecraft::client::render::block
