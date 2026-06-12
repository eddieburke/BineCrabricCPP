#include "net/minecraft/world/light/LightUpdate.hpp"

namespace net::minecraft {

bool LightUpdate::expand(int expandMinX, int expandMinY, int expandMinZ, int expandMaxX, int expandMaxY, int expandMaxZ)
{
    if (expandMinX >= minX && expandMinY >= minY && expandMinZ >= minZ && expandMaxX <= maxX && expandMaxY <= maxY
        && expandMaxZ <= maxZ) {
        return true;
    }

    constexpr int margin = 1;
    if (expandMinX >= minX - margin && expandMinY >= minY - margin && expandMinZ >= minZ - margin
        && expandMaxX <= maxX + margin && expandMaxY <= maxY + margin && expandMaxZ <= maxZ + margin) {
        int mergedMinX = expandMinX;
        int mergedMinY = expandMinY;
        int mergedMinZ = expandMinZ;
        int mergedMaxX = expandMaxX;
        int mergedMaxY = expandMaxY;
        int mergedMaxZ = expandMaxZ;
        if (mergedMinX > minX) {
            mergedMinX = minX;
        }
        if (mergedMinY > minY) {
            mergedMinY = minY;
        }
        if (mergedMinZ > minZ) {
            mergedMinZ = minZ;
        }
        if (mergedMaxX < maxX) {
            mergedMaxX = maxX;
        }
        if (mergedMaxY < maxY) {
            mergedMaxY = maxY;
        }
        if (mergedMaxZ < maxZ) {
            mergedMaxZ = maxZ;
        }
        const int oldVolume = (maxX - minX) * (maxY - minY) * (maxZ - minZ);
        const int newVolume = (mergedMaxX - mergedMinX) * (mergedMaxY - mergedMinY) * (mergedMaxZ - mergedMinZ);
        if (newVolume - oldVolume <= 2) {
            minX = mergedMinX;
            minY = mergedMinY;
            minZ = mergedMinZ;
            maxX = mergedMaxX;
            maxY = mergedMaxY;
            maxZ = mergedMaxZ;
            return true;
        }
    }
    return false;
}

} // namespace net::minecraft
