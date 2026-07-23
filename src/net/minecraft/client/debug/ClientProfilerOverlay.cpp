#include "net/minecraft/client/debug/ClientProfilerOverlay.hpp"
#include <GL/gl.h>
#include <chrono>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
namespace net::minecraft::client::debug {
namespace {
std::int64_t nanoTime() {
 return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
     .count();
}
class DebugChartScope {
 public:
 DebugChartScope() : saved_(render::RenderSystem::getShadow()) {
  render::RenderSystem::disableCull();
  render::RenderSystem::disableTexture();
 }
 ~DebugChartScope() {
  render::RenderSystem::setShadow(saved_);
 }
 DebugChartScope(const DebugChartScope&) = delete;
 DebugChartScope& operator=(const DebugChartScope&) = delete;

 private:
 render::RenderSystem::StateShadow saved_;
};
void renderProfilerChartInViewport(int viewportWidth, int viewportHeight) {
 constexpr std::int64_t frameBudgetNs = 16666666LL;
 const int frameTimeIndex = ClientProfilerOverlay::frameTimeIndex;
 const auto& frameTimes = ClientProfilerOverlay::frameTimes;
 const auto& tickTimes = ClientProfilerOverlay::tickTimes;
 const DebugChartScope chartCaps;
 render::RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
 render::RenderSystem::clear(0x00000100);
 render::RenderSystem::matrixMode(0x1701);
 render::RenderSystem::loadIdentity();
 render::RenderSystem::ortho(0.0, viewportWidth, viewportHeight, 0.0, 1000.0, 3000.0);
 render::RenderSystem::matrixMode(0x1700);
 render::RenderSystem::loadIdentity();
 render::RenderSystem::translate(0.0f, 0.0f, -2000.0f);
 glLineWidth(1.0f);
 render::Tessellator& tessellator = render::Tessellator::INSTANCE;
 const int budgetPixels = static_cast<int>(frameBudgetNs / 200000LL);
 tessellator.start(7);
 tessellator.color(0x20000000);
 tessellator.vertex(0.0, static_cast<double>(viewportHeight - budgetPixels), 0.0);
 tessellator.vertex(0.0, static_cast<double>(viewportHeight), 0.0);
 tessellator.vertex(static_cast<double>(frameTimes.size()), static_cast<double>(viewportHeight), 0.0);
 tessellator.vertex(static_cast<double>(frameTimes.size()), static_cast<double>(viewportHeight - budgetPixels), 0.0);
 tessellator.color(0x20200000);
 tessellator.vertex(0.0, static_cast<double>(viewportHeight - budgetPixels * 2), 0.0);
 tessellator.vertex(0.0, static_cast<double>(viewportHeight - budgetPixels), 0.0);
 tessellator.vertex(static_cast<double>(frameTimes.size()), static_cast<double>(viewportHeight - budgetPixels), 0.0);
 tessellator.vertex(
     static_cast<double>(frameTimes.size()), static_cast<double>(viewportHeight - budgetPixels * 2), 0.0);
 tessellator.draw();
 std::int64_t frameSum = 0;
 for(const std::int64_t frameTime : frameTimes) {
  frameSum += frameTime;
 }
 const int averagePixels = std::clamp(
     static_cast<int>(frameSum / 200000LL / static_cast<std::int64_t>(frameTimes.size())), 0, viewportHeight);
 tessellator.start(7);
 tessellator.color(0x20400000);
 tessellator.vertex(0.0, static_cast<double>(viewportHeight - averagePixels), 0.0);
 tessellator.vertex(0.0, static_cast<double>(viewportHeight), 0.0);
 tessellator.vertex(static_cast<double>(frameTimes.size()), static_cast<double>(viewportHeight), 0.0);
 tessellator.vertex(
     static_cast<double>(frameTimes.size()), static_cast<double>(viewportHeight - averagePixels), 0.0);
 tessellator.draw();
 tessellator.start(1);
 for(int i = 0; i < static_cast<int>(frameTimes.size()); ++i) {
  const int shade = ((i - frameTimeIndex) & static_cast<int>(frameTimes.size() - 1)) * 255 /
                    static_cast<int>(frameTimes.size());
  int red = shade * shade / 255;
  red = red * red / 255;
  red = red * red / 255;
  red = red * red / 255;
  if(frameTimes[static_cast<std::size_t>(i)] > frameBudgetNs) {
   tessellator.color(0xFF000000 + red * 65536);
  } else {
   tessellator.color(0xFF000000 + red * 256);
  }
  const long maxH = static_cast<long>(viewportHeight);
  const long framePixels =
      std::clamp(static_cast<long>(frameTimes[static_cast<std::size_t>(i)] / 200000LL), 0L, maxH);
  const long tickPixels =
      std::clamp(static_cast<long>(tickTimes[static_cast<std::size_t>(i)] / 200000LL), 0L, framePixels);
  tessellator.vertex(static_cast<float>(i) + 0.5f, static_cast<float>(maxH - framePixels) + 0.5f, 0.0);
  tessellator.vertex(static_cast<float>(i) + 0.5f, static_cast<float>(maxH) + 0.5f, 0.0);
  tessellator.color(0xFF000000 + red * 65536 + red * 256 + red);
  tessellator.vertex(static_cast<float>(i) + 0.5f, static_cast<float>(maxH - framePixels) + 0.5f, 0.0);
  tessellator.vertex(
      static_cast<float>(i) + 0.5f, static_cast<float>(maxH - (framePixels - tickPixels)) + 0.5f, 0.0);
 }
 tessellator.draw();
}
} // namespace
void ClientProfilerOverlay::recordFrameTime(Minecraft& client) {
 client.timeAfterLastTick = nanoTime();
}
void ClientProfilerOverlay::renderProfilerChart(Minecraft& client, std::int64_t tickTime) {
 if(client.timeAfterLastTick == -1) {
  client.timeAfterLastTick = nanoTime();
 }
 const std::int64_t now = nanoTime();
 tickTimes[static_cast<std::size_t>(frameTimeIndex & (tickTimes.size() - 1))] = tickTime;
 frameTimes[static_cast<std::size_t>(frameTimeIndex++ & (frameTimes.size() - 1))] = now - client.timeAfterLastTick;
 client.timeAfterLastTick = now;
 renderProfilerChartInViewport(client.displayWidth, client.displayHeight);
}
std::string ClientProfilerOverlay::getRenderChunkDebugInfo(const Minecraft& client) {
 return client.worldRenderer != nullptr ? client.worldRenderer->getChunkDebugInfo() : std::string{};
}
std::string ClientProfilerOverlay::getRenderEntityDebugInfo(const Minecraft& client) {
 return client.worldRenderer != nullptr ? client.worldRenderer->getEntityDebugInfo() : std::string{};
}
std::string ClientProfilerOverlay::getChunkSourceDebugInfo(const Minecraft& client) {
 if(client.world == nullptr) {
  return {};
 }
 if(ChunkSource* chunkSource = client.world->getChunkSource()) {
  return chunkSource->getDebugInfo();
 }
 return client.world->describe();
}
std::string ClientProfilerOverlay::getWorldDebugInfo(const Minecraft& client) {
 if(client.world == nullptr) {
  return {};
 }
 return "P: " + client.particleManager.toString() + ". T: All: " + std::to_string(client.world->entities().size());
}
} // namespace net::minecraft::client::debug
