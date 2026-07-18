#pragma once
#include <atomic>
#include <filesystem>
#include <string>
#include <thread>
#include "net/minecraft/util/http/HttpClient.hpp"
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::resource {
using net::minecraft::util::http::fetchUrl;
using net::minecraft::util::http::HttpHeader;
using net::minecraft::util::http::HttpRequest;
using net::minecraft::util::http::httpRequest;
using net::minecraft::util::http::HttpResponse;
using net::minecraft::util::http::kBetacraftProxyHost;
using net::minecraft::util::http::kBetacraftProxyPortBeta173;
class ResourceDownloadThread {
public:
  ResourceDownloadThread(std::filesystem::path resourcesDirectory, Minecraft* minecraft);
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
  std::atomic_bool cancelled_{false};
  std::atomic_bool started_{false};
  std::thread worker_;
};
} // namespace net::minecraft::client::resource
