#include "net/minecraft/client/render/texture/DynamicTextureDetail.hpp"
#include "net/minecraft/block/Block.hpp"
#include <cstddef>
namespace net::minecraft::client::render::texture::detail {
int blockTextureId(int blockId, int fallback) {
  using net::minecraft::block::Block;
  const std::size_t i = static_cast<std::size_t>(blockId);
  return Block::BLOCKS[i] != nullptr ? Block::BLOCKS[i]->textureId : fallback;
}
} // namespace net::minecraft::client::render::texture::detail
