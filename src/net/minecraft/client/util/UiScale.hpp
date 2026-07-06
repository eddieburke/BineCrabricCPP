#pragma once
#include "net/minecraft/client/option/GameOptions.hpp"
#include <cmath>
#include <utility>
namespace net::minecraft::client::util {
struct UiScale {
  int scaledWidth = 0;
  int scaledHeight = 0;
  double rawWidth = 0.0;
  double rawHeight = 0.0;
  int factor = 1;
};
inline UiScale uiScale(const option::GameOptions& options, int fbWidth, int fbHeight) noexcept {
  UiScale s;
  s.scaledWidth = fbWidth;
  s.scaledHeight = fbHeight;
  int target = options.guiScale;
  if(target == 0) {
    target = 1000;
  }
  while(s.factor < target && s.scaledWidth / (s.factor + 1) >= 320 && s.scaledHeight / (s.factor + 1) >= 240) {
    ++s.factor;
  }
  s.rawWidth = static_cast<double>(s.scaledWidth) / static_cast<double>(s.factor);
  s.rawHeight = static_cast<double>(s.scaledHeight) / static_cast<double>(s.factor);
  s.scaledWidth = static_cast<int>(std::ceil(s.rawWidth));
  s.scaledHeight = static_cast<int>(std::ceil(s.rawHeight));
  return s;
}
inline std::pair<int, int> mapScreenMouse(int displayWidth, int displayHeight, int scaledWidth, int scaledHeight,
                                          int eventX, int eventY) noexcept {
  const int scaledX = displayWidth > 0 ? eventX * scaledWidth / displayWidth : eventX;
  const int scaledY =
      displayHeight > 0 ? scaledHeight - eventY * scaledHeight / displayHeight - 1 : scaledHeight - eventY - 1;
  return {scaledX, scaledY};
}
} // namespace net::minecraft::client::util
