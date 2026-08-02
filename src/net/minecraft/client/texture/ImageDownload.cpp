#include "net/minecraft/client/texture/ImageDownload.hpp"
#include <utility>
#include "net/minecraft/client/texture/SkinImageProcessor.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
namespace net::minecraft::client::texture {
ImageDownload::ImageDownload(std::string url, ImageProcessor* textureProcessor, bool useBetacraftProxy)
    : url_(std::move(url)), useBetacraftProxy_(useBetacraftProxy), pending_(std::make_shared<PendingResult>()) {
 const std::string urlSnapshot = url_;
 const auto pending = pending_;
 net::minecraft::util::concurrent::ThreadCoordinator::instance()
     .pool(net::minecraft::util::concurrent::Domain::Io)
     .submit([urlSnapshot, useBetacraftProxy, textureProcessor, pending]() {
  Result result;
  try {
   RasterImage loaded = TextureManager::loadRasterFromUrl(urlSnapshot, useBetacraftProxy);
   if(loaded.width > 0 && loaded.height > 0) {
    result.slimArms = SkinImageProcessor::detectSlimArms(loaded);
    result.image = textureProcessor != nullptr ? textureProcessor->process(loaded) : loaded;
   }
  } catch(const std::exception&) {
  }
  pending->completed.tryPush(std::move(result));
 });
}
void ImageDownload::applyCompleted() {
 Result result;
 if(!pending_->completed.tryPop(result)) {
  return;
 }
 image = std::move(result.image);
 slimArms = result.slimArms;
}
} // namespace net::minecraft::client::texture
