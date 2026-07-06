#include "net/minecraft/server/dedicated/gui/DedicatedServerGui.hpp"
#include "net/minecraft/server/dedicated/gui/LogHandler.hpp"
#include "net/minecraft/server/dedicated/gui/PlayerListGui.hpp"
#include "net/minecraft/server/dedicated/gui/PlayerStatsGui.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/ServerLog.hpp"
#include "net/minecraft/server/platform/win32/MessageLoop.hpp"
#include "net/minecraft/server/platform/win32/WindowClass.hpp"
#include <CommCtrl.h>
#ifdef _MSC_VER
#pragma comment(lib, "Comctl32.lib")
#endif
#include <condition_variable>
#include <mutex>
#include <thread>
namespace net::minecraft::server::dedicated::gui {
namespace {
constexpr wchar_t kFrameWindowClass[] = L"MinecraftDedicatedServerGui";
constexpr int kFrameWidth = 854;
constexpr int kFrameHeight = 480;
constexpr int kStatsPanelWidth = 280;
constexpr int kMargin = 8;
constexpr int kGroupInset = 8;
constexpr int kStatsChildWidth = PlayerStatsGui::kWidth;
constexpr int kStatsChildHeight = PlayerStatsGui::kHeight;
std::wstring readWindowTextUtf8(HWND hwnd) {
  const int length = GetWindowTextLengthW(hwnd);
  if(length <= 0) {
    return {};
  }
  std::wstring wide(static_cast<std::size_t>(length), L'\0');
  GetWindowTextW(hwnd, wide.data(), length + 1);
  return wide;
}
std::string wideToUtf8(const std::wstring& text) {
  if(text.empty()) {
    return {};
  }
  const int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr,
                                         nullptr);
  if(length <= 0) {
    return {};
  }
  std::string utf8(static_cast<std::size_t>(length), '\0');
  WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), utf8.data(), length, nullptr, nullptr);
  return utf8;
}
LRESULT CALLBACK logEditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR idSubclass,
                                     DWORD_PTR refData) {
  if(message == LogHandler::WM_APP_APPEND_LOG) {
    return handleLogEditMessage(hwnd, message, wParam, lParam);
  }
  return DefSubclassProc(hwnd, message, wParam, lParam);
}
LRESULT CALLBACK playerListSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR idSubclass,
                                        DWORD_PTR refData) {
  if(message == PlayerListGui::WM_APP_REFRESH_PLAYERS) {
    return handlePlayerListMessage(hwnd, message, wParam, lParam);
  }
  return DefSubclassProc(hwnd, message, wParam, lParam);
}
} // namespace
void DedicatedServerGui::create(MinecraftServer& server) {
  std::mutex readyMutex;
  std::condition_variable readyCv;
  bool ready = false;
  std::thread guiThread([&server, &readyMutex, &readyCv, &ready]() {
    DedicatedServerGui gui(server);
    const bool windowCreated = gui.createWindow();
    {
      std::lock_guard<std::mutex> lock(readyMutex);
      ready = true;
    }
    readyCv.notify_one();
    if(windowCreated) {
      platform::win32::MessageLoop::run();
    }
  });
  {
    std::unique_lock<std::mutex> lock(readyMutex);
    readyCv.wait(lock, [&ready] { return ready; });
  }
  guiThread.detach();
}
DedicatedServerGui::DedicatedServerGui(MinecraftServer& server) : server_(server) {}
DedicatedServerGui::~DedicatedServerGui() {
  if(logHandler_ != nullptr) {
    logHandler_->detach();
  }
  if(commandEditHwnd_ != nullptr) {
    RemoveWindowSubclass(commandEditHwnd_, commandEditProc, 1);
  }
  if(logEditHwnd_ != nullptr) {
    RemoveWindowSubclass(logEditHwnd_, logEditSubclassProc, 2);
  }
  if(playerListGui_ != nullptr && playerListGui_->hwnd() != nullptr) {
    RemoveWindowSubclass(playerListGui_->hwnd(), playerListSubclassProc, 3);
  }
  if(playerStatsGui_ != nullptr) {
    playerStatsGui_->stopTimer();
  }
}
void DedicatedServerGui::sendMessage(const std::string& message) {
  ::net::minecraft::server::ServerLog::LOGGER.info(message);
}
std::string DedicatedServerGui::getName() {
  return "CONSOLE";
}
void DedicatedServerGui::submitCommand() {
  if(commandEditHwnd_ == nullptr) {
    return;
  }
  const std::string command = wideToUtf8(readWindowTextUtf8(commandEditHwnd_));
  SetWindowTextW(commandEditHwnd_, L"");
  std::string trimmed = command;
  while(!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) {
    trimmed.erase(trimmed.begin());
  }
  while(!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t')) {
    trimmed.pop_back();
  }
  if(!trimmed.empty()) {
    server_.queueCommands(trimmed, *this);
  }
}
bool DedicatedServerGui::createWindow() {
  INITCOMMONCONTROLSEX commonControls{sizeof(commonControls), ICC_STANDARD_CLASSES};
  InitCommonControlsEx(&commonControls);
  HINSTANCE instance = GetModuleHandleW(nullptr);
  static platform::win32::WindowClass frameClass{kFrameWindowClass, DedicatedServerGui::frameWindowProc, instance};
  const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
  const int originX = (screenWidth - kFrameWidth) / 2;
  const int originY = (screenHeight - kFrameHeight) / 2;
  frameHwnd_ = CreateWindowExW(0, frameClass.className().c_str(), L"Minecraft server",
                               WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, originX, originY, kFrameWidth,
                               kFrameHeight, nullptr, nullptr, instance, this);
  if(frameHwnd_ == nullptr) {
    return false;
  }
  ShowWindow(frameHwnd_, SW_SHOW);
  UpdateWindow(frameHwnd_);
  return true;
}
void DedicatedServerGui::onClose() {
  server_.stop();
  while(!server_.stopped) {
    Sleep(100);
  }
  DestroyWindow(frameHwnd_);
  platform::win32::MessageLoop::postQuit(0);
  ExitProcess(0);
}
LRESULT CALLBACK DedicatedServerGui::frameWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  DedicatedServerGui* gui = reinterpret_cast<DedicatedServerGui*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if(message == WM_NCCREATE) {
    const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
    gui = reinterpret_cast<DedicatedServerGui*>(createStruct->lpCreateParams);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(gui));
    if(gui != nullptr) {
      gui->frameHwnd_ = hwnd;
    }
    return TRUE;
  }
  if(gui == nullptr) {
    return DefWindowProcW(hwnd, message, wParam, lParam);
  }
  switch(message) {
  case WM_CREATE: {
    const HINSTANCE instance = reinterpret_cast<LPCREATESTRUCTW>(lParam)->hInstance;
    CreateWindowExW(0, L"BUTTON", L"Stats", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, kMargin, kMargin, kStatsPanelWidth,
                    kFrameHeight - kMargin * 2, hwnd, nullptr, instance, nullptr);
    const int statsX = kMargin + kGroupInset;
    const int statsY = kMargin + 20;
    gui->playerStatsGui_ = std::make_unique<PlayerStatsGui>();
    gui->playerStatsGui_->create(hwnd, instance, statsX, statsY, kStatsChildWidth, kStatsChildHeight);
    gui->playerStatsGui_->startTimer();
    CreateWindowExW(0, L"BUTTON", L"Players", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, kMargin, statsY + kStatsChildHeight + 8,
                    kStatsPanelWidth, kFrameHeight - (statsY + kStatsChildHeight + 8) - kMargin, hwnd, nullptr, instance,
                    nullptr);
    const int listY = statsY + kStatsChildHeight + 28;
    const int listHeight = kFrameHeight - listY - kMargin - 8;
    gui->playerListGui_ = std::make_unique<PlayerListGui>(gui->server_);
    gui->playerListGui_->create(hwnd, instance, statsX, listY, kStatsChildWidth, listHeight);
    const int logX = kMargin + kStatsPanelWidth + kMargin;
    const int logWidth = kFrameWidth - logX - kMargin;
    CreateWindowExW(0, L"BUTTON", L"Log and chat", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, logX, kMargin, logWidth,
                    kFrameHeight - kMargin * 2, hwnd, nullptr, instance, nullptr);
    const int logEditY = kMargin + 20;
    const int commandHeight = 24;
    const int logEditHeight = kFrameHeight - logEditY - commandHeight - kMargin - 16;
    gui->logEditHwnd_ =
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | ES_WANTRETURN,
                        logX + kGroupInset, logEditY, logWidth - kGroupInset * 2, logEditHeight, hwnd, nullptr, instance,
                        nullptr);
    gui->commandEditHwnd_ = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, logX + kGroupInset,
        logEditY + logEditHeight + 8, logWidth - kGroupInset * 2, commandHeight, hwnd, nullptr, instance, nullptr);
    SetWindowSubclass(gui->logEditHwnd_, logEditSubclassProc, 2, 0);
    SetWindowSubclass(gui->playerListGui_->hwnd(), playerListSubclassProc, 3, 0);
    SetWindowSubclass(gui->commandEditHwnd_, commandEditProc, 1, reinterpret_cast<DWORD_PTR>(gui));
    {
      auto handler = std::make_unique<LogHandler>(gui->logEditHwnd_);
      gui->logHandler_ = handler.get();
      ::net::minecraft::server::ServerLog::LOGGER.addHandler(std::move(handler));
    }
    return 0;
  }
  case WM_CLOSE:
    gui->onClose();
    return 0;
  case WM_DESTROY:
    platform::win32::MessageLoop::postQuit(0);
    return 0;
  default:
    break;
  }
  return DefWindowProcW(hwnd, message, wParam, lParam);
}
LRESULT CALLBACK DedicatedServerGui::commandEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
                                                     UINT_PTR idSubclass, DWORD_PTR refData) {
  auto* gui = reinterpret_cast<DedicatedServerGui*>(refData);
  if(message == WM_KEYDOWN && wParam == VK_RETURN && gui != nullptr) {
    gui->submitCommand();
    return 0;
  }
  return DefSubclassProc(hwnd, message, wParam, lParam);
}
} // namespace net::minecraft::server::dedicated::gui
