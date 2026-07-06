#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "net/minecraft/server/ServerLog.hpp"
#include <windows.h>
#include <string>
namespace net::minecraft::server::dedicated::gui {
class LogHandler final : public ::net::minecraft::server::LogHandler {
public:
  static constexpr UINT WM_APP_APPEND_LOG = WM_APP + 1;
  explicit LogHandler(HWND logEditHwnd);
  ~LogHandler() override = default;
  LogHandler(const LogHandler&) = delete;
  LogHandler& operator=(const LogHandler&) = delete;
  void publish(const LogRecord& record) override;
  void detach() noexcept;
  void applyAppend(HWND logEditHwnd, const std::wstring& text);
  [[nodiscard]] HWND hwnd() const noexcept {
    return logEditHwnd_;
  }

private:
  [[nodiscard]] static std::wstring formatRecord(const LogRecord& record);
  HWND logEditHwnd_ = nullptr;
  int lineCharCounts_[1024]{};
  int bufferIndex_ = 0;
};
LRESULT handleLogEditMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
} // namespace net::minecraft::server::dedicated::gui
