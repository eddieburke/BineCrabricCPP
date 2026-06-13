#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::resource {

// Betacraft proxy for Beta 1.7.3 (Java: -Dhttp.proxyHost=betacraft.ee -Dhttp.proxyPort=11705).
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
};

struct HttpResponse {
    int statusCode = 0;
    std::vector<std::uint8_t> body;

    [[nodiscard]] bool ok() const noexcept
    {
        return statusCode >= 200 && statusCode < 300;
    }

    [[nodiscard]] std::string bodyAsString() const
    {
        return std::string(body.begin(), body.end());
    }
};

[[nodiscard]] HttpResponse httpRequest(const HttpRequest& request);
[[nodiscard]] HttpResponse fetchUrl(const std::string& url, bool useBetacraftProxy = true);

class ResourceDownloadThread {
public:
    ResourceDownloadThread(std::filesystem::path runDirectory, Minecraft* minecraft);
    ~ResourceDownloadThread();

    void start();
    void cancel();
    void reload();

    std::filesystem::path resourcesDirectory;

private:
    void run();
    void loadFromDirectory(const std::filesystem::path& directory, const std::string& type);
    void loadFromUrl(const std::string& path, long long size, int type);

    Minecraft* minecraft_ = nullptr;
    std::atomic_bool cancelled_ {false};
    std::atomic_bool started_ {false};
    std::thread worker_;
};

} // namespace net::minecraft::client::resource
