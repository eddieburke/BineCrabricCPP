#pragma once
#include "net/minecraft/client/gui/screen/LoadingDisplay.hpp"
#include <cstdint>
#include <string>
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::render {
// Faithful port of net.minecraft.client.render.ProgressRenderer (beta 1.7.3 MCP).
class ProgressRenderer : public gui::screen::LoadingDisplay {
public:
  explicit ProgressRenderer(Minecraft* minecraft = nullptr);
  void progressStart(const std::string& title) override;
  void progressStartNoAbort(std::string title);
  void start(const std::string& title);
  void progressStage(const std::string& stage) override;
  void progressStagePercentage(int percentage) override;

private:
  void setupLoadingProjection();
  void renderLoadingFrame(int percentage);
  [[nodiscard]] bool checkRunningOrAbort() const;
  Minecraft* minecraft = nullptr;
  std::string stage{};
  std::string title{};
  std::int64_t lastTime = 0;
  bool noAbort = false;
};
} // namespace net::minecraft::client::render
