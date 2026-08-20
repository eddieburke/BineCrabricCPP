#include <gtest/gtest.h>
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/entity/EntityRenderers.hpp"
#include "net/minecraft/entity/passive/SheepEntity.hpp"
namespace net::minecraft::client::render::entity::test {
namespace {
class SheepRendererProbe final : public SheepEntityRenderer {
 public:
 SheepRendererProbe() : SheepEntityRenderer(nullptr, nullptr, 0.0f) {}
 using SheepEntityRenderer::bindTexture;
};
}
TEST(EntityLighting, SheepWoolColorDoesNotContainWorldBrightness) {
 SheepRendererProbe renderer;
 net::minecraft::entity::passive::SheepEntity sheep;
 sheep.setColor(0);
 ASSERT_TRUE(renderer.bindTexture(sheep, 0, 0.0f));
 const float* color = core::constColor();
 EXPECT_FLOAT_EQ(color[0], 1.0f);
 EXPECT_FLOAT_EQ(color[1], 1.0f);
 EXPECT_FLOAT_EQ(color[2], 1.0f);
 EXPECT_FLOAT_EQ(color[3], 1.0f);
 sheep.setColor(14);
 ASSERT_TRUE(renderer.bindTexture(sheep, 0, 0.0f));
 color = core::constColor();
 EXPECT_FLOAT_EQ(color[0], net::minecraft::entity::passive::SheepEntity::COLORS[14][0]);
 EXPECT_FLOAT_EQ(color[1], net::minecraft::entity::passive::SheepEntity::COLORS[14][1]);
 EXPECT_FLOAT_EQ(color[2], net::minecraft::entity::passive::SheepEntity::COLORS[14][2]);
 EXPECT_FLOAT_EQ(color[3], 1.0f);
}
}
