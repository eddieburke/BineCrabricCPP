#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <memory>
#include <string>
#include "net/minecraft/server/command/CommandOutput.hpp"
namespace net::minecraft::server {
class MinecraftServer;
}
namespace net::minecraft::server::dedicated::gui {
class LogHandler;
class PlayerListGui;
class PlayerStatsGui;
class DedicatedServerGui : public command::CommandOutput {
public:
  static void create(MinecraftServer& server);
  explicit DedicatedServerGui(MinecraftServer& server);
  ~DedicatedServerGui() override;
  DedicatedServerGui(const DedicatedServerGui&) = delete;
  DedicatedServerGui& operator=(const DedicatedServerGui&) = delete;
  void sendMessage(const std::string& message) override;
  [[nodiscard]] std::string getName() override;
  void submitCommand();

private:
  static LRESULT CALLBACK frameWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK
  commandEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR idSubclass, DWORD_PTR refData);
  [[nodiscard]] bool createWindow();
  void onClose();
  MinecraftServer& server_;
  HWND frameHwnd_ = nullptr;
  HWND logEditHwnd_ = nullptr;
  HWND commandEditHwnd_ = nullptr;
  LogHandler* logHandler_ = nullptr;
  std::unique_ptr<PlayerStatsGui> playerStatsGui_;
  std::unique_ptr<PlayerListGui> playerListGui_;
};
} // namespace net::minecraft::server::dedicated::gui
