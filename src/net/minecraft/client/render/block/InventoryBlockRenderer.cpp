#include "net/minecraft/client/render/block/InventoryBlockRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderType.hpp"

namespace net::minecraft::client::render::block {

namespace {

// Same fake-cube shade multipliers as CubeBlockRenderer::renderFlat.
constexpr float kShadeBottom = 0.5f;
constexpr float kShadeTop = 1.0f;
constexpr float kShadeHoriz = 0.8f;
constexpr float kShadeNorthSouth = 0.6f;

struct ScopedDisableLighting {
    ScopedDisableLighting() { net::minecraft::client::gl::GL11::glDisable(net::minecraft::client::gl::GL11::GL_LIGHTING); }
    ~ScopedDisableLighting() { net::minecraft::client::gl::GL11::glEnable(net::minecraft::client::gl::GL11::GL_LIGHTING); }

    ScopedDisableLighting(const ScopedDisableLighting&) = delete;
    ScopedDisableLighting& operator=(const ScopedDisableLighting&) = delete;
};

void applyInventoryFaceColor(float shade, float red, float green, float blue, float brightness)
{
    net::minecraft::client::gl::GL11::glColor4f(red * shade * brightness, green * shade * brightness, blue * shade * brightness,
        1.0f);
}

void drawInventoryCubeFaces(BlockFaceRenderer& faces, net::minecraft::block::Block& block, int metadata, float red, float green,
    float blue, float brightness)
{
    Tessellator& tessellator = render::INSTANCE;

    applyInventoryFaceColor(kShadeBottom, red, green, blue, brightness);
    tessellator.startQuads();
    tessellator.normal(0.0f, -1.0f, 0.0f);
    faces.renderBottomFace(block, 0.0, 0.0, 0.0, block.getTexture(0, metadata));
    tessellator.draw();

    applyInventoryFaceColor(kShadeTop, red, green, blue, brightness);
    tessellator.startQuads();
    tessellator.normal(0.0f, 1.0f, 0.0f);
    faces.renderTopFace(block, 0.0, 0.0, 0.0, block.getTexture(1, metadata));
    tessellator.draw();

    applyInventoryFaceColor(kShadeHoriz, red, green, blue, brightness);
    tessellator.startQuads();
    tessellator.normal(0.0f, 0.0f, -1.0f);
    faces.renderEastFace(block, 0.0, 0.0, 0.0, block.getTexture(2, metadata));
    tessellator.draw();

    applyInventoryFaceColor(kShadeHoriz, red, green, blue, brightness);
    tessellator.startQuads();
    tessellator.normal(0.0f, 0.0f, 1.0f);
    faces.renderWestFace(block, 0.0, 0.0, 0.0, block.getTexture(3, metadata));
    tessellator.draw();

    applyInventoryFaceColor(kShadeNorthSouth, red, green, blue, brightness);
    tessellator.startQuads();
    tessellator.normal(-1.0f, 0.0f, 0.0f);
    faces.renderNorthFace(block, 0.0, 0.0, 0.0, block.getTexture(4, metadata));
    tessellator.draw();

    applyInventoryFaceColor(kShadeNorthSouth, red, green, blue, brightness);
    tessellator.startQuads();
    tessellator.normal(1.0f, 0.0f, 0.0f);
    faces.renderSouthFace(block, 0.0, 0.0, 0.0, block.getTexture(5, metadata));
    tessellator.draw();
}

} // namespace

void InventoryBlockRenderer::render(net::minecraft::block::Block& block, int metadata, float brightness)
{
    const int renderType = block.getRenderType();
    Tessellator& tessellator = render::INSTANCE;

    float red = 1.0f;
    float green = 1.0f;
    float blue = 1.0f;
    if (ctx_.inventoryColorEnabled) {
        const int blockColor = block.getColor(metadata);
        red = static_cast<float>(blockColor >> 16 & 0xFF) / 255.0f;
        green = static_cast<float>(blockColor >> 8 & 0xFF) / 255.0f;
        blue = static_cast<float>(blockColor & 0xFF) / 255.0f;
    }

    // Shared faceState persists from the last world render; if AO is left enabled the
    // face renderers emit stale per-vertex colors that override the inventory glColor
    // (symptom: leaves/tinted blocks render gray in the inventory). Force flat faces.
    ctx_.faceState.useAo = false;

    // Beta relied on fixed-function lighting + normals here; bake per-face shade into
    // glColor instead (matches CubeBlockRenderer::renderFlat and avoids fullbright icons
    // when lighting state is wrong).
    const ScopedDisableLighting disableLighting;

    if (renderType == BlockRenderType::FULL_CUBE || renderType == BlockRenderType::PISTON) {
        if (renderType == BlockRenderType::PISTON) {
            metadata = 1;
        }
        block.setupRenderBoundingBox();
        net::minecraft::client::gl::GL11::glTranslatef(-0.5f, -0.5f, -0.5f);
        drawInventoryCubeFaces(faces_, block, metadata, red, green, blue, brightness);
        net::minecraft::client::gl::GL11::glTranslatef(0.5f, 0.5f, 0.5f);
    } else if (renderType == BlockRenderType::CROSS) {
        applyInventoryFaceColor(1.0f, red, green, blue, brightness);
        tessellator.startQuads();
        tessellator.normal(0.0f, -1.0f, 0.0f);
        cross_.render(block, metadata, -0.5, -0.5, -0.5);
        tessellator.draw();
    } else if (renderType == BlockRenderType::CACTUS) {
        block.setupRenderBoundingBox();
        net::minecraft::client::gl::GL11::glTranslatef(-0.5f, -0.5f, -0.5f);
        const float faceInset = 0.0625f;
        applyInventoryFaceColor(kShadeBottom, red, green, blue, brightness);
        tessellator.startQuads();
        tessellator.normal(0.0f, -1.0f, 0.0f);
        faces_.renderBottomFace(block, 0.0, 0.0, 0.0, block.getTexture(0));
        tessellator.draw();
        applyInventoryFaceColor(kShadeTop, red, green, blue, brightness);
        tessellator.startQuads();
        tessellator.normal(0.0f, 1.0f, 0.0f);
        faces_.renderTopFace(block, 0.0, 0.0, 0.0, block.getTexture(1));
        tessellator.draw();
        applyInventoryFaceColor(kShadeHoriz, red, green, blue, brightness);
        tessellator.startQuads();
        tessellator.normal(0.0f, 0.0f, -1.0f);
        tessellator.translate(0.0f, 0.0f, faceInset);
        faces_.renderEastFace(block, 0.0, 0.0, 0.0, block.getTexture(2));
        tessellator.translate(0.0f, 0.0f, -faceInset);
        tessellator.draw();
        applyInventoryFaceColor(kShadeHoriz, red, green, blue, brightness);
        tessellator.startQuads();
        tessellator.normal(0.0f, 0.0f, 1.0f);
        tessellator.translate(0.0f, 0.0f, -faceInset);
        faces_.renderWestFace(block, 0.0, 0.0, 0.0, block.getTexture(3));
        tessellator.translate(0.0f, 0.0f, faceInset);
        tessellator.draw();
        applyInventoryFaceColor(kShadeNorthSouth, red, green, blue, brightness);
        tessellator.startQuads();
        tessellator.normal(-1.0f, 0.0f, 0.0f);
        tessellator.translate(faceInset, 0.0f, 0.0f);
        faces_.renderNorthFace(block, 0.0, 0.0, 0.0, block.getTexture(4));
        tessellator.translate(-faceInset, 0.0f, 0.0f);
        tessellator.draw();
        applyInventoryFaceColor(kShadeNorthSouth, red, green, blue, brightness);
        tessellator.startQuads();
        tessellator.normal(1.0f, 0.0f, 0.0f);
        tessellator.translate(-faceInset, 0.0f, 0.0f);
        faces_.renderSouthFace(block, 0.0, 0.0, 0.0, block.getTexture(5));
        tessellator.translate(faceInset, 0.0f, 0.0f);
        tessellator.draw();
        net::minecraft::client::gl::GL11::glTranslatef(0.5f, 0.5f, 0.5f);
    } else if (renderType == BlockRenderType::CROP) {
        applyInventoryFaceColor(1.0f, red, green, blue, brightness);
        tessellator.startQuads();
        tessellator.normal(0.0f, -1.0f, 0.0f);
        crop_.render(block, metadata, -0.5, -0.5, -0.5);
        tessellator.draw();
    } else if (renderType == BlockRenderType::TORCH) {
        applyInventoryFaceColor(1.0f, red, green, blue, brightness);
        tessellator.startQuads();
        tessellator.normal(0.0f, -1.0f, 0.0f);
        torch_.renderTiltedTorch(block, -0.5, -0.5, -0.5, 0.0, 0.0);
        tessellator.draw();
    } else if (renderType == BlockRenderType::STAIRS) {
        for (int i = 0; i < 2; ++i) {
            if (i == 0) {
                block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
            }
            if (i == 1) {
                block.setBoundingBox(0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f);
            }
            net::minecraft::client::gl::GL11::glTranslatef(-0.5f, -0.5f, -0.5f);
            drawInventoryCubeFaces(faces_, block, metadata, red, green, blue, brightness);
            net::minecraft::client::gl::GL11::glTranslatef(0.5f, 0.5f, 0.5f);
        }
    } else if (renderType == BlockRenderType::FENCE) {
        for (int i = 0; i < 4; ++i) {
            float postHalfWidth = 0.125f;
            if (i == 0) {
                block.setBoundingBox(0.5f - postHalfWidth, 0.0f, 0.0f, 0.5f + postHalfWidth, 1.0f, postHalfWidth * 2.0f);
            }
            if (i == 1) {
                block.setBoundingBox(0.5f - postHalfWidth, 0.0f, 1.0f - postHalfWidth * 2.0f, 0.5f + postHalfWidth, 1.0f, 1.0f);
            }
            postHalfWidth = 0.0625f;
            if (i == 2) {
                block.setBoundingBox(0.5f - postHalfWidth, 1.0f - postHalfWidth * 3.0f, -postHalfWidth * 2.0f, 0.5f + postHalfWidth, 1.0f - postHalfWidth, 1.0f + postHalfWidth * 2.0f);
            }
            if (i == 3) {
                block.setBoundingBox(0.5f - postHalfWidth, 0.5f - postHalfWidth * 3.0f, -postHalfWidth * 2.0f, 0.5f + postHalfWidth, 0.5f - postHalfWidth, 1.0f + postHalfWidth * 2.0f);
            }
            net::minecraft::client::gl::GL11::glTranslatef(-0.5f, -0.5f, -0.5f);
            drawInventoryCubeFaces(faces_, block, metadata, red, green, blue, brightness);
            net::minecraft::client::gl::GL11::glTranslatef(0.5f, 0.5f, 0.5f);
        }
        block.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    }
}

} // namespace net::minecraft::client::render::block
