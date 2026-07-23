#pragma once
// Shared helpers for dynamic texture sprite ports (beta 1.7.3).
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::render::texture::detail {
inline double mathRandom() {
 static JavaRandom random;
 return random.nextDouble();
}
inline int blockTextureId(int blockId, int fallback) {
 using net::minecraft::block::Block;
 const std::size_t i = static_cast<std::size_t>(blockId);
 return Block::BLOCKS[i] != nullptr ? Block::BLOCKS[i]->textureId : fallback;
}
inline void clamp01(float& f) {
 if(f > 1.0f) {
  f = 1.0f;
 }
 if(f < 0.0f) {
  f = 0.0f;
 }
}
inline void writePixel(std::array<std::uint8_t, 1024>& pixels, int index, int r, int g, int b, int a) {
 pixels[static_cast<std::size_t>(index) * 4 + 0] = static_cast<std::uint8_t>(r);
 pixels[static_cast<std::size_t>(index) * 4 + 1] = static_cast<std::uint8_t>(g);
 pixels[static_cast<std::size_t>(index) * 4 + 2] = static_cast<std::uint8_t>(b);
 pixels[static_cast<std::size_t>(index) * 4 + 3] = static_cast<std::uint8_t>(a);
}
inline void copySpriteArgb(const std::vector<std::uint32_t>& argb,
                           int atlasWidth,
                           int sprite,
                           std::array<std::uint32_t, 256>& out) {
 const int u = (sprite % 16) * 16;
 const int v = (sprite / 16) * 16;
 for(int y = 0; y < 16; ++y) {
  for(int x = 0; x < 16; ++x) {
   const std::size_t src = static_cast<std::size_t>(v + y) * static_cast<std::size_t>(atlasWidth) +
                           static_cast<std::size_t>(u + x);
   const std::size_t dst = static_cast<std::size_t>(y) * 16U + static_cast<std::size_t>(x);
   if(src < argb.size()) {
    out[dst] = argb[src];
   }
  }
 }
}
} // namespace net::minecraft::client::render::texture::detail
