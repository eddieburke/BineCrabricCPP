#pragma once
namespace net::minecraft {
class PacketTracker {
 public:
 void update(int packetSize) {
  ++count_;
  size_ += static_cast<long>(packetSize);
 }
 [[nodiscard]] int count() const noexcept {
  return count_;
 }
 [[nodiscard]] long size() const noexcept {
  return size_;
 }

 private:
 int count_ = 0;
 long size_ = 0;
};
} // namespace net::minecraft
