#include "net/minecraft/client/render/block/BlockVertexEmitter.hpp"
#include <cmath>
namespace net::minecraft::client::render::block {
void quadNormal(const double ax, const double ay, const double az, const double bx, const double by, const double bz,
                const double cx, const double cy, const double cz, float& nx, float& ny, float& nz) noexcept {
  const double e1x = bx - ax;
  const double e1y = by - ay;
  const double e1z = bz - az;
  const double e2x = cx - ax;
  const double e2y = cy - ay;
  const double e2z = cz - az;
  const double rawX = e1y * e2z - e1z * e2y;
  const double rawY = e1z * e2x - e1x * e2z;
  const double rawZ = e1x * e2y - e1y * e2x;
  const double len = std::sqrt(rawX * rawX + rawY * rawY + rawZ * rawZ);
  if(len < 1.0e-8) {
    nx = 0.0f;
    ny = 1.0f;
    nz = 0.0f;
    return;
  }
  const float inv = static_cast<float>(1.0 / len);
  nx = static_cast<float>(rawX) * inv;
  ny = static_cast<float>(rawY) * inv;
  nz = static_cast<float>(rawZ) * inv;
}
void emitBlockVertex(Tessellator& tess, const float nx, const float ny, const float nz, const double x, const double y,
                     const double z, const double u, const double v) {
  tess.normal(nx, ny, nz);
  tess.vertex(x, y, z, u, v);
}
} // namespace net::minecraft::client::render::block
