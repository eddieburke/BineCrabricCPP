#pragma once
#include "net/minecraft/client/render/Tessellator.hpp"
namespace net::minecraft::client::render::block {
void quadNormal(double ax, double ay, double az, double bx, double by, double bz, double cx, double cy, double cz,
                float& nx, float& ny, float& nz) noexcept;
void emitBlockVertex(Tessellator& tess, float nx, float ny, float nz, double x, double y, double z, double u, double v);
} // namespace net::minecraft::client::render::block
