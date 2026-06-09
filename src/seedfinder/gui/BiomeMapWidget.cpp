#include "seedfinder/gui/BiomeMapWidget.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/Biomes.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"

#include <algorithm>
#include <vector>

namespace net::minecraft::client::gui::widget {
namespace {

constexpr int kMaxTextureSize = 256;

} // namespace

BiomeMapWidget::~BiomeMapWidget()
{
}

void BiomeMapWidget::clear(Minecraft& minecraft)
{
    if (textureId_ != 0) {
        minecraft.textureManager.deleteTexture(textureId_);
        textureId_ = 0;
    }
    textureSize_ = 0;
    hasMap_ = false;
    seed_ = 0;
}

std::uint32_t BiomeMapWidget::biomeColor(std::uint8_t biomeId)
{
    Biomes::init();
    const auto id = static_cast<BiomeId>(biomeId);
    const int grass = Biomes::byId(id).grassColor;
    return 0xFF000000U | static_cast<std::uint32_t>(grass & 0xFFFFFF);
}

void BiomeMapWidget::build(
    Minecraft& minecraft,
    std::uint64_t seed,
    int centerBlockX,
    int centerBlockZ,
    int radiusChunks)
{
    clear(minecraft);

    const int radiusBlocks = std::max(16, radiusChunks * 16);
    const int blocksPerSide = radiusBlocks * 2;
    const int textureSize = std::min(kMaxTextureSize, blocksPerSide);
    const int step = std::max(1, blocksPerSide / textureSize);

    const int startX = centerBlockX - radiusBlocks;
    const int startZ = centerBlockZ - radiusBlocks;

    net::minecraft::BiomeSource source(static_cast<std::int64_t>(seed));
    std::vector<net::minecraft::BiomeInfo> biomes;
    source.getBiomesInArea(biomes, startX, startZ, blocksPerSide, blocksPerSide);

    texture::RasterImage image {};
    image.width = textureSize;
    image.height = textureSize;
    image.argb.resize(static_cast<std::size_t>(textureSize * textureSize), 0xFF101010U);

    for (int py = 0; py < textureSize; ++py) {
        for (int px = 0; px < textureSize; ++px) {
            const int bx = std::min(blocksPerSide - 1, px * step);
            const int bz = std::min(blocksPerSide - 1, py * step);
            const std::size_t biomeIndex =
                static_cast<std::size_t>(bx + bz * blocksPerSide);
            if (biomeIndex >= biomes.size()) {
                continue;
            }
            const std::uint8_t biomeId = static_cast<std::uint8_t>(biomes[biomeIndex].id);
            image.argb[static_cast<std::size_t>(py * textureSize + px)] = biomeColor(biomeId);
        }
    }

    textureId_ = minecraft.textureManager.load(image);
    textureSize_ = textureSize;
    seed_ = seed;

    const int spawnOffsetX = centerBlockX - startX;
    const int spawnOffsetZ = centerBlockZ - startZ;
    spawnPixelX_ = std::clamp(spawnOffsetX / step, 0, textureSize - 1);
    spawnPixelZ_ = std::clamp(spawnOffsetZ / step, 0, textureSize - 1);
    hasMap_ = textureId_ != 0;
}

void BiomeMapWidget::render(Minecraft& minecraft, int x, int y, int width, int height)
{
    fill(x - 1, y - 1, x + width + 1, y + height + 1, 0xFF303030U);
    fill(x, y, x + width, y + height, 0xFF101010U);

    if (!hasMap_ || textureId_ == 0 || textureSize_ <= 0) {
        if (minecraft.textRenderer != nullptr) {
            drawCenteredTextWithShadow(*minecraft.textRenderer, "Select a seed to preview map", x + width / 2,
                y + height / 2 - 4, 0x808080);
        }
        return;
    }

    gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, textureId_);
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);

    render::Tessellator& tessellator = render::Tessellator::INSTANCE;
    tessellator.startQuads();
    tessellator.vertex(static_cast<double>(x), static_cast<double>(y + height), 0.0, 0.0, 1.0);
    tessellator.vertex(static_cast<double>(x + width), static_cast<double>(y + height), 0.0, 1.0, 1.0);
    tessellator.vertex(static_cast<double>(x + width), static_cast<double>(y), 0.0, 1.0, 0.0);
    tessellator.vertex(static_cast<double>(x), static_cast<double>(y), 0.0, 0.0, 0.0);
    tessellator.draw();

    const float scaleX = static_cast<float>(width) / static_cast<float>(textureSize_);
    const float scaleY = static_cast<float>(height) / static_cast<float>(textureSize_);
    const int markerX = x + static_cast<int>(static_cast<float>(spawnPixelX_) * scaleX);
    const int markerY = y + static_cast<int>(static_cast<float>(spawnPixelZ_) * scaleY);

    gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
    constexpr int markerRadius = 3;
    fill(markerX - markerRadius, markerY - 1, markerX + markerRadius + 1, markerY + 2, 0xFFFF4040U);
    fill(markerX - 1, markerY - markerRadius, markerX + 2, markerY + markerRadius + 1, 0xFFFF4040U);
    fill(markerX, markerY, markerX + 1, markerY + 1, 0xFFFFFFFFU);
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
}

} // namespace net::minecraft::client::gui::widget
