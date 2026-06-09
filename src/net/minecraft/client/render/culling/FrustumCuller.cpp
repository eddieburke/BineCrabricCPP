#include "net/minecraft/client/render/culling/FrustumCuller.hpp"

#include "net/minecraft/client/render/culling/Frustum.hpp"

namespace net::minecraft::client::render {

void FrustumCuller::prepare(double x, double y, double z)
{
    offsetX_ = x;
    offsetY_ = y;
    offsetZ_ = z;
    frustum_ = &Frustum::getInstance();
}

bool FrustumCuller::isVisible(const net::minecraft::Box& box) const
{
    if (frustum_ == nullptr) {
        return true;
    }
    return frustum_->intersects(box.minX - offsetX_, box.minY - offsetY_, box.minZ - offsetZ_, box.maxX - offsetX_,
        box.maxY - offsetY_, box.maxZ - offsetZ_);
}

} // namespace net::minecraft::client::render
