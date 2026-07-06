#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "net/minecraft/util/Tickable.hpp"
#include <windows.h>
#include <string>
#include <vector>
namespace net::minecraft::server {
class MinecraftServer;
}
namespace net::minecraft::server::dedicated::gui {
class PlayerListGui : public ::net::minecraft::util::Tickable {
public:
  static constexpr UINT WM_APP_REFRESH_PLAYERS = WM_APP + 3;
  explicit PlayerListGui(MinecraftServer& server);
  ~PlayerListGui() override;
  PlayerListGui(const PlayerListGui&) = delete;
  PlayerListGui& operator=(const PlayerListGui&) = delete;
  HWND create(HWND parent, HINSTANCE instance, int x, int y, int width, int height);
  void tick() override;
  void applyPlayerNames(const std::vector<std::wstring>& names);
  [[nodiscard]] HWND hwnd() const noexcept {
    return listboxHwnd_;
  }

private:
  MinecraftServer& server_;
  HWND listboxHwnd_ = nullptr;
  int tick_ = 0;
};
LRESULT handlePlayerListMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
} // namespace net::minecraft::server::dedicated::gui
