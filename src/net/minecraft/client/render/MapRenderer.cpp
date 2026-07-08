#include "net/minecraft/client/render/MapRenderer.hpp"

#include "net/minecraft/block/MapColor.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/map/MapState.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"

namespace net::minecraft::client::render {
namespace {
struct TexturedCorner {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
};

void appendTransformedQuad(Tessellator& tessellator,
                           const net::minecraft::util::math::Matrix4f& model,
                           const TexturedCorner (&corners)[4]) {
    for (int i = 0; i < 4; ++i) {
        const TexturedCorner& corner = corners[i];
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        model.transformPoint(corner.x, corner.y, corner.z, x, y, z);
        tessellator.vertex(x, y, z, corner.u, corner.v);
    }
}
}  // namespace

MapRenderer::MapRenderer(font::TextRenderer* textRendererIn,
                         net::minecraft::client::texture::TextureManager* textureManagerIn)
    : textRenderer_(textRendererIn) {
    colors_.fill(0);
    if (textureManagerIn != nullptr) {
        net::minecraft::client::texture::RasterImage image{};
        image.width = 128;
        image.height = 128;
        image.argb.resize(128 * 128, 0);
        texture_ = textureManagerIn->load(image);
    }
}

void MapRenderer::render(net::minecraft::PlayerEntity& player,
                         net::minecraft::client::texture::TextureManager& textureManagerIn,
                         const net::minecraft::map::MapState& mapState) {
    (void) player;
    for (int n = 0; n < 16384; ++n) {
        const std::uint8_t colorByte = mapState.colors[static_cast<std::size_t>(n)];
        if (colorByte / 4 == 0) {
            colors_[static_cast<std::size_t>(n)] = static_cast<std::int32_t>((((n + n / 128) & 1) * 8) + 16) << 24;
            continue;
        }
        const std::size_t colorIndex = static_cast<std::size_t>(colorByte / 4);
        if (colorIndex >= net::minecraft::block::MapColor::COLORS.size() ||
            net::minecraft::block::MapColor::COLORS[colorIndex] == nullptr) {
            colors_[static_cast<std::size_t>(n)] = 0;
            continue;
        }
        int rgb = net::minecraft::block::MapColor::COLORS[colorIndex]->color;
        const int shade = colorByte & 3;
        int brightness = 220;
        if (shade == 2) {
            brightness = 255;
        } else if (shade == 0) {
            brightness = 180;
        }
        int r = ((rgb >> 16) & 0xFF) * brightness / 255;
        int g = ((rgb >> 8) & 0xFF) * brightness / 255;
        int b = (rgb & 0xFF) * brightness / 255;
        colors_[static_cast<std::size_t>(n)] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
    if (texture_ != 0) {
        net::minecraft::client::texture::RasterImage upload{};
        upload.width = 128;
        upload.height = 128;
        upload.argb.resize(128 * 128);
        for (int i = 0; i < 16384; ++i) {
            upload.argb[static_cast<std::size_t>(i)] = static_cast<std::uint32_t>(colors_[static_cast<std::size_t>(i)]);
        }
        textureManagerIn.load(upload, texture_);
    }
    const int originX = 0;
    const int originY = 0;
    Tessellator& tessellator = Tessellator::INSTANCE;
    const gl::preset::TexturedGuiNoAlphaTest guiCaps;
    const gl::preset::BlockOverlay alphaCaps;
    const float inset = 0.0f;
    gl::bindTexture(gl::cap::Texture2D, texture_);
    tessellator.startQuads();
    tessellator.vertex(
        static_cast<float>(originX) + inset, static_cast<float>(originY + 128) - inset, -0.01f, 0.0f, 1.0f);
    tessellator.vertex(
        static_cast<float>(originX + 128) - inset, static_cast<float>(originY + 128) - inset, -0.01f, 1.0f, 1.0f);
    tessellator.vertex(
        static_cast<float>(originX + 128) - inset, static_cast<float>(originY) + inset, -0.01f, 1.0f, 0.0f);
    tessellator.vertex(static_cast<float>(originX) + inset, static_cast<float>(originY) + inset, -0.01f, 0.0f, 0.0f);
    tessellator.draw();
    if (!mapState.icons.empty()) {
        textureManagerIn.bindTexture(textureManagerIn.getTextureId("misc/mapicons.png"));
        float baseModelView[16]{};
        gl::getFloatv(gl::matrix_::ModelViewMatrix, baseModelView);
        tessellator.startQuads();
        for (const net::minecraft::map::MapState::MapIcon& mapIcon : mapState.icons) {
            net::minecraft::util::math::Matrix4f model;
            model.set(baseModelView);
            model.translate(static_cast<float>(originX) + static_cast<float>(mapIcon.x) / 2.0f + 64.0f,
                            static_cast<float>(originY) + static_cast<float>(mapIcon.z) / 2.0f + 64.0f,
                            -0.02f);
            model.rotate(static_cast<float>(mapIcon.rotation * 360) / 16.0f, 0.0f, 0.0f, 1.0f);
            model.scale(4.0f, 4.0f, 3.0f);
            model.translate(-0.125f, 0.125f, 0.0f);
            const float u0 = static_cast<float>(mapIcon.type % 4) / 4.0f;
            const float v0 = static_cast<float>(mapIcon.type / 4) / 4.0f;
            const float u1 = static_cast<float>(mapIcon.type % 4 + 1) / 4.0f;
            const float v1 = static_cast<float>(mapIcon.type / 4 + 1) / 4.0f;
            const TexturedCorner corners[4] = {
                {-1.0f, 1.0f, 0.0f, u0, v0},
                {1.0f, 1.0f, 0.0f, u1, v0},
                {1.0f, -1.0f, 0.0f, u1, v1},
                {-1.0f, -1.0f, 0.0f, u0, v1},
            };
            appendTransformedQuad(tessellator, model, corners);
        }
        tessellator.draw();
    }
    if (textRenderer_ != nullptr) {
        gl::pushMatrix();
        gl::translatef(0.0f, 0.0f, -0.04f);
        textRenderer_->draw(mapState.id, originX, originY, static_cast<int>(0xFF000000u));
        gl::popMatrix();
    }
}
}  // namespace net::minecraft::client::render
