#include "net/minecraft/client/render/block/FallingBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::client::render::block {

void FallingBlockRenderer::renderFallingBlockEntity(net::minecraft::block::Block& block, net::minecraft::World* world, int x, int y, int z)
{
    ctx_.faceState.useAo = false;
    constexpr float shadeBottom = 0.5f;
    constexpr float shadeTop = 1.0f;
    constexpr float shadeNorthSouth = 0.8f;
    constexpr float shadeEastWest = 0.6f;
    Tessellator& tessellator = render::INSTANCE;
    tessellator.startQuads();
    float blockBrightness = block.getLuminance(world, x, y, z);
    float neighborBrightness = block.getLuminance(world, x, y - 1, z);
    if (neighborBrightness < blockBrightness) {
        neighborBrightness = blockBrightness;
    }
    tessellator.color(shadeBottom * neighborBrightness, shadeBottom * neighborBrightness, shadeBottom * neighborBrightness);
    faces_.renderBottomFace(block, -0.5, -0.5, -0.5, block.getTexture(0));
    neighborBrightness = block.getLuminance(world, x, y + 1, z);
    if (neighborBrightness < blockBrightness) {
        neighborBrightness = blockBrightness;
    }
    tessellator.color(shadeTop * neighborBrightness, shadeTop * neighborBrightness, shadeTop * neighborBrightness);
    faces_.renderTopFace(block, -0.5, -0.5, -0.5, block.getTexture(1));
    neighborBrightness = block.getLuminance(world, x, y, z - 1);
    if (neighborBrightness < blockBrightness) {
        neighborBrightness = blockBrightness;
    }
    tessellator.color(shadeNorthSouth * neighborBrightness, shadeNorthSouth * neighborBrightness, shadeNorthSouth * neighborBrightness);
    faces_.renderEastFace(block, -0.5, -0.5, -0.5, block.getTexture(2));
    neighborBrightness = block.getLuminance(world, x, y, z + 1);
    if (neighborBrightness < blockBrightness) {
        neighborBrightness = blockBrightness;
    }
    tessellator.color(shadeNorthSouth * neighborBrightness, shadeNorthSouth * neighborBrightness, shadeNorthSouth * neighborBrightness);
    faces_.renderWestFace(block, -0.5, -0.5, -0.5, block.getTexture(3));
    neighborBrightness = block.getLuminance(world, x - 1, y, z);
    if (neighborBrightness < blockBrightness) {
        neighborBrightness = blockBrightness;
    }
    tessellator.color(shadeEastWest * neighborBrightness, shadeEastWest * neighborBrightness, shadeEastWest * neighborBrightness);
    faces_.renderNorthFace(block, -0.5, -0.5, -0.5, block.getTexture(4));
    neighborBrightness = block.getLuminance(world, x + 1, y, z);
    if (neighborBrightness < blockBrightness) {
        neighborBrightness = blockBrightness;
    }
    tessellator.color(shadeEastWest * neighborBrightness, shadeEastWest * neighborBrightness, shadeEastWest * neighborBrightness);
    faces_.renderSouthFace(block, -0.5, -0.5, -0.5, block.getTexture(5));
    tessellator.draw();
}

} // namespace net::minecraft::client::render::block
