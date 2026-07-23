#include <random>
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/entity/LightningEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::entity {
void LightningEntityRenderer::render(
    const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) {
 (void)yaw;
 (void)tickDelta;
 const auto* lightning = dynamic_cast<const net::minecraft::LightningEntity*>(&entity);
 if(lightning == nullptr) {
  return;
 }
 Tessellator& tessellator = Tessellator::INSTANCE;
 render::RenderPassScope passScope(render::RenderType::entityCutout());
 render::RenderSystem::disableLighting();
 double offsets[8]{};
 double offsets2[8]{};
 double offsetX = 0.0;
 double offsetZ = 0.0;
 std::mt19937_64 rng(static_cast<std::uint64_t>(lightning->seed));
 std::uniform_int_distribution<int> jitter(-5, 5);
 for(int i = 7; i >= 0; --i) {
  offsets[i] = offsetX;
  offsets2[i] = offsetZ;
  offsetX += static_cast<double>(jitter(rng));
  offsetZ += static_cast<double>(jitter(rng));
 }
 for(int pass = 0; pass < 4; ++pass) {
  std::mt19937_64 rng2(static_cast<std::uint64_t>(lightning->seed));
  std::uniform_int_distribution<int> jitterFine(-15, 15);
  for(int branch = 0; branch < 3; ++branch) {
   int segmentEnd = 7;
   int segmentStart = 0;
   if(branch > 0) {
    segmentEnd = 7 - branch;
   }
   if(branch > 0) {
    segmentStart = segmentEnd - 2;
   }
   double relX = offsets[segmentEnd] - offsetX;
   double relZ = offsets2[segmentEnd] - offsetZ;
   for(int k = segmentEnd; k >= segmentStart; --k) {
    const double prevRelX = relX;
    const double prevRelZ = relZ;
    if(branch == 0) {
     relX += static_cast<double>(jitter(rng2));
     relZ += static_cast<double>(jitter(rng2));
    } else {
     relX += static_cast<double>(jitterFine(rng2));
     relZ += static_cast<double>(jitterFine(rng2));
    }
    tessellator.start(5); // GL_TRIANGLE_STRIP
    constexpr float colorScale = 0.5f;
    tessellator.color(0.9f * colorScale, 0.9f * colorScale, 1.0f * colorScale, 0.3f);
    double outerRadius = 0.1 + static_cast<double>(pass) * 0.2;
    if(branch == 0) {
     outerRadius *= static_cast<double>(k) * 0.1 + 1.0;
    }
    double innerRadius = 0.1 + static_cast<double>(pass) * 0.2;
    if(branch == 0) {
     innerRadius *= static_cast<double>(k - 1) * 0.1 + 1.0;
    }
    for(int seg = 0; seg < 5; ++seg) {
     double vtxLowX = x + 0.5 - outerRadius;
     double vtxLowZ = z + 0.5 - outerRadius;
     if(seg == 1 || seg == 2) {
      vtxLowX += outerRadius * 2.0;
     }
     if(seg == 2 || seg == 3) {
      vtxLowZ += outerRadius * 2.0;
     }
     double vtxHighX = x + 0.5 - innerRadius;
     double vtxHighZ = z + 0.5 - innerRadius;
     if(seg == 1 || seg == 2) {
      vtxHighX += innerRadius * 2.0;
     }
     if(seg == 2 || seg == 3) {
      vtxHighZ += innerRadius * 2.0;
     }
     tessellator.vertex(vtxHighX + relX, y + static_cast<double>(k * 16), vtxHighZ + relZ);
     tessellator.vertex(vtxLowX + prevRelX, y + static_cast<double>((k + 1) * 16), vtxLowZ + prevRelZ);
    }
    tessellator.draw();
   }
  }
 }
 render::RenderSystem::enableLighting();
}
} // namespace net::minecraft::client::render::entity
#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/LightningEntity.hpp"
namespace net::minecraft::entity {
std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> LightningEntity::ClientRenderer::create() {
 return std::make_unique<::net::minecraft::client::render::entity::LightningEntityRenderer>();
}
} // namespace net::minecraft::entity
namespace {
static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::LightningEntity> autoRendererReg;
} // namespace
