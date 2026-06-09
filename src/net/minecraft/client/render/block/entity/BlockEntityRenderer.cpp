#include "net/minecraft/client/render/block/entity/BlockEntityRenderer.hpp"

#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"

namespace net::minecraft::client::render::block::entity {

void BlockEntityRenderer::bindTexture(const std::string& path)
{
    if (dispatcher == nullptr || dispatcher->textureManager == nullptr) {
        return;
    }
    const int textureId = dispatcher->textureManager->getTextureId(path);
    net::minecraft::client::gl::GL11::glBindTexture(net::minecraft::client::gl::GL11::GL_TEXTURE_2D, textureId);
}

net::minecraft::client::font::TextRenderer* BlockEntityRenderer::getTextRenderer() const
{
    return dispatcher != nullptr ? dispatcher->getTextRenderer() : nullptr;
}

} // namespace net::minecraft::client::render::block::entity
