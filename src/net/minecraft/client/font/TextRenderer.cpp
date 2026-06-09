#include "net/minecraft/client/font/TextRenderer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/client/util/GlAllocationUtils.hpp"
#include "net/minecraft/util/CharacterUtils.hpp"

#include <cctype>
#include <memory>
#include <stdexcept>
#include <vector>

namespace net::minecraft::client::font {

namespace {

constexpr int kGlTexture2D = 0x0DE1;
constexpr unsigned int kGlCompile = 0x1300;
constexpr unsigned int kGlUnsignedInt = 0x1405;

int indexOfChar(const std::string& haystack, char needle)
{
    const auto pos = haystack.find(needle);
    return pos == std::string::npos ? -1 : static_cast<int>(pos);
}

std::vector<std::string> splitWords(const std::string& text)
{
    std::vector<std::string> words;
    std::size_t start = 0;
    while (start < text.size()) {
        while (start < text.size() && text[start] == ' ') {
            ++start;
        }
        if (start >= text.size()) {
            break;
        }
        std::size_t end = text.find(' ', start);
        if (end == std::string::npos) {
            end = text.size();
        }
        words.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    return words;
}

bool isSectionSign(const std::string& text, std::size_t index)
{
    // UTF-8 encoding of U+00A7 (formatting code prefix in chat/GUI strings).
    return index + 1 < text.size() &&
           static_cast<unsigned char>(text[index]) == 0xC2 &&
           static_cast<unsigned char>(text[index + 1]) == 0xA7;
}

void initGlyphWidths(const texture::RasterImage& fontImage, std::array<int, 256>& characterWidths)
{
    const int imageWidth = fontImage.width;
    for (int i = 0; i < 256; ++i) {
        const int col = i % 16;
        const int row = i / 16;
        int width = 7;
        for (; width >= 0; --width) {
            const int px = col * 8 + width;
            bool empty = true;
            for (int j = 0; j < 8 && empty; ++j) {
                const int py = row * 8 + j;
                const std::uint32_t pixel = fontImage.argb[static_cast<std::size_t>(py) * imageWidth + px];
                if ((pixel & 0xFFU) > 0) {
                    empty = false;
                }
            }
            if (!empty) {
                break;
            }
        }
        if (i == 32) {
            width = 2;
        }
        characterWidths[static_cast<std::size_t>(i)] = width + 2;
    }
}

void buildDisplayLists(const texture::RasterImage& fontImage, std::array<int, 256>& characterWidths, int boundPage,
    texture::TextureManager& textureManager, option::GameOptions& options, int& boundTextureOut)
{
    initGlyphWidths(fontImage, characterWidths);
    boundTextureOut = textureManager.load(fontImage);

    render::Tessellator& tessellator = render::INSTANCE;
    for (int glyph = 0; glyph < 256; ++glyph) {
        gl::GL11::glNewList(boundPage + glyph, static_cast<int>(kGlCompile));
        tessellator.startQuads();
        const int u = (glyph % 16) * 8;
        const int v = (glyph / 16) * 8;
        constexpr float size = 7.99f;
        tessellator.vertex(0.0, 0.0 + size, 0.0, static_cast<double>(u) / 128.0, (static_cast<double>(v) + size) / 128.0);
        tessellator.vertex(0.0 + size, 0.0 + size, 0.0, (static_cast<double>(u) + size) / 128.0,
            (static_cast<double>(v) + size) / 128.0);
        tessellator.vertex(0.0 + size, 0.0, 0.0, (static_cast<double>(u) + size) / 128.0, static_cast<double>(v) / 128.0);
        tessellator.vertex(0.0, 0.0, 0.0, static_cast<double>(u) / 128.0, static_cast<double>(v) / 128.0);
        tessellator.draw();
        gl::GL11::glTranslatef(static_cast<float>(characterWidths[static_cast<std::size_t>(glyph)]), 0.0f, 0.0f);
        gl::GL11::glEndList();
    }

    for (int colorIndex = 0; colorIndex < 32; ++colorIndex) {
        int r = ((colorIndex >> 3) & 1) * 85;
        int g = ((colorIndex >> 2) & 1) * 170 + r;
        int b = ((colorIndex >> 1) & 1) * 170 + r;
        int extra = ((colorIndex >> 0) & 1) * 170 + r;
        if (colorIndex == 6) {
            g += 85;
        }
        const bool dark = colorIndex >= 16;
        if (options.isAnaglyphActive()) {
            const int mixR = (r * 30 + g * 59 + b * 11) / 100;
            const int mixG = (r * 30 + g * 70) / 100;
            const int mixB = (r * 30 + extra * 70) / 100;
            r = mixR;
            g = mixG;
            b = mixB;
        }
        if (dark) {
            r /= 4;
            g /= 4;
            b /= 4;
        }
        gl::GL11::glNewList(boundPage + 256 + colorIndex, static_cast<int>(kGlCompile));
        gl::GL11::glColor3f(static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f);
        gl::GL11::glEndList();
    }
}

} // namespace

TextRenderer::TextRenderer(option::GameOptions& options, const std::string& fontPath, texture::TextureManager& textureManager)
{
    pageBuffer_.reserve(kPageBufferCapacity);
    texture::RasterImage fontImage =
        texture::TextureManager::loadRasterFromFile(texture::TextureManager::resolveResourcePath(fontPath));
    if (fontImage.width <= 0 || fontImage.height <= 0) {
        throw std::runtime_error("TextRenderer: failed to load font image " + fontPath);
    }
    boundPage_ = util::GlAllocationUtils::generateDisplayLists(288);
    buildDisplayLists(fontImage, characterWidths_, boundPage_, textureManager, options, boundTexture);
}

TextRenderer::TextRenderer(option::GameOptions& options, const texture::RasterImage& fontImage,
    texture::TextureManager& textureManager)
{
    pageBuffer_.reserve(kPageBufferCapacity);
    if (fontImage.width <= 0 || fontImage.height <= 0) {
        throw std::runtime_error("TextRenderer: invalid font image");
    }
    boundPage_ = util::GlAllocationUtils::generateDisplayLists(288);
    buildDisplayLists(fontImage, characterWidths_, boundPage_, textureManager, options, boundTexture);
}

std::unique_ptr<TextRenderer> TextRenderer::create(
    option::GameOptions& options, texture::TextureManager& textureManager, const std::string& fontPath)
{
    texture::RasterImage fontImage =
        texture::TextureManager::loadRasterFromFile(texture::TextureManager::resolveResourcePath(fontPath));
    if (fontImage.width > 0 && fontImage.height > 0) {
        return std::make_unique<TextRenderer>(options, fontPath, textureManager);
    }
    texture::RasterImage fallback;
    fallback.width = 128;
    fallback.height = 128;
    fallback.argb.assign(128 * 128, 0x00000000U);
    for (int glyph = 32; glyph < 127; ++glyph) {
        const int col = glyph % 16;
        const int row = glyph / 16;
        for (int py = 1; py < 7; ++py) {
            for (int px = 1; px < 7; ++px) {
                fallback.argb[static_cast<std::size_t>(row * 8 + py) * 128 + col * 8 + px] = 0xFFFFFFFFU;
            }
        }
    }
    return std::make_unique<TextRenderer>(options, fallback, textureManager);
}

void TextRenderer::flushPageBuffer()
{
    if (pageBuffer_.empty()) {
        return;
    }
    gl::GL11::glCallLists(static_cast<int>(pageBuffer_.size()), kGlUnsignedInt, pageBuffer_.data());
    pageBuffer_.clear();
}

void TextRenderer::appendList(int listId)
{
    if (pageBuffer_.size() >= kPageBufferCapacity) {
        flushPageBuffer();
    }
    pageBuffer_.push_back(static_cast<unsigned int>(listId));
}

void TextRenderer::drawWithShadow(const std::string& text, int x, int y, int color)
{
    draw(text, x + 1, y + 1, color, true);
    draw(text, x, y, color, false);
}

void TextRenderer::draw(const std::string& text, int x, int y, int color)
{
    draw(text, x, y, color, false);
}

void TextRenderer::draw(const std::string& text, int x, int y, int color, bool shadow)
{
    if (text.empty()) {
        return;
    }

    if (shadow) {
        const int alpha = color & 0xFF000000;
        color = ((color & 0xFCFCFC) >> 2) + alpha;
    }

    gl::GL11::glBindTexture(kGlTexture2D, static_cast<unsigned int>(boundTexture));
    const float rf = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    const float gf = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
    const float bf = static_cast<float>(color & 0xFF) / 255.0f;
    float af = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
    if (af == 0.0f) {
        af = 1.0f;
    }
    gl::GL11::glColor4f(rf, gf, bf, af);

    pageBuffer_.clear();
    gl::GL11::glPushMatrix();
    gl::GL11::glTranslatef(static_cast<float>(x), static_cast<float>(y), 0.0f);

    const std::string& valid = CharacterUtils::validCharacters();
    for (std::size_t i = 0; i < text.size();) {
        if (isSectionSign(text, i) && i + 2 < text.size()) {
            int colorCode = indexOfChar("0123456789abcdef",
                static_cast<char>(std::tolower(static_cast<unsigned char>(text[i + 2]))));
            if (colorCode < 0 || colorCode > 15) {
                colorCode = 15;
            }
            appendList(boundPage_ + 256 + colorCode + (shadow ? 16 : 0));
            i += 3;
            continue;
        }

        const int glyphIndex = indexOfChar(valid, text[i]);
        if (glyphIndex >= 0) {
            appendList(boundPage_ + glyphIndex + 32);
        }
        ++i;
    }

    flushPageBuffer();
    gl::GL11::glPopMatrix();
}

int TextRenderer::getWidth(const std::string& text) const
{
    if (text.empty()) {
        return 0;
    }

    int width = 0;
    const std::string& valid = CharacterUtils::validCharacters();
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (isSectionSign(text, i)) {
            i += 3;
            continue;
        }
        const int glyphIndex = indexOfChar(valid, text[i]);
        if (glyphIndex >= 0) {
            width += characterWidths_[static_cast<std::size_t>(glyphIndex + 32)];
        }
    }
    return width;
}

void TextRenderer::drawSplit(const std::string& text, int x, int y, int width, int color)
{
    if (text.find('\n') != std::string::npos) {
        std::size_t start = 0;
        while (start < text.size()) {
            const std::size_t newline = text.find('\n', start);
            const std::string line = text.substr(start, newline == std::string::npos ? std::string::npos : newline - start);
            drawSplit(line, x, y, width, color);
            y += splitAndGetHeight(line, width);
            if (newline == std::string::npos) {
                break;
            }
            start = newline + 1;
        }
        return;
    }

    const std::vector<std::string> words = splitWords(text);
    std::size_t index = 0;
    while (index < words.size()) {
        std::string line = words[index++] + " ";
        while (index < words.size() && getWidth(line + words[index]) < width) {
            line += words[index++] + " ";
        }
        while (getWidth(line) > width) {
            std::size_t cut = 0;
            while (cut < line.size() && getWidth(line.substr(0, cut + 1)) <= width) {
                ++cut;
            }
            const std::string part = line.substr(0, cut);
            if (!part.empty()) {
                draw(part, x, y, color);
                y += 8;
            }
            line = line.substr(cut);
        }
        if (!line.empty()) {
            draw(line, x, y, color);
            y += 8;
        }
    }
}

int TextRenderer::splitAndGetHeight(const std::string& text, int width) const
{
    if (text.find('\n') != std::string::npos) {
        int total = 0;
        std::size_t start = 0;
        while (start < text.size()) {
            const std::size_t newline = text.find('\n', start);
            const std::string line = text.substr(start, newline == std::string::npos ? std::string::npos : newline - start);
            total += splitAndGetHeight(line, width);
            if (newline == std::string::npos) {
                break;
            }
            start = newline + 1;
        }
        return total;
    }

    const std::vector<std::string> words = splitWords(text);
    int height = 0;
    std::size_t index = 0;
    while (index < words.size()) {
        std::string line = words[index++] + " ";
        while (index < words.size() && getWidth(line + words[index]) < width) {
            line += words[index++] + " ";
        }
        while (getWidth(line) > width) {
            std::size_t cut = 0;
            while (cut < line.size() && getWidth(line.substr(0, cut + 1)) <= width) {
                ++cut;
            }
            if (!line.substr(0, cut).empty()) {
                height += 8;
            }
            line = line.substr(cut);
        }
        if (!line.empty()) {
            height += 8;
        }
    }
    if (height < 8) {
        height += 8;
    }
    return height;
}

} // namespace net::minecraft::client::font
