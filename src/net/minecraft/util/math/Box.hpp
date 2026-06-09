#pragma once

namespace net::minecraft::util::math {

class Box {
public:
    static Box create(double minX, double minY, double minZ, double maxX, double maxY, double maxZ)
    {
        return Box(minX, minY, minZ, maxX, maxY, maxZ);
    }

    static Box createCached(double minX, double minY, double minZ, double maxX, double maxY, double maxZ)
    {
        return create(minX, minY, minZ, maxX, maxY, maxZ);
    }

    [[nodiscard]] Box expand(double x, double y, double z) const
    {
        return Box(minX - x, minY - y, minZ - z, maxX + x, maxY + y, maxZ + z);
    }

    double minX = 0.0;
    double minY = 0.0;
    double minZ = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    double maxZ = 0.0;

private:
    Box(double minX, double minY, double minZ, double maxX, double maxY, double maxZ)
        : minX(minX),
          minY(minY),
          minZ(minZ),
          maxX(maxX),
          maxY(maxY),
          maxZ(maxZ)
    {
    }
};

} // namespace net::minecraft::util::math
