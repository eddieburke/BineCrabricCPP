#pragma once

#include <filesystem>

namespace net::minecraft::client {

class LogoCanvas {
public:
    LogoCanvas();

    void paint(void* graphics = nullptr) const noexcept;

    int size() const noexcept;
    const std::filesystem::path& logoPath() const noexcept;
    bool logoAvailable() const noexcept;

private:
    int size_ = 100;
    std::filesystem::path logoPath_;
    bool logoAvailable_ = false;
};

} // namespace net::minecraft::client
