#pragma once

#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft {

class World;

class Feature {
public:
    virtual ~Feature() = default;

    virtual bool generate(World* world, JavaRandom& random, int x, int y, int z) = 0;

    virtual void prepare(double /*d0*/, double /*d1*/, double /*d2*/)
    {
    }
};

} // namespace net::minecraft
