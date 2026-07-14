#include "net/minecraft/server/platform/win32/WindowClass.hpp"
namespace net::minecraft::server::platform::win32 {
namespace {
constexpr ULONG_PTR kIconApplication = 32512;
constexpr ULONG_PTR kCursorArrow = 32512;
} // namespace
WindowClass::WindowClass(const std::wstring& className,
                         WNDPROC windowProc,
                         HINSTANCE instance,
                         HICON icon,
                         HCURSOR cursor,
                         HBRUSH background)
    : className_(className), instance_(instance) {
  WNDCLASSEXW windowClass{};
  windowClass.cbSize = sizeof(windowClass);
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = windowProc;
  windowClass.hInstance = instance_;
  windowClass.hIcon = icon != nullptr ? icon : LoadIconW(nullptr, reinterpret_cast<LPCWSTR>(kIconApplication));
  windowClass.hCursor = cursor != nullptr ? cursor : LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(kCursorArrow));
  windowClass.hbrBackground = background != nullptr ? background : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  windowClass.lpszClassName = className_.c_str();
  atom_ = RegisterClassExW(&windowClass);
  registered_ = atom_ != 0;
}
WindowClass::WindowClass(WindowClass&& other) noexcept
    : className_(std::move(other.className_)),
      atom_(other.atom_),
      instance_(other.instance_),
      registered_(other.registered_) {
  other.atom_ = 0;
  other.registered_ = false;
}
WindowClass& WindowClass::operator=(WindowClass&& other) noexcept {
  if(this != &other) {
    unregister();
    className_ = std::move(other.className_);
    atom_ = other.atom_;
    instance_ = other.instance_;
    registered_ = other.registered_;
    other.atom_ = 0;
    other.registered_ = false;
  }
  return *this;
}
WindowClass::~WindowClass() {
  unregister();
}
void WindowClass::unregister() noexcept {
  if(registered_ && instance_ != nullptr) {
    UnregisterClassW(className_.c_str(), instance_);
    registered_ = false;
    atom_ = 0;
  }
}
} // namespace net::minecraft::server::platform::win32
