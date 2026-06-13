#include "net/minecraft/client/gui/DrawContext.hpp"

#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/platform/GuiGlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"

namespace net::minecraft::client::gui {

namespace {

void unpackColor(std::uint32_t color, float& r, float& g, float& b, float& a)
{
    a = static_cast<float>((color >> 24U) & 0xFFU) / 255.0f;
    if (a <= 0.0f) {
        a = 1.0f;
    }
    r = static_cast<float>((color >> 16U) & 0xFFU) / 255.0f;
    g = static_cast<float>((color >> 8U) & 0xFFU) / 255.0f;
    b = static_cast<float>(color & 0xFFU) / 255.0f;
}

} // namespace

void DrawContext::fill(int x1, int y1, int x2, int y2, std::uint32_t color)
{
    if (x1 < x2) {
        std::swap(x1, x2);
    }
    if (y1 < y2) {
        std::swap(y1, y2);
    }

    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;
    unpackColor(color, r, g, b, a);

    render::Tessellator& tessellator = render::INSTANCE;
    render::platform::GuiGlState::enableStandardBlend();
    {
        render::platform::ScopedNoTexture2D noTexture;
        gl::GL11::glColor4f(r, g, b, a);
        draw::quad(tessellator, x1, y1, x2, y2);
    }
    render::platform::GuiGlState::disableBlend();
}

void DrawContext::fillGradient(int x1, int y1, int x2, int y2, std::uint32_t colorStart, std::uint32_t colorEnd)
{
    float r0 = 0.0f;
    float g0 = 0.0f;
    float b0 = 0.0f;
    float a0 = 0.0f;
    float r1 = 0.0f;
    float g1 = 0.0f;
    float b1 = 0.0f;
    float a1 = 0.0f;
    unpackColor(colorStart, r0, g0, b0, a0);
    unpackColor(colorEnd, r1, g1, b1, a1);

    render::platform::ScopedNoTexture2D noTexture;
    render::platform::GuiGlState::beginAlphaText();
    {
        render::platform::ScopedSmoothShade smoothShade;
        render::Tessellator& tessellator = render::INSTANCE;
        tessellator.startQuads();
        tessellator.color(r0, g0, b0, a0);
        tessellator.vertex(static_cast<double>(x2), static_cast<double>(y1), 0.0);
        tessellator.vertex(static_cast<double>(x1), static_cast<double>(y1), 0.0);
        tessellator.color(r1, g1, b1, a1);
        tessellator.vertex(static_cast<double>(x1), static_cast<double>(y2), 0.0);
        tessellator.vertex(static_cast<double>(x2), static_cast<double>(y2), 0.0);
        tessellator.draw();
    }
    render::platform::GuiGlState::endAlphaText();
}

void DrawContext::drawTexture(int x, int y, int u, int v, int width, int height)
{
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
    constexpr float texel = 0.00390625f;
    render::Tessellator& tessellator = render::INSTANCE;
    draw::texturedQuad(tessellator, x, y, x + width, y + height,
        static_cast<float>(u + 0) * texel, static_cast<float>(v + 0) * texel,
        static_cast<float>(u + width) * texel, static_cast<float>(v + height) * texel,
        zOffset);
}

void DrawContext::drawCenteredTextWithShadow(font::TextRenderer& textRenderer, const std::string& text, int x, int y,
    int color)
{
    textRenderer.drawWithShadow(text, x - textRenderer.getWidth(text) / 2, y, color);
}

void DrawContext::drawTextWithShadow(font::TextRenderer& textRenderer, const std::string& text, int x, int y, int color)
{
    textRenderer.drawWithShadow(text, x, y, color);
}

} // namespace net::minecraft::client::gui
