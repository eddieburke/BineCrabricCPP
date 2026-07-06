#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
namespace net::minecraft::server::platform::win32 {
class MessageLoop {
public:
  [[nodiscard]] static int run();
  static void postQuit(int exitCode = 0) noexcept;
};
} // namespace net::minecraft::server::platform::win32
