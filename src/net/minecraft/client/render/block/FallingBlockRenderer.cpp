#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderers.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::render::block {
void FallingBlockRenderer::renderFallingBlockEntity(
    net::minecraft::block::Block& block, net::minecraft::World* world, int x, int y, int z) {
 ctx_.faceState.useAo = false;
 ctx_.renderBounds = block.getCollisionShapeLocal();
 // A falling block is drawn as a loose entity, so point the light sampler at
 // the world it is falling through rather than any meshing region.
 ctx_.lightRegion = nullptr;
 ctx_.lightWorld = world;
 ctx_.blockEmission = block.emission();
 constexpr float shadeBottom = 0.5f;
 constexpr float shadeTop = 1.0f;
 constexpr float shadeNorthSouth = 0.8f;
 constexpr float shadeEastWest = 0.6f;
 Tessellator& tessellator = *ctx_.tess;
 tessellator.startQuads();
 // The lightmap supplies the absolute light level; the colour keeps the shade.
 const float neighborBrightness = 1.0f;
 ctx_.sampleFaceLight(x, y - 1, z);
 tessellator.color(
     shadeBottom * neighborBrightness, shadeBottom * neighborBrightness, shadeBottom * neighborBrightness);
 faces_.renderBottomFace(block, -0.5, -0.5, -0.5, block.getTexture(0));
 ctx_.sampleFaceLight(x, y + 1, z);
 tessellator.color(shadeTop * neighborBrightness, shadeTop * neighborBrightness, shadeTop * neighborBrightness);
 faces_.renderTopFace(block, -0.5, -0.5, -0.5, block.getTexture(1));
 ctx_.sampleFaceLight(x, y, z - 1);
 tessellator.color(shadeNorthSouth * neighborBrightness,
                   shadeNorthSouth * neighborBrightness,
                   shadeNorthSouth * neighborBrightness);
 faces_.renderEastFace(block, -0.5, -0.5, -0.5, block.getTexture(2));
 ctx_.sampleFaceLight(x, y, z + 1);
 tessellator.color(shadeNorthSouth * neighborBrightness,
                   shadeNorthSouth * neighborBrightness,
                   shadeNorthSouth * neighborBrightness);
 faces_.renderWestFace(block, -0.5, -0.5, -0.5, block.getTexture(3));
 ctx_.sampleFaceLight(x - 1, y, z);
 tessellator.color(
     shadeEastWest * neighborBrightness, shadeEastWest * neighborBrightness, shadeEastWest * neighborBrightness);
 faces_.renderNorthFace(block, -0.5, -0.5, -0.5, block.getTexture(4));
 ctx_.sampleFaceLight(x + 1, y, z);
 tessellator.color(
     shadeEastWest * neighborBrightness, shadeEastWest * neighborBrightness, shadeEastWest * neighborBrightness);
 faces_.renderSouthFace(block, -0.5, -0.5, -0.5, block.getTexture(5));
 tessellator.draw();
}
} // namespace net::minecraft::client::render::block
