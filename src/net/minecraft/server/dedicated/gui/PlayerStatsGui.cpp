#include "net/minecraft/server/dedicated/gui/PlayerStatsGui.hpp"
#include <Psapi.h>
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/server/platform/win32/Win32Raii.hpp"
#include "net/minecraft/server/platform/win32/WindowClass.hpp"
#ifdef _MSC_VER
#pragma comment(lib, "Psapi.lib")
#endif
#include <malloc.h>
#include <sstream>
namespace net::minecraft::server::dedicated::gui {
namespace {
constexpr wchar_t kStatsWindowClass[] = L"MinecraftPlayerStatsGui";
std::wstring utf8ToWide(const std::string& text) {
 if(text.empty()) {
  return {};
 }
 const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
 if(length <= 0) {
  return {};
 }
 std::wstring wide(static_cast<std::size_t>(length), L'\0');
 MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wide.data(), length);
 return wide;
}
} // namespace
PlayerStatsGui::PlayerStatsGui() = default;
HWND PlayerStatsGui::create(HWND parent, HINSTANCE instance, int x, int y, int width, int height) {
 static platform::win32::WindowClass windowClass{kStatsWindowClass, PlayerStatsGui::windowProc, instance};
 hwnd_ = CreateWindowExW(0,
                         windowClass.className().c_str(),
                         L"",
                         WS_CHILD | WS_VISIBLE,
                         x,
                         y,
                         width,
                         height,
                         parent,
                         nullptr,
                         instance,
                         this);
 return hwnd_;
}
void PlayerStatsGui::startTimer() {
 if(hwnd_ != nullptr) {
  SetTimer(hwnd_, kStatsTimerId, 500, nullptr);
  update();
 }
}
void PlayerStatsGui::stopTimer() {
 if(hwnd_ != nullptr) {
  KillTimer(hwnd_, kStatsTimerId);
 }
}
void PlayerStatsGui::update() {
 _heapmin();
 PROCESS_MEMORY_COUNTERS counters{};
 counters.cb = sizeof(counters);
 GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters));
 MEMORYSTATUSEX memoryStatus{};
 memoryStatus.dwLength = sizeof(memoryStatus);
 GlobalMemoryStatusEx(&memoryStatus);
 const unsigned long long usedBytes = counters.WorkingSetSize;
 const unsigned long long totalVirtual = memoryStatus.ullTotalVirtual;
 const unsigned long long availVirtual = memoryStatus.ullAvailVirtual;
 const unsigned long long freePercent = totalVirtual > 0 ? (availVirtual * 100ULL) / totalVirtual : 0ULL;
 std::ostringstream line0;
 line0 << "Memory use: " << (usedBytes / 1024ULL / 1024ULL) << " mb (" << freePercent << "% free)";
 lines_[0] = utf8ToWide(line0.str());
 std::ostringstream line1;
 line1 << "Threads: " << net::minecraft::Connection::readThreadCounter.load(std::memory_order_relaxed) << " + "
       << net::minecraft::Connection::writeThreadCounter.load(std::memory_order_relaxed);
 lines_[1] = utf8ToWide(line1.str());
 const int maxPercent = totalVirtual > 0 ? static_cast<int>((usedBytes * 100ULL) / totalVirtual) : 0;
 memoryUsePercentage_[static_cast<std::size_t>(memoryUsage_++ & 0xFF)] = maxPercent;
 if(hwnd_ != nullptr) {
  InvalidateRect(hwnd_, nullptr, FALSE);
 }
}
LRESULT CALLBACK PlayerStatsGui::windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
 PlayerStatsGui* stats = reinterpret_cast<PlayerStatsGui*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
 if(message == WM_NCCREATE) {
  const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
  stats = reinterpret_cast<PlayerStatsGui*>(createStruct->lpCreateParams);
  SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(stats));
  if(stats != nullptr) {
   stats->hwnd_ = hwnd;
  }
  return TRUE;
 }
 if(stats == nullptr) {
  return DefWindowProcW(hwnd, message, wParam, lParam);
 }
 switch(message) {
 case WM_TIMER:
  if(wParam == kStatsTimerId) {
   stats->update();
   return 0;
  }
  break;
 case WM_ERASEBKGND:
  return 1;
 case WM_PAINT: {
  PAINTSTRUCT paintStruct{};
  HDC hdc = BeginPaint(hwnd, &paintStruct);
  stats->paint(hdc);
  EndPaint(hwnd, &paintStruct);
  return 0;
 }
 case WM_DESTROY:
  stats->stopTimer();
  break;
 default:
  break;
 }
 return DefWindowProcW(hwnd, message, wParam, lParam);
}
void PlayerStatsGui::paint(HDC hdc) const {
 using platform::win32::makeUniqueHbrush;
 using platform::win32::makeUniqueHfont;
 RECT clientRect{};
 GetClientRect(hwnd_, &clientRect);
 const int width = clientRect.right - clientRect.left;
 auto whiteBrush = makeUniqueHbrush(CreateSolidBrush(RGB(255, 255, 255)));
 FillRect(hdc, &clientRect, whiteBrush.get());
 for(int x = 0; x < 256 && x < width; ++x) {
  const int sample = memoryUsePercentage_[static_cast<std::size_t>((x + memoryUsage_) & 0xFF)];
  const int barHeight = sample;
  const int green = sample + 28;
  auto barBrush = makeUniqueHbrush(CreateSolidBrush(RGB(0, green, 0)));
  RECT barRect{x, 100 - barHeight, x + 1, 100};
  FillRect(hdc, &barRect, barBrush.get());
 }
 SetBkMode(hdc, TRANSPARENT);
 SetTextColor(hdc, RGB(0, 0, 0));
 auto font = makeUniqueHfont(CreateFontW(16,
                                         0,
                                         0,
                                         0,
                                         FW_NORMAL,
                                         FALSE,
                                         FALSE,
                                         FALSE,
                                         DEFAULT_CHARSET,
                                         OUT_DEFAULT_PRECIS,
                                         CLIP_DEFAULT_PRECIS,
                                         DEFAULT_QUALITY,
                                         DEFAULT_PITCH | FF_DONTCARE,
                                         L"Segoe UI"));
 HFONT previousFont = reinterpret_cast<HFONT>(SelectObject(hdc, font.get()));
 for(int line = 0; line < static_cast<int>(lines_.size()); ++line) {
  const std::wstring& text = lines_[static_cast<std::size_t>(line)];
  if(text.empty()) {
   continue;
  }
  TextOutW(hdc, 32, 116 + line * 16, text.c_str(), static_cast<int>(text.size()));
 }
 SelectObject(hdc, previousFont);
}
} // namespace net::minecraft::server::dedicated::gui
