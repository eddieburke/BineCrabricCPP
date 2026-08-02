#pragma once
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <pthread.h>
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
// Descriptive OS name for the current thread.
inline void setCurrentThreadName(std::string_view name) noexcept {
#if defined(_WIN32)
 if(name.empty()) {
  return;
 }
 // SetThreadDescription (Win10 1607+); resolve lazily so older Windows no-ops
 // instead of failing to link. Round-trip through uintptr_t silences
 // -Wcast-function-type (GetProcAddress returns an opaque FARPROC).
 using SetThreadDescriptionFn = HRESULT(WINAPI*)(HANDLE, PCWSTR);
 static const SetThreadDescriptionFn setThreadDescription = []() -> SetThreadDescriptionFn {
  const HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
  if(kernel32 == nullptr) {
   return nullptr;
  }
  const auto proc = reinterpret_cast<std::uintptr_t>(GetProcAddress(kernel32, "SetThreadDescription"));
  return reinterpret_cast<SetThreadDescriptionFn>(proc);
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
#elif defined(__linux__)
 if(name.empty()) {
  return;
 }
 // Linux (e.g. SteamOS) names the thread via pthread_setname_np, which is
 // truncated to 15 bytes in the kernel "comm" buffer (max 14 chars + NUL).
 // glibc's pthread_setname_np may only be declared with _GNU_SOURCE, so declare
 // the ABI ourselves to stay bindable under strict -std=c++20 builds.
 extern "C" int pthread_setname_np(pthread_t thread, const char* tname) noexcept;
 char buf[16];
 const std::size_t n = name.size() < sizeof(buf) - 1 ? name.size() : sizeof(buf) - 1;
 std::memcpy(buf, name.data(), n);
 buf[n] = '\0';
 pthread_setname_np(pthread_self(), buf);
#else
 (void)name;
#endif
}
} // namespace net::minecraft::util::concurrent