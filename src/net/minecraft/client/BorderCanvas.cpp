#include "net/minecraft/client/BorderCanvas.hpp"

namespace net::minecraft::client {

BorderCanvas::BorderCanvas(int size)
    : size_(size)
{
}

int BorderCanvas::size() const noexcept
{
    return size_;
}

int BorderCanvas::preferredWidth() const noexcept
{
    return size_;
}

int BorderCanvas::preferredHeight() const noexcept
{
    return size_;
}

} // namespace net::minecraft::client
