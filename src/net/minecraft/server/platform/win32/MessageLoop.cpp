#include "net/minecraft/server/platform/win32/MessageLoop.hpp"
namespace net::minecraft::server::platform::win32 {
int MessageLoop::run() {
  MSG message{};
  while(GetMessageW(&message, nullptr, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  return static_cast<int>(message.wParam);
}
void MessageLoop::postQuit(int exitCode) noexcept {
  PostQuitMessage(exitCode);
}
} // namespace net::minecraft::server::platform::win32
