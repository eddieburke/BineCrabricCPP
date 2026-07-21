#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"

namespace net::minecraft::client::render {
RenderCameraState& RenderCameraState::instance() noexcept {
 static RenderCameraState state;
 return state;
}
void RenderCameraState::setFrame(FrameRenderCamera camera) noexcept {
 frame_ = camera;
}
void RenderCameraState::clearFrame() noexcept {
 frame_ = {};
}

} // namespace net::minecraft::client::render
