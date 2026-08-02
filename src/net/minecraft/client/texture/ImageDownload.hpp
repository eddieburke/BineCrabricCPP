#pragma once
#include <atomic>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include "net/minecraft/client/texture/ImageProcessor.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/util/concurrent/Channel.hpp"
namespace net::minecraft::client::texture {
class ImageDownload {
 public:
 ImageDownload(std::string url, ImageProcessor* textureProcessor, bool useBetacraftProxy = true);
 [[nodiscard]] const std::string& url() const noexcept {
  return url_;
 }
 void applyCompleted();
 std::optional<RasterImage> image;
 bool slimArms = false;
 int requestCount = 1;
 int textureId = -1;
 bool uploaded = false;

 private:
 std::string url_;
 bool useBetacraftProxy_ = true;
 struct Result {
  std::optional<RasterImage> image;
  bool slimArms = false;
 };
 struct PendingResult {
  net::minecraft::util::concurrent::Channel<Result> completed{1};
 };
 std::shared_ptr<PendingResult> pending_;
};
} // namespace net::minecraft::client::texture
