// CameraPositionTracker — the iris port of CameraUniforms.CameraPositionTracker
// (X/Z-only origin shifting in 30000-block increments, 1000-block teleport check).
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CameraUniforms.java
#include <gtest/gtest.h>
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::client::render::CameraPositionTracker;
TEST(CameraPositionTracker, ShiftsXzOnlyOnWalkAcrossTheRangeBoundary) {
 CameraPositionTracker tracker;
 tracker.update(29999.5, 0.0, 0.0);
 EXPECT_DOUBLE_EQ(tracker.current(0), 29999.5);
 EXPECT_DOUBLE_EQ(tracker.current(1), 0.0);
 tracker.update(30001.25, -30001.75, 60002.5);
 // X: |30001.25| > 30000 → shift -(30001.25 - 1.25) = -30000.
 EXPECT_DOUBLE_EQ(tracker.current(0), 1.25);
 // Y: never shifted by Java's updateShift (CameraUniforms.java:104-107).
 EXPECT_DOUBLE_EQ(tracker.current(1), -30001.75);
 // Z: 60002.5 → shift -60000.
 EXPECT_DOUBLE_EQ(tracker.current(2), 2.5);
}
TEST(CameraPositionTracker, ShiftsNegativeCoordinatesInTheSameDirection) {
 CameraPositionTracker tracker;
 tracker.update(-29999.5, 0.0, 0.0);
 EXPECT_DOUBLE_EQ(tracker.current(0), -29999.5);
 tracker.update(-30001.75, 0.0, 0.0);
 // Java remainder: -30001.75 % 30000 = -1.75; shift = -(-30001.75 + 1.75) = 30000
 // → current = -1.75 (the "camera world position mod 30000" value).
 EXPECT_DOUBLE_EQ(tracker.current(0), -1.75);
}
TEST(CameraPositionTracker, PreviousMirrorsTheLastFrameAndStartsAtZero) {
 CameraPositionTracker tracker;
 tracker.update(100.0, 200.0, 300.0);
 EXPECT_DOUBLE_EQ(tracker.previous(0), 0.0);
 EXPECT_DOUBLE_EQ(tracker.previous(1), 0.0);
 EXPECT_DOUBLE_EQ(tracker.previous(2), 0.0);
 EXPECT_DOUBLE_EQ(tracker.previousUnshifted(1), 0.0);
 tracker.update(400.0, 500.0, 600.0);
 EXPECT_DOUBLE_EQ(tracker.previous(0), 100.0);
 EXPECT_DOUBLE_EQ(tracker.previous(1), 200.0);
 EXPECT_DOUBLE_EQ(tracker.previous(2), 300.0);
 EXPECT_DOUBLE_EQ(tracker.previousUnshifted(0), 100.0);
}
TEST(CameraPositionTracker, TeleportWithinRangeDoesNotReshift) {
 // Java getShift: -(value - value % 30000). For |value| < 30000 the remainder IS
 // the value, so the delta is always 0 — the 1000-block teleport check only fires
 // (with a nonzero result) at the exact |value| == 30000 boundary. Port replicates
 // Java verbatim, quirks included (CameraUniforms.java:81-88).
 CameraPositionTracker tracker;
 tracker.update(2000.0, 0.0, 0.0);
 EXPECT_DOUBLE_EQ(tracker.current(0), 2000.0);
 // |value| 29500 <= 30000 and |value - prev| = 31500 > 1000, but the computed
 // delta is -( -29500 - (-29500) ) = 0: no shift, the raw position is kept.
 tracker.update(-29500.0, 0.0, 0.0);
 EXPECT_DOUBLE_EQ(tracker.current(0), -29500.0);
 tracker.update(500.0, 0.0, 0.0);
 EXPECT_DOUBLE_EQ(tracker.current(0), 500.0);
}
TEST(CameraPositionTracker, ExactRangeBoundaryShifts) {
 // At value == +-30000 exactly the walk check (|value| > 30000) does not fire but
 // the teleport check does, and value % 30000 == 0 → a full 30000-block shift lands
 // the camera position on zero, exactly like Java.
 CameraPositionTracker tracker;
 tracker.update(2000.0, 0.0, 0.0);
 tracker.update(30000.0, 0.0, 0.0);
 EXPECT_DOUBLE_EQ(tracker.current(0), 0.0);
 // The -30000 shift persists: the next in-range position renders shifted.
 tracker.update(100.0, 0.0, 0.0);
 EXPECT_DOUBLE_EQ(tracker.current(0), -29900.0);
}
TEST(CameraPositionTracker, UnshiftedPositionsStayExact) {
 CameraPositionTracker tracker;
 tracker.update(60002.5, 82.53274536132812, -514.0264282226562);
 EXPECT_DOUBLE_EQ(tracker.currentUnshifted(0), 60002.5);
 EXPECT_DOUBLE_EQ(tracker.currentUnshifted(1), 82.53274536132812);
 EXPECT_DOUBLE_EQ(tracker.currentUnshifted(2), -514.0264282226562);
}
} // namespace
} // namespace net::minecraft::test
