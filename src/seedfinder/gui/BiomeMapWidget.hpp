#pragma once

#include "net/minecraft/client/gui/DrawContext.hpp"

#include <cstdint>

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::gui::widget {

// Top-down biome preview (AMIDST-style) using beta 1.7.3 grass colors.
class BiomeMapWidget : public gui::DrawContext {
public:
    ~BiomeMapWidget() override;

    void clear(Minecraft& minecraft);
    void build(Minecraft& minecraft, std::uint64_t seed, int centerBlockX, int centerBlockZ, int radiusChunks);

    void render(Minecraft& minecraft, int x, int y, int width, int height);

    [[nodiscard]] bool hasMap() const noexcept { return hasMap_; }
    [[nodiscard]] std::uint64_t seed() const noexcept { return seed_; }

private:
    [[nodiscard]] static std::uint32_t biomeColor(std::uint8_t biomeId);

    int textureId_ = 0;
    int textureSize_ = 0;
    int spawnPixelX_ = 0;
    int spawnPixelZ_ = 0;
    std::uint64_t seed_ = 0;
    bool hasMap_ = false;
};

} // namespace net::minecraft::client::gui::widget
