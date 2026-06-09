#include "net/minecraft/client/render/block/entity/PistonBlockEntityRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/PistonBlock.hpp"
#include "net/minecraft/block/PistonHeadBlock.hpp"
#include "net/minecraft/block/entity/PistonBlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"

namespace net::minecraft::client::render::block::entity {

void PistonBlockEntityRenderer::setWorld(net::minecraft::World* world)
{
    blockRenderManager.setBlockView(world);
}

void PistonBlockEntityRenderer::render(const net::minecraft::block::entity::BlockEntity& blockEntity, double x, double y, double z, float tickDelta)
{
    const auto* piston = dynamic_cast<const net::minecraft::block::entity::PistonBlockEntity*>(&blockEntity);
    if (piston == nullptr) {
        return;
    }

    net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(piston->getPushedBlockId())];
    if (block == nullptr || piston->getProgress(tickDelta) >= 1.0f) {
        return;
    }

    Tessellator& tessellator = render::INSTANCE;
    bindTexture("/terrain.png");
    platform::Lighting::turnOff();
    gl::GL11::glBlendFunc(770, 771);
    gl::GL11::glEnable(3042);
    gl::GL11::glDisable(2884);
    if (Minecraft::isAmbientOcclusionEnabled()) {
        gl::GL11::glShadeModel(7425);
    } else {
        gl::GL11::glShadeModel(7424);
    }

    tessellator.startQuads();
    tessellator.translate(
        static_cast<double>(static_cast<float>(x) - static_cast<float>(piston->x) + piston->getRenderOffsetX(tickDelta)),
        static_cast<double>(static_cast<float>(y) - static_cast<float>(piston->y) + piston->getRenderOffsetY(tickDelta)),
        static_cast<double>(static_cast<float>(z) - static_cast<float>(piston->z) + piston->getRenderOffsetZ(tickDelta)));
    tessellator.color(1, 1, 1);

    if (block == net::minecraft::block::Block::PISTON_HEAD && piston->getProgress(tickDelta) < 0.5f) {
        blockRenderManager.renderPistonHeadWithoutCulling(block->id, piston->x, piston->y, piston->z, false);
    } else if (piston->isSource() && !piston->isExtending()) {
        auto* pistonBlock = dynamic_cast<net::minecraft::block::PistonBlock*>(block);
        auto* pistonHead = dynamic_cast<net::minecraft::block::PistonHeadBlock*>(net::minecraft::block::Block::PISTON_HEAD);
        if (pistonBlock != nullptr && pistonHead != nullptr) {
            pistonHead->setSprite(pistonBlock->getTopTexture());
            blockRenderManager.renderPistonHeadWithoutCulling(pistonHead->id, piston->x, piston->y, piston->z, piston->getProgress(tickDelta) < 0.5f);
            pistonHead->clearSprite();
            tessellator.translate(static_cast<double>(static_cast<float>(x) - static_cast<float>(piston->x)),
                static_cast<double>(static_cast<float>(y) - static_cast<float>(piston->y)),
                static_cast<double>(static_cast<float>(z) - static_cast<float>(piston->z)));
            blockRenderManager.renderExtendedPiston(block->id, piston->x, piston->y, piston->z);
        }
    } else {
        blockRenderManager.renderWithoutCulling(block->id, piston->x, piston->y, piston->z);
    }

    tessellator.translate(0.0, 0.0, 0.0);
    tessellator.draw();
    platform::Lighting::turnOn();
}

} // namespace net::minecraft::client::render::block::entity
