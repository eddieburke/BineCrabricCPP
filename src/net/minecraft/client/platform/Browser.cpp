#include "net/minecraft/client/platform/Browser.hpp"
#include <cstdint>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <shellapi.h>
#endif
namespace net::minecraft::client::platform {
namespace {
std::wstring utf8ToWide(const std::string& text) {
  if(text.empty()) {
    return {};
  }
#ifdef _WIN32
  const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
  if(size <= 0) {
    return {};
  }
  std::wstring wide(static_cast<std::size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wide.data(), size);
  return wide;
#else
  return std::wstring(text.begin(), text.end());
#endif
}
} // namespace
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
  const std::wstring wideUrl = utf8ToWide(url);
  if(wideUrl.empty()) {
    return false;
  }
  const HINSTANCE result = ShellExecuteW(nullptr, L"open", wideUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  return reinterpret_cast<std::intptr_t>(result) > 32;
#else
  return false;
#endif
}
} // namespace net::minecraft::client::platform
