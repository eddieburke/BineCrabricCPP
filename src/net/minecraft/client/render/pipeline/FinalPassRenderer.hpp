#pragma once

namespace net::minecraft::client::render {
class ColorTargets;
class Pipeline;
class PackInstance;

// reset + packWroteToScreen_ bookkeeping (FinalPassRenderer.java:283-286) and presents
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/FinalPassRenderer.java
class FinalPassRenderer {
 public:
  explicit FinalPassRenderer(Pipeline* pipeline) : pipeline_(pipeline) {}

  void finish(ColorTargets& targets, bool wroteToScreen);

  void presentToScreen(PackInstance* scenePack, int screenWidth, int screenHeight);

 private:
  Pipeline* pipeline_ = nullptr;
};

} // namespace net::minecraft::client::render
