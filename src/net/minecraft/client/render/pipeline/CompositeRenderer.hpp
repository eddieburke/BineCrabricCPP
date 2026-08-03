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

// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/pipeline/CompositeRenderer.java
class CompositeRenderer {
 public:
  explicit CompositeRenderer(Pipeline* pipeline) : pipeline_(pipeline) {}

  bool render(PackInstance& pack, const std::vector<std::size_t>& passes, bool present,
              const std::string& stage, int shadowDepthTextureId, int shadowOpaqueDepthTextureId,
              const int* shadowColorTextureIds, int shadowColorTextureCount,
              shadowmap::ShadowTargets* shadowTargets, const int* shadowColorAltTextureIds);

 private:
  Pipeline* pipeline_ = nullptr;
};

} // namespace net::minecraft::client::render
