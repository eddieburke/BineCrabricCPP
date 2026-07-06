#pragma once
namespace net::minecraft::util {
class Tickable {
public:
  virtual ~Tickable() = default;
  virtual void tick() = 0;
};
} // namespace net::minecraft::util
