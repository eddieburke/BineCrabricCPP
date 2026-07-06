// ClientNetworkHandler packet handlers for screen handlers / inventory sync: open and
// close screens, window items, slot updates, properties and transaction acknowledgement.
// Split out of ClientNetworkHandler.cpp for separation of concerns; see
// ClientNetworkHandlerInternal.hpp.
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandlerInternal.hpp"
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/inventory/SimpleInventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include <fstream>
#include <memory>
namespace net::minecraft::client::multiplayer {
void ClientNetworkHandler::onOpenScreen(const OpenScreenS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->player == nullptr) {
    return;
  }
  auto* player = minecraft->player;
  if(packet.screenHandlerId == 0) {
    openScreenInventory_ =
        std::make_unique<SimpleInventory>(packet.name, static_cast<std::size_t>(packet.inventorySize));
    player->openChestScreen(openScreenInventory_.get());
    player->currentScreenHandler->syncId = packet.syncId;
  } else if(packet.screenHandlerId == 2) {
    openScreenFurnace_ = std::make_unique<block::entity::FurnaceBlockEntity>();
    player->openFurnaceScreen(openScreenFurnace_.get());
    player->currentScreenHandler->syncId = packet.syncId;
  } else if(packet.screenHandlerId == 3) {
    openScreenDispenser_ = std::make_unique<block::entity::DispenserBlockEntity>();
    player->openDispenserScreen(openScreenDispenser_.get());
    player->currentScreenHandler->syncId = packet.syncId;
  } else if(packet.screenHandlerId == 1) {
    player->openCraftingScreen(MathHelper::floor(player->x), MathHelper::floor(player->y),
                               MathHelper::floor(player->z));
    player->currentScreenHandler->syncId = packet.syncId;
  }
}
void ClientNetworkHandler::onScreenHandlerSlotUpdate(const ScreenHandlerSlotUpdateS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->player == nullptr) {
    return;
  }
  auto* player = minecraft->player;
  if(packet.syncId == -1) {
    player->inventory.setCursorStack(packet.stack);
    return;
  }
  if(packet.syncId == 0 && packet.slot >= 36 && packet.slot < 45) {
    if(screen::slot::Slot* slot = player->playerScreenHandler.getSlot(packet.slot)) {
      ItemStack existing = slot->getStack();
      ItemStack incoming = packet.stack;
      if(!incoming.empty() && (existing.empty() || existing.count < incoming.count)) {
        incoming.bobbingAnimationTime = 5;
      }
      player->playerScreenHandler.setStackInSlot(packet.slot, incoming);
    }
    return;
  }
  if(player->currentScreenHandler != nullptr && packet.syncId == player->currentScreenHandler->syncId) {
    player->currentScreenHandler->setStackInSlot(packet.slot, packet.stack);
  }
}
void ClientNetworkHandler::onScreenHandlerAcknowledgement(const ScreenHandlerAcknowledgementPacket& packet) {
  if(minecraft == nullptr || minecraft->player == nullptr) {
    return;
  }
  screen::ScreenHandler* screenHandler = nullptr;
  if(packet.syncId == 0) {
    screenHandler = &minecraft->player->playerScreenHandler;
  } else if(minecraft->player->currentScreenHandler != nullptr &&
            packet.syncId == minecraft->player->currentScreenHandler->syncId) {
    screenHandler = minecraft->player->currentScreenHandler;
  }
  if(screenHandler == nullptr) {
    return;
  }
  if(packet.accepted) {
    screenHandler->onAcknowledgementAccepted(static_cast<std::uint16_t>(packet.actionType));
  } else {
    screenHandler->onAcknowledgementDenied(static_cast<std::uint16_t>(packet.actionType));
    sendPacket(ScreenHandlerAcknowledgementPacket{packet.syncId, packet.actionType, true});
  }
}
void ClientNetworkHandler::onInventory(const InventoryS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->player == nullptr) {
    return;
  }
  auto* player = minecraft->player;
  if(packet.syncId == 0) {
    player->playerScreenHandler.updateSlotStacks(packet.contents);
  } else if(player->currentScreenHandler != nullptr && packet.syncId == player->currentScreenHandler->syncId) {
    player->currentScreenHandler->updateSlotStacks(packet.contents);
  }
}
void ClientNetworkHandler::onScreenHandlerPropertyUpdate(const ScreenHandlerPropertyUpdateS2CPacket& packet) {
  if(minecraft == nullptr || minecraft->player == nullptr || minecraft->player->currentScreenHandler == nullptr) {
    return;
  }
  if(minecraft->player->currentScreenHandler->syncId == packet.syncId) {
    minecraft->player->currentScreenHandler->setProperty(packet.propertyId, packet.value);
  }
}
void ClientNetworkHandler::onCloseScreen(const CloseScreenS2CPacket& /*packet*/) {
  if(minecraft == nullptr || minecraft->player == nullptr) {
    return;
  }
  if(auto* multiplayerPlayer = dynamic_cast<MultiplayerClientPlayerEntity*>(minecraft->player)) {
    multiplayerPlayer->closeHandledScreen();
  } else {
    minecraft->player->closeHandledScreen();
  }
  openScreenInventory_.reset();
  openScreenFurnace_.reset();
  openScreenDispenser_.reset();
}
} // namespace net::minecraft::client::multiplayer
