#pragma once
#include <algorithm>
#include <array>
#include <vector>

#include "net/minecraft/client/model/Vertex.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"

namespace net::minecraft::client::model {
class Quad {
   public:
    std::vector<Vertex> vertices;
    int verticesCount = 0;
    bool flipNormal = false;
    Quad() = default;

    explicit Quad(std::vector<Vertex> verticesIn)
        : vertices(std::move(verticesIn)), verticesCount(static_cast<int>(vertices.size())) {
    }

    explicit Quad(const std::array<Vertex, 4>& verticesIn)
        : vertices(verticesIn.begin(), verticesIn.end()), verticesCount(4) {
    }

    Quad(std::vector<Vertex> verticesIn, int u1, int v1, int u2, int v2) : Quad(std::move(verticesIn)) {
        remap(u1, v1, u2, v2);
    }

    Quad(const std::array<Vertex, 4>& verticesIn, int u1, int v1, int u2, int v2) : Quad(verticesIn) {
        remap(u1, v1, u2, v2);
    }

    void flip() {
        std::reverse(vertices.begin(), vertices.end());
    }

    void emitTo(render::Tessellator& tessellator, float scale) const {
        if (vertices.size() < 3) {
            return;
        }
        const Vec3d edgeA = vertices[1].pos.relativize(vertices[0].pos);
        const Vec3d edgeB = vertices[1].pos.relativize(vertices[2].pos);
        Vec3d normal = edgeB.crossProduct(edgeA).normalize();
        if (flipNormal) {
            normal = normal * -1.0;
        }
        tessellator.normal(static_cast<float>(normal.x), static_cast<float>(normal.y), static_cast<float>(normal.z));
        for (const Vertex& vertex : vertices) {
            tessellator.vertex(vertex.pos.x * scale, vertex.pos.y * scale, vertex.pos.z * scale, vertex.u, vertex.v);
        }
    }

    void render(render::Tessellator& tessellator, float scale) const {
        if (vertices.size() < 3) {
            return;
        }
        tessellator.startQuads();
        emitTo(tessellator, scale);
        tessellator.draw();
    }

   private:
    void remap(int u1, int v1, int u2, int v2) {
        if (vertices.size() < 4) {
            return;
        }
        constexpr float uvInsetU = 0.0015625f;
        constexpr float uvInsetV = 0.003125f;
        vertices[0] =
            vertices[0].remap(static_cast<float>(u2) / 64.0f - uvInsetU, static_cast<float>(v1) / 32.0f + uvInsetV);
        vertices[1] =
            vertices[1].remap(static_cast<float>(u1) / 64.0f + uvInsetU, static_cast<float>(v1) / 32.0f + uvInsetV);
        vertices[2] =
            vertices[2].remap(static_cast<float>(u1) / 64.0f + uvInsetU, static_cast<float>(v2) / 32.0f - uvInsetV);
        vertices[3] =
            vertices[3].remap(static_cast<float>(u2) / 64.0f - uvInsetU, static_cast<float>(v2) / 32.0f - uvInsetV);
    }
};
}  // namespace net::minecraft::client::model
