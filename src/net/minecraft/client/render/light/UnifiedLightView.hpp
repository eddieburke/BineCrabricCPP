#pragma once
#include <memory>
namespace net::minecraft {
class World;
namespace client::render {
struct FrameRenderCamera;
}
namespace client::gl {
class ShaderProgram;
}
namespace client::render::light {
class UnifiedLightView {
public:
  static constexpr int kTileSize = 32;
  UnifiedLightView();
  ~UnifiedLightView();
  UnifiedLightView(const UnifiedLightView&) = delete;
  UnifiedLightView& operator=(const UnifiedLightView&) = delete;
  UnifiedLightView(UnifiedLightView&&) noexcept;
  UnifiedLightView& operator=(UnifiedLightView&&) noexcept;
  void update(World* world, double eyeX, double eyeY, double eyeZ);
  bool bind(gl::ShaderProgram& shader,
            int textureUnit,
            const net::minecraft::client::render::FrameRenderCamera& camera,
            int viewportWidth,
            int viewportHeight);
  void reset();
  void destroy();
  [[nodiscard]] bool ready() const;
  [[nodiscard]] int count() const;
  [[nodiscard]] unsigned int positionTexture() const;
  [[nodiscard]] unsigned int colorTexture() const;

private:
  struct State;
  std::unique_ptr<State> state_;
};
} // namespace client::render::light
} // namespace net::minecraft
