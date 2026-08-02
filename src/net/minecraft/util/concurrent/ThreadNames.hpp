#pragma once
#include <cstdlib>
#include <string>
#include <string_view>
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace net::minecraft::util::concurrent {
// Domain this thread belongs to; used by GL/main-thread confinement asserts.
inline thread_local Domain tl_domain = Domain::Compute;
inline thread_local bool tl_is_main_thread = false;
// Marks the calling thread as the game's main (GL) thread, captured at startup.
inline void setMainThread() noexcept {
 tl_is_main_thread = true;
}
// Debug-only guard for GL/main-thread-confined state (WI-5 alphaTestRef etc.).
inline void assertOnMainThread() noexcept {
#ifndef NDEBUG
 if(!tl_is_main_thread) {
  std::abort();
 }
#endif
}
// Descriptive OS name for the current thread. Uses SetThreadDescription (Windows
// 10 1607+, loaded dynamically so older systems no-op); falls back to a no-op.
inline void setCurrentThreadName(std::string_view name) noexcept {
#ifdef _WIN32
 if(name.empty()) {
  return;
 }
 using SetThreadDescriptionFn = HRESULT(WINAPI*)(HANDLE, PCWSTR);
 static const SetThreadDescriptionFn setThreadDescription = []() -> SetThreadDescriptionFn {
  const HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
  if(kernel32 == nullptr) {
   return nullptr;
  }
  return reinterpret_cast<SetThreadDescriptionFn>(GetProcAddress(kernel32, "SetThreadDescription"));
 }();
 if(setThreadDescription == nullptr) {
  return;
 }
 const int wide = MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), nullptr, 0);
 if(wide <= 0) {
  return;
 }
 std::wstring buffer(static_cast<std::size_t>(wide), L'\0');
 MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), buffer.data(), wide);
 setThreadDescription(GetCurrentThread(), buffer.c_str());
#else
 (void)name;
#endif
}
} // namespace net::minecraft::util::concurrent
