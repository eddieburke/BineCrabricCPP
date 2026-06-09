#include "net/minecraft/client/render/chunk/ChunkDrawTransform.hpp"

#include "net/minecraft/client/gl/GL11.hpp"

namespace net::minecraft::client::render::chunk {

void ChunkDrawTransform::applyGl() const noexcept
{
    gl::GL11::glTranslatef(static_cast<float>(renderX), static_cast<float>(renderY), static_cast<float>(renderZ));
    gl::GL11::glTranslatef(static_cast<float>(-sizeZ) / 2.0f, static_cast<float>(-sizeY) / 2.0f,
        static_cast<float>(-sizeZ) / 2.0f);
    gl::GL11::glScalef(kScale, kScale, kScale);
    gl::GL11::glTranslatef(static_cast<float>(sizeZ) / 2.0f, static_cast<float>(sizeY) / 2.0f,
        static_cast<float>(sizeZ) / 2.0f);
}

} // namespace net::minecraft::client::render::chunk
