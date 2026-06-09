#include "net/minecraft/client/render/platform/StereoCompositor.hpp"

#include "net/minecraft/client/gl/GL11.hpp"

namespace net::minecraft::client::render::platform {

namespace {

constexpr int kDepthTest = 0x0B71;
constexpr int kLighting = 0x0B50;
constexpr int kFog = 0x0B60;
constexpr int kAlphaTest = 0x0BC0;
constexpr int kTexture2D = 0x0DE1;
constexpr int kBlend = 0x0BE2;
constexpr int kOne = 0x0001;
constexpr int kZero = 0x0000;
constexpr int kProjection = 0x1701;
constexpr int kModelview = 0x1700;
constexpr int kSrcAlpha = 0x0302;
constexpr int kOneMinusSrcAlpha = 0x0303;
constexpr int kQuads = 0x0007;
constexpr int kColorBufferBit = 0x00004000;
constexpr int kDepthBufferBit = 0x00000100;

void beginCompositePass(int displayWidth, int displayHeight) noexcept
{
    gl::GL11::glDisable(kDepthTest);
    gl::GL11::glDisable(kLighting);
    gl::GL11::glDisable(kFog);
    gl::GL11::glDisable(kAlphaTest);
    gl::GL11::glEnable(kTexture2D);
    gl::GL11::glEnable(kBlend);
    gl::GL11::glBlendFunc(kOne, kZero);

    gl::GL11::glMatrixMode(kProjection);
    gl::GL11::glPushMatrix();
    gl::GL11::glLoadIdentity();
    gl::GL11::glOrtho(0.0, static_cast<double>(displayWidth), static_cast<double>(displayHeight), 0.0, -1.0, 1.0);
    gl::GL11::glMatrixMode(kModelview);
    gl::GL11::glPushMatrix();
    gl::GL11::glLoadIdentity();
}

void endCompositePass() noexcept
{
    gl::GL11::glPopMatrix();
    gl::GL11::glMatrixMode(kProjection);
    gl::GL11::glPopMatrix();
    gl::GL11::glMatrixMode(kModelview);

    gl::GL11::glColorMask(true, true, true, true);
    gl::GL11::glDisable(kBlend);
    gl::GL11::glBlendFunc(kSrcAlpha, kOneMinusSrcAlpha);
    gl::GL11::glEnable(kDepthTest);
}

} // namespace

void StereoCompositor::drawTexturedRect(int x, int y, int w, int h, unsigned int texture, int displayWidth,
    int displayHeight) noexcept
{
    (void)displayWidth;
    (void)displayHeight;
    gl::GL11::glBindTexture(kTexture2D, static_cast<int>(texture));
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    gl::GL11::glBegin(kQuads);
    gl::GL11::glTexCoord2d(0.0, 0.0);
    gl::GL11::glVertex3d(static_cast<double>(x), static_cast<double>(y), 0.0);
    gl::GL11::glTexCoord2d(1.0, 0.0);
    gl::GL11::glVertex3d(static_cast<double>(x + w), static_cast<double>(y), 0.0);
    gl::GL11::glTexCoord2d(1.0, 1.0);
    gl::GL11::glVertex3d(static_cast<double>(x + w), static_cast<double>(y + h), 0.0);
    gl::GL11::glTexCoord2d(0.0, 1.0);
    gl::GL11::glVertex3d(static_cast<double>(x), static_cast<double>(y + h), 0.0);
    gl::GL11::glEnd();
    gl::GL11::glBindTexture(kTexture2D, 0);
}

void StereoCompositor::compositeAnaglyph(unsigned int eye0Texture, unsigned int eye1Texture, int displayWidth,
    int displayHeight) noexcept
{
    gl::GL11::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::GL11::glClear(kColorBufferBit | kDepthBufferBit);

    beginCompositePass(displayWidth, displayHeight);

    // Eye 0 wrote GB in legacy dual-pass; replicate via color mask on full RGB texture.
    gl::GL11::glColorMask(false, true, true, true);
    drawTexturedRect(0, 0, displayWidth, displayHeight, eye0Texture, displayWidth, displayHeight);

    gl::GL11::glColorMask(true, false, false, true);
    drawTexturedRect(0, 0, displayWidth, displayHeight, eye1Texture, displayWidth, displayHeight);

    endCompositePass();
}

void StereoCompositor::compositeSideBySide(unsigned int eye0Texture, unsigned int eye1Texture, int displayWidth,
    int displayHeight) noexcept
{
    const int halfWidth = displayWidth / 2;

    gl::GL11::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::GL11::glClear(kColorBufferBit | kDepthBufferBit);

    beginCompositePass(displayWidth, displayHeight);

    gl::GL11::glColorMask(true, true, true, true);
    drawTexturedRect(0, 0, halfWidth, displayHeight, eye0Texture, displayWidth, displayHeight);
    drawTexturedRect(halfWidth, 0, halfWidth, displayHeight, eye1Texture, displayWidth, displayHeight);

    endCompositePass();
}

} // namespace net::minecraft::client::render::platform
