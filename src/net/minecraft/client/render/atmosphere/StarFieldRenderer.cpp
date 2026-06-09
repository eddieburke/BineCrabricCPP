#include "net/minecraft/client/render/atmosphere/StarFieldRenderer.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"

#include <cmath>
#include <random>

namespace net::minecraft::client::render::atmosphere {

namespace {
constexpr float kPi = 3.14159265f;
constexpr int kGlCompile = 0x1300;
} // namespace

void StarFieldRenderer::build()
{
    if (glList_ == 0) {
        glList_ = gl::GL11::glGenLists(1);
    }

    std::mt19937 random(10842U);
    std::uniform_real_distribution<double> unit(-1.0, 1.0);
    std::uniform_real_distribution<double> sizeDist(0.25, 0.5);
    std::uniform_real_distribution<double> spinDist(0.0, static_cast<double>(kPi) * 2.0);

    Tessellator& tessellator = INSTANCE;
    gl::GL11::glNewList(glList_, kGlCompile);
    tessellator.startQuads();
    for (int i = 0; i < 1500; ++i) {
        double dx = unit(random);
        double dy = unit(random);
        double dz = unit(random);
        const double starSize = sizeDist(random);
        const double lenSq = dx * dx + dy * dy + dz * dz;
        if (lenSq < 1.0 && lenSq > 0.01) {
            const double invLen = 1.0 / std::sqrt(lenSq);
            dx *= invLen;
            dy *= invLen;
            dz *= invLen;
            const double cx = dx * 100.0;
            const double cy = dy * 100.0;
            const double cz = dz * 100.0;
            const double yaw = std::atan2(dx, dz);
            const double sinYaw = std::sin(yaw);
            const double cosYaw = std::cos(yaw);
            const double pitch = std::atan2(std::sqrt(dx * dx + dz * dz), dy);
            const double sinPitch = std::sin(pitch);
            const double cosPitch = std::cos(pitch);
            const double spin = spinDist(random);
            const double sinSpin = std::sin(spin);
            const double cosSpin = std::cos(spin);
            for (int corner = 0; corner < 4; ++corner) {
                const double localX = static_cast<double>((corner & 2) - 1) * starSize;
                const double localZ = static_cast<double>(((corner + 1) & 2) - 1) * starSize;
                const double rotatedZ = localX * cosSpin - localZ * sinSpin;
                const double rotatedX = localZ * cosSpin + localX * sinSpin;
                const double pitchedY = rotatedZ * sinPitch + 0.0 * cosPitch;
                const double pitchedX = 0.0 * sinPitch - rotatedZ * cosPitch;
                const double worldX = pitchedX * sinYaw - rotatedX * cosYaw;
                const double worldZ = rotatedX * sinYaw + pitchedX * cosYaw;
                tessellator.vertex(cx + worldX, cy + pitchedY, cz + worldZ);
            }
        }
    }
    tessellator.draw();
    gl::GL11::glEndList();
}

void StarFieldRenderer::draw(float brightness) const
{
    if (glList_ == 0 || brightness <= 0.0f) {
        return;
    }
    gl::GL11::glColor4f(brightness, brightness, brightness, brightness);
    gl::GL11::glCallList(glList_);
}

void StarFieldRenderer::release()
{
    if (glList_ > 0) {
        gl::GL11::glDeleteLists(glList_, 1);
        glList_ = 0;
    }
}

} // namespace net::minecraft::client::render::atmosphere
