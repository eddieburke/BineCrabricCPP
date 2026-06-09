#include "net/minecraft/util/math/Vec3dClient.hpp"

#include <deque>

namespace net::minecraft::util::math {

namespace {
// deque keeps element addresses stable on growth (Java ArrayList holds stable object refs).
std::deque<ClientVec3d> g_cache;
int g_cacheCount = 0;
} // namespace

ClientVec3d& ClientVec3d::createCached(double nx, double ny, double nz)
{
    if (g_cacheCount >= static_cast<int>(g_cache.size())) {
        g_cache.emplace_back();
    }
    return g_cache[static_cast<std::size_t>(g_cacheCount++)].set(nx, ny, nz);
}

void ClientVec3d::clearCache()
{
    g_cache.clear();
    g_cacheCount = 0;
}

void ClientVec3d::resetCacheCount()
{
    g_cacheCount = 0;
}

} // namespace net::minecraft::util::math
