#pragma once
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include "net/minecraft/util/http/HttpClient.hpp"
#include "net/minecraft/util/concurrent/Channel.hpp"
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
 void tick();
 std::filesystem::path resourcesDirectory;

 private:
 struct PendingResource {
  std::string path;
  std::filesystem::path file;
 };
 struct State {
  explicit State(std::filesystem::path root) : resourcesDirectory(std::move(root)) {
  }
  std::filesystem::path resourcesDirectory;
  std::atomic_bool cancelled{false};
  std::atomic_bool started{false};
  net::minecraft::util::concurrent::Channel<PendingResource> completed{16};
 };
 static void run(const std::shared_ptr<State>& state);
 static void loadFromDirectory(const std::shared_ptr<State>& state,
                               const std::filesystem::path& directory,
                               const std::string& type);
 static void loadFromUrl(const std::shared_ptr<State>& state, const std::string& path, long long size, int type);
 Minecraft* minecraft_ = nullptr;
 std::shared_ptr<State> state_;
};
} // namespace net::minecraft::client::resource
