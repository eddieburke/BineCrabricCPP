#include "net/minecraft/client/LogoCanvas.hpp"

#include <filesystem>

namespace net::minecraft::client {

LogoCanvas::LogoCanvas()
    : logoPath_(std::filesystem::path(MINECRAFT_NATIVE_RESOURCE_DIR) / "gui" / "logo.png"),
      logoAvailable_(std::filesystem::exists(logoPath_))
{
}

void LogoCanvas::paint(void* /*graphics*/) const noexcept
{
    // Native UI code can render the logo later; this keeps the port self-contained.
}

int LogoCanvas::size() const noexcept
{
    return size_;
}

const std::filesystem::path& LogoCanvas::logoPath() const noexcept
{
    return logoPath_;
}

bool LogoCanvas::logoAvailable() const noexcept
{
    return logoAvailable_;
}

} // namespace net::minecraft::client
