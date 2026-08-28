#pragma once
#include "net/minecraft/client/render/block/BlockRenderContext.hpp"
namespace net::minecraft::client::render::block {
struct CornerSample {
 float blockLight = 15.0f;
 float skyLight = 15.0f;
 float occlusion = 1.0f;
};
// Corners are indexed geometrically as [uHigh][vHigh] in the face's own plane, not
// in any renderer's winding. `face` is the usual 0..5 (down, up, east, west, north,
// south).
struct FaceCornerSamples {
 CornerSample corners[2][2];
 int face = 0;
 // Bilinear blend at a block-local 0..1 point.
 [[nodiscard]] CornerSample blend(double lx, double ly, double lz) const;
 // The same corners reordered to the winding the face renderer for `face` emits.
 void toWinding(CornerSample (&out)[4]) const;
};
// (faceX, faceY, faceZ) is the cell the face samples its light from; only the caller
// knows its renderBounds, so the caller resolves it.
FaceCornerSamples sampleCubeFaceCorners(BlockRenderContext& ctx, int face, int faceX, int faceY, int faceZ);
} // namespace net::minecraft::client::render::block
