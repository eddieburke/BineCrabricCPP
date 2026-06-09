#include "net/minecraft/client/render/world/ChunkRenderer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"

namespace net::minecraft::client::render::world {

// GL_UNSIGNED_INT — list IDs are stored as int but passed as unsigned.
static constexpr unsigned int kGlUnsignedInt = 0x1405u;

void ChunkRenderer::init(int x, int y, int z, double offsetX, double offsetY, double offsetZ) noexcept
{
    initialized_ = true;
    listBuffer_.clear();
    x_ = x;
    y_ = y;
    z_ = z;
    offsetX_ = static_cast<float>(offsetX);
    offsetY_ = static_cast<float>(offsetY);
    offsetZ_ = static_cast<float>(offsetZ);
}

bool ChunkRenderer::isAt(int x, int y, int z) const noexcept
{
    return initialized_ && x == x_ && y == y_ && z == z_;
}

void ChunkRenderer::addGlList(int glList)
{
    listBuffer_.push_back(glList);
}

void ChunkRenderer::render()
{
    if (!initialized_ || listBuffer_.empty()) {
        return;
    }
    gl::GL11::glPushMatrix();
    gl::GL11::glTranslatef(
        static_cast<float>(x_) - offsetX_,
        static_cast<float>(y_) - offsetY_,
        static_cast<float>(z_) - offsetZ_);
    gl::GL11::glCallLists(
        static_cast<int>(listBuffer_.size()),
        kGlUnsignedInt,
        listBuffer_.data());
    gl::GL11::glPopMatrix();
}

void ChunkRenderer::clear() noexcept
{
    initialized_ = false;
    listBuffer_.clear();
}

} // namespace net::minecraft::client::render::world
