#include "net/minecraft/client/render/block/entity/BlockEntityRenderer.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
namespace net::minecraft::client::render::block::entity {
void BlockEntityRenderer::bindTexture(const std::string& path) {
 if(dispatcher == nullptr || dispatcher->textureManager == nullptr) {
  return;
 }
 const int textureId = dispatcher->textureManager->getTextureId(path);
 render::RenderSystem::activeTexture(0x84C0);
 render::RenderSystem::enableTexture();
 render::RenderSystem::bindTexture(0x0DE1, textureId);
}
font::TextRenderer* BlockEntityRenderer::getTextRenderer() const {
 return dispatcher != nullptr ? dispatcher->getTextRenderer() : nullptr;
}
} // namespace net::minecraft::client::render::block::entity
