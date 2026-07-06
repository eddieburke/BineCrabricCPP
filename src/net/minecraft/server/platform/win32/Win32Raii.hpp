#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <memory>
namespace net::minecraft::server::platform::win32 {
struct HwndDeleter {
  void operator()(HWND hwnd) const noexcept {
    if(hwnd != nullptr) {
      DestroyWindow(hwnd);
    }
  }
};
struct HfontDeleter {
  void operator()(HFONT font) const noexcept {
    if(font != nullptr) {
      DeleteObject(font);
    }
  }
};
struct HbrushDeleter {
  void operator()(HBRUSH brush) const noexcept {
    if(brush != nullptr) {
      DeleteObject(brush);
    }
  }
};
struct ReleaseDcDeleter {
  HWND owner = nullptr;
  void operator()(HDC hdc) const noexcept {
    if(hdc != nullptr && owner != nullptr) {
      ReleaseDC(owner, hdc);
    }
  }
};
struct DeleteDcDeleter {
  void operator()(HDC hdc) const noexcept {
    if(hdc != nullptr) {
      DeleteDC(hdc);
    }
  }
};
using UniqueHwnd = std::unique_ptr<HWND__, HwndDeleter>;
using UniqueHfont = std::unique_ptr<HFONT__, HfontDeleter>;
using UniqueHbrush = std::unique_ptr<HBRUSH__, HbrushDeleter>;
using UniqueReleaseDc = std::unique_ptr<HDC__, ReleaseDcDeleter>;
using UniqueDeleteDc = std::unique_ptr<HDC__, DeleteDcDeleter>;
[[nodiscard]] inline UniqueHwnd makeUniqueHwnd(HWND hwnd) noexcept {
  return UniqueHwnd(hwnd);
}
[[nodiscard]] inline UniqueHfont makeUniqueHfont(HFONT font) noexcept {
  return UniqueHfont(font);
}
[[nodiscard]] inline UniqueHbrush makeUniqueHbrush(HBRUSH brush) noexcept {
  return UniqueHbrush(brush);
}
[[nodiscard]] inline UniqueReleaseDc makeUniqueReleaseDc(HWND owner, HDC hdc) noexcept {
  ReleaseDcDeleter deleter{owner};
  return UniqueReleaseDc(hdc, deleter);
}
[[nodiscard]] inline UniqueDeleteDc makeUniqueDeleteDc(HDC hdc) noexcept {
  return UniqueDeleteDc(hdc);
}
} // namespace net::minecraft::server::platform::win32
