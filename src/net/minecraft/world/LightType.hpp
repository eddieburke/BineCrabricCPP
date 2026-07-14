#pragma once
namespace net::minecraft {
enum class LightType {
  Sky = 15,
  Block = 0
};
inline int lightValue(LightType type) {
  return type == LightType::Sky ? 15 : 0;
}
} // namespace net::minecraft
