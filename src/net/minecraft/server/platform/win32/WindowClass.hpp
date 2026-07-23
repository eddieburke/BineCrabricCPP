#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
namespace net::minecraft::server::platform::win32 {
class WindowClass {
 public:
 WindowClass(const std::wstring& className,
             WNDPROC windowProc,
             HINSTANCE instance,
             HICON icon = nullptr,
             HCURSOR cursor = nullptr,
             HBRUSH background = nullptr);
 WindowClass(const WindowClass&) = delete;
 WindowClass& operator=(const WindowClass&) = delete;
 WindowClass(WindowClass&& other) noexcept;
 WindowClass& operator=(WindowClass&& other) noexcept;
 ~WindowClass();
 [[nodiscard]] const std::wstring& className() const noexcept {
  return className_;
 }
 [[nodiscard]] ATOM atom() const noexcept {
  return atom_;
 }
 [[nodiscard]] bool registered() const noexcept {
  return registered_;
 }

 private:
 void unregister() noexcept;
 std::wstring className_;
 ATOM atom_ = 0;
 HINSTANCE instance_ = nullptr;
 bool registered_ = false;
};
} // namespace net::minecraft::server::platform::win32
