#pragma once

#include "net/minecraft/client/texture/ImageProcessor.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"

#include <atomic>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <thread>

namespace net::minecraft::client::texture {

class ImageDownload {
public:
    ImageDownload(std::string url, ImageProcessor* textureProcessor, bool useBetacraftProxy = true);

    [[nodiscard]] const std::string& url() const noexcept { return url_; }

    std::optional<RasterImage> image;
    int requestCount = 1;
    int textureId = -1;
    bool uploaded = false;

private:
    std::string url_;
    bool useBetacraftProxy_ = true;
};

} // namespace net::minecraft::client::texture
