#include "net/minecraft/client/render/platform/GlState.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"

namespace net::minecraft::client::render::platform {

void GlState::enableWorldDefaults() noexcept
{
    gl::GL11::glEnable(gl::GL11::GL_CULL_FACE);
    gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
    gl::GL11::glDepthMask(true);
}

void GlState::beginSolidTerrainPass(int terrainTextureId) noexcept
{
    setFogEnabled(true);
    gl::GL11::glEnable(gl::GL11::GL_CULL_FACE);
    gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
    gl::GL11::glDepthMask(true);
    gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, static_cast<unsigned int>(terrainTextureId));
    if (Minecraft::isAmbientOcclusionEnabled()) {
        gl::GL11::glShadeModel(gl::GL11::GL_SMOOTH);
    } else {
        gl::GL11::glShadeModel(gl::GL11::GL_FLAT);
    }
    gl::GL11::glNormal3f(0.0f, -1.0f, 0.0f);
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    gl::GL11::glEnable(gl::GL11::GL_NORMALIZE);
    gl::GL11::glColorMaterial(gl::GL11::GL_FRONT_AND_BACK, gl::GL11::GL_AMBIENT_AND_DIFFUSE);
    Lighting::turnOff();
}

void GlState::endSolidTerrainPass() noexcept
{
    gl::GL11::glShadeModel(gl::GL11::GL_FLAT);
    Lighting::turnOn();
}

void GlState::beginTranslucentTerrainPass(int terrainTextureId) noexcept
{
    gl::GL11::glEnable(gl::GL11::GL_BLEND);
    gl::GL11::glDisable(gl::GL11::GL_CULL_FACE);
    gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, static_cast<unsigned int>(terrainTextureId));
}

void GlState::endTranslucentTerrainPass() noexcept
{
    gl::GL11::glDepthMask(true);
    gl::GL11::glEnable(gl::GL11::GL_CULL_FACE);
    gl::GL11::glDisable(gl::GL11::GL_BLEND);
}

void GlState::beginSkyPass() noexcept
{
    Lighting::turnOff();
}

void GlState::endSkyPass() noexcept
{
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void GlState::beginCloudPass() noexcept
{
    setFogEnabled(false);
}

void GlState::endCloudPass() noexcept
{
    setFogEnabled(false);
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void GlState::setFogEnabled(const bool enabled) noexcept
{
    if (enabled) {
        gl::GL11::glEnable(gl::GL11::GL_FOG);
    } else {
        gl::GL11::glDisable(gl::GL11::GL_FOG);
    }
}

void GlState::clearDepthForHand() noexcept
{
    gl::GL11::glClear(gl::GL11::GL_DEPTH_BUFFER_BIT);
}

} // namespace net::minecraft::client::render::platform
