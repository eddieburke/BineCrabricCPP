/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.network;

import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.c2s.play.ClickSlotC2SPacket;
import net.minecraft.network.packet.c2s.play.ClientCommandC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerActionC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInputC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractBlockC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractEntityC2SPacket;
import net.minecraft.network.packet.c2s.play.UpdateSelectedSlotC2SPacket;
import net.minecraft.network.packet.handshake.HandshakePacket;
import net.minecraft.network.packet.login.LoginHelloPacket;
import net.minecraft.network.packet.play.ChatMessagePacket;
import net.minecraft.network.packet.play.DisconnectPacket;
import net.minecraft.network.packet.play.EntityAnimationPacket;
import net.minecraft.network.packet.play.PlayerMovePacket;
import net.minecraft.network.packet.play.PlayerRespawnPacket;
import net.minecraft.network.packet.play.ScreenHandlerAcknowledgementPacket;
import net.minecraft.network.packet.play.UpdateSignPacket;
import net.minecraft.network.packet.s2c.play.BlockUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ChunkDataS2CPacket;
import net.minecraft.network.packet.s2c.play.ChunkDeltaUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ChunkStatusUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.CloseScreenS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityDestroyS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityEquipmentUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityPositionS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityS2CPacket;
import net.minecraft.network.packet.s2c.play.EntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityStatusS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityTrackerUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityVehicleSetS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityVelocityUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ExplosionS2CPacket;
import net.minecraft.network.packet.s2c.play.GameStateChangeS2CPacket;
import net.minecraft.network.packet.s2c.play.GlobalEntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.HealthUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.IncreaseStatS2CPacket;
import net.minecraft.network.packet.s2c.play.InventoryS2CPacket;
import net.minecraft.network.packet.s2c.play.ItemEntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.ItemPickupAnimationS2CPacket;
import net.minecraft.network.packet.s2c.play.LivingEntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.MapUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.OpenScreenS2CPacket;
import net.minecraft.network.packet.s2c.play.PaintingEntitySpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayNoteSoundS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayerSleepUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayerSpawnPositionS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayerSpawnS2CPacket;
import net.minecraft.network.packet.s2c.play.ScreenHandlerPropertyUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ScreenHandlerSlotUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.WorldEventS2CPacket;
import net.minecraft.network.packet.s2c.play.WorldTimeUpdateS2CPacket;

public abstract class NetworkHandler {
    public abstract boolean isServerSide();

    public void handleChunkData(ChunkDataS2CPacket packet) {
    }

    public void handle(Packet packet) {
    }

    public void onDisconnected(String reason, Object[] objects) {
    }

    public void onDisconnect(DisconnectPacket packet) {
        this.handle(packet);
    }

    public void onHello(LoginHelloPacket packet) {
        this.handle(packet);
    }

    public void onPlayerMove(PlayerMovePacket packet) {
        this.handle(packet);
    }

    public void onChunkDeltaUpdate(ChunkDeltaUpdateS2CPacket packet) {
        this.handle(packet);
    }

    public void handlePlayerAction(PlayerActionC2SPacket packet) {
        this.handle(packet);
    }

    public void onBlockUpdate(BlockUpdateS2CPacket packet) {
        this.handle(packet);
    }

    public void onChunkStatusUpdate(ChunkStatusUpdateS2CPacket packet) {
        this.handle(packet);
    }

    public void onPlayerSpawn(PlayerSpawnS2CPacket packet) {
        this.handle(packet);
    }

    public void onEntity(EntityS2CPacket packet) {
        this.handle(packet);
    }

    public void onEntityPosition(EntityPositionS2CPacket packet) {
        this.handle(packet);
    }

    public void onPlayerInteractBlock(PlayerInteractBlockC2SPacket packet) {
        this.handle(packet);
    }

    public void onUpdateSelectedSlot(UpdateSelectedSlotC2SPacket packet) {
        this.handle(packet);
    }

    public void onEntityDestroy(EntityDestroyS2CPacket packet) {
        this.handle(packet);
    }

    public void onItemEntitySpawn(ItemEntitySpawnS2CPacket packet) {
        this.handle(packet);
    }

    public void onItemPickupAnimation(ItemPickupAnimationS2CPacket packet) {
        this.handle(packet);
    }

    public void onChatMessage(ChatMessagePacket packet) {
        this.handle(packet);
    }

    public void onEntitySpawn(EntitySpawnS2CPacket packet) {
        this.handle(packet);
    }

    public void onEntityAnimation(EntityAnimationPacket packet) {
        this.handle(packet);
    }

    public void handleClientCommand(ClientCommandC2SPacket packet) {
        this.handle(packet);
    }

    public void onHandshake(HandshakePacket packet) {
        this.handle(packet);
    }

    public void onLivingEntitySpawn(LivingEntitySpawnS2CPacket packet) {
        this.handle(packet);
    }

    public void onWorldTimeUpdate(WorldTimeUpdateS2CPacket packet) {
        this.handle(packet);
    }

    public void onPlayerSpawnPosition(PlayerSpawnPositionS2CPacket packet) {
        this.handle(packet);
    }

    public void onEntityVelocityUpdate(EntityVelocityUpdateS2CPacket packet) {
        this.handle(packet);
    }

    public void onEntityTrackerUpdate(EntityTrackerUpdateS2CPacket packet) {
        this.handle(packet);
    }

    public void onEntityVehicleSet(EntityVehicleSetS2CPacket packet) {
        this.handle(packet);
    }

    public void handleInteractEntity(PlayerInteractEntityC2SPacket packet) {
        this.handle(packet);
    }

    public void onEntityStatus(EntityStatusS2CPacket packet) {
        this.handle(packet);
    }

    public void onHealthUpdate(HealthUpdateS2CPacket packet) {
        this.handle(packet);
    }

    public void onPlayerRespawn(PlayerRespawnPacket packet) {
        this.handle(packet);
    }

    public void onExplosion(ExplosionS2CPacket packet) {
        this.handle(packet);
    }

    public void onOpenScreen(OpenScreenS2CPacket packet) {
        this.handle(packet);
    }

    public void onCloseScreen(CloseScreenS2CPacket packet) {
        this.handle(packet);
    }

    public void onClickSlot(ClickSlotC2SPacket packet) {
        this.handle(packet);
    }

    public void onScreenHandlerSlotUpdate(ScreenHandlerSlotUpdateS2CPacket packet) {
        this.handle(packet);
    }

    public void onInventory(InventoryS2CPacket packet) {
        this.handle(packet);
    }

    public void handleUpdateSign(UpdateSignPacket packet) {
        this.handle(packet);
    }

    public void onScreenHandlerPropertyUpdate(ScreenHandlerPropertyUpdateS2CPacket packet) {
        this.handle(packet);
    }

    public void onEntityEquipmentUpdate(EntityEquipmentUpdateS2CPacket packet) {
        this.handle(packet);
    }

    public void onScreenHandlerAcknowledgement(ScreenHandlerAcknowledgementPacket packet) {
        this.handle(packet);
    }

    public void onPaintingEntitySpawn(PaintingEntitySpawnS2CPacket packet) {
        this.handle(packet);
    }

    public void onPlayNoteSound(PlayNoteSoundS2CPacket packet) {
        this.handle(packet);
    }

    public void onIncreaseStat(IncreaseStatS2CPacket packet) {
        this.handle(packet);
    }

    public void onPlayerSleepUpdate(PlayerSleepUpdateS2CPacket packet) {
        this.handle(packet);
    }

    public void onPlayerInput(PlayerInputC2SPacket packet) {
        this.handle(packet);
    }

    public void onGameStateChange(GameStateChangeS2CPacket packet) {
        this.handle(packet);
    }

    public void onLightningEntitySpawn(GlobalEntitySpawnS2CPacket packet) {
        this.handle(packet);
    }

    public void onMapUpdate(MapUpdateS2CPacket packet) {
        this.handle(packet);
    }

    public void onWorldEvent(WorldEventS2CPacket packet) {
        this.handle(packet);
    }
}

