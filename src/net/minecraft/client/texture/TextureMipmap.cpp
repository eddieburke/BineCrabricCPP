#include "net/minecraft/client/texture/TextureMipmap.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace net::minecraft::client::texture {
namespace {
std::uint8_t blendedChannel(const std::uint8_t* a,
                            const std::uint8_t* b,
                            const std::uint8_t* c,
                            const std::uint8_t* d,
                            int channel,
                            int alphaSum) {
 if(alphaSum == 0) {
  return 0;
 }
 const auto weightedSquare = [channel](const std::uint8_t* pixel) {
  const double value = pixel[channel];
  return value * value * pixel[3];
 };
 const double linear = (weightedSquare(a) + weightedSquare(b) + weightedSquare(c) + weightedSquare(d)) /
                       static_cast<double>(alphaSum);
 return static_cast<std::uint8_t>(std::clamp(std::lround(std::sqrt(linear)), 0L, 255L));
}
} // namespace
void downsampleRgbaMipmap(std::span<const std::uint8_t> source,
                          int sourceWidth,
                          int sourceHeight,
                          std::span<std::uint8_t> destination,
                          int tileColumns,
                          int tileRows) {
 if(sourceWidth < 2 || sourceHeight < 2 || tileColumns < 1 || tileRows < 1 ||
    sourceWidth % (tileColumns * 2) != 0 || sourceHeight % (tileRows * 2) != 0) {
  throw std::invalid_argument("Invalid mipmap dimensions");
 }
 const std::size_t sourceBytes = static_cast<std::size_t>(sourceWidth) * sourceHeight * 4;
 const int targetWidth = sourceWidth / 2;
 const int targetHeight = sourceHeight / 2;
 const std::size_t targetBytes = static_cast<std::size_t>(targetWidth) * targetHeight * 4;
 if(source.size() < sourceBytes || destination.size() < targetBytes) {
  throw std::invalid_argument("Mipmap buffer is too small");
 }
 const int sourceTileWidth = sourceWidth / tileColumns;
 const int sourceTileHeight = sourceHeight / tileRows;
 const int targetTileWidth = sourceTileWidth / 2;
 const int targetTileHeight = sourceTileHeight / 2;
 for(int tileY = 0; tileY < tileRows; ++tileY) {
  for(int tileX = 0; tileX < tileColumns; ++tileX) {
   for(int y = 0; y < targetTileHeight; ++y) {
    for(int x = 0; x < targetTileWidth; ++x) {
     const int sourceX = tileX * sourceTileWidth + x * 2;
     const int sourceY = tileY * sourceTileHeight + y * 2;
     const std::uint8_t* a = source.data() + static_cast<std::size_t>(sourceX + sourceY * sourceWidth) * 4;
     const std::uint8_t* b = a + 4;
     const std::uint8_t* c = a + static_cast<std::size_t>(sourceWidth) * 4;
     const std::uint8_t* d = c + 4;
     const int targetX = tileX * targetTileWidth + x;
     const int targetY = tileY * targetTileHeight + y;
     std::uint8_t* out = destination.data() + static_cast<std::size_t>(targetX + targetY * targetWidth) * 4;
     const int alphaSum = a[3] + b[3] + c[3] + d[3];
     out[0] = blendedChannel(a, b, c, d, 0, alphaSum);
     out[1] = blendedChannel(a, b, c, d, 1, alphaSum);
     out[2] = blendedChannel(a, b, c, d, 2, alphaSum);
     out[3] = static_cast<std::uint8_t>((alphaSum + 2) / 4);
    }
   }
  }
 }
}
std::vector<RgbaMipmapLevel> buildRgbaMipmaps(std::span<const std::uint8_t> base,
                                              int width,
                                              int height,
                                              int maxLevels,
                                              int tileColumns,
                                              int tileRows) {
 std::vector<RgbaMipmapLevel> levels;
 if(width <= 0 || height <= 0 || maxLevels <= 0) {
  return levels;
 }
 std::span<const std::uint8_t> source = base;
 int sourceWidth = width;
 int sourceHeight = height;
 for(int level = 0; level < maxLevels; ++level) {
  if(sourceWidth % (tileColumns * 2) != 0 || sourceHeight % (tileRows * 2) != 0) {
   break;
  }
  RgbaMipmapLevel mip;
  mip.width = sourceWidth / 2;
  mip.height = sourceHeight / 2;
  mip.pixels.resize(static_cast<std::size_t>(mip.width) * mip.height * 4);
  downsampleRgbaMipmap(source, sourceWidth, sourceHeight, mip.pixels, tileColumns, tileRows);
  levels.push_back(std::move(mip));
  source = levels.back().pixels;
  sourceWidth = levels.back().width;
  sourceHeight = levels.back().height;
 }
 return levels;
}
} // namespace net::minecraft::client::texture
