#pragma once

#include "net/minecraft/client/texture/TextureManager.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace net::minecraft::client::option {
class GameOptions;
}

namespace net::minecraft::client::texture {
class TextureManager;
}

namespace net::minecraft::client::font {

// Faithful port of net.minecraft.client.font.TextRenderer (beta 1.7.3).
class TextRenderer {
public:
    TextRenderer(option::GameOptions& options, const std::string& fontPath, texture::TextureManager& textureManager);
    TextRenderer(option::GameOptions& options, const texture::RasterImage& fontImage, texture::TextureManager& textureManager);

    static std::unique_ptr<TextRenderer> create(
        option::GameOptions& options, texture::TextureManager& textureManager, const std::string& fontPath);

    void drawWithShadow(const std::string& text, int x, int y, int color);
    void draw(const std::string& text, int x, int y, int color);
    void draw(const std::string& text, int x, int y, int color, bool shadow);

    [[nodiscard]] int getWidth(const std::string& text) const;
    void drawSplit(const std::string& text, int x, int y, int width, int color);
    [[nodiscard]] int splitAndGetHeight(const std::string& text, int width) const;

    int boundTexture = 0;

private:
    void flushPageBuffer();
    void appendList(int listId);

    std::array<int, 256> characterWidths_{};
    int boundPage_ = 0;
    std::vector<unsigned int> pageBuffer_;
    static constexpr int kPageBufferCapacity = 1024;
};

} // namespace net::minecraft::client::font
