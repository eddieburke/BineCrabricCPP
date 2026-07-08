#include "net/minecraft/client/Screenshot.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <GL/gl.h>
#include <Windows.h>
#include <gdiplus.h>

namespace net::minecraft::client {
namespace {
int getEncoderClsid(const WCHAR* format, CLSID* clsid) {
    UINT numEncoders = 0;
    UINT size = 0;
    if (Gdiplus::GetImageEncodersSize(&numEncoders, &size) != Gdiplus::Ok || size == 0) {
        return -1;
    }
    std::vector<std::uint8_t> buffer(size);
    auto* encoders = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
    if (Gdiplus::GetImageEncoders(numEncoders, size, encoders) != Gdiplus::Ok) {
        return -1;
    }
    for (UINT i = 0; i < numEncoders; ++i) {
        if (std::wcscmp(encoders[i].MimeType, format) == 0) {
            *clsid = encoders[i].Clsid;
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::string timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    const std::tm localTime = *std::localtime(&nowTime);
    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d_%H.%M.%S");
    return stream.str();
}
}  // namespace

std::string Screenshot::take(const std::filesystem::path& gameDir, int width, int height) {
    try {
        const std::filesystem::path screenshotDir = gameDir / "screenshots";
        std::filesystem::create_directories(screenshotDir);
        static std::vector<std::uint8_t> pixelBytes;
        static std::vector<std::uint32_t> argbPixels;
        const std::size_t byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u;
        const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        pixelBytes.resize(byteCount);
        argbPixels.resize(pixelCount);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixelBytes.data());
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                const int sourceY = height - y - 1;
                const std::size_t sourceIndex = static_cast<std::size_t>(x + sourceY * width) * 3u;
                const std::uint32_t r = pixelBytes[sourceIndex + 0];
                const std::uint32_t g = pixelBytes[sourceIndex + 1];
                const std::uint32_t b = pixelBytes[sourceIndex + 2];
                argbPixels[static_cast<std::size_t>(x + y * width)] = 0xFF000000u | (r << 16) | (g << 8) | b;
            }
        }
        const std::string baseName = timestamp();
        std::filesystem::path outFile;
        int suffix = 1;
        do {
            outFile =
                screenshotDir / (baseName + (suffix == 1 ? std::string{} : "_" + std::to_string(suffix)) + ".png");
            ++suffix;
        } while (std::filesystem::exists(outFile));
        Gdiplus::GdiplusStartupInput startupInput;
        ULONG_PTR token = 0;
        if (Gdiplus::GdiplusStartup(&token, &startupInput, nullptr) != Gdiplus::Ok) {
            throw std::runtime_error("Failed to start GDI+");
        }
        const Gdiplus::Bitmap bitmap(
            width, height, width * 4, PixelFormat32bppARGB, reinterpret_cast<BYTE*>(argbPixels.data()));
        CLSID pngClsid{};
        if (getEncoderClsid(L"image/png", &pngClsid) < 0) {
            Gdiplus::GdiplusShutdown(token);
            throw std::runtime_error("PNG encoder not available");
        }
        const Gdiplus::Status status =
            const_cast<Gdiplus::Bitmap&>(bitmap).Save(outFile.wstring().c_str(), &pngClsid, nullptr);
        Gdiplus::GdiplusShutdown(token);
        if (status != Gdiplus::Ok) {
            throw std::runtime_error("GDI+ failed to save screenshot");
        }
        return "Saved screenshot as " + outFile.filename().string();
    } catch (const std::exception& ex) {
        return std::string("Failed to save: ") + ex.what();
    } catch (...) {
        return "Failed to save: unknown error";
    }
}
}  // namespace net::minecraft::client
