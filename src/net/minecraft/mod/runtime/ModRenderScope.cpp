#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
#include <vector>
namespace net::minecraft::mod::runtime {
namespace {
struct ModWorldDrawFrame {
  net::minecraft::World* world = nullptr;
  float tickDelta = 0.0f;
};
std::vector<ModWorldDrawFrame>& drawFrames() {
  thread_local std::vector<ModWorldDrawFrame> value;
  return value;
}
} // namespace
void ModWorldDrawContext::begin(net::minecraft::World* world, const float tickDelta) noexcept {
  drawFrames().push_back({world, tickDelta});
}
void ModWorldDrawContext::end() noexcept {
  if(!drawFrames().empty()) {
    drawFrames().pop_back();
  }
}
net::minecraft::World* ModWorldDrawContext::world() noexcept {
  return drawFrames().empty() ? nullptr : drawFrames().back().world;
}
float ModWorldDrawContext::tickDelta() noexcept {
  return drawFrames().empty() ? 0.0f : drawFrames().back().tickDelta;
}
bool ModWorldDrawContext::active() noexcept {
  return !drawFrames().empty() && drawFrames().back().world != nullptr;
}
ScopedModWorldDrawContext::ScopedModWorldDrawContext(net::minecraft::World* world, const float tickDelta) noexcept {
  ModWorldDrawContext::begin(world, tickDelta);
  entered_ = true;
}
ScopedModWorldDrawContext::~ScopedModWorldDrawContext() {
  if(entered_) {
    ModWorldDrawContext::end();
  }
}
} // namespace net::minecraft::mod::runtime
