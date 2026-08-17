#include "net/minecraft/client/platform/Browser.hpp"
#include <cstdint>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <shellapi.h>
#else
#include <cerrno>
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif
namespace net::minecraft::client::platform {
namespace {
#ifdef _WIN32
std::wstring utf8ToWide(const std::string& text) {
 if(text.empty()) {
  return {};
 }
 const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
 if(size <= 0) {
  return {};
 }
 std::wstring wide(static_cast<std::size_t>(size), L'\0');
 MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wide.data(), size);
 return wide;
}
bool openExternal(const std::wstring& target) {
 if(target.empty()) {
  return false;
 }
 const HINSTANCE result = ShellExecuteW(nullptr, L"open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
 return reinterpret_cast<std::intptr_t>(result) > 32;
}
#else
bool openExternal(const std::string& target) {
 if(target.empty()) {
  return false;
 }
 const char* opener =
#if defined(__APPLE__)
     "open";
#else
     "xdg-open";
#endif
 char* arguments[] = {const_cast<char*>(opener), const_cast<char*>(target.c_str()), nullptr};
 pid_t process{};
 if(posix_spawnp(&process, opener, nullptr, nullptr, arguments, environ) != 0) {
  return false;
 }
 int status = 0;
 while(waitpid(process, &status, 0) < 0) {
  if(errno != EINTR) {
   return false;
  }
 }
 return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
#endif
}
std::string deviceCodeLoginUrl(const std::string& verificationUri, const std::string& userCode) {
 if(verificationUri.empty()) {
  return {};
 }
 const char separator = verificationUri.find('?') == std::string::npos ? '?' : '&';
 return verificationUri + separator + "otc=" + userCode;
}
bool openUrlInBrowser(const std::string& url) {
 if(url.empty()) {
  return false;
 }
#ifdef _WIN32
 return openExternal(utf8ToWide(url));
#else
 return openExternal(url);
#endif
}
bool openPath(const std::filesystem::path& path) {
#ifdef _WIN32
 return openExternal(path.wstring());
#else
 return openExternal(path.string());
#endif
}
}
