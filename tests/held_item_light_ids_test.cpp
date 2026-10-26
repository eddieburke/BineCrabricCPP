#include <gtest/gtest.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/client/render/uniforms/FrameData.hpp"
#include "net/minecraft/item/ItemStack.hpp"
namespace net::minecraft::test {
namespace {
std::string rethinkingVoxelsFile(const std::filesystem::path& relative) {
 std::ifstream in(std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / "rethinking-voxels_r0.1-beta9" /
                  "shaders" / relative);
 std::stringstream text;
 text << in.rdbuf();
 return text.str();
}
} // namespace
TEST(HeldItemLightIds, HeldTorchReportsItsBlockPropertiesIdLikeIrisBlockItems) {
 ASSERT_NE(rethinkingVoxelsFile("block.properties").find("block.10496=torch wall_torch"), std::string::npos);
 ASSERT_NE(rethinkingVoxelsFile("lib/materials/shadowChecks.glsl").find("mat == 10496"), std::string::npos);
 ASSERT_NE(block::Block::TORCH, nullptr);
 std::array<int, 256> ids{};
 ids.fill(-1);
 ids[static_cast<std::size_t>(block::Block::TORCH->id)] = 10496;
 client::render::setShaderBlockIds(ids);
 EXPECT_EQ(client::render::item::ItemModelRenderer::shaderId(ItemStack(block::Block::TORCH)), 10496);
 ASSERT_NE(block::Block::STONE, nullptr);
 EXPECT_EQ(client::render::item::ItemModelRenderer::shaderId(ItemStack(block::Block::STONE)), 0);
 const ItemStack stick(280);
 ASSERT_NE(stick.getItem(), nullptr);
 EXPECT_EQ(client::render::item::ItemModelRenderer::shaderId(stick), -1);
 ids.fill(0);
 client::render::setShaderBlockIds(ids);
}
TEST(HeldItemLightIds, HeldItemIdsAreInvalidWithoutAPlayer) {
 const client::render::FrameRenderCamera camera{};
 const auto values =
     client::render::buildShaderFrameData(64, 64, 0.0f, 0, false, false, camera, camera, nullptr, 0.0f);
 EXPECT_EQ(values.heldItemId, -1);
 EXPECT_EQ(values.heldItemId2, -1);
 EXPECT_EQ(values.heldBlockLightValue, 0);
 EXPECT_EQ(values.heldBlockLightValue2, 0);
}
} // namespace net::minecraft::test
