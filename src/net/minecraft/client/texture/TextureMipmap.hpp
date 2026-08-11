#pragma once
#include <cstdint>
#include <span>
#include <vector>
namespace net::minecraft::client::texture {
struct RgbaMipmapLevel {
 int width = 0;
 int height = 0;
 std::vector<std::uint8_t> pixels;
};
void downsampleRgbaMipmap(std::span<const std::uint8_t> source,
                          int sourceWidth,
                          int sourceHeight,
                          std::span<std::uint8_t> destination,
                          int tileColumns = 1,
                          int tileRows = 1);
[[nodiscard]] std::vector<RgbaMipmapLevel> buildRgbaMipmaps(std::span<const std::uint8_t> base,
                                                            int width,
                                                            int height,
                                                            int maxLevels,
                                                            int tileColumns = 1,
                                                            int tileRows = 1);
} // namespace net::minecraft::client::texture
