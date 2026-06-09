#include "net/minecraft/util/math/Vec3dClient.hpp"

namespace net::minecraft::util::math {

namespace {
std::vector<ClientVec3d> g_cache;
int g_cacheCount = 0;
} // namespace

ClientVec3d& ClientVec3d::createCached(double nx, double ny, double nz)
{
    if (g_cacheCount >= static_cast<int>(g_cache.size())) {
        g_cache.emplace_back();
    }
    return g_cache[static_cast<std::size_t>(g_cacheCount++)].set(nx, ny, nz);
}

} // namespace net::minecraft::util::math
