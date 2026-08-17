#include <gtest/gtest.h>
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/input/Keys.hpp"
namespace {
namespace input = net::minecraft::client::input;
namespace keys = net::minecraft::client::input::keys;
} // namespace
TEST(ShiftClick, LeftShiftAloneMarksTheClick) {
 input::InputSystem& system = input::InputSystem::instance();
 system.pushKeyEvent(keys::kLShift, true);
 EXPECT_TRUE(system.isKeyDown(keys::kLShift));
 system.pushKeyEvent(keys::kLShift, false);
 EXPECT_FALSE(system.isKeyDown(keys::kLShift));
}
TEST(ShiftClick, RightShiftAloneMarksTheClick) {
 input::InputSystem& system = input::InputSystem::instance();
 system.pushKeyEvent(keys::kRShift, true);
 EXPECT_TRUE(system.isKeyDown(keys::kRShift));
 system.pushKeyEvent(keys::kRShift, false);
 EXPECT_FALSE(system.isKeyDown(keys::kRShift));
}
