#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <array>
#include <string>
namespace net::minecraft::server::dedicated::gui {
class PlayerStatsGui {
 public:
 static constexpr int kWidth = 256;
 static constexpr int kHeight = 196;
 static constexpr UINT WM_APP_STATS_REPAINT = WM_APP + 2;
 static constexpr UINT_PTR kStatsTimerId = 1;
 PlayerStatsGui();
 HWND create(HWND parent, HINSTANCE instance, int x, int y, int width, int height);
 void startTimer();
 void stopTimer();
 void update();
 [[nodiscard]] HWND hwnd() const noexcept {
  return hwnd_;
 }

 private:
 static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
 void paint(HDC hdc) const;
 HWND hwnd_ = nullptr;
 std::array<int, 256> memoryUsePercentage_{};
 int memoryUsage_ = 0;
 std::array<std::wstring, 10> lines_{};
};
} // namespace net::minecraft::server::dedicated::gui
