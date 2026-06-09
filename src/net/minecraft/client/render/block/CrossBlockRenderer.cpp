#include "net/minecraft/client/render/block/CrossBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/BlockView.hpp"

namespace net::minecraft::client::render::block {

namespace option = net::minecraft::client::option;

bool CrossBlockRenderer::render(net::minecraft::block::Block& block, int x, int y, int z)
{
    Tessellator& tessellator = render::INSTANCE;
    float brightness = block.getLuminance(ctx_.blockView, x, y, z);
    int colorMult = block.getColorMultiplier(ctx_.blockView, x, y, z);
    float red = (float)(colorMult >> 16 & 0xFF) / 255.0f;
    float green = (float)(colorMult >> 8 & 0xFF) / 255.0f;
    float blue = (float)(colorMult & 0xFF) / 255.0f;
    tessellator.color(brightness * red, brightness * green, brightness * blue);
    double dx = x;
    double dy = y;
    double dz = z;
    if (net::minecraft::block::Block::GRASS != nullptr && &block == net::minecraft::block::Block::GRASS
        && (Minecraft::INSTANCE == nullptr || option::resolve(Minecraft::INSTANCE->options).fancyGrass)) {
        std::int64_t l = (long)(x * 3129871) ^ (long)z * 116129781L ^ (long)y;
        l = l * l * 42317861L + l * 11L;
        dx += ((double)((float)(l >> 16 & 0xFL) / 15.0f) - 0.5) * 0.5;
        dy += ((double)((float)(l >> 20 & 0xFL) / 15.0f) - 1.0) * 0.2;
        dz += ((double)((float)(l >> 24 & 0xFL) / 15.0f) - 0.5) * 0.5;
    }
    render(block, ctx_.blockView->getBlockMeta(x, y, z), dx, dy, dz);
    return true;
}

void CrossBlockRenderer::render(net::minecraft::block::Block& block, int metadata, double x, double y, double z)
{
    Tessellator& tessellator = render::INSTANCE;
    int tex = block.getTexture(0, metadata);
    if (ctx_.textureOverride >= 0) {
        tex = ctx_.textureOverride;
    }
    int texU = (tex & 0xF) << 4;
    int texV = tex & 0xF0;
    double uMin = (float)texU / 256.0f;
    double uMax = ((float)texU + 15.99f) / 256.0f;
    double vMin = (float)texV / 256.0f;
    double vMax = ((float)texV + 15.99f) / 256.0f;
    double x0 = x + 0.5 - (double)0.45f;
    double x1 = x + 0.5 + (double)0.45f;
    double z0 = z + 0.5 - (double)0.45f;
    double z1 = z + 0.5 + (double)0.45f;
    tessellator.vertex(x0, y + 1.0, z0, uMin, vMin);
    tessellator.vertex(x0, y + 0.0, z0, uMin, vMax);
    tessellator.vertex(x1, y + 0.0, z1, uMax, vMax);
    tessellator.vertex(x1, y + 1.0, z1, uMax, vMin);
    tessellator.vertex(x1, y + 1.0, z1, uMin, vMin);
    tessellator.vertex(x1, y + 0.0, z1, uMin, vMax);
    tessellator.vertex(x0, y + 0.0, z0, uMax, vMax);
    tessellator.vertex(x0, y + 1.0, z0, uMax, vMin);
    tessellator.vertex(x0, y + 1.0, z1, uMin, vMin);
    tessellator.vertex(x0, y + 0.0, z1, uMin, vMax);
    tessellator.vertex(x1, y + 0.0, z0, uMax, vMax);
    tessellator.vertex(x1, y + 1.0, z0, uMax, vMin);
    tessellator.vertex(x1, y + 1.0, z0, uMin, vMin);
    tessellator.vertex(x1, y + 0.0, z0, uMin, vMax);
    tessellator.vertex(x0, y + 0.0, z1, uMax, vMax);
    tessellator.vertex(x0, y + 1.0, z1, uMax, vMin);
}

} // namespace net::minecraft::client::render::block
