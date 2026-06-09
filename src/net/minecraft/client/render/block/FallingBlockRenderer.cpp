#include "net/minecraft/client/render/block/FallingBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::client::render::block {

void FallingBlockRenderer::renderFallingBlockEntity(net::minecraft::block::Block& block, net::minecraft::World* world, int x, int y, int z)
{
    ctx_.faceState.useAo = false;
    float f = 0.5f;
    float f2 = 1.0f;
    float f3 = 0.8f;
    float f4 = 0.6f;
    Tessellator& tessellator = render::INSTANCE;
    tessellator.startQuads();
    float f5 = block.getLuminance(world, x, y, z);
    float f6 = block.getLuminance(world, x, y - 1, z);
    if (f6 < f5) { f6 = f5; }
    tessellator.color(f * f6, f * f6, f * f6);
    faces_.renderBottomFace(block, -0.5, -0.5, -0.5, block.getTexture(0));
    f6 = block.getLuminance(world, x, y + 1, z);
    if (f6 < f5) { f6 = f5; }
    tessellator.color(f2 * f6, f2 * f6, f2 * f6);
    faces_.renderTopFace(block, -0.5, -0.5, -0.5, block.getTexture(1));
    f6 = block.getLuminance(world, x, y, z - 1);
    if (f6 < f5) { f6 = f5; }
    tessellator.color(f3 * f6, f3 * f6, f3 * f6);
    faces_.renderEastFace(block, -0.5, -0.5, -0.5, block.getTexture(2));
    f6 = block.getLuminance(world, x, y, z + 1);
    if (f6 < f5) { f6 = f5; }
    tessellator.color(f3 * f6, f3 * f6, f3 * f6);
    faces_.renderWestFace(block, -0.5, -0.5, -0.5, block.getTexture(3));
    f6 = block.getLuminance(world, x - 1, y, z);
    if (f6 < f5) { f6 = f5; }
    tessellator.color(f4 * f6, f4 * f6, f4 * f6);
    faces_.renderNorthFace(block, -0.5, -0.5, -0.5, block.getTexture(4));
    f6 = block.getLuminance(world, x + 1, y, z);
    if (f6 < f5) { f6 = f5; }
    tessellator.color(f4 * f6, f4 * f6, f4 * f6);
    faces_.renderSouthFace(block, -0.5, -0.5, -0.5, block.getTexture(5));
    tessellator.draw();
}

} // namespace net::minecraft::client::render::block
