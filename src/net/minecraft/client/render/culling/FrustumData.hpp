#pragma once

#include <array>

namespace net::minecraft::client::render {

class FrustumData {
public:
    [[nodiscard]] bool intersects(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) const
    {
        for (int i = 0; i < 6; ++i) {
            const double a = frustum[static_cast<std::size_t>(i)][0];
            const double b = frustum[static_cast<std::size_t>(i)][1];
            const double c = frustum[static_cast<std::size_t>(i)][2];
            const double d = frustum[static_cast<std::size_t>(i)][3];
            if (a * minX + b * minY + c * minZ + d > 0.0
                || a * maxX + b * minY + c * minZ + d > 0.0
                || a * minX + b * maxY + c * minZ + d > 0.0
                || a * maxX + b * maxY + c * minZ + d > 0.0
                || a * minX + b * minY + c * maxZ + d > 0.0
                || a * maxX + b * minY + c * maxZ + d > 0.0
                || a * minX + b * maxY + c * maxZ + d > 0.0
                || a * maxX + b * maxY + c * maxZ + d > 0.0) {
                continue;
            }
            return false;
        }
        return true;
    }

    std::array<std::array<float, 4>, 6> frustum {};
    std::array<float, 16> projectionMatrix {};
    std::array<float, 16> modelMatrix {};
    std::array<float, 16> clipMatrix {};
};

} // namespace net::minecraft::client::render
