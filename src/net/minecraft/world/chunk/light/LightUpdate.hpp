#pragma once

#include "net/minecraft/world/LightType.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"

namespace net::minecraft {

class World;

class LightUpdate {
public:
    LightUpdate(LightType lightType, int minX, int minY, int minZ, int maxX, int maxY, int maxZ)
        : lightType(lightType),
          minX(minX), minY(minY), minZ(minZ),
          maxX(maxX), maxY(maxY), maxZ(maxZ)
    {
    }

    void updateLight(World* world);

    bool expand(int expandMinX, int expandMinY, int expandMinZ, int expandMaxX, int expandMaxY, int expandMaxZ);

    [[nodiscard]] bool overlapsRegion(int queryMinX, int queryMinY, int queryMinZ, int queryMaxX, int queryMaxY,
        int queryMaxZ) const noexcept
    {
        return minX <= queryMaxX && maxX >= queryMinX && minY <= queryMaxY && maxY >= queryMinY && minZ <= queryMaxZ
            && maxZ >= queryMinZ;
    }

    LightType lightType;
    int minX = 0;
    int minY = 0;
    int minZ = 0;
    int maxX = 0;
    int maxY = 0;
    int maxZ = 0;
};

} // namespace net::minecraft
