// ComputeDispatcher::workGroups against its Java original, case for case.
//
// Java (Iris ComputeProgram.getWorkGroups(width, height)):
//
//   if (absoluteWorkGroups != null) {
//       cachedWorkGroups = absoluteWorkGroups;
//   } else if (relativeWorkGroups != null) {
//       cachedWorkGroups = new Vector3i(
//           (int) Math.ceil(Math.ceil((width  * relativeWorkGroups.x)) / localSize[0]),
//           (int) Math.ceil(Math.ceil((height * relativeWorkGroups.y)) / localSize[1]), 1);
//   } else {
//       cachedWorkGroups = new Vector3i(
//           (int) Math.ceil(width / localSize[0]), (int) Math.ceil(height / localSize[1]), 1);
//   }
//
// and localSize is read off the LINKED program:
//
//   IrisRenderSystem.getProgramiv(program, GL43C.GL_COMPUTE_WORK_GROUP_SIZE, localSize);
//
// Only two directives feed this — `const ivec3 workGroups` and
// `const vec2 workGroupsRender` (ProgramSet's compute loop calls
// ComputeDirectiveParser for those keys and no others).
#include <gtest/gtest.h>
#include <array>
#include <cmath>
#include <utility>
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaders/ComputeDispatcher.hpp"
namespace net::minecraft::test {
namespace {
using client::render::PackPass;
using client::render::ComputeDispatcher::workGroups;
// A pass that declared neither directive: Java's third branch.
PackPass undeclaredPass() {
 PackPass pass;
 pass.type = "compute";
 pass.name = "composite";
 return pass;
}
PackPass absolutePass(int x, int y, int z) {
 PackPass pass = undeclaredPass();
 pass.relativeGroups = false;
 pass.groups[0] = x;
 pass.groups[1] = y;
 pass.groups[2] = z;
 return pass;
}
PackPass relativePass(float x, float y) {
 PackPass pass = undeclaredPass();
 pass.relativeGroups = true;
 pass.groupScale[0] = x;
 pass.groupScale[1] = y;
 return pass;
}
} // namespace
// `const ivec3 workGroups` is absolute: the viewport and the local size are not
// consulted at all. This is the branch every voxel-volume sweep relies on — a
// pack that asks for ivec3(16, 12, 16) must get 3072 groups whatever the window
// size is.
TEST(ComputeDispatchParity, AbsoluteWorkGroupsIgnoreViewportAndLocalSize) {
 const PackPass pass = absolutePass(16, 12, 16);
 const int localSize[3] = {8, 8, 8};
 const std::array<unsigned int, 3> small = workGroups(pass, localSize, 320, 240);
 const std::array<unsigned int, 3> large = workGroups(pass, localSize, 3840, 2160);
 EXPECT_EQ(small, (std::array<unsigned int, 3>{16u, 12u, 16u}));
 EXPECT_EQ(large, small);
 const int otherLocalSize[3] = {1, 1, 1};
 EXPECT_EQ(workGroups(pass, otherLocalSize, 1920, 1080), small);
}
// An absolute dispatch is the only way to get a Z extent above 1. Java's other
// two branches hardcode z = 1, so a 3D volume sweep that loses its absolute
// directive collapses to a single slab.
TEST(ComputeDispatchParity, OnlyAbsoluteDispatchesHaveDepth) {
 const int localSize[3] = {8, 8, 8};
 EXPECT_EQ(workGroups(absolutePass(16, 12, 16), localSize, 1920, 1080)[2], 16u);
 EXPECT_EQ(workGroups(relativePass(1.0f, 1.0f), localSize, 1920, 1080)[2], 1u);
 EXPECT_EQ(workGroups(undeclaredPass(), localSize, 1920, 1080)[2], 1u);
}
// Java's fallback for a pass that declared nothing is ceil(width / localSize),
// which is the relative case at scale (1, 1). Both spellings must agree, or the
// engine's default is not Java's default.
TEST(ComputeDispatchParity, UndeclaredMatchesRelativeAtUnitScale) {
 const int localSize[3] = {16, 16, 1};
 for(const auto [width, height] : {std::pair{1920, 1080}, std::pair{1366, 768}, std::pair{800, 600}}) {
  EXPECT_EQ(workGroups(undeclaredPass(), localSize, width, height),
            workGroups(relativePass(1.0f, 1.0f), localSize, width, height))
      << width << "x" << height;
 }
}
// ceil(width / localSize), spelled out against hand-computed Java values.
TEST(ComputeDispatchParity, UnitScaleRoundsUpToWholeGroups) {
 const int localSize[3] = {16, 16, 1};
 // 1920/16 = 120 exactly; 1080/16 = 67.5 -> 68.
 EXPECT_EQ(workGroups(undeclaredPass(), localSize, 1920, 1080),
           (std::array<unsigned int, 3>{120u, 68u, 1u}));
 // 1366/16 = 85.375 -> 86; 768/16 = 48 exactly.
 EXPECT_EQ(workGroups(undeclaredPass(), localSize, 1366, 768),
           (std::array<unsigned int, 3>{86u, 48u, 1u}));
}
// The relative branch, swept against a direct transcription of Java's
// expression. `ceil(ceil(a) / L) == ceil(a / L)` for a positive real and a
// positive integer L, so the inner ceil Java writes is not observable — but
// this asserts the whole expression rather than that identity, so the engine
// stays pinned to Java's form if either side is ever changed to something where
// the difference does show (a floor, a truncating int divide, a rounded scale).
TEST(ComputeDispatchParity, RelativeDispatchMatchesJavasExpression) {
 const auto java = [](int width, int height, float scaleX, float scaleY, const int (&local)[3]) {
  return std::array<unsigned int, 3>{
      static_cast<unsigned int>(
          std::ceil(std::ceil(static_cast<double>(width) * scaleX) / static_cast<double>(local[0]))),
      static_cast<unsigned int>(
          std::ceil(std::ceil(static_cast<double>(height) * scaleY) / static_cast<double>(local[1]))),
      1u};
 };
 const int localSizes[][3] = {{8, 8, 1}, {16, 16, 1}, {32, 4, 1}, {1, 1, 1}};
 const std::pair<int, int> viewports[] = {{1920, 1080}, {1366, 768}, {1279, 719}, {800, 601}, {17, 13}};
 const std::pair<float, float> scales[] = {{1.0f, 1.0f}, {0.5f, 0.5f}, {0.25f, 1.0f}, {0.75f, 0.75f}};
 for(const auto& local : localSizes) {
  for(const auto& [width, height] : viewports) {
   for(const auto& [scaleX, scaleY] : scales) {
    EXPECT_EQ(workGroups(relativePass(scaleX, scaleY), local, width, height),
              java(width, height, scaleX, scaleY, local))
        << "local " << local[0] << "x" << local[1] << ", viewport " << width << "x" << height << ", scale "
        << scaleX << "x" << scaleY;
   }
  }
 }
}
// The dispatch must never be empty; Java's Math.ceil of a positive quotient is
// always >= 1, and a zero group count would skip the pass silently.
TEST(ComputeDispatchParity, NeverDispatchesZeroGroups) {
 const int localSize[3] = {32, 32, 1};
 const std::array<unsigned int, 3> tiny = workGroups(relativePass(0.01f, 0.01f), localSize, 16, 16);
 EXPECT_GE(tiny[0], 1u);
 EXPECT_GE(tiny[1], 1u);
 EXPECT_EQ(tiny[2], 1u);
}
// A local size the driver reports as 0 (or that was never queried) must not
// divide the viewport by zero. Java gets a real value from
// GL_COMPUTE_WORK_GROUP_SIZE; this is the belt-and-braces guard on the path
// where the query has not run yet.
TEST(ComputeDispatchParity, ZeroLocalSizeDoesNotDivideByZero) {
 const int localSize[3] = {0, 0, 0};
 const std::array<unsigned int, 3> groups = workGroups(relativePass(1.0f, 1.0f), localSize, 64, 32);
 EXPECT_EQ(groups, (std::array<unsigned int, 3>{64u, 32u, 1u}));
}
} // namespace net::minecraft::test
