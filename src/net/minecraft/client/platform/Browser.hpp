#pragma once
#include <filesystem>
#include <string>
namespace net::minecraft::client::platform {
[[nodiscard]] bool openUrlInBrowser(const std::string& url);
[[nodiscard]] bool openPath(const std::filesystem::path& path);
[[nodiscard]] std::string deviceCodeLoginUrl(const std::string& verificationUri, const std::string& userCode);
}
