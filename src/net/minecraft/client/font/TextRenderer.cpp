#include "net/minecraft/client/font/TextRenderer.hpp"
#include <cctype>
#include <memory>
#include <stdexcept>
#include <vector>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/util/CharacterUtils.hpp"
namespace net::minecraft::client::font {
namespace {
int indexOfChar(const std::string& haystack, char needle) {
 const auto pos = haystack.find(needle);
 return pos == std::string::npos ? -1 : static_cast<int>(pos);
}
std::vector<std::string> splitWords(const std::string& text) {
 std::vector<std::string> words;
 std::size_t start = 0;
 while(start < text.size()) {
  while(start < text.size() && text[start] == ' ') {
   ++start;
  }
  if(start >= text.size()) {
   break;
  }
  std::size_t end = text.find(' ', start);
  if(end == std::string::npos) {
   end = text.size();
  }
  words.push_back(text.substr(start, end - start));
  start = end + 1;
 }
 return words;
}
bool isSectionSign(const std::string& text, std::size_t index) {
 if(index + 1 < text.size() && static_cast<unsigned char>(text[index]) == 0xC2 &&
    static_cast<unsigned char>(text[index + 1]) == 0xA7) {
  return true;
 }
 return index < text.size() && static_cast<unsigned char>(text[index]) == 0xA7;
}
void initGlyphWidths(const texture::RasterImage& fontImage, std::array<int, 256>& characterWidths) {
 const int imageWidth = fontImage.width;
 for(int i = 0; i < 256; ++i) {
  const int col = i % 16;
  const int row = i / 16;
  int width = 7;
  for(; width >= 0; --width) {
   const int px = col * 8 + width;
   bool empty = true;
   for(int j = 0; j < 8 && empty; ++j) {
    const int py = row * 8 + j;
    const std::uint32_t pixel = fontImage.argb[static_cast<std::size_t>(py) * imageWidth + px];
    if((pixel & 0xFFU) > 0) {
     empty = false;
    }
   }
   if(!empty) {
    break;
   }
  }
  if(i == 32) {
   width = 2;
  }
  characterWidths[static_cast<std::size_t>(i)] = width + 2;
 }
}
texture::RasterImage loadFontRasterFromPath(const std::string& fontPath) {
 texture::RasterImage fontImage =
     texture::TextureManager::loadRasterFromFile(texture::TextureManager::resolveResourcePath(fontPath));
 if(fontImage.width <= 0 || fontImage.height <= 0) {
  throw std::runtime_error("TextRenderer: failed to load font image " + fontPath);
 }
 return fontImage;
}
int getGlyphIndex(unsigned char ch, const std::string& valid) {
 if(ch < 128) {
  return static_cast<int>(ch);
 }
 const int idx = indexOfChar(valid, static_cast<char>(ch));
 if(idx >= 0) {
  return idx + 160;
 }
 return static_cast<int>(ch);
}
} // namespace
TextRenderer::TextRenderer(option::GameOptions& options,
                           const std::string& fontPath,
                           texture::TextureManager& textureManager)
    : TextRenderer(options, loadFontRasterFromPath(fontPath), textureManager) {
}
TextRenderer::TextRenderer(option::GameOptions& /*options*/,
                           const texture::RasterImage& fontImage,
                           texture::TextureManager& textureManager) {
 if(fontImage.width <= 0 || fontImage.height <= 0) {
  throw std::runtime_error("TextRenderer: invalid font image");
 }
 initGlyphWidths(fontImage, characterWidths_);
 boundTexture = textureManager.load(fontImage);
 for(int colorIndex = 0; colorIndex < 32; ++colorIndex) {
  int r = ((colorIndex >> 3) & 1) * 85;
  int g = ((colorIndex >> 2) & 1) * 170 + r;
  int b = ((colorIndex >> 1) & 1) * 170 + r;
  if(colorIndex == 6) {
   g += 85;
  }
  const bool dark = colorIndex >= 16;
  if(dark) {
   r /= 4;
   g /= 4;
   b /= 4;
  }
  colors_[static_cast<std::size_t>(colorIndex)] =
      FontColor{static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f};
 }
}
std::unique_ptr<TextRenderer> TextRenderer::create(option::GameOptions& options,
                                                   texture::TextureManager& textureManager,
                                                   const std::string& fontPath) {
 texture::RasterImage fontImage =
     texture::TextureManager::loadRasterFromFile(texture::TextureManager::resolveResourcePath(fontPath));
 if(fontImage.width > 0 && fontImage.height > 0) {
  return std::make_unique<TextRenderer>(options, fontImage, textureManager);
 }
 ClientLog::LOGGER.log(LogLevel::Warning,
                       "TextRenderer: font '" + fontPath + "' failed to load, using synthetic fallback font atlas");
 texture::RasterImage fallback;
 fallback.width = 128;
 fallback.height = 128;
 fallback.argb.assign(128 * 128, 0x00000000U);
 for(int glyph = 32; glyph < 127; ++glyph) {
  const int col = glyph % 16;
  const int row = glyph / 16;
  for(int py = 1; py < 7; ++py) {
   for(int px = 1; px < 7; ++px) {
    fallback.argb[static_cast<std::size_t>(row * 8 + py) * 128 + col * 8 + px] = 0xFFFFFFFFU;
   }
  }
 }
 return std::make_unique<TextRenderer>(options, fallback, textureManager);
}
void TextRenderer::appendGlyphQuad(
    render::Tessellator& tessellator, int glyph, float penX, float r, float g, float b, float a) const {
 const int u = (glyph % 16) * 8;
 const int v = (glyph / 16) * 8;
 constexpr float size = 7.99f;
 tessellator.color(r, g, b, a);
 tessellator.vertex(static_cast<double>(penX),
                    static_cast<double>(size),
                    0.0,
                    static_cast<double>(u) / 128.0,
                    (static_cast<double>(v) + size) / 128.0);
 tessellator.vertex(static_cast<double>(penX) + size,
                    static_cast<double>(size),
                    0.0,
                    (static_cast<double>(u) + size) / 128.0,
                    (static_cast<double>(v) + size) / 128.0);
 tessellator.vertex(static_cast<double>(penX) + size,
                    0.0,
                    0.0,
                    (static_cast<double>(u) + size) / 128.0,
                    static_cast<double>(v) / 128.0);
 tessellator.vertex(
     static_cast<double>(penX), 0.0, 0.0, static_cast<double>(u) / 128.0, static_cast<double>(v) / 128.0);
}
void TextRenderer::drawWithShadow(const std::string& text, int x, int y, int color) {
 draw(text, x + 1, y + 1, color, true);
 draw(text, x, y, color, false);
}
void TextRenderer::draw(const std::string& text, int x, int y, int color) {
 draw(text, x, y, color, false);
}
void TextRenderer::draw(const std::string& text, int x, int y, int color, bool shadow) {
 if(text.empty()) {
  return;
 }
 if(shadow) {
  const int alpha = color & 0xFF000000;
  color = ((color & 0xFCFCFC) >> 2) + alpha;
 }
 const float rf = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
 const float gf = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
 const float bf = static_cast<float>(color & 0xFF) / 255.0f;
 float af = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
 if(af == 0.0f) {
  af = 1.0f;
 }
 const render::RenderSystem::StateShadow savedShadow = render::RenderSystem::getShadow();
 const render::RenderPassScope passScope(render::RenderType::text());
 if(savedShadow.depthTest) {
  render::RenderSystem::enableDepthTest();
 }
 render::RenderSystem::enableTexture();
 render::RenderSystem::bindTexture(static_cast<int>(boundTexture));
 render::RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
 float currentR = rf;
 float currentG = gf;
 float currentB = bf;
 render::RenderSystem::pushMatrix();
 render::RenderSystem::translate(static_cast<float>(x), static_cast<float>(y), 0.0f);
 render::Tessellator& tessellator = render::INSTANCE;
 tessellator.startQuads();
 float penX = 0.0f;
 const std::string& valid = CharacterUtils::validCharacters();
 for(std::size_t i = 0; i < text.size();) {
  if(isSectionSign(text, i)) {
   const std::size_t codeIndex = static_cast<unsigned char>(text[i]) == 0xC2 ? i + 2 : i + 1;
   if(codeIndex >= text.size()) {
    break;
   }
   int colorCode = indexOfChar("0123456789abcdef",
                               static_cast<char>(std::tolower(static_cast<unsigned char>(text[codeIndex]))));
   if(colorCode < 0 || colorCode > 15) {
    colorCode = 15;
   }
   const FontColor& palette = colors_[static_cast<std::size_t>(colorCode + (shadow ? 16 : 0))];
   currentR = palette.r;
   currentG = palette.g;
   currentB = palette.b;
   i = codeIndex + 1;
   continue;
  }
  const unsigned char ch = static_cast<unsigned char>(text[i]);
  const int glyph = getGlyphIndex(ch, valid);
  if(glyph >= 0 && glyph < 256) {
   appendGlyphQuad(tessellator, glyph, penX, currentR, currentG, currentB, af);
   penX += static_cast<float>(characterWidths_[static_cast<std::size_t>(glyph)]);
  }
  ++i;
 }
 tessellator.draw();
 render::RenderSystem::popMatrix();
 render::RenderSystem::setShadow(savedShadow);
}
int TextRenderer::getWidth(const std::string& text) const {
 if(text.empty()) {
  return 0;
 }
 int width = 0;
 const std::string& valid = CharacterUtils::validCharacters();
 for(std::size_t i = 0; i < text.size(); ++i) {
  if(isSectionSign(text, i)) {
   i += static_cast<unsigned char>(text[i]) == 0xC2 ? 2 : 1;
   continue;
  }
  const unsigned char ch = static_cast<unsigned char>(text[i]);
  const int glyph = getGlyphIndex(ch, valid);
  if(glyph >= 0 && glyph < 256) {
   width += characterWidths_[static_cast<std::size_t>(glyph)];
  }
 }
 return width;
}
void TextRenderer::drawSplit(const std::string& text, int x, int y, int width, int color) {
 if(text.find('\n') != std::string::npos) {
  std::size_t start = 0;
  while(start < text.size()) {
   const std::size_t newline = text.find('\n', start);
   const std::string line =
       text.substr(start, newline == std::string::npos ? std::string::npos : newline - start);
   drawSplit(line, x, y, width, color);
   y += splitAndGetHeight(line, width);
   if(newline == std::string::npos) {
    break;
   }
   start = newline + 1;
  }
  return;
 }
 const std::vector<std::string> words = splitWords(text);
 std::size_t index = 0;
 while(index < words.size()) {
  std::string line = words[index++] + " ";
  while(index < words.size() && getWidth(line + words[index]) < width) {
   line += words[index++] + " ";
  }
  while(getWidth(line) > width) {
   std::size_t cut = 0;
   while(cut < line.size() && getWidth(line.substr(0, cut + 1)) <= width) {
    ++cut;
   }
   const std::string part = line.substr(0, cut);
   if(!part.empty()) {
    draw(part, x, y, color);
    y += 8;
   }
   line = line.substr(cut);
  }
  if(!line.empty()) {
   draw(line, x, y, color);
   y += 8;
  }
 }
}
int TextRenderer::splitAndGetHeight(const std::string& text, int width) const {
 if(text.find('\n') != std::string::npos) {
  int total = 0;
  std::size_t start = 0;
  while(start < text.size()) {
   const std::size_t newline = text.find('\n', start);
   const std::string line =
       text.substr(start, newline == std::string::npos ? std::string::npos : newline - start);
   total += splitAndGetHeight(line, width);
   if(newline == std::string::npos) {
    break;
   }
   start = newline + 1;
  }
  return total;
 }
 const std::vector<std::string> words = splitWords(text);
 int height = 0;
 std::size_t index = 0;
 while(index < words.size()) {
  std::string line = words[index++] + " ";
  while(index < words.size() && getWidth(line + words[index]) < width) {
   line += words[index++] + " ";
  }
  while(getWidth(line) > width) {
   std::size_t cut = 0;
   while(cut < line.size() && getWidth(line.substr(0, cut + 1)) <= width) {
    ++cut;
   }
   if(!line.substr(0, cut).empty()) {
    height += 8;
   }
   line = line.substr(cut);
  }
  if(!line.empty()) {
   height += 8;
  }
 }
 if(height < 8) {
  height += 8;
 }
 return height;
}
} // namespace net::minecraft::client::font
