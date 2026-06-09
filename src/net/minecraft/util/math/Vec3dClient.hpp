#pragma once

#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <vector>

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
        const float f = MathHelper::cos(angle);
        const float f2 = MathHelper::sin(angle);
        const double d = x;
        const double d2 = y * static_cast<double>(f) + z * static_cast<double>(f2);
        const double d3 = z * static_cast<double>(f) - y * static_cast<double>(f2);
        x = d;
        y = d2;
        z = d3;
    }

    void rotateY(float angle)
    {
        const float f = MathHelper::cos(angle);
        const float f2 = MathHelper::sin(angle);
        const double d = z * static_cast<double>(f) + x * static_cast<double>(f2);
        const double d2 = x * static_cast<double>(f) - z * static_cast<double>(f2);
        z = d;
        x = d2;
    }

    static ClientVec3d& createCached(double nx, double ny, double nz);
};

} // namespace net::minecraft::util::math
