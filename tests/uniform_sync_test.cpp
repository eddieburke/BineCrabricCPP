#include <gtest/gtest.h>
#include <chrono>
#include <cstdio>
#include "net/minecraft/client/gl/UniformSync.hpp"

namespace net::minecraft::client::gl {
namespace {
struct Fixture {
  UniformSnapshot snap;
  PipelineState pipeline;
  EngineLighting lighting;
  float modelView[16]{};
  float projection[16]{};
};
} // namespace

TEST(UniformSync, FirstDiffUploadsEverything) {
  Fixture f;
  const UniformDelta d = diffAndUpdate(f.snap, f.pipeline, f.lighting, f.modelView, f.projection);
  EXPECT_TRUE(d.pipeline);
  EXPECT_TRUE(d.lighting);
  EXPECT_TRUE(d.modelView);
  EXPECT_TRUE(d.projection);
}

TEST(UniformSync, CleanStateUploadsNothing) {
  Fixture f;
  diffAndUpdate(f.snap, f.pipeline, f.lighting, f.modelView, f.projection);
  f.pipeline.dirty = false;
  const UniformDelta d = diffAndUpdate(f.snap, f.pipeline, f.lighting, f.modelView, f.projection);
  EXPECT_FALSE(d.pipeline);
  EXPECT_FALSE(d.lighting);
  EXPECT_FALSE(d.modelView);
  EXPECT_FALSE(d.projection);
}

TEST(UniformSync, DetectsIsolatedChanges) {
  Fixture f;
  diffAndUpdate(f.snap, f.pipeline, f.lighting, f.modelView, f.projection);
  f.pipeline.dirty = false;

  f.modelView[12] = 5.0f;
  UniformDelta d = diffAndUpdate(f.snap, f.pipeline, f.lighting, f.modelView, f.projection);
  EXPECT_TRUE(d.modelView);
  EXPECT_FALSE(d.projection);
  EXPECT_FALSE(d.pipeline);
  EXPECT_FALSE(d.lighting);

  f.lighting.ambient[0] = 0.4f;
  d = diffAndUpdate(f.snap, f.pipeline, f.lighting, f.modelView, f.projection);
  EXPECT_TRUE(d.lighting);
  EXPECT_FALSE(d.modelView);

  f.pipeline.fogStart = 12.0f;
  f.pipeline.dirty = true; // as every GlDraw shim write does
  d = diffAndUpdate(f.snap, f.pipeline, f.lighting, f.modelView, f.projection);
  EXPECT_TRUE(d.pipeline);
  EXPECT_FALSE(d.lighting);
}

// Benchmark: the diff must be far cheaper than the ~20 uniform uploads it replaces.
// Prints ns/call for the steady-state (nothing changed) path so regressions are
// measured, not guessed. Budget is deliberately loose to stay CI-safe.
TEST(UniformSync, BenchSteadyStateDiff) {
  Fixture f;
  diffAndUpdate(f.snap, f.pipeline, f.lighting, f.modelView, f.projection);
  f.pipeline.dirty = false;
  constexpr int kIters = 2000000;
  int changed = 0;
  const auto t0 = std::chrono::steady_clock::now();
  for(int i = 0; i < kIters; ++i) {
    const UniformDelta d = diffAndUpdate(f.snap, f.pipeline, f.lighting, f.modelView, f.projection);
    changed += d.pipeline | d.lighting | d.modelView | d.projection;
  }
  const auto t1 = std::chrono::steady_clock::now();
  const double nsPerCall =
      std::chrono::duration<double, std::nano>(t1 - t0).count() / kIters;
  std::printf("[bench] UniformSync steady-state diff: %.1f ns/call\n", nsPerCall);
  EXPECT_EQ(changed, 0);
  EXPECT_LT(nsPerCall, 1000.0); // generous ceiling; typical is tens of ns
}
} // namespace net::minecraft::client::gl
