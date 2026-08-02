#pragma once

namespace net::minecraft::client::render {
class ColorTargets;
class Pipeline;
class PackInstance;

// Final-pass runner mirroring Iris' FinalPassRenderer: owns the end-of-final-stage mipmap
// reset + packWroteToScreen_ bookkeeping (FinalPassRenderer.java:283-286) and presents
// colortex0 / the `final` program to the screen (renderFinalPass, 197-319). The final
// program's fullscreen draw shares CompositeRenderer's loop (CompositeRenderer.cpp calls
// this class's finish() from its present tail); presentToScreen is the shader/copy path.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/FinalPassRenderer.java
class FinalPassRenderer {
 public:
  explicit FinalPassRenderer(Pipeline* pipeline) : pipeline_(pipeline) {}

  // FinalPassRenderer.renderFinalPass tail: resets mipmapping on every target and records
  // whether the final stage wrote to the screen (packWroteToScreen_).
  void finish(ColorTargets& targets, bool wroteToScreen);

  // Presents colortex0 through the `final` program (or the copy fallback), the C++ analog
  // of FinalPassRenderer.renderFinalPass' shader/copy paths.
  void presentToScreen(PackInstance* scenePack, int screenWidth, int screenHeight);

 private:
  Pipeline* pipeline_ = nullptr;
};

} // namespace net::minecraft::client::render
