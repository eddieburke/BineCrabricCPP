#include <gtest/gtest.h>
#include "net/minecraft/mod/HookBus.hpp"
namespace net::minecraft::test {
namespace {
struct OwnedHookEvent {
  int value = 0;
};
struct PermanentHookEvent {
  int value = 0;
};
} // namespace
TEST(HookBus, RemovesOnlyListenersForOwner) {
  int firstOwner = 0;
  int secondOwner = 0;
  mod::HookBus bus;
  bus.subscribeOwned<OwnedHookEvent>(&firstOwner, 0, [](OwnedHookEvent& event) { event.value += 1; });
  bus.subscribeOwned<OwnedHookEvent>(&secondOwner, 0, [](OwnedHookEvent& event) { event.value += 10; });
  bus.unsubscribeOwner(&firstOwner);
  OwnedHookEvent event;
  bus.publish(event);
  EXPECT_EQ(event.value, 10);
  bus.unsubscribeOwner(&secondOwner);
}
TEST(HookBus, PreservesUnownedListeners) {
  int owner = 0;
  mod::HookBus bus;
  bus.subscribe<PermanentHookEvent>(0, [](PermanentHookEvent& event) { ++event.value; });
  bus.unsubscribeOwner(&owner);
  PermanentHookEvent event;
  bus.publish(event);
  EXPECT_EQ(event.value, 1);
}
} // namespace net::minecraft::test
