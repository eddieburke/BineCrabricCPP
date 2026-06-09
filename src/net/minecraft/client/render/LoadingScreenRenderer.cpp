#include "net/minecraft/client/render/LoadingScreenRenderer.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/util/DisplayManager.hpp"
#include "net/minecraft/client/util/UiScale.hpp"

namespace net::minecraft::client::render {

void LoadingScreenRenderer::renderLoadingScreen(Minecraft& client)
{
    const util::UiScale scale =
        util::uiScale(client.options, client.displayWidth, client.displayHeight);
    gl::GL11::glClear(gl::GL11::GL_COLOR_BUFFER_BIT | gl::GL11::GL_DEPTH_BUFFER_BIT);
    gl::GL11::glMatrixMode(gl::GL11::GL_PROJECTION);
    gl::GL11::glLoadIdentity();
    gl::GL11::glOrtho(0.0, scale.rawWidth, scale.rawHeight, 0.0, 1000.0, 3000.0);
    gl::GL11::glMatrixMode(gl::GL11::GL_MODELVIEW);
    gl::GL11::glLoadIdentity();
    gl::GL11::glTranslatef(0.0f, 0.0f, -2000.0f);
    gl::GL11::glViewport(0, 0, client.displayWidth, client.displayHeight);
    gl::GL11::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    Tessellator& tessellator = Tessellator::INSTANCE;
    gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
    gl::GL11::glDisable(gl::GL11::GL_FOG);
    gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, client.textureManager.getTextureId("/title/mojang.png"));
    tessellator.startQuads();
    tessellator.color(0xFFFFFF);
    tessellator.vertex(0.0, scale.rawHeight, 0.0, 0.0, 0.0);
    tessellator.vertex(scale.rawWidth, scale.rawHeight, 0.0, 0.0, 0.0);
    tessellator.vertex(scale.rawWidth, 0.0, 0.0, 0.0, 0.0);
    tessellator.vertex(0.0, 0.0, 0.0, 0.0, 0.0);
    tessellator.draw();
    constexpr int logoW = 256;
    constexpr int logoH = 256;
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    tessellator.color(0xFFFFFF);
    draw(client, (scale.scaledWidth - logoW) / 2, (scale.scaledHeight - logoH) / 2, 0, 0, logoW, logoH);
    gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
    gl::GL11::glDisable(gl::GL11::GL_FOG);
    gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
    gl::GL11::glAlphaFunc(gl::GL11::GL_GREATER, 0.1f);
#ifdef _WIN32
    util::DisplayManager::present();
#endif
}

void LoadingScreenRenderer::draw(Minecraft& client, int x, int y, int u, int v, int width, int height)
{
    (void)client;
    constexpr float texel = 0.00390625f;
    Tessellator& tessellator = Tessellator::INSTANCE;
    tessellator.startQuads();
    tessellator.vertex(x + 0, y + height, 0.0, static_cast<float>(u + 0) * texel, static_cast<float>(v + height) * texel);
    tessellator.vertex(x + width, y + height, 0.0, static_cast<float>(u + width) * texel, static_cast<float>(v + height) * texel);
    tessellator.vertex(x + width, y + 0, 0.0, static_cast<float>(u + width) * texel, static_cast<float>(v + 0) * texel);
    tessellator.vertex(x + 0, y + 0, 0.0, static_cast<float>(u + 0) * texel, static_cast<float>(v + 0) * texel);
    tessellator.draw();
}

} // namespace net::minecraft::client::render
