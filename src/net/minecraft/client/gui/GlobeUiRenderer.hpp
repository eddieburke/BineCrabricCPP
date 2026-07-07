#pragma once
#include <string>
#include <vector>
namespace net::minecraft::client::gui::globe {
struct DrawParams {
  int globeX = 0;
  int globeY = 0;
  int globeSize = 0;
  int guiWidth = 0;
  int guiHeight = 0;
  float pinLat = 0.0f;
  float pinLon = 0.0f;
  float yawDeg = 0.0f;
  float pitchDeg = 0.0f;
  float camDist = 2.05f;
};
struct PickParams {
  int mouseX = 0;
  int mouseY = 0;
  int globeX = 0;
  int globeY = 0;
  int globeSize = 0;
  float yawDeg = 0.0f;
  float pitchDeg = 0.0f;
  float camDist = 2.05f;
};
void clearCoastPaths();
void loadCoastText(const std::string& text);
void render(const DrawParams& params);
[[nodiscard]] bool pickLatLon(const PickParams& params, float& outLat, float& outLon);
[[nodiscard]] bool containsGlobePoint(int mouseX, int mouseY, int globeX, int globeY, int globeSize);
} // namespace net::minecraft::client::gui::globe
