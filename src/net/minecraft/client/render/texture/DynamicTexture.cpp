#include "net/minecraft/client/render/texture/DynamicTexture.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"

namespace net::minecraft::client::render::texture {

void DynamicTexture::bind(::net::minecraft::client::texture::TextureManager& textureManager)
{
    if (atlas == 0) {
        gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D,
            static_cast<unsigned int>(textureManager.getTextureId("/terrain.png")));
    } else if (atlas == 1) {
        gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D,
            static_cast<unsigned int>(textureManager.getTextureId("/gui/items.png")));
    }
}

} // namespace net::minecraft::client::render::texture
