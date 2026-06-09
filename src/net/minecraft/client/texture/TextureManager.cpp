#include "net/minecraft/client/texture/TextureManager.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/texture/DynamicTexture.hpp"
#include "net/minecraft/client/render/texture/FireSprite.hpp"
#include "net/minecraft/client/render/texture/LavaSideSprite.hpp"
#include "net/minecraft/client/render/texture/LavaSprite.hpp"
#include "net/minecraft/client/render/texture/NetherPortalSprite.hpp"
#include "net/minecraft/client/render/texture/WaterSideSprite.hpp"
#include "net/minecraft/client/render/texture/WaterSprite.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/resource/pack/TexturePacks.hpp"
#include "net/minecraft/client/texture/ImageDownload.hpp"
#include "net/minecraft/client/texture/SkinImageProcessor.hpp"
#include "net/minecraft/client/util/GlAllocationUtils.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include "net/minecraft/client/util/DisplayManager.hpp"
#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"
#include <windows.h>
#include <gdiplus.h>
#endif

namespace net::minecraft::client::texture {

namespace {

#ifdef _WIN32
void ensureGdiplusStarted()
{
    static bool started = false;
    static ULONG_PTR token = 0;
    if (started) {
        return;
    }
    Gdiplus::GdiplusStartupInput startupInput;
    if (Gdiplus::GdiplusStartup(&token, &startupInput, nullptr) == Gdiplus::Ok) {
        started = true;
    }
}
#endif

constexpr int kGlTexture2D = 0x0DE1;
constexpr int kGlRgba = 0x1908;
constexpr int kGlUnsignedByte = 0x1401;
constexpr int kGlNearest = 0x2600;              // GL_NEAREST (9728)
constexpr int kGlLinear = 0x2601;               // GL_LINEAR (9729)
constexpr int kGlNearestMipmapLinear = 0x2702;  // GL_NEAREST_MIPMAP_LINEAR (9986)
constexpr int kGlLinearMipmapLinear = 0x2703;   // GL_LINEAR_MIPMAP_LINEAR (9987)
constexpr int kGlClamp = 0x2900;                // GL_CLAMP (10496)
constexpr int kGlRepeat = 0x2901;               // GL_REPEAT (10497)

int smoothBlend(int color1, int color2)
{
    const int alpha1 = (color1 >> 24) & 0xFF;
    const int alpha2 = (color2 >> 24) & 0xFF;
    return ((alpha1 + alpha2) >> 1 << 24) + (((color1 & 0xFEFEFE) + (color2 & 0xFEFEFE)) >> 1);
}

int getPixelInt(const std::uint8_t* data, int index)
{
    const std::size_t offset = static_cast<std::size_t>(index) * 4U;
    return static_cast<int>(data[offset + 0]) |
        (static_cast<int>(data[offset + 1]) << 8) |
        (static_cast<int>(data[offset + 2]) << 16) |
        (static_cast<int>(data[offset + 3]) << 24);
}

void putPixelInt(std::uint8_t* data, int index, int color)
{
    const std::size_t offset = static_cast<std::size_t>(index) * 4U;
    data[offset + 0] = static_cast<std::uint8_t>(color & 0xFF);
    data[offset + 1] = static_cast<std::uint8_t>((color >> 8) & 0xFF);
    data[offset + 2] = static_cast<std::uint8_t>((color >> 16) & 0xFF);
    data[offset + 3] = static_cast<std::uint8_t>((color >> 24) & 0xFF);
}

void uploadDynamicMipmapLevels(int sprite, std::vector<std::uint8_t>& imageBuffer)
{
    for (int level = 1; level <= 4; ++level) {
        const int sourceSize = 16 >> (level - 1);
        const int targetSize = 16 >> level;
        for (int y = 0; y < targetSize; ++y) {
            for (int x = 0; x < targetSize; ++x) {
                const int topLeft = getPixelInt(imageBuffer.data(), x * 2 + 0 + (y * 2 + 0) * sourceSize);
                const int topRight = getPixelInt(imageBuffer.data(), x * 2 + 1 + (y * 2 + 0) * sourceSize);
                const int bottomRight = getPixelInt(imageBuffer.data(), x * 2 + 1 + (y * 2 + 1) * sourceSize);
                const int bottomLeft = getPixelInt(imageBuffer.data(), x * 2 + 0 + (y * 2 + 1) * sourceSize);
                const int blended = smoothBlend(smoothBlend(topLeft, topRight), smoothBlend(bottomRight, bottomLeft));
                putPixelInt(imageBuffer.data(), x + y * targetSize, blended);
            }
        }
        gl::GL11::glTexSubImage2D(
            gl::GL11::GL_TEXTURE_2D,
            level,
            (sprite % 16) * targetSize,
            (sprite / 16) * targetSize,
            targetSize,
            targetSize,
            kGlRgba,
            kGlUnsignedByte,
            imageBuffer.data());
    }
}

RasterImage makeMissingImage()
{
    RasterImage img;
    img.width = 64;
    img.height = 64;
    img.argb.assign(static_cast<std::size_t>(img.width * img.height), 0xFFFFFFFFU);
    return img;
}

} // namespace

bool TextureManager::MIPMAP = false;
bool TextureManager::MIPMAP_LINEAR = false;

TextureManager::TextureManager(option::GameOptions* options)
    : gameOptions_(options)
{
    // OpenGL calls require an active context (created in Minecraft::init).
}

void TextureManager::setTexturePacks(resource::pack::TexturePacks* texturePacks)
{
    texturePacks_ = texturePacks;
}

RasterImage TextureManager::loadRasterForResource(const std::string& resourcePath)
{
    std::string normalized = resourcePath;
    while (!normalized.empty() && (normalized.front() == '/' || normalized.front() == '\\')) {
        normalized.erase(normalized.begin());
    }
    if (texturePacks_ != nullptr && texturePacks_->selected != nullptr) {
        const std::vector<std::uint8_t> bytes = texturePacks_->selected->getResource(normalized);
        if (!bytes.empty()) {
            return loadRasterFromBytes(bytes);
        }
    }
    return {};
}

TextureManager::~TextureManager() = default;

void TextureManager::ensureMissingTexture()
{
    if (missingTextureReady_) {
        return;
    }
#ifdef _WIN32
    util::DisplayManager::ensureGlContext();
#endif
    unsigned int id = 0;
    util::GlAllocationUtils::generateTextureName(id);
    const RasterImage missing = makeMissingImage();
    load(missing, static_cast<int>(id));
    images_[static_cast<int>(id)] = missing;
    missingTextureId_ = static_cast<int>(id);
    missingTextureReady_ = true;
}

std::filesystem::path TextureManager::resolveResourcePath(const std::string& path)
{
    std::string normalized = path;
    while (!normalized.empty() && (normalized.front() == '/' || normalized.front() == '\\')) {
        normalized.erase(normalized.begin());
    }
    return std::filesystem::path(MINECRAFT_NATIVE_RESOURCE_DIR) / normalized;
}

#ifdef _WIN32
RasterImage TextureManager::loadRasterFromFile(const std::filesystem::path& filePath)
{
    RasterImage out;
    ensureGdiplusStarted();

    Gdiplus::Bitmap bitmap(filePath.wstring().c_str());
    if (bitmap.GetLastStatus() != Gdiplus::Ok) {
        return out;
    }

    out.width = static_cast<int>(bitmap.GetWidth());
    out.height = static_cast<int>(bitmap.GetHeight());
    out.argb.resize(static_cast<std::size_t>(out.width) * static_cast<std::size_t>(out.height));

    Gdiplus::Rect rect(0, 0, out.width, out.height);
    Gdiplus::BitmapData data{};
    if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) == Gdiplus::Ok) {
        const auto* src = static_cast<const std::uint8_t*>(data.Scan0);
        for (int y = 0; y < out.height; ++y) {
            const std::uint8_t* row = src + static_cast<std::size_t>(y) * data.Stride;
            for (int x = 0; x < out.width; ++x) {
                const std::uint8_t b = row[x * 4 + 0];
                const std::uint8_t g = row[x * 4 + 1];
                const std::uint8_t r = row[x * 4 + 2];
                const std::uint8_t a = row[x * 4 + 3];
                out.argb[static_cast<std::size_t>(y) * out.width + x] =
                    (static_cast<std::uint32_t>(a) << 24U) |
                    (static_cast<std::uint32_t>(r) << 16U) |
                    (static_cast<std::uint32_t>(g) << 8U) |
                    static_cast<std::uint32_t>(b);
            }
        }
        bitmap.UnlockBits(&data);
    }

    return out;
}
#else
RasterImage TextureManager::loadRasterFromFile(const std::filesystem::path&)
{
    return {};
}
#endif

#ifdef _WIN32
RasterImage TextureManager::loadRasterFromBytes(const std::vector<std::uint8_t>& bytes)
{
    RasterImage out;
    if (bytes.empty()) {
        return out;
    }
    ensureGdiplusStarted();
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes.size());
    if (memory == nullptr) {
        return out;
    }
    void* locked = GlobalLock(memory);
    if (locked == nullptr) {
        GlobalFree(memory);
        return out;
    }
    std::copy(bytes.begin(), bytes.end(), static_cast<std::uint8_t*>(locked));
    GlobalUnlock(memory);

    IStream* stream = nullptr;
    if (CreateStreamOnHGlobal(memory, TRUE, &stream) != S_OK) {
        GlobalFree(memory);
        return out;
    }

    Gdiplus::Bitmap bitmap(stream);
    stream->Release();
    if (bitmap.GetLastStatus() != Gdiplus::Ok) {
        return out;
    }

    out.width = static_cast<int>(bitmap.GetWidth());
    out.height = static_cast<int>(bitmap.GetHeight());
    out.argb.resize(static_cast<std::size_t>(out.width) * static_cast<std::size_t>(out.height));
    Gdiplus::Rect rect(0, 0, out.width, out.height);
    Gdiplus::BitmapData data {};
    if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) == Gdiplus::Ok) {
        const auto* src = static_cast<const std::uint8_t*>(data.Scan0);
        for (int y = 0; y < out.height; ++y) {
            const std::uint8_t* row = src + static_cast<std::size_t>(y) * data.Stride;
            for (int x = 0; x < out.width; ++x) {
                const std::uint8_t b = row[x * 4 + 0];
                const std::uint8_t g = row[x * 4 + 1];
                const std::uint8_t r = row[x * 4 + 2];
                const std::uint8_t a = row[x * 4 + 3];
                out.argb[static_cast<std::size_t>(y) * out.width + x] =
                    (static_cast<std::uint32_t>(a) << 24U) |
                    (static_cast<std::uint32_t>(r) << 16U) |
                    (static_cast<std::uint32_t>(g) << 8U) |
                    static_cast<std::uint32_t>(b);
            }
        }
        bitmap.UnlockBits(&data);
    }
    return out;
}
#else
RasterImage TextureManager::loadRasterFromBytes(const std::vector<std::uint8_t>&)
{
    return {};
}
#endif

RasterImage TextureManager::loadRasterFromUrl(const std::string& url, bool useBetacraftProxy)
{
#ifdef _WIN32
    constexpr std::string_view filePrefix = "file://";
    if (url.starts_with(filePrefix)) {
        return loadRasterFromFile(std::filesystem::path(url.substr(filePrefix.size())));
    }
    if (std::filesystem::path direct(url); std::filesystem::exists(direct)) {
        return loadRasterFromFile(direct);
    }
    if (!url.starts_with("http://") && !url.starts_with("https://")) {
        return loadRasterFromFile(resolveResourcePath(url));
    }

    const resource::HttpResponse response = resource::fetchUrl(url, useBetacraftProxy);
    if (!response.ok()) {
        return {};
    }
    return loadRasterFromBytes(response.body);
#else
    constexpr std::string_view filePrefix = "file://";
    if (url.starts_with(filePrefix)) {
        return loadRasterFromFile(std::filesystem::path(url.substr(filePrefix.size())));
    }
    if (std::filesystem::path direct(url); std::filesystem::exists(direct)) {
        return loadRasterFromFile(direct);
    }
    return loadRasterFromFile(resolveResourcePath(url));
#endif
}

void TextureManager::deleteTexture(int textureId)
{
    images_.erase(textureId);
    const unsigned int glId = static_cast<unsigned int>(textureId);
    gl::GL11::glDeleteTextures(1, &glId);
}

int TextureManager::downloadTexture(const std::string& url, const std::string& backup)
{
    ImageDownload* imageDownload = nullptr;
    const auto it = downloadedImages_.find(url);
    if (it != downloadedImages_.end()) {
        imageDownload = it->second.get();
    }
    if (imageDownload != nullptr && imageDownload->image.has_value() && !imageDownload->uploaded) {
        if (imageDownload->textureId < 0) {
            imageDownload->textureId = load(*imageDownload->image);
        } else {
            load(*imageDownload->image, imageDownload->textureId);
        }
        imageDownload->uploaded = true;
    }
    if (imageDownload == nullptr || imageDownload->textureId < 0) {
        if (backup.empty()) {
            return -1;
        }
        return getTextureId(backup);
    }
    return imageDownload->textureId;
}

ImageDownload* TextureManager::downloadImage(const std::string& url, ImageProcessor* processor, bool useBetacraftProxy)
{
    if (url.empty()) {
        return nullptr;
    }
    const auto it = downloadedImages_.find(url);
    if (it == downloadedImages_.end()) {
        auto download = std::make_unique<ImageDownload>(url, processor, useBetacraftProxy);
        ImageDownload* ptr = download.get();
        downloadedImages_.emplace(url, std::move(download));
        return ptr;
    }
    ++it->second->requestCount;
    return it->second.get();
}

void TextureManager::downloadSkinImage(const std::string& url)
{
    static SkinImageProcessor skinProcessor;
    downloadImage(url, &skinProcessor, true);
}

void TextureManager::downloadCapeImage(const std::string& url)
{
    static SkinImageProcessor skinProcessor;
    downloadImage(url, &skinProcessor, false);
}

void TextureManager::releaseImage(const std::string& url)
{
    const auto it = downloadedImages_.find(url);
    if (it == downloadedImages_.end()) {
        return;
    }
    ImageDownload* imageDownload = it->second.get();
    --imageDownload->requestCount;
    if (imageDownload->requestCount <= 0) {
        if (imageDownload->textureId >= 0) {
            deleteTexture(imageDownload->textureId);
        }
        downloadedImages_.erase(it);
    }
}

const std::vector<int>& TextureManager::getColors(const std::string& path)
{
    const auto cached = colors_.find(path);
    if (cached != colors_.end()) {
        return cached->second;
    }

    std::string resourcePath = path;
    if (resourcePath.rfind("##", 0) == 0) {
        resourcePath = resourcePath.substr(2);
    } else if (resourcePath.rfind("%clamp%", 0) == 0) {
        resourcePath = resourcePath.substr(7);
    } else if (resourcePath.rfind("%blur%", 0) == 0) {
        resourcePath = resourcePath.substr(6);
    }

    std::vector<int> colors;
    const RasterImage image = loadRasterForResource(resourcePath);
    if (image.width > 0 && image.height > 0) {
        colors.resize(image.argb.size());
        for (std::size_t i = 0; i < image.argb.size(); ++i) {
            colors[i] = static_cast<int>(image.argb[i]);
        }
    } else {
        ensureMissingTexture();
        const RasterImage& missing = images_.at(missingTextureId_);
        colors.resize(missing.argb.size());
        for (std::size_t i = 0; i < missing.argb.size(); ++i) {
            colors[i] = static_cast<int>(missing.argb[i]);
        }
    }

    const auto [inserted, _] = colors_.emplace(path, std::move(colors));
    return inserted->second;
}

int TextureManager::getTextureId(const std::string& path)
{
    ensureMissingTexture();
    const auto it = textures_.find(path);
    if (it != textures_.end()) {
        return it->second;
    }

    std::string resourcePath = path;
    bool requestedBlur = false;
    bool requestedClamp = false;
    if (resourcePath.rfind("##", 0) == 0) {
        resourcePath = resourcePath.substr(2);
    } else if (resourcePath.rfind("%clamp%", 0) == 0) {
        requestedClamp = true;
        resourcePath = resourcePath.substr(7);
    } else if (resourcePath.rfind("%blur%", 0) == 0) {
        requestedBlur = true;
        resourcePath = resourcePath.substr(6);
    }

    RasterImage image = loadRasterForResource(resourcePath);
    if (image.width <= 0 || image.height <= 0) {
        textures_[path] = missingTextureId_;
        return missingTextureId_;
    }

    const bool previousBlur = blur;
    const bool previousClamp = clamp;
    blur = requestedBlur;
    clamp = requestedClamp;
    const int id = load(image);
    blur = previousBlur;
    clamp = previousClamp;
    textures_[path] = id;
    return id;
}

void TextureManager::reload()
{
    if (texturePacks_ == nullptr) {
        return;
    }
    for (const auto& [textureId, image] : images_) {
        load(image, textureId);
    }
    for (auto& entry : downloadedImages_) {
        entry.second->uploaded = false;
    }
    for (const auto& [path, textureId] : textures_) {
        std::string resourcePath = path;
        bool requestedBlur = false;
        bool requestedClamp = false;
        if (resourcePath.rfind("##", 0) == 0) {
            resourcePath = resourcePath.substr(2);
        } else if (resourcePath.rfind("%clamp%", 0) == 0) {
            requestedClamp = true;
            resourcePath = resourcePath.substr(7);
        } else if (resourcePath.rfind("%blur%", 0) == 0) {
            requestedBlur = true;
            resourcePath = resourcePath.substr(6);
        }
        const RasterImage image = loadRasterForResource(resourcePath);
        if (image.width <= 0 || image.height <= 0) {
            continue;
        }
        const bool previousBlur = blur;
        const bool previousClamp = clamp;
        blur = requestedBlur;
        clamp = requestedClamp;
        load(image, textureId);
        blur = previousBlur;
        clamp = previousClamp;
    }
    for (auto& [path, colors] : colors_) {
        std::string resourcePath = path;
        if (resourcePath.rfind("##", 0) == 0) {
            resourcePath = resourcePath.substr(2);
        } else if (resourcePath.rfind("%clamp%", 0) == 0) {
            resourcePath = resourcePath.substr(7);
        } else if (resourcePath.rfind("%blur%", 0) == 0) {
            resourcePath = resourcePath.substr(6);
        }
        const RasterImage image = loadRasterForResource(resourcePath);
        if (image.width <= 0 || image.height <= 0) {
            continue;
        }
        colors.resize(image.argb.size());
        for (std::size_t i = 0; i < image.argb.size(); ++i) {
            colors[i] = static_cast<int>(image.argb[i]);
        }
    }
}

void TextureManager::bindTexture(int id)
{
    if (id < 0) {
        return;
    }
    gl::GL11::glBindTexture(kGlTexture2D, id);
}

int TextureManager::load(const RasterImage& image)
{
#ifdef _WIN32
    util::DisplayManager::ensureGlContext();
#endif
    unsigned int id = 0;
    util::GlAllocationUtils::generateTextureName(id);
    load(image, static_cast<int>(id));
    images_[static_cast<int>(id)] = image;
    return static_cast<int>(id);
}

void TextureManager::load(const RasterImage& image, int id)
{
    if (image.width <= 0 || image.height <= 0) {
        return;
    }

    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(image.width) * image.height * 4);
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const std::uint32_t pixel = image.argb[static_cast<std::size_t>(y) * image.width + x];
            const std::uint8_t a = static_cast<std::uint8_t>((pixel >> 24U) & 0xFFU);
            std::uint8_t r = static_cast<std::uint8_t>((pixel >> 16U) & 0xFFU);
            std::uint8_t g = static_cast<std::uint8_t>((pixel >> 8U) & 0xFFU);
            std::uint8_t b = static_cast<std::uint8_t>(pixel & 0xFFU);

            const std::size_t o = (static_cast<std::size_t>(y) * image.width + x) * 4;
            rgba[o + 0] = r;
            rgba[o + 1] = g;
            rgba[o + 2] = b;
            rgba[o + 3] = a;
        }
    }

    gl::GL11::glBindTexture(kGlTexture2D, static_cast<unsigned int>(id));
    // Faithful to TextureManager.load: default GL_NEAREST (pixelated), mipmap or
    // blur override it; wrap is REPEAT unless clamp.
    if (MIPMAP) {
        const int minFilter = MIPMAP_LINEAR ? kGlLinearMipmapLinear : kGlNearestMipmapLinear;
        gl::GL11::glTexParameteri(kGlTexture2D, gl::GL11::GL_TEXTURE_MIN_FILTER, minFilter);
        gl::GL11::glTexParameteri(kGlTexture2D, gl::GL11::GL_TEXTURE_MAG_FILTER, kGlNearest);
    } else {
        gl::GL11::glTexParameteri(kGlTexture2D, gl::GL11::GL_TEXTURE_MIN_FILTER, kGlNearest);
        gl::GL11::glTexParameteri(kGlTexture2D, gl::GL11::GL_TEXTURE_MAG_FILTER, kGlNearest);
    }
    if (blur) {
        gl::GL11::glTexParameteri(kGlTexture2D, gl::GL11::GL_TEXTURE_MIN_FILTER, kGlLinear);
        gl::GL11::glTexParameteri(kGlTexture2D, gl::GL11::GL_TEXTURE_MAG_FILTER, kGlLinear);
    }
    if (clamp) {
        gl::GL11::glTexParameteri(kGlTexture2D, gl::GL11::GL_TEXTURE_WRAP_S, kGlClamp);
        gl::GL11::glTexParameteri(kGlTexture2D, gl::GL11::GL_TEXTURE_WRAP_T, kGlClamp);
    } else {
        gl::GL11::glTexParameteri(kGlTexture2D, gl::GL11::GL_TEXTURE_WRAP_S, kGlRepeat);
        gl::GL11::glTexParameteri(kGlTexture2D, gl::GL11::GL_TEXTURE_WRAP_T, kGlRepeat);
    }
    gl::GL11::glTexImage2D(kGlTexture2D, 0, kGlRgba, image.width, image.height, 0, kGlRgba, kGlUnsignedByte, rgba.data());
}

namespace {

bool shouldTickDynamicTexture(const option::GameOptions* options,
    const net::minecraft::client::render::texture::DynamicTexture* texture)
{
    if (texture == nullptr) {
        return false;
    }
    if (options == nullptr) {
        return true;
    }
    if (dynamic_cast<const net::minecraft::client::render::texture::WaterSprite*>(texture) != nullptr
        || dynamic_cast<const net::minecraft::client::render::texture::WaterSideSprite*>(texture) != nullptr) {
        return options->ofAnimatedWater == 0;
    }
    if (dynamic_cast<const net::minecraft::client::render::texture::LavaSprite*>(texture) != nullptr
        || dynamic_cast<const net::minecraft::client::render::texture::LavaSideSprite*>(texture) != nullptr) {
        return options->ofAnimatedLava == 0;
    }
    if (dynamic_cast<const net::minecraft::client::render::texture::FireSprite*>(texture) != nullptr) {
        return options->ofAnimatedFire;
    }
    if (dynamic_cast<const net::minecraft::client::render::texture::NetherPortalSprite*>(texture) != nullptr) {
        return options->ofAnimatedPortal;
    }
    return true;
}

} // namespace

void TextureManager::addDynamicTexture(net::minecraft::client::render::texture::DynamicTexture* texture)
{
    if (texture == nullptr) {
        return;
    }
    dynamicTextures_.push_back(texture);
    texture->tick();
}

void TextureManager::tick()
{
    std::vector<std::uint8_t> imageBuffer(1024);

    for (auto* texture : dynamicTextures_) {
        if (texture == nullptr) {
            continue;
        }
        if (shouldTickDynamicTexture(gameOptions_, texture)) {
            texture->tick();
        }
        std::copy(texture->pixels.begin(), texture->pixels.end(), imageBuffer.begin());
        texture->bind(*this);

        for (int replicateX = 0; replicateX < texture->replicate; ++replicateX) {
            for (int replicateY = 0; replicateY < texture->replicate; ++replicateY) {
                gl::GL11::glTexSubImage2D(
                    gl::GL11::GL_TEXTURE_2D,
                    0,
                    (texture->sprite % 16) * 16 + replicateX * 16,
                    (texture->sprite / 16) * 16 + replicateY * 16,
                    16,
                    16,
                    kGlRgba,
                    kGlUnsignedByte,
                    imageBuffer.data());

                if (MIPMAP) {
                    uploadDynamicMipmapLevels(texture->sprite, imageBuffer);
                }
            }
        }
    }

    for (auto* texture : dynamicTextures_) {
        if (texture == nullptr || texture->copyTo <= 0) {
            continue;
        }
        std::copy(texture->pixels.begin(), texture->pixels.end(), imageBuffer.begin());
        gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, static_cast<unsigned int>(texture->copyTo));
        gl::GL11::glTexSubImage2D(
            gl::GL11::GL_TEXTURE_2D,
            0,
            0,
            0,
            16,
            16,
            kGlRgba,
            kGlUnsignedByte,
            imageBuffer.data());
        if (MIPMAP) {
            uploadDynamicMipmapLevels(0, imageBuffer);
        }
    }
}

} // namespace net::minecraft::client::texture
