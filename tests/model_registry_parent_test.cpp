#include <algorithm>
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
TEST_F(ModelRegistryParentTest, TvBlockModelMapsItsBodyTexture) {
 writeModel("models/camera/tv/tv.json", R"({
    "format_version":"1.21.11",
    "texture_size":[16,16],
    "textures":{"body":"tv_body"},
    "elements":[{"from":[0,0,0],"to":[16,16,16],"faces":{
      "north":{"uv":[0,0,8,8],"texture":"#body"},
      "east":{"uv":[0,0,8,8],"texture":"#body"},
      "south":{"uv":[0,0,8,8],"texture":"#body"},
      "west":{"uv":[0,0,8,8],"texture":"#body"},
      "up":{"uv":[8,0,16,8],"texture":"#body"},
      "down":{"uv":[0,8,8,16],"texture":"#body"}
    }}]
  })");
 std::string error;
 const BakedModel* model = load("models/camera/tv/tv.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches.size(), 1u);
 EXPECT_EQ(model->batches[0].texturePath, "models/camera/tv/tv_body.png");
 EXPECT_EQ(model->batches[0].quads.size(), 6u);
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
// Blockbench writes uvs in "texture_size" space, which is independent of the
// texture image's pixel size. tv_body.png is 32x32 under texture_size [16,16],
// so uv 8 must land halfway across the image, not a quarter of the way.
TEST_F(ModelRegistryParentTest, FaceUvsAreNormalizedByTextureSizeNotImageSize) {
 writeModel("models/uv/half.json", R"({
    "texture_size":[16,16],
    "textures":{"body":"body"},
    "elements":[{"from":[0,0,0],"to":[16,16,16],"faces":{
      "north":{"uv":[0,0,8,8],"texture":"#body"}
    }}]
  })");
 std::string error;
 const BakedModel* model = load("models/uv/half.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches.size(), 1u);
 ASSERT_EQ(model->batches[0].quads.size(), 1u);
 const BakedQuad& quad = model->batches[0].quads[0];
 float maxU = 0.0f;
 float maxV = 0.0f;
 for(const BakedVertex& vertex : quad.vertices) {
  maxU = std::max(maxU, vertex.u);
  maxV = std::max(maxV, vertex.v);
 }
 EXPECT_FLOAT_EQ(maxU, 0.5f);
 EXPECT_FLOAT_EQ(maxV, 0.5f);
}
TEST_F(ModelRegistryParentTest, NonDefaultTextureSizeRescalesUvs) {
 writeModel("models/uv/wide.json", R"({
    "texture_size":[32,64],
    "textures":{"body":"body"},
    "elements":[{"from":[0,0,0],"to":[16,16,16],"faces":{
      "north":{"uv":[0,0,32,64],"texture":"#body"}
    }}]
  })");
 std::string error;
 const BakedModel* model = load("models/uv/wide.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches[0].quads.size(), 1u);
 const BakedQuad& quad = model->batches[0].quads[0];
 float maxU = 0.0f;
 float maxV = 0.0f;
 for(const BakedVertex& vertex : quad.vertices) {
  maxU = std::max(maxU, vertex.u);
  maxV = std::max(maxV, vertex.v);
 }
 EXPECT_FLOAT_EQ(maxU, 1.0f);
 EXPECT_FLOAT_EQ(maxV, 1.0f);
}
TEST_F(ModelRegistryParentTest, TextureSizeIsInheritedFromParent) {
 writeModel("models/uv/parent.json", R"({"texture_size":[32,32]})");
 writeModel("models/uv/child.json", R"({
    "parent":"parent",
    "textures":{"body":"body"},
    "elements":[{"from":[0,0,0],"to":[16,16,16],"faces":{
      "north":{"uv":[0,0,16,16],"texture":"#body"}
    }}]
  })");
 std::string error;
 const BakedModel* model = load("models/uv/child.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches[0].quads.size(), 1u);
 float maxU = 0.0f;
 for(const BakedVertex& vertex : model->batches[0].quads[0].vertices) {
  maxU = std::max(maxU, vertex.u);
 }
 EXPECT_FLOAT_EQ(maxU, 0.5f);
}
// repair_table's real shape: a base and a slab meeting at y=13. The two faces
// on that seam are sealed inside the model and z-fight if kept, so the baker
// drops exactly that pair and nothing else.
TEST_F(ModelRegistryParentTest, SealedSeamBetweenStackedElementsIsDropped) {
 writeModel("models/seal/table.json", R"({
    "textures":{"all":"all"},
    "elements":[
      {"from":[0,0,0],"to":[16,13,16],"faces":{
        "north":{"texture":"#all"},"east":{"texture":"#all"},"south":{"texture":"#all"},
        "west":{"texture":"#all"},"up":{"texture":"#all"},"down":{"texture":"#all"}}},
      {"from":[0,13,0],"to":[16,16,16],"faces":{
        "north":{"texture":"#all"},"east":{"texture":"#all"},"south":{"texture":"#all"},
        "west":{"texture":"#all"},"up":{"texture":"#all"},"down":{"texture":"#all"}}}
    ]
  })");
 std::string error;
 const BakedModel* model = load("models/seal/table.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches.size(), 1u);
 EXPECT_EQ(model->batches[0].quads.size(), 10u);
 for(const BakedQuad& quad : model->batches[0].quads) {
  for(const BakedVertex& vertex : quad.vertices) {
   const bool onSeam = vertex.y > 0.81f && vertex.y < 0.82f;
   const bool horizontalFace = quad.face == ModelFace::Up || quad.face == ModelFace::Down;
   EXPECT_FALSE(onSeam && horizontalFace) << "seam face survived baking";
  }
 }
 // The outer shell is intact, so the model still measures a full cube.
 EXPECT_FLOAT_EQ(model->bounds.min[1], 0.0f);
 EXPECT_FLOAT_EQ(model->bounds.max[1], 1.0f);
}
// A crossed billboard (simple_lantern, the tripod) is built from zero-thickness
// planes whose two faces are the same rectangle. Those must both survive — they
// are the whole model, not an interior seam.
TEST_F(ModelRegistryParentTest, ThinPlaneKeepsBothOfItsFaces) {
 writeModel("models/seal/plane.json", R"({
    "textures":{"0":"tripod"},
    "elements":[
      {"from":[0,0,8],"to":[16,16,8],
       "rotation":{"angle":45,"axis":"y","origin":[8,8,8]},
       "faces":{"north":{"uv":[0,0,16,16],"texture":"#0"},
                "south":{"uv":[0,0,16,16],"texture":"#0"}}},
      {"from":[0,0,8],"to":[16,16,8],
       "rotation":{"angle":-45,"axis":"y","origin":[8,8,8]},
       "faces":{"north":{"uv":[0,0,16,16],"texture":"#0"},
                "south":{"uv":[0,0,16,16],"texture":"#0"}}}
    ]
  })");
 std::string error;
 const BakedModel* model = load("models/seal/plane.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches.size(), 1u);
 EXPECT_EQ(model->batches[0].quads.size(), 4u);
 EXPECT_EQ(model->batches[0].texturePath, "models/seal/tripod.png");
}
// Unresolvable references drop their face silently; a model that is nothing but
// those is a load failure rather than an empty draw.
TEST_F(ModelRegistryParentTest, MissingTextureReferenceDropsOnlyThatFace) {
 writeModel("models/seal/partial.json", R"({
    "textures":{"0":"real"},
    "elements":[{"from":[0,0,8],"to":[16,16,8],"faces":{
      "north":{"uv":[0,0,16,16],"texture":"#0"},
      "south":{"uv":[0,0,16,16],"texture":"#0"},
      "up":{"uv":[0,0,16,16],"texture":"#missing"},
      "down":{"uv":[0,0,16,16],"texture":"#missing"}
    }}]
  })");
 std::string error;
 const BakedModel* model = load("models/seal/partial.json", error);
 ASSERT_NE(model, nullptr) << error;
 ASSERT_EQ(model->batches.size(), 1u);
 EXPECT_EQ(model->batches[0].quads.size(), 2u);
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
