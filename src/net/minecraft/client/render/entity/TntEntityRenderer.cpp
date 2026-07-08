#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/entity/TntEntity.hpp"

namespace net::minecraft::client::render::entity {
namespace {
constexpr int kTntBlockId = 46;
}

TntEntityRenderer::TntEntityRenderer() {
    shadowRadius = 0.5f;
}

void TntEntityRenderer::render(
    const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) {
    (void) yaw;
    const auto* tnt = dynamic_cast<const net::minecraft::TntEntity*>(&entity);
    if (tnt == nullptr) {
        return;
    }
    gl::pushMatrix();
    gl::translatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    if (static_cast<float>(tnt->fuse) - tickDelta + 1.0f < 10.0f) {
        float pulse = 1.0f - (static_cast<float>(tnt->fuse) - tickDelta + 1.0f) / 10.0f;
        if (pulse < 0.0f) {
            pulse = 0.0f;
        }
        if (pulse > 1.0f) {
            pulse = 1.0f;
        }
        pulse *= pulse;
        pulse *= pulse;
        const float scale = 1.0f + pulse * 0.3f;
        gl::scalef(scale, scale, scale);
    }
    const float flash = (1.0f - (static_cast<float>(tnt->fuse) - tickDelta + 1.0f) / 100.0f) * 0.8f;
    bindTexture("/terrain.png");
    if (net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[kTntBlockId]) {
        blockRenderManager_.render(*block, 0, entity.getBrightnessAtEyes(tickDelta));
        if (tnt->fuse / 5 % 2 == 0) {
            const gl::preset::TntFlash flashCaps;
            render::platform::Lighting::turnOff();
            gl::blendFunc(gl::blend::SrcAlpha, gl::blend::DstAlpha);
            gl::color4f(1.0f, 1.0f, 1.0f, flash);
            blockRenderManager_.render(*block, 0, 1.0f);
            gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
            render::platform::Lighting::turnOn();
        }
    }
    gl::popMatrix();
}
}  // namespace net::minecraft::client::render::entity

#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/TntEntity.hpp"

namespace net::minecraft::entity {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> TntEntity::ClientRenderer::create() {
    return std::make_unique<::net::minecraft::client::render::entity::TntEntityRenderer>();
}
}  // namespace net::minecraft::entity

namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::TntEntity> autoRendererReg;
}  // namespace
