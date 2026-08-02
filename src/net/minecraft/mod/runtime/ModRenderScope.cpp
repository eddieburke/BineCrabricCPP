#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
#include <vector>
namespace net::minecraft::mod::runtime {
namespace {
struct ModWorldDrawFrame {
 net::minecraft::World* world = nullptr;
 float tickDelta = 0.0f;
 ModDrawLayer layer = ModDrawLayer::Auto;
};
std::vector<ModWorldDrawFrame>& drawFrames() {
 thread_local std::vector<ModWorldDrawFrame> value;
 return value;
}
} // namespace
void ModWorldDrawContext::begin(net::minecraft::World* world,
                                const float tickDelta,
                                const ModDrawLayer layer) noexcept {
 drawFrames().push_back({world, tickDelta, layer});
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
ModDrawLayer ModWorldDrawContext::layer() noexcept {
 return drawFrames().empty() ? ModDrawLayer::Auto : drawFrames().back().layer;
}
bool ModWorldDrawContext::active() noexcept {
 return !drawFrames().empty() && drawFrames().back().world != nullptr;
}
ScopedModWorldDrawContext::ScopedModWorldDrawContext(net::minecraft::World* world,
                                                     const float tickDelta,
                                                     const ModDrawLayer layer) noexcept {
 ModWorldDrawContext::begin(world, tickDelta, layer);
 entered_ = true;
}
ScopedModWorldDrawContext::~ScopedModWorldDrawContext() {
 if(entered_) {
  ModWorldDrawContext::end();
 }
}
} // namespace net::minecraft::mod::runtime
