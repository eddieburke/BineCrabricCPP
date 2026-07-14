#pragma once
#include <array>
#include "net/minecraft/client/render/texture/DynamicTexture.hpp"
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::render::texture {
class ClockSprite : public DynamicTexture {
public:
  explicit ClockSprite(Minecraft& minecraft);
  void tick() override;

private:
  Minecraft& minecraft_;
  std::array<std::uint32_t, 256> clock{};
  std::array<std::uint32_t, 256> dial{};
  double angle = 0.0;
  double angleDelta = 0.0;
};
} // namespace net::minecraft::client::render::texture
