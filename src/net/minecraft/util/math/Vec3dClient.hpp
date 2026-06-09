#pragma once

#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft::util::math {

// Client-only Vec3d helpers (createCached pool + rotation), faithful to Vec3d.java.
struct ClientVec3d : Vec3d {
    using Vec3d::Vec3d;

    ClientVec3d& set(double nx, double ny, double nz)
    {
        x = nx;
        y = ny;
        z = nz;
        return *this;
    }

    void rotateX(float angle)
    {
        const float cosAngle = MathHelper::cos(angle);
        const float sinAngle = MathHelper::sin(angle);
        const double prevX = x;
        const double newY = y * static_cast<double>(cosAngle) + z * static_cast<double>(sinAngle);
        const double newZ = z * static_cast<double>(cosAngle) - y * static_cast<double>(sinAngle);
        x = prevX;
        y = newY;
        z = newZ;
    }

    void rotateY(float angle)
    {
        const float cosAngle = MathHelper::cos(angle);
        const float sinAngle = MathHelper::sin(angle);
        const double prevY = y;
        const double newX = x * static_cast<double>(cosAngle) + z * static_cast<double>(sinAngle);
        const double newZ = z * static_cast<double>(cosAngle) - x * static_cast<double>(sinAngle);
        x = newX;
        y = prevY;
        z = newZ;
    }

    static ClientVec3d& createCached(double nx, double ny, double nz);
    static void clearCache();
    static void resetCacheCount();
};

} // namespace net::minecraft::util::math
