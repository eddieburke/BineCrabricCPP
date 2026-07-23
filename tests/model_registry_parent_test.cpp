#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <gtest/gtest.h>
#include "net/minecraft/mod/model/ModModels.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
namespace net::minecraft::mod::model {
namespace {
std::atomic<unsigned long long> gModelParentTestId{0};
class ModelRegistryParentTest : public testing::Test {
 protected:
 void SetUp() override {
  runtime::host().shutdown();
  const std::string suffix = std::to_string(++gModelParentTestId);
  modId_ = "model_parent_test_" + suffix;
  root_ = std::filesystem::temp_directory_path() / "minecraft_native_model_parent_tests" / suffix;
  modRoot_ = root_ / "mods" / modId_;
  std::filesystem::remove_all(root_);
  writeFile(modRoot_ / "mod.json", "{\"id\":\"" + modId_ + "\",\"enabled\":false}");
  runtime::host().initialize(root_);
 }
 void TearDown() override {
  runtime::host().shutdown();
  std::filesystem::remove_all(root_);
 }
 void writeModel(const std::string& path, const std::string& json) {
  writeFile(modRoot_ / std::filesystem::path(path), json);
 }
 const BakedModel* load(const std::string& path, std::string& error) {
  const int handle = loadBakedModel(modId_, path, error);
  return bakedModelForHandle(handle);
 }

 private:
 static void writeFile(const std::filesystem::path& path, const std::string& contents) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::binary);
  output << contents;
 }
 std::filesystem::path root_;
 std::filesystem::path modRoot_;
 std::string modId_;
};
TEST_F(ModelRegistryParentTest, SelfParentKeepsOwnElementsAndTextures) {
 writeModel("models/camera/camera.json", R"({
    "parent":"camera",
    "textures":{"0":"camera_front"},
    "elements":[{"from":[0,0,0],"to":[16,16,16],"faces":{"north":{"texture":"#0"}}}]
  })");
 std::string error;
 const BakedModel* model = load("models/camera/camera.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches.size(), 1u);
 EXPECT_EQ(model->batches[0].texturePath, "models/camera/camera_front.png");
 EXPECT_EQ(model->batches[0].quads.size(), 1u);
}
TEST_F(ModelRegistryParentTest, MultilevelParentUsesChildTextureOverride) {
 writeModel("models/shared/base.json", R"({
    "textures":{"surface":"base_surface"},
    "elements":[{"from":[0,0,0],"to":[16,16,16],"faces":{"north":{"texture":"#surface"}}}]
  })");
 writeModel("models/shared/mid.json", R"({"parent":"base"})");
 writeModel("models/camera/child.json", R"({
    "parent":"models/shared/mid",
    "textures":{"surface":"child_surface"}
  })");
 std::string error;
 const BakedModel* model = load("models/camera/child.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches.size(), 1u);
 EXPECT_EQ(model->batches[0].texturePath, "models/camera/child_surface.png");
}
TEST_F(ModelRegistryParentTest, ParentDirectTextureUsesParentDirectory) {
 writeModel("models/shared/base/base.json", R"({
    "elements":[{"from":[0,0,0],"to":[16,16,16],"faces":{"north":{"texture":"base_face"}}}]
  })");
 writeModel("models/camera/child.json", R"({"parent":"models/shared/base/base"})");
 std::string error;
 const BakedModel* model = load("models/camera/child.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches.size(), 1u);
 EXPECT_EQ(model->batches[0].texturePath, "models/shared/base/base_face.png");
}
TEST_F(ModelRegistryParentTest, NamespacedParentAndTextureResolveUnderAssets) {
 writeModel("assets/demo/models/block/base.json", R"({
    "textures":{"surface":"demo:block/base"},
    "elements":[{"from":[0,0,0],"to":[16,16,16],"faces":{"north":{"texture":"#surface"}}}]
  })");
 writeModel("models/camera/child.json", R"({"parent":"demo:block/base"})");
 std::string error;
 const BakedModel* model = load("models/camera/child.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches.size(), 1u);
 EXPECT_EQ(model->batches[0].texturePath, "assets/demo/textures/block/base.png");
}
TEST_F(ModelRegistryParentTest, BlockbenchDefaultNamespaceParentResolvesUnderMinecraftAssets) {
 writeModel("assets/minecraft/models/block/base.json", R"({
    "textures":{"surface":"minecraft:block/base"},
    "elements":[{"from":[0,0,0],"to":[16,16,16],"faces":{"north":{"texture":"#surface"}}}]
  })");
 writeModel("models/camera/child.json", R"({"parent":"block/base"})");
 std::string error;
 const BakedModel* model = load("models/camera/child.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches.size(), 1u);
 EXPECT_EQ(model->batches[0].texturePath, "assets/minecraft/textures/block/base.png");
}
TEST_F(ModelRegistryParentTest, ParentCycleStopsAfterMergingAvailableData) {
 writeModel("models/cycle/a.json", R"({"parent":"b"})");
 writeModel("models/cycle/b.json", R"({
    "parent":"a",
    "textures":{"surface":"b"},
    "elements":[{"from":[0,0,0],"to":[16,16,16],"faces":{"north":{"texture":"#surface"}}}]
  })");
 std::string error;
 const BakedModel* model = load("models/cycle/a.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches.size(), 1u);
 EXPECT_EQ(model->batches[0].texturePath, "models/cycle/b.png");
}
}
} // namespace net::minecraft::mod::model
