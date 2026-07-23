#include <gtest/gtest.h>
#include "net/minecraft/client/render/shaderpack/PackManifest.hpp"
namespace net::minecraft::client::render::shaderpack {
TEST(PackManifestTest, RequiresKnownProgramStage) {
 PackManifest manifest;
 std::string error;
 EXPECT_FALSE(PackManifest::parse(
     R"({"programs":{"main":{"vsh":"a.vsh","fsh":"a.fsh"}}})", manifest, error));
 EXPECT_NE(error.find("stage"), std::string::npos);
}
TEST(PackManifestTest, RejectsNonPostPassProgram) {
 PackManifest manifest;
 std::string error;
 EXPECT_FALSE(PackManifest::parse(
     R"({"programs":{"terrain":{"stage":"terrain","vsh":"a.vsh","fsh":"a.fsh"}},"passes":[{"program":"terrain"}]})",
     manifest,
     error));
 EXPECT_NE(error.find("post"), std::string::npos);
}
TEST(PackManifestTest, AcceptsPostProgramWithDeclaredResources) {
 PackManifest manifest;
 std::string error;
 EXPECT_TRUE(PackManifest::parse(
     R"({"programs":{"composite":{"stage":"post","vsh":"a.vsh","fsh":"a.fsh"}},"targets":{"half":{"format":"RGBA8","scale":0.5}},"passes":[{"program":"composite","inputs":["colortex0","depthtex0"],"output":"half"}]})",
     manifest,
     error));
 ASSERT_TRUE(manifest.valid());
 EXPECT_EQ(manifest.programs.at("composite").stage, "post");
}
} // namespace net::minecraft::client::render::shaderpack
