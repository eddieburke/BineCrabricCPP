#include "net/minecraft/network/packet/PacketRegistry.hpp"

#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/packet/Packets.hpp"

#include <mutex>

namespace net::minecraft {

void Packet::ensureRegistered()
{
    PacketRegistry::bootstrap();
}

void PacketRegistry::bootstrap()
{
    static std::once_flag once;
    std::call_once(once, []() {
        Packet::registerPacket<KeepAlivePacket>(0, true, true);
        Packet::registerPacket<LoginHelloPacket>(1, true, true);
        Packet::registerPacket<HandshakePacket>(2, true, true);
        Packet::registerPacket<ChatMessagePacket>(3, true, true);
        Packet::registerPacket<WorldTimeUpdateS2CPacket>(4, true, false);
        Packet::registerPacket<EntityEquipmentUpdateS2CPacket>(5, true, false);
        Packet::registerPacket<PlayerSpawnPositionS2CPacket>(6, true, false);
        Packet::registerPacket<PlayerInteractEntityC2SPacket>(7, false, true);
        Packet::registerPacket<HealthUpdateS2CPacket>(8, true, false);
        Packet::registerPacket<PlayerRespawnPacket>(9, true, true);
        Packet::registerPacket<PlayerMovePacket>(10, true, true);
        Packet::registerPacket<PlayerMovePositionAndOnGroundPacket>(11, true, true);
        Packet::registerPacket<PlayerMoveLookAndOnGroundPacket>(12, true, true);
        Packet::registerPacket<PlayerMoveFullPacket>(13, true, true);
        Packet::registerPacket<PlayerActionC2SPacket>(14, false, true);
        Packet::registerPacket<PlayerInteractBlockC2SPacket>(15, false, true);
        Packet::registerPacket<UpdateSelectedSlotC2SPacket>(16, false, true);
        Packet::registerPacket<PlayerSleepUpdateS2CPacket>(17, true, false);
        Packet::registerPacket<EntityAnimationPacket>(18, true, true);
        Packet::registerPacket<ClientCommandC2SPacket>(19, false, true);
        Packet::registerPacket<PlayerSpawnS2CPacket>(20, true, false);
        Packet::registerPacket<ItemEntitySpawnS2CPacket>(21, true, false);
        Packet::registerPacket<ItemPickupAnimationS2CPacket>(22, true, false);
        Packet::registerPacket<EntitySpawnS2CPacket>(23, true, false);
        Packet::registerPacket<LivingEntitySpawnS2CPacket>(24, true, false);
        Packet::registerPacket<PaintingEntitySpawnS2CPacket>(25, true, false);
        Packet::registerPacket<PlayerInputC2SPacket>(27, false, true);
        Packet::registerPacket<EntityVelocityUpdateS2CPacket>(28, true, false);
        Packet::registerPacket<EntityDestroyS2CPacket>(29, true, false);
        Packet::registerPacket<EntityS2CPacket>(30, true, false);
        Packet::registerPacket<EntityMoveRelativeS2CPacket>(31, true, false);
        Packet::registerPacket<EntityRotateS2CPacket>(32, true, false);
        Packet::registerPacket<EntityRotateAndMoveRelativeS2CPacket>(33, true, false);
        Packet::registerPacket<EntityPositionS2CPacket>(34, true, false);
        Packet::registerPacket<EntityStatusS2CPacket>(38, true, false);
        Packet::registerPacket<EntityVehicleSetS2CPacket>(39, true, false);
        Packet::registerPacket<EntityTrackerUpdateS2CPacket>(40, true, false);
        Packet::registerPacket<ChunkStatusUpdateS2CPacket>(50, true, false);
        Packet::registerPacket<ChunkDataS2CPacket>(51, true, false);
        Packet::registerPacket<ChunkDeltaUpdateS2CPacket>(52, true, false);
        Packet::registerPacket<BlockUpdateS2CPacket>(53, true, false);
        Packet::registerPacket<PlayNoteSoundS2CPacket>(54, true, false);
        Packet::registerPacket<ExplosionS2CPacket>(60, true, false);
        Packet::registerPacket<WorldEventS2CPacket>(61, true, false);
        Packet::registerPacket<GameStateChangeS2CPacket>(70, true, false);
        Packet::registerPacket<GlobalEntitySpawnS2CPacket>(71, true, false);
        Packet::registerPacket<OpenScreenS2CPacket>(100, true, false);
        Packet::registerPacket<CloseScreenS2CPacket>(101, true, true);
        Packet::registerPacket<ClickSlotC2SPacket>(102, false, true);
        Packet::registerPacket<ScreenHandlerSlotUpdateS2CPacket>(103, true, false);
        Packet::registerPacket<InventoryS2CPacket>(104, true, false);
        Packet::registerPacket<ScreenHandlerPropertyUpdateS2CPacket>(105, true, false);
        Packet::registerPacket<ScreenHandlerAcknowledgementPacket>(106, true, true);
        Packet::registerPacket<UpdateSignPacket>(130, true, true);
        Packet::registerPacket<MapUpdateS2CPacket>(131, true, false);
        Packet::registerPacket<IncreaseStatS2CPacket>(200, true, false);
        Packet::registerPacket<DisconnectPacket>(255, true, true);
    });
}

} // namespace net::minecraft
