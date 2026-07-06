#pragma once
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client::render {
class Culler {
public:
  virtual ~Culler() = default;
  virtual void prepare(double x, double y, double z) = 0;
  [[nodiscard]] virtual bool isVisible(const net::minecraft::Box& box) const = 0;
};
} // namespace net::minecraft::client::render
