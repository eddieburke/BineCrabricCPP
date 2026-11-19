#include <array>
#include <chrono>
#include <memory>
#include <gtest/gtest.h>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/screen/ChatScreen.hpp"
#include "net/minecraft/client/gui/screen/GameMenuScreen.hpp"
#include "net/minecraft/client/gui/screen/ingame/DoubleChestScreen.hpp"
#include "net/minecraft/client/gui/screen/ingame/InventoryScreen.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/input/Keys.hpp"
#include "net/minecraft/client/platform/glfw/Window.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/inventory/SimpleInventory.hpp"
#include "net/minecraft/world/ClientWorld.hpp"

namespace net::minecraft::test {
namespace {
namespace input = client::input;
namespace screens = client::gui::screen;
using client::platform::glfw::Window;

class InputTestMinecraft final : public client::Minecraft {
 public:
 using client::Minecraft::Minecraft;
 void handleCrash(const util::crash::CrashReport&) override {}
};

class ScreenInput : public testing::Test {
 protected:
 void SetUp() override {
  Window::setActive(true);
  input.clearOnDeactivate();
  minecraft.options.optionsFile.clear();
  minecraft.world = &world;
  minecraft.player = &player;
  minecraft.focused = true;
  minecraft.interactionManager = std::make_unique<client::InteractionManager>(&minecraft);
 }

 void TearDown() override {
  minecraft.setScreen(nullptr);
  input.clearOnDeactivate();
  minecraft.player = nullptr;
  minecraft.world = nullptr;
  Window::setActive(wasActive);
 }

 void tickInput() {
  input.beginFrame(minecraft);
  if(minecraft.currentScreen() != nullptr) {
   input.drainScreenEvents(*minecraft.currentScreen());
  }
  input.pollGame(minecraft);
 }

 void setMovementKeys(bool down) {
  for(int key : {minecraft.options.forwardKey.code, minecraft.options.leftKey.code,
                 minecraft.options.jumpKey.code, minecraft.options.sneakKey.code}) {
   input.pushKeyEvent(key, down);
  }
 }

 void expectNoMovement() {
  EXPECT_FLOAT_EQ(input.movement().forward, 0.0f);
  EXPECT_FLOAT_EQ(input.movement().sideways, 0.0f);
  EXPECT_FALSE(input.movement().jumping);
  EXPECT_FALSE(input.movement().sneaking);
  EXPECT_FALSE(player.isSneaking());
 }

 const bool wasActive = Window::isActive();
 input::InputSystem& input = input::InputSystem::instance();
 InputTestMinecraft minecraft{854, 480, false};
 ClientWorld world{nullptr, 12345ULL, 0};
 entity::player::ClientPlayerEntity player{&minecraft, &world, minecraft.session, minecraft.options, 0};
};

TEST_F(ScreenInput, GameplayMovementUsesAcceptedPressesAndReleases) {
 setMovementKeys(true);
 tickInput();
 EXPECT_FLOAT_EQ(input.movement().forward, 0.3f);
 EXPECT_FLOAT_EQ(input.movement().sideways, 0.3f);
 EXPECT_TRUE(input.movement().jumping);
 EXPECT_TRUE(player.isSneaking());

 setMovementKeys(false);
 tickInput();
 expectNoMovement();
}

TEST_F(ScreenInput, ModalScreensClearHeldMovementAndBlockNewMovement) {
 SimpleInventory chest("Chest", 27);
 std::array<std::unique_ptr<screens::Screen>, 4> modalScreens{
     std::make_unique<screens::ingame::InventoryScreen>(&player.playerScreenHandler),
     std::make_unique<screens::ingame::DoubleChestScreen>(&player.inventory, &chest),
     std::make_unique<screens::ChatScreen>(),
     std::make_unique<screens::GameMenuScreen>()};
 for(auto& screen : modalScreens) {
  minecraft.setScreen(nullptr);
  input.clearOnDeactivate();
  setMovementKeys(true);
  tickInput();
  ASSERT_TRUE(player.isSneaking());

  minecraft.setScreen(std::move(screen));
  tickInput();
  expectNoMovement();
  setMovementKeys(false);
  tickInput();
  setMovementKeys(true);
  tickInput();
  expectNoMovement();
  EXPECT_TRUE(input.isKeyDown(input::keys::kLShift));
  EXPECT_TRUE(input.modifiers().shift);
 }
 minecraft.setScreen(nullptr);
}

TEST_F(ScreenInput, InventoryBlocksHotbarFogAndCameraShortcuts) {
 minecraft.setScreen(std::make_unique<screens::ingame::InventoryScreen>(&player.playerScreenHandler));
 const int viewDistance = minecraft.options.viewDistance;
 const bool thirdPerson = minecraft.options.thirdPerson;
 input.pushKeyEvent(input::keys::kHotbar9, true);
 input.pushKeyEvent(minecraft.options.fogKey.code, true);
 input.pushKeyEvent(input::keys::kF5, true);
 tickInput();

 EXPECT_EQ(player.inventory.selectedSlot, 0);
 EXPECT_EQ(minecraft.options.viewDistance, viewDistance);
 EXPECT_EQ(minecraft.options.thirdPerson, thirdPerson);
 ASSERT_NE(minecraft.currentScreen(), nullptr);
 EXPECT_FALSE(minecraft.currentScreen()->passEvents);
}

TEST_F(ScreenInput, ShiftClickTransfersItemsWithoutSneaking) {
 minecraft.setScreen(std::make_unique<screens::ingame::InventoryScreen>(&player.playerScreenHandler));
 auto* screen = minecraft.currentScreen();
 auto* slot = player.playerScreenHandler.getSlot(&player.inventory, 9);
 ASSERT_NE(slot, nullptr);
 for(int shift : {input::keys::kLShift, input::keys::kRShift}) {
  player.inventory.setStack(0, {});
  player.inventory.setStack(9, ItemStack(1, 8, 0));
  input.pushKeyEvent(shift, true);
  tickInput();
  screen->mouseClicked((screen->width() - 176) / 2 + slot->x + 8,
                       (screen->height() - 166) / 2 + slot->y + 8, 0);

  EXPECT_TRUE(player.inventory.getStack(9).empty());
  EXPECT_EQ(player.inventory.getStack(0).count, 8);
  EXPECT_TRUE(player.inventory.getCursorStack().empty());
  expectNoMovement();
  input.pushKeyEvent(shift, false);
  tickInput();
 }
}

TEST_F(ScreenInput, ClosingInventoryDoesNotReplayHeldKeysOrQueuedShortcuts) {
 minecraft.setScreen(std::make_unique<screens::ingame::InventoryScreen>(&player.playerScreenHandler));
 setMovementKeys(true);
 tickInput();
 input.pushKeyEvent(input::keys::kEscape, true);
 input.pushKeyEvent(input::keys::kHotbar9, true);
 tickInput();

 EXPECT_EQ(minecraft.currentScreen(), nullptr);
 EXPECT_EQ(player.inventory.selectedSlot, 0);
 EXPECT_TRUE(input.isKeyDown(input::keys::kLShift));
 expectNoMovement();
 tickInput();
 expectNoMovement();

 setMovementKeys(false);
 setMovementKeys(true);
 tickInput();
 EXPECT_FLOAT_EQ(input.movement().forward, 0.3f);
 EXPECT_TRUE(input.movement().jumping);
 EXPECT_TRUE(player.isSneaking());
}

TEST_F(ScreenInput, OpeningInventoryDiscardsQueuedGameplayInput) {
 const int viewDistance = minecraft.options.viewDistance;
 input.pushKeyEvent(minecraft.options.inventoryKey.code, true);
 input.pushKeyEvent(input::keys::kHotbar9, true);
 input.pushKeyEvent(minecraft.options.fogKey.code, true);
 setMovementKeys(true);
 tickInput();

 ASSERT_NE(dynamic_cast<screens::ingame::InventoryScreen*>(minecraft.currentScreen()), nullptr);
 EXPECT_EQ(player.inventory.selectedSlot, 0);
 EXPECT_EQ(minecraft.options.viewDistance, viewDistance);
 expectNoMovement();
}

class ScreenOpeningInteractionManager final : public client::InteractionManager {
 public:
 using client::InteractionManager::InteractionManager;
 bool interactItem(PlayerEntity*, World*, ItemStack*) override {
  minecraft->setScreen(std::make_unique<screens::ingame::InventoryScreen>(
      &minecraft->player->playerScreenHandler));
  return true;
 }
};

TEST_F(ScreenInput, OpeningScreenFromMouseDiscardsQueuedGameplayInput) {
 minecraft.interactionManager = std::make_unique<ScreenOpeningInteractionManager>(&minecraft);
 minecraft.lastTickTime = std::chrono::duration_cast<std::chrono::milliseconds>(
     std::chrono::system_clock::now().time_since_epoch()).count();
 player.inventory.setStack(0, ItemStack(1, 1, 0));
 input.pushMouseButtonEvent(1, true, 100, 100);
 input.pushMouseWheelEvent(-120, 100, 100);
 input.pushKeyEvent(input::keys::kHotbar9, true);
 setMovementKeys(true);
 tickInput();

 ASSERT_NE(dynamic_cast<screens::ingame::InventoryScreen*>(minecraft.currentScreen()), nullptr);
 EXPECT_EQ(player.inventory.selectedSlot, 0);
 expectNoMovement();
}

TEST_F(ScreenInput, ExplicitPassEventsStillAllowsGameplayMovement) {
 auto screen = std::make_unique<screens::Screen>();
 screen->passEvents = true;
 minecraft.setScreen(std::move(screen));
 setMovementKeys(true);
 tickInput();

 EXPECT_FLOAT_EQ(input.movement().forward, 0.3f);
 EXPECT_FLOAT_EQ(input.movement().sideways, 0.3f);
 EXPECT_TRUE(input.movement().jumping);
 EXPECT_TRUE(player.isSneaking());
}
}
}
