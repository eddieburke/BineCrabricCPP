#include "net/minecraft/client/texture/ImageDownload.hpp"

#include <utility>

namespace net::minecraft::client::texture {

ImageDownload::ImageDownload(std::string url, ImageProcessor* textureProcessor, bool useBetacraftProxy)
    : url_(std::move(url)),
      useBetacraftProxy_(useBetacraftProxy)
{
    std::thread([this, textureProcessor]() {
        try {
            RasterImage loaded = TextureManager::loadRasterFromUrl(url_, useBetacraftProxy_);
            if (loaded.width > 0 && loaded.height > 0) {
                image = textureProcessor != nullptr ? textureProcessor->process(loaded) : loaded;
            }
        } catch (const std::exception&) {
            image.reset();
        }
    }).detach();
}

} // namespace net::minecraft::client::texture
