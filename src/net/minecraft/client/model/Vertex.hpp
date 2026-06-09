#pragma once

#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft::client::model {

class Vertex {
public:
    Vec3d pos {};
    float u = 0.0f;
    float v = 0.0f;

    Vertex() = default;

    Vertex(float x, float y, float z, float uIn, float vIn)
        : Vertex(Vec3d{x, y, z}, uIn, vIn)
    {
    }

    Vertex(const Vertex& vertex, float uIn, float vIn)
        : pos(vertex.pos),
          u(uIn),
          v(vIn)
    {
    }

    Vertex(Vec3d posIn, float uIn, float vIn)
        : pos(posIn),
          u(uIn),
          v(vIn)
    {
    }

    [[nodiscard]] Vertex remap(float uIn, float vIn) const
    {
        return Vertex(*this, uIn, vIn);
    }
};

} // namespace net::minecraft::client::model
