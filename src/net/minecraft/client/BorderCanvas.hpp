#pragma once
namespace net::minecraft::client {
class BorderCanvas {
public:
  explicit BorderCanvas(int size = 0);
  int size() const noexcept;
  int preferredWidth() const noexcept;
  int preferredHeight() const noexcept;

private:
  int size_ = 0;
};
} // namespace net::minecraft::client
