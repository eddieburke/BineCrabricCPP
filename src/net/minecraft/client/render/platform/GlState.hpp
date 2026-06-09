#pragma once

namespace net::minecraft::client::render::platform {

// Centralizes world-render GL state transitions to match beta 1.7.3 GameRenderer.java.
// GUI/HUD ortho passes use GuiGlState instead.
class GlState {
public:
    static void enableWorldDefaults() noexcept;
    static void beginSolidTerrainPass(int terrainTextureId) noexcept;
    static void endSolidTerrainPass() noexcept;
    static void beginTranslucentTerrainPass(int terrainTextureId) noexcept;
    static void endTranslucentTerrainPass() noexcept;
    static void beginSkyPass() noexcept;
    static void endSkyPass() noexcept;
    static void beginCloudPass() noexcept;
    static void endCloudPass() noexcept;
    static void clearDepthForHand() noexcept;

    static void setFogEnabled(bool enabled) noexcept;
};

} // namespace net::minecraft::client::render::platform
