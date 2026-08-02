#include <gtest/gtest.h>
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
namespace net::minecraft::test {
namespace {
using client::render::world::ColumnLightingGate;
}
TEST(LightingReadyGate, FreshlyCreatedColumnIsHeldUntilItsRegionDrains) {
 ColumnLightingGate gate;
 gate.gate(3, -2);
 // A freshly-created column's first mesh stays held (lit() == false) until
 // doLightingUpdates applies a drained region covering the column.
 EXPECT_FALSE(gate.lit(3, -2));
 gate.release(3, -2);
 EXPECT_TRUE(gate.lit(3, -2));
}
TEST(LightingReadyGate, ReleasedColumnMeshesNormally) {
 ColumnLightingGate gate;
 gate.gate(0, 0);
 gate.release(0, 0);
 // Once lit the column is meshed normally; a repeat release is a no-op.
 EXPECT_TRUE(gate.lit(0, 0));
 gate.release(0, 0);
 EXPECT_TRUE(gate.lit(0, 0));
 EXPECT_TRUE(gate.empty());
}
TEST(LightingReadyGate, ReleaseAllNeverStallsNoBoxColumns) {
 ColumnLightingGate gate;
 gate.gate(0, 0);
 gate.gate(1, 0);
 gate.gate(-3, 4);
 // The engine going fully idle releases every held column (non-optional
 // completion): a column with no pending boxes is never held forever.
 gate.releaseAll();
 EXPECT_TRUE(gate.lit(0, 0));
 EXPECT_TRUE(gate.lit(1, 0));
 EXPECT_TRUE(gate.lit(-3, 4));
 EXPECT_TRUE(gate.empty());
}
TEST(LightingReadyGate, UngatedColumnsAreNeverHeld) {
 ColumnLightingGate gate;
 EXPECT_TRUE(gate.lit(5, 5));
}
TEST(LightingReadyGate, ReleaseIsIdempotentForUnknownColumns) {
 ColumnLightingGate gate;
 gate.gate(2, 2);
 gate.release(2, 2);
 gate.release(2, 2);
 gate.release(9, 9);
 EXPECT_TRUE(gate.lit(2, 2));
 EXPECT_TRUE(gate.empty());
}
TEST(LightingReadyGate, RegatedColumnStaysHeldUntilNextRelease) {
 ColumnLightingGate gate;
 gate.gate(7, 7);
 gate.release(7, 7);
 gate.gate(7, 7);
 EXPECT_FALSE(gate.lit(7, 7));
  gate.release(7, 7);
  EXPECT_TRUE(gate.lit(7, 7));
}
} // namespace net::minecraft::test
