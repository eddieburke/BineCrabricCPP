#pragma once

#include "net/minecraft/client/texture/ImageProcessor.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace net::minecraft::client::texture {

class SkinImageProcessor : public ImageProcessor {
public:
    [[nodiscard]] RasterImage process(const RasterImage& image) override
    {
        RasterImage result;
        result.width = 64;
        result.height = 32;
        result.argb.assign(static_cast<std::size_t>(result.width * result.height), 0);

        const int copyWidth = std::min(image.width, result.width);
        const int copyHeight = std::min(image.height, result.height);
        for (int y = 0; y < copyHeight; ++y) {
            for (int x = 0; x < copyWidth; ++x) {
                result.argb[static_cast<std::size_t>(x + y * result.width)] =
                    image.argb[static_cast<std::size_t>(x + y * image.width)];
            }
        }

        data_ = result.argb.data();
        width_ = result.width;
        height_ = result.height;
        setOpaque(0, 0, 32, 16);
        setTransparent(32, 0, 64, 32);
        setOpaque(0, 16, 64, 32);
        return result;
    }

private:
    void setTransparent(int x1, int y1, int x2, int y2)
    {
        if (hasTransparency(x1, y1, x2, y2)) {
            return;
        }
        for (int x = x1; x < x2; ++x) {
            for (int y = y1; y < y2; ++y) {
                data_[static_cast<std::size_t>(x + y * width_)] &= 0x00FFFFFFU;
            }
        }
    }

    void setOpaque(int x1, int y1, int x2, int y2)
    {
        for (int x = x1; x < x2; ++x) {
            for (int y = y1; y < y2; ++y) {
                data_[static_cast<std::size_t>(x + y * width_)] |= 0xFF000000U;
            }
        }
    }

    [[nodiscard]] bool hasTransparency(int x1, int y1, int x2, int y2) const
    {
        for (int x = x1; x < x2; ++x) {
            for (int y = y1; y < y2; ++y) {
                const std::uint32_t pixel = data_[static_cast<std::size_t>(x + y * width_)];
                if (((pixel >> 24U) & 0xFFU) < 128U) {
                    return true;
                }
            }
        }
        return false;
    }

    std::uint32_t* data_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

} // namespace net::minecraft::client::texture
