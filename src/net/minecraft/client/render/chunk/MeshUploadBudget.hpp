#pragma once
#include <cstddef>
namespace net::minecraft::client::render::chunk {
class MeshUploadBudget {
 public:
 explicit MeshUploadBudget(std::size_t limit = 8u * 1024u * 1024u) : limit_(limit) {}
 [[nodiscard]] bool tryConsume(std::size_t bytes) noexcept {
  if(used_ != 0 && (used_ >= limit_ || bytes > limit_ - used_)) return false;
  used_ += bytes;
  return true;
 }
 [[nodiscard]] std::size_t used() const noexcept { return used_; }
 private:
 std::size_t limit_;
 std::size_t used_ = 0;
};
}
