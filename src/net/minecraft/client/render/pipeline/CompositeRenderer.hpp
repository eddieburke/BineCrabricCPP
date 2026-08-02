#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace net::minecraft::client::render {
namespace shadowmap {
struct ShadowTargets;
}
class Pipeline;
class PackInstance;

// Fullscreen-pass runner mirroring Iris' CompositeRenderer (renderAll + per-pass
// framebuffer/mipmap/viewport/flip tail + createComputes scheduling). The C++ client
// merged the Java CompositeRenderer/ShadowCompositeRenderer/FinalPassRenderer renderAll
// loops into one stage-keyed loop, so this class is the shared loop; the shadow-comp and
// final-specific branches are inlined as file-local helpers / FinalPassRenderer hooks
// inside render().
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/CompositeRenderer.java
class CompositeRenderer {
 public:
  explicit CompositeRenderer(Pipeline* pipeline) : pipeline_(pipeline) {}

  // Renders one fullscreen stage (begin/prepare/deferred/composite and, via the
  // shadow/final runners, shadowcomp + the final present). `present` is true only for
  // the composite stage. Mirrors CompositeRenderer.renderAll() + createComputes.
  bool render(PackInstance& pack, const std::vector<std::size_t>& passes, bool present,
              const std::string& stage, int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
              const int* shadowColorTextureIds, int shadowColorTextureCount,
              shadowmap::ShadowTargets* shadowTargets, const int* shadowColorAltTextureIds);

 private:
  Pipeline* pipeline_ = nullptr;
};

} // namespace net::minecraft::client::render
