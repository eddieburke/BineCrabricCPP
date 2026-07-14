#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace net::minecraft::util::http {
inline constexpr const char* kBetacraftProxyHost = "betacraft.ee";
inline constexpr unsigned short kBetacraftProxyPortBeta173 = 11705;
struct HttpHeader {
  std::string name;
  std::string value;
};
struct HttpRequest {
  std::string method = "GET";
  std::string url;
  std::vector<HttpHeader> headers;
  std::string body;
  bool useBetacraftProxy = false;
  int connectTimeoutMs = 15'000;
  int sendTimeoutMs = 15'000;
  int receiveTimeoutMs = 30'000;
  std::size_t maxResponseBytes = 16U * 1024U * 1024U;
  const std::atomic_bool* cancelled = nullptr;
};
struct HttpResponse {
  int statusCode = 0;
  std::vector<std::uint8_t> body;
  [[nodiscard]] bool ok() const noexcept {
    return statusCode >= 200 && statusCode < 300;
  }
  [[nodiscard]] std::string bodyAsString() const {
    return std::string(body.begin(), body.end());
  }
};
[[nodiscard]] HttpResponse httpRequest(const HttpRequest& request);
[[nodiscard]] HttpResponse fetchUrl(const std::string& url, bool useBetacraftProxy = false);
} // namespace net::minecraft::util::http
