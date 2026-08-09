#pragma once
#include <array>
#include <cstddef>
#include <optional>
namespace net::minecraft::client::render {
class AsyncDepthSampler {
 public:
 AsyncDepthSampler() = default;
 ~AsyncDepthSampler();
 AsyncDepthSampler(const AsyncDepthSampler&) = delete;
 AsyncDepthSampler& operator=(const AsyncDepthSampler&) = delete;
 [[nodiscard]] std::optional<float> pollAndIssue(int x, int y);
 void destroy();

 private:
 struct Slot {
  unsigned buffer = 0;
  void* fence = nullptr;
  bool pending = false;
 };
 bool ensureBuffers();
 std::array<Slot, 3> slots_{};
 std::size_t issueCursor_ = 0;
 std::size_t consumeCursor_ = 0;
};
}
