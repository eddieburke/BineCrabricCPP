#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"
#include "net/minecraft/block/entity/SignBlockEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/packet/BlockPackets.hpp"
#include "net/minecraft/network/packet/ChatPackets.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
#include "net/minecraft/network/packet/InventoryPackets.hpp"
#include "net/minecraft/network/packet/PlayerPackets.hpp"
#include "net/minecraft/screen/slot/Slot.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/PlayerManager.hpp"
#include "net/minecraft/server/ServerLog.hpp"
#include "net/minecraft/util/CharacterUtils.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
namespace net::minecraft::server::network {
namespace {
constexpr int kClientCommandPressShift = 1;
constexpr int kClientCommandReleaseShift = 2;
constexpr int kClientCommandStopSleeping = 3;
} // namespace
ServerPlayNetworkHandler::ServerPlayNetworkHandler(MinecraftServer* server, Connection* connection,
                                                   ::net::minecraft::entity::player::ServerPlayerEntity* player)
    : server_(server), connection_(connection), player_(player) {
  if(player_ != nullptr) {
    player_->networkHandler = this;
    teleportTargetX_ = player_->x;
    teleportTargetY_ = player_->y;
    teleportTargetZ_ = player_->z;
  }
  if(connection_ != nullptr) {
    connection_->setNetworkHandler(*this);
  }
}
void ServerPlayNetworkHandler::tick() {
  moved_ = false;
  if(connection_ != nullptr) {
    connection_->tick();
  }
  if(player_ != nullptr && server_ != nullptr) {
    server_->playerManager.sendPendingChunks(player_);
  }
  if(ticks_ - lastKeepAliveTime_ > 20) {
    sendPacket(KeepAlivePacket{});
    lastKeepAliveTime_ = ticks_;
  }
  ++ticks_;
}
void ServerPlayNetworkHandler::setPlayer(::net::minecraft::entity::player::ServerPlayerEntity* player) {
  player_ = player;
  if(player_ != nullptr) {
    player_->networkHandler = this;
    teleportTargetX_ = player_->x;
    teleportTargetY_ = player_->y;
    teleportTargetZ_ = player_->z;
  }
}
std::size_t ServerPlayNetworkHandler::getBlockDataSendQueueSize() const {
  if(connection_ == nullptr) {
    return 0;
  }
  return connection_->getDelayedSendQueueSize();
}
void ServerPlayNetworkHandler::disconnect(const std::string& reason) {
  if(player_ != nullptr) {
    player_->onDisconnect();
  }
  if(connection_ != nullptr) {
    DisconnectPacket packet;
    packet.reason = reason;
    sendPacket(packet);
    connection_->disconnect();
  }
  if(player_ != nullptr && server_ != nullptr) {
    ChatMessagePacket leftPacket;
    leftPacket.chatMessage = std::string("\xC2\xA7"
                                         "e") +
                             player_->name + " left the game.";
    server_->playerManager.sendToAll(leftPacket);
    server_->playerManager.disconnect(player_);
  }
  disconnected = true;
}
void ServerPlayNetworkHandler::sendMessage(const std::string& message) {
  ChatMessagePacket packet;
  packet.chatMessage = std::string("\xC2\xA7"
                                   "7") +
                       message;
  sendPacket(packet);
}
std::string ServerPlayNetworkHandler::getName() {
  return player_ != nullptr ? player_->name : std::string{};
}
void ServerPlayNetworkHandler::onPlayerInput(const PlayerInputC2SPacket& packet) {
  if(player_ == nullptr) {
    return;
  }
  player_->updateInput(packet.sideways, packet.forward, packet.jumping, packet.sneaking, packet.pitch, packet.yaw);
}
void ServerPlayNetworkHandler::handleClientCommand(const ClientCommandC2SPacket& packet) {
  if(player_ == nullptr) {
    return;
  }
  if(packet.mode == kClientCommandPressShift) {
    player_->setSneaking(true);
  } else if(packet.mode == kClientCommandReleaseShift) {
    player_->setSneaking(false);
  } else if(packet.mode == kClientCommandStopSleeping) {
    player_->wakeUp(false, true, true);
    teleported_ = false;
  }
}
void ServerPlayNetworkHandler::teleport(double x, double y, double z, float yaw, float pitch) {
  if(player_ == nullptr) {
    return;
  }
  teleported_ = false;
  teleportTargetX_ = x;
  teleportTargetY_ = y;
  teleportTargetZ_ = z;
  player_->setPositionAndAngles(x, y, z, yaw, pitch);
  PlayerMoveFullPacket packet;
  packet.setMove(x, y, y + static_cast<double>(player_->getEyeHeight()), z, yaw, pitch, false);
  sendPacket(packet);
}
void ServerPlayNetworkHandler::onPlayerMove(const PlayerMovePacket& packet) {
  if(player_ == nullptr || server_ == nullptr) {
    return;
  }
  ServerWorld* serverWorld = server_->getWorld(player_->dimensionId);
  if(serverWorld == nullptr) {
    return;
  }
  moved_ = true;
  const double startY = player_->y;
  if(!teleported_) {
    const double deltaToTargetY = packet.feetY - teleportTargetY_;
    if(packet.x == teleportTargetX_ && deltaToTargetY * deltaToTargetY < 0.01 && packet.z == teleportTargetZ_) {
      teleported_ = true;
    }
  }
  if(!teleported_) {
    return;
  }
  if(player_->vehicle != nullptr) {
    float yaw = player_->yaw;
    float pitch = player_->pitch;
    player_->vehicle->updatePassengerPosition();
    const double savedX = player_->x;
    const double savedY = player_->y;
    const double savedZ = player_->z;
    double moveX = 0.0;
    double moveZ = 0.0;
    if(packet.changeLook) {
      yaw = packet.yaw;
      pitch = packet.pitch;
    }
    if(packet.changePosition && packet.feetY == -999.0 && packet.stance == -999.0) {
      moveX = packet.x;
      moveZ = packet.z;
    }
    player_->onGround = packet.onGround;
    player_->playerTick(true);
    player_->move(moveX, 0.0, moveZ);
    player_->setPositionAndAngles(savedX, savedY, savedZ, yaw, pitch);
    player_->velocityX = moveX;
    player_->velocityZ = moveZ;
    if(player_->vehicle != nullptr) {
      serverWorld->tickVehicle(player_->vehicle, true);
      player_->vehicle->updatePassengerPosition();
    }
    if(server_ != nullptr) {
      server_->playerManager.updatePlayerChunks(player_);
    }
    teleportTargetX_ = player_->x;
    teleportTargetY_ = player_->y;
    teleportTargetZ_ = player_->z;
    serverWorld->updateEntity(player_, true);
    return;
  }
  if(player_->isSleeping()) {
    player_->playerTick(true);
    player_->setPositionAndAngles(teleportTargetX_, teleportTargetY_, teleportTargetZ_, player_->yaw,
                                  player_->pitch);
    serverWorld->updateEntity(player_, true);
    return;
  }
  teleportTargetX_ = player_->x;
  teleportTargetY_ = player_->y;
  teleportTargetZ_ = player_->z;
  double targetX = player_->x;
  double targetY = player_->y;
  double targetZ = player_->z;
  float targetYaw = player_->yaw;
  float targetPitch = player_->pitch;
  PlayerMovePacket workingPacket = packet;
  if(workingPacket.changePosition && workingPacket.feetY == -999.0 && workingPacket.stance == -999.0) {
    workingPacket.changePosition = false;
  }
  if(workingPacket.changePosition) {
    targetX = workingPacket.x;
    targetY = workingPacket.feetY;
    targetZ = workingPacket.z;
    const double stanceDelta = workingPacket.stance - workingPacket.feetY;
    if(!player_->isSleeping() && (stanceDelta > 1.65 || stanceDelta < 0.1)) {
      disconnect("Illegal stance");
      return;
    }
    if(std::abs(workingPacket.x) > 3.2E7 || std::abs(workingPacket.z) > 3.2E7) {
      disconnect("Illegal position");
      return;
    }
  }
  if(workingPacket.changeLook) {
    targetYaw = workingPacket.yaw;
    targetPitch = workingPacket.pitch;
  }
  player_->playerTick(true);
  player_->cameraOffset = 0.0f;
  player_->setPositionAndAngles(teleportTargetX_, teleportTargetY_, teleportTargetZ_, targetYaw, targetPitch);
  double deltaX = targetX - player_->x;
  double deltaY = targetY - player_->y;
  double deltaZ = targetZ - player_->z;
  const double distanceSq = deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
  if(distanceSq > 100.0) {
    disconnect("You moved too quickly :( (Hacking?)");
    return;
  }
  constexpr float collisionMargin = 0.0625f;
  const bool hadSpaceBeforeMove =
      serverWorld->getEntityCollisions(player_, player_->boundingBox.contract(collisionMargin)).empty();
  player_->move(deltaX, deltaY, deltaZ);
  deltaX = targetX - player_->x;
  deltaY = targetY - player_->y;
  if(deltaY > -0.5 || deltaY < 0.5) {
    deltaY = 0.0;
  }
  deltaZ = targetZ - player_->z;
  const double afterMoveDistanceSq = deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
  bool movedWrongly = false;
  if(afterMoveDistanceSq > 0.0625 && !player_->isSleeping()) {
    movedWrongly = true;
  }
  player_->setPositionAndAngles(targetX, targetY, targetZ, targetYaw, targetPitch);
  const bool hasSpaceAfterMove =
      serverWorld->getEntityCollisions(player_, player_->boundingBox.contract(collisionMargin)).empty();
  if(hadSpaceBeforeMove && (movedWrongly || !hasSpaceAfterMove) && !player_->isSleeping()) {
    teleport(teleportTargetX_, teleportTargetY_, teleportTargetZ_, targetYaw, targetPitch);
    return;
  }
  // Floating/flying detection: Java keys off whether a real block sits below the
  // player (isAnyBlockInBox over the feet box stretched 0.55 down), NOT the
  // client-reported onGround flag. Using onGround here false-kicked grounded
  // players whose client transiently reported onGround=false.
  const Box floatBox =
      player_->boundingBox.expand(collisionMargin, collisionMargin, collisionMargin).stretch(0.0, -0.55, 0.0);
  if(!server_->flightEnabled && !serverWorld->isAnyBlockInBox(floatBox)) {
    if(deltaY >= -0.03125) {
      ++floatingTime_;
      if(floatingTime_ > 80) {
        disconnect("Flying is not enabled on this server");
        return;
      }
    }
  } else {
    floatingTime_ = 0;
  }
  player_->onGround = packet.onGround;
  if(server_ != nullptr) {
    server_->playerManager.updatePlayerChunks(player_);
  }
  player_->handleFall(player_->y - startY, packet.onGround);
}
void ServerPlayNetworkHandler::handlePlayerAction(const PlayerActionC2SPacket& packet) {
  if(player_ == nullptr || server_ == nullptr) {
    return;
  }
  ServerWorld* serverWorld = server_->getWorld(player_->dimensionId);
  if(serverWorld == nullptr) {
    return;
  }
  if(packet.action == 4) {
    player_->dropSelectedItem();
    return;
  }
  serverWorld->bypassSpawnProtection =
      serverWorld->dimension == nullptr || serverWorld->dimension->id != 0 || server_->playerManager.isOperator(player_->name);
  const bool bypassSpawnProtection = serverWorld->bypassSpawnProtection;
  bool checkDistance = packet.action == 0 || packet.action == 2;
  if(checkDistance) {
    const double dx = player_->x - (static_cast<double>(packet.x) + 0.5);
    const double dy = player_->y - (static_cast<double>(packet.y) + 0.5);
    const double dz = player_->z - (static_cast<double>(packet.z) + 0.5);
    if(dx * dx + dy * dy + dz * dz > 36.0) {
      serverWorld->bypassSpawnProtection = false;
      return;
    }
  }
  const Vec3i spawnPos = serverWorld->getSpawnPos();
  int spawnDistance = std::abs(packet.x - spawnPos.x);
  if(std::abs(packet.z - spawnPos.z) > spawnDistance) {
    spawnDistance = std::abs(packet.z - spawnPos.z);
  }
  if(packet.action == 0) {
    if(spawnDistance > 16 || bypassSpawnProtection) {
      player_->interactionManager.onBlockBreakingAction(packet.x, packet.y, packet.z, packet.direction);
    } else if(player_->networkHandler != nullptr) {
      BlockUpdateS2CPacket blockPacket;
      blockPacket.x = packet.x;
      blockPacket.y = packet.y;
      blockPacket.z = packet.z;
      blockPacket.blockRawId = serverWorld->getBlockId(packet.x, packet.y, packet.z);
      blockPacket.blockMetadata = serverWorld->getBlockMeta(packet.x, packet.y, packet.z);
      player_->networkHandler->sendPacket(blockPacket);
    }
  } else if(packet.action == 2) {
    player_->interactionManager.continueMining(packet.x, packet.y, packet.z);
    if(serverWorld->getBlockId(packet.x, packet.y, packet.z) != 0 && player_->networkHandler != nullptr) {
      BlockUpdateS2CPacket blockPacket;
      blockPacket.x = packet.x;
      blockPacket.y = packet.y;
      blockPacket.z = packet.z;
      blockPacket.blockRawId = serverWorld->getBlockId(packet.x, packet.y, packet.z);
      blockPacket.blockMetadata = serverWorld->getBlockMeta(packet.x, packet.y, packet.z);
      player_->networkHandler->sendPacket(blockPacket);
    }
  } else if(packet.action == 3) {
    const double dx = player_->x - (static_cast<double>(packet.x) + 0.5);
    const double dy = player_->y - (static_cast<double>(packet.y) + 0.5);
    const double dz = player_->z - (static_cast<double>(packet.z) + 0.5);
    if(dx * dx + dy * dy + dz * dz < 256.0 && player_->networkHandler != nullptr) {
      BlockUpdateS2CPacket blockPacket;
      blockPacket.x = packet.x;
      blockPacket.y = packet.y;
      blockPacket.z = packet.z;
      blockPacket.blockRawId = serverWorld->getBlockId(packet.x, packet.y, packet.z);
      blockPacket.blockMetadata = serverWorld->getBlockMeta(packet.x, packet.y, packet.z);
      player_->networkHandler->sendPacket(blockPacket);
    }
  }
  serverWorld->bypassSpawnProtection = false;
}
void ServerPlayNetworkHandler::onPlayerInteractBlock(const PlayerInteractBlockC2SPacket& packet) {
  if(player_ == nullptr || server_ == nullptr) {
    return;
  }
  ServerWorld* serverWorld = server_->getWorld(player_->dimensionId);
  if(serverWorld == nullptr) {
    return;
  }
  ItemStack* itemStack = player_->inventory.getSelectedItem();
  serverWorld->bypassSpawnProtection =
      serverWorld->dimension == nullptr || serverWorld->dimension->id != 0 || server_->playerManager.isOperator(player_->name);
  const bool bypassSpawnProtection = serverWorld->bypassSpawnProtection;
  if(packet.side == 255) {
    if(itemStack == nullptr || itemStack->empty()) {
      serverWorld->bypassSpawnProtection = false;
      return;
    }
    player_->interactionManager.interactItem(player_, serverWorld, itemStack);
  } else {
    int spawnDistance = std::abs(packet.x - serverWorld->getSpawnPos().x);
    if(std::abs(packet.z - serverWorld->getSpawnPos().z) > spawnDistance) {
      spawnDistance = std::abs(packet.z - serverWorld->getSpawnPos().z);
    }
    if(teleported_ && player_->getSquaredDistance(static_cast<double>(packet.x) + 0.5, static_cast<double>(packet.y) + 0.5, static_cast<double>(packet.z) + 0.5) < 64.0 &&
       (spawnDistance > 16 || bypassSpawnProtection)) {
      player_->interactionManager.interactBlock(player_, serverWorld, itemStack, packet.x, packet.y, packet.z, packet.side);
    }
    if(player_->networkHandler != nullptr) {
      BlockUpdateS2CPacket blockPacket;
      blockPacket.x = packet.x;
      blockPacket.y = packet.y;
      blockPacket.z = packet.z;
      blockPacket.blockRawId = serverWorld->getBlockId(packet.x, packet.y, packet.z);
      blockPacket.blockMetadata = serverWorld->getBlockMeta(packet.x, packet.y, packet.z);
      player_->networkHandler->sendPacket(blockPacket);
    }
    int neighborX = packet.x;
    int neighborY = packet.y;
    int neighborZ = packet.z;
    if(packet.side == 0) {
      --neighborY;
    } else if(packet.side == 1) {
      ++neighborY;
    } else if(packet.side == 2) {
      --neighborZ;
    } else if(packet.side == 3) {
      ++neighborZ;
    } else if(packet.side == 4) {
      --neighborX;
    } else if(packet.side == 5) {
      ++neighborX;
    }
    if(player_->networkHandler != nullptr) {
      BlockUpdateS2CPacket neighborPacket;
      neighborPacket.x = neighborX;
      neighborPacket.y = neighborY;
      neighborPacket.z = neighborZ;
      neighborPacket.blockRawId = serverWorld->getBlockId(neighborX, neighborY, neighborZ);
      neighborPacket.blockMetadata = serverWorld->getBlockMeta(neighborX, neighborY, neighborZ);
      player_->networkHandler->sendPacket(neighborPacket);
    }
  }
  itemStack = player_->inventory.getSelectedItem();
  if(itemStack != nullptr && itemStack->count == 0) {
    player_->inventory.main[static_cast<std::size_t>(player_->inventory.selectedSlot)] = ItemStack{};
  }
  player_->skipPacketSlotUpdates = true;
  player_->inventory.main[static_cast<std::size_t>(player_->inventory.selectedSlot)] =
      player_->inventory.main[static_cast<std::size_t>(player_->inventory.selectedSlot)].copy();
  if(player_->currentScreenHandler != nullptr) {
    screen::slot::Slot* slot =
        player_->currentScreenHandler->getSlot(&player_->inventory, player_->inventory.selectedSlot);
    player_->currentScreenHandler->sendContentUpdates();
    player_->skipPacketSlotUpdates = false;
    ItemStack* selected = player_->inventory.getSelectedItem();
    if(slot != nullptr && player_->networkHandler != nullptr &&
       !ItemStack::areEqual(selected != nullptr ? *selected : ItemStack{}, packet.stack)) {
      ScreenHandlerSlotUpdateS2CPacket slotPacket;
      slotPacket.syncId = player_->currentScreenHandler->syncId;
      slotPacket.slot = slot->id;
      slotPacket.stack = selected != nullptr ? *selected : ItemStack{};
      player_->networkHandler->sendPacket(slotPacket);
    }
  } else {
    player_->skipPacketSlotUpdates = false;
  }
  serverWorld->bypassSpawnProtection = false;
}
void ServerPlayNetworkHandler::onDisconnected(const std::string& reason, const std::vector<std::string>& args) {
  (void)args;
  if(player_ != nullptr) {
    ServerLog::LOGGER.info(player_->name + " lost connection: " + reason);
  }
  if(player_ != nullptr && server_ != nullptr) {
    ChatMessagePacket leftPacket;
    leftPacket.chatMessage = std::string("\xC2\xA7"
                                         "e") +
                             player_->name + " left the game.";
    server_->playerManager.sendToAll(leftPacket);
    server_->playerManager.disconnect(player_);
  }
  disconnected = true;
}
void ServerPlayNetworkHandler::onKeepAlive(const KeepAlivePacket&) {
  lastKeepAliveTime_ = ticks_;
}
void ServerPlayNetworkHandler::handle(const Packet&) {
  ServerLog::LOGGER.log(LogLevel::Warning, "ServerPlayNetworkHandler wasn't prepared to deal with a packet");
  disconnect("Protocol error, unexpected packet");
}
void ServerPlayNetworkHandler::onUpdateSelectedSlot(const UpdateSelectedSlotC2SPacket& packet) {
  if(player_ == nullptr) {
    return;
  }
  if(packet.selectedSlot < 0 || packet.selectedSlot > PlayerInventory::getHotbarSize()) {
    ServerLog::LOGGER.log(LogLevel::Warning, player_->name + " tried to set an invalid carried item");
    return;
  }
  player_->inventory.selectedSlot = packet.selectedSlot;
}
void ServerPlayNetworkHandler::onChatMessage(const ChatMessagePacket& packet) {
  if(player_ == nullptr || server_ == nullptr) {
    return;
  }
  std::string message = packet.chatMessage;
  if(message.size() > 100) {
    disconnect("Chat message too long");
    return;
  }
  const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
  message.erase(message.begin(), std::find_if(message.begin(), message.end(), notSpace));
  message.erase(std::find_if(message.rbegin(), message.rend(), notSpace).base(), message.end());
  const std::string& valid = CharacterUtils::validCharacters();
  for(char c : message) {
    if(valid.find(c) == std::string::npos) {
      disconnect("Illegal characters in chat");
      return;
    }
  }
  if(!message.empty() && message.front() == '/') {
    handleCommand(message);
    return;
  }
  message = "<" + player_->name + "> " + message;
  ServerLog::LOGGER.info(message);
  ChatMessagePacket broadcastPacket;
  broadcastPacket.chatMessage = message;
  server_->playerManager.sendToAll(broadcastPacket);
}
void ServerPlayNetworkHandler::handleCommand(const std::string& message) {
  if(player_ == nullptr || server_ == nullptr) {
    return;
  }
  std::string lowered = message;
  std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if(lowered.rfind("/me ", 0) == 0) {
    std::string action = "* " + player_->name + " " + message.substr(message.find(' ') + 1);
    const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    action.erase(action.begin(), std::find_if(action.begin(), action.end(), notSpace));
    action.erase(std::find_if(action.rbegin(), action.rend(), notSpace).base(), action.end());
    ServerLog::LOGGER.info(action);
    ChatMessagePacket packet;
    packet.chatMessage = action;
    server_->playerManager.sendToAll(packet);
    return;
  }
  if(lowered.rfind("/kill", 0) == 0) {
    player_->damage(nullptr, 1000);
    return;
  }
  if(lowered.rfind("/tell ", 0) == 0) {
    std::istringstream stream(message);
    std::string command;
    std::string target;
    stream >> command >> target;
    if(!target.empty()) {
      std::string whisper = message.substr(message.find(' '));
      whisper = whisper.substr(whisper.find(' '));
      const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
      whisper.erase(whisper.begin(), std::find_if(whisper.begin(), whisper.end(), notSpace));
      whisper.erase(std::find_if(whisper.rbegin(), whisper.rend(), notSpace).base(), whisper.end());
      whisper = std::string("\xC2\xA7"
                            "7") +
                player_->name + " whispers " + whisper;
      ServerLog::LOGGER.info(whisper + " to " + target);
      ChatMessagePacket whisperPacket;
      whisperPacket.chatMessage = whisper;
      if(!server_->playerManager.sendPacket(target, whisperPacket)) {
        ChatMessagePacket errorPacket;
        errorPacket.chatMessage = "\xC2\xA7"
                                  "cThere's no player by that name online.";
        sendPacket(errorPacket);
      }
    }
    return;
  }
  if(server_->playerManager.isOperator(player_->name)) {
    ServerLog::LOGGER.info(player_->name + " issued server command: " + message.substr(1));
    server_->queueCommands(message.substr(1), *this);
  } else {
    ServerLog::LOGGER.info(player_->name + " tried command: " + message.substr(1));
  }
}
void ServerPlayNetworkHandler::onEntityAnimation(const EntityAnimationPacket& packet) {
  if(player_ != nullptr && packet.animationId == 1) {
    player_->swingHand();
  }
}
void ServerPlayNetworkHandler::onDisconnect(const DisconnectPacket& packet) {
  (void)packet;
  if(connection_ != nullptr) {
    connection_->disconnect("disconnect.quitting", {});
  }
}
void ServerPlayNetworkHandler::handleInteractEntity(const PlayerInteractEntityC2SPacket& packet) {
  if(player_ == nullptr || server_ == nullptr) {
    return;
  }
  ServerWorld* serverWorld = server_->getWorld(player_->dimensionId);
  if(serverWorld == nullptr) {
    return;
  }
  Entity* entity = serverWorld->getEntity(packet.entityId);
  if(entity != nullptr && player_->canSee(entity) && player_->getSquaredDistance(*entity) < 36.0) {
    if(packet.isLeftClick == 0) {
      player_->interact(entity);
    } else if(packet.isLeftClick == 1) {
      player_->attack(entity);
    }
  }
}
void ServerPlayNetworkHandler::onPlayerRespawn(const PlayerRespawnPacket& packet) {
  (void)packet;
  if(player_ == nullptr || server_ == nullptr || player_->health > 0) {
    return;
  }
  player_ = server_->playerManager.respawnPlayer(player_, 0);
}
void ServerPlayNetworkHandler::onCloseScreen(const CloseScreenS2CPacket& packet) {
  (void)packet;
  if(player_ != nullptr) {
    player_->onHandledScreenClosed();
  }
}
void ServerPlayNetworkHandler::onClickSlot(const ClickSlotC2SPacket& packet) {
  if(player_ == nullptr || player_->currentScreenHandler == nullptr) {
    return;
  }
  if(player_->currentScreenHandler->syncId != packet.syncId || !player_->currentScreenHandler->canOpen(player_)) {
    return;
  }
  ItemStack result = player_->currentScreenHandler->onSlotClick(packet.slot, packet.button, packet.holdingShift, player_);
  if(ItemStack::areEqual(packet.stack, result)) {
    if(player_->networkHandler != nullptr) {
      ScreenHandlerAcknowledgementPacket ack(packet.syncId, packet.actionType, true);
      player_->networkHandler->sendPacket(ack);
    }
    player_->skipPacketSlotUpdates = true;
    player_->currentScreenHandler->sendContentUpdates();
    player_->updateCursorStack();
    player_->skipPacketSlotUpdates = false;
  } else if(player_->networkHandler != nullptr) {
    transactions_[player_->currentScreenHandler->syncId] = packet.actionType;
    ScreenHandlerAcknowledgementPacket ack(packet.syncId, packet.actionType, false);
    player_->networkHandler->sendPacket(ack);
    player_->currentScreenHandler->updatePlayerList(player_, false);
    std::vector<ItemStack> stacks;
    stacks.reserve(player_->currentScreenHandler->slots.size());
    for(screen::slot::Slot* slot : player_->currentScreenHandler->slots) {
      stacks.push_back(slot != nullptr ? slot->getStack() : ItemStack{});
    }
    player_->onContentsUpdate(*player_->currentScreenHandler, stacks);
  }
}
void ServerPlayNetworkHandler::onScreenHandlerAcknowledgement(const ScreenHandlerAcknowledgementPacket& packet) {
  if(player_ == nullptr || player_->currentScreenHandler == nullptr) {
    return;
  }
  const auto found = transactions_.find(player_->currentScreenHandler->syncId);
  if(found != transactions_.end() && packet.actionType == found->second &&
     player_->currentScreenHandler->syncId == packet.syncId && !player_->currentScreenHandler->canOpen(player_)) {
    player_->currentScreenHandler->updatePlayerList(player_, true);
  }
}
void ServerPlayNetworkHandler::handleUpdateSign(const UpdateSignPacket& packet) {
  if(player_ == nullptr || server_ == nullptr) {
    return;
  }
  ServerWorld* serverWorld = server_->getWorld(player_->dimensionId);
  if(serverWorld == nullptr || !serverWorld->isPosLoaded(packet.x, packet.y, packet.z)) {
    return;
  }
  block::entity::BlockEntity* blockEntity = serverWorld->getBlockEntity(packet.x, packet.y, packet.z);
  auto* signBlockEntity = dynamic_cast<block::entity::SignBlockEntity*>(blockEntity);
  if(signBlockEntity != nullptr && !signBlockEntity->isEditable()) {
    server_->warn("Player " + player_->name + " just tried to change non-editable sign");
    return;
  }
  UpdateSignPacket sanitized = packet;
  const std::string& valid = CharacterUtils::validCharacters();
  for(std::size_t line = 0; line < sanitized.text.size(); ++line) {
    bool validLine = true;
    if(sanitized.text[line].size() > 15) {
      validLine = false;
    } else {
      for(char c : sanitized.text[line]) {
        if(valid.find(c) == std::string::npos) {
          validLine = false;
          break;
        }
      }
    }
    if(!validLine) {
      sanitized.text[line] = "!?";
    }
  }
  if(signBlockEntity != nullptr) {
    for(std::size_t line = 0; line < sanitized.text.size(); ++line) {
      signBlockEntity->texts[line] = sanitized.text[line];
    }
    signBlockEntity->setEditable(false);
    signBlockEntity->markDirty();
    serverWorld->blockUpdateEvent(packet.x, packet.y, packet.z);
  }
}
} // namespace net::minecraft::server::network
