#pragma once
// Default-browser integration shared by client UI flows.
#include <string>
namespace net::minecraft::client::platform {
[[nodiscard]] bool openUrlInBrowser(const std::string& url);
[[nodiscard]] std::string deviceCodeLoginUrl(const std::string& verificationUri, const std::string& userCode);
} // namespace net::minecraft::client::platform
