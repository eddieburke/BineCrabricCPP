#pragma once
#include <array>
#include "net/minecraft/client/render/texture/DynamicTexture.hpp"
namespace net::minecraft::client::render::texture {
class FireSprite : public DynamicTexture {
public:
  explicit FireSprite(int index);
  void tick() override;

protected:
  std::array<float, 320> current{};
  std::array<float, 320> next{};
};
} // namespace net::minecraft::client::render::texture
