/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.network;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.InetAddress;
import java.net.Socket;
import java.net.URL;
import java.util.List;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.entity.DispenserBlockEntity;
import net.minecraft.block.entity.FurnaceBlockEntity;
import net.minecraft.block.entity.SignBlockEntity;
import net.minecraft.client.Minecraft;
import net.minecraft.client.MultiplayerInteractionManager;
import net.minecraft.client.gui.screen.DisconnectedScreen;
import net.minecraft.client.gui.screen.DownloadingTerrainScreen;
import net.minecraft.client.network.MultiplayerClientPlayerEntity;
import net.minecraft.client.network.OtherPlayerEntity;
import net.minecraft.client.particle.PickupParticle;
import net.minecraft.entity.Entity;
import net.minecraft.entity.EntityRegistry;
import net.minecraft.entity.FallingBlockEntity;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.LightningEntity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.TntEntity;
import net.minecraft.entity.decoration.painting.PaintingEntity;
import net.minecraft.entity.player.ClientPlayerEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.projectile.ArrowEntity;
import net.minecraft.entity.projectile.FireballEntity;
import net.minecraft.entity.projectile.FishingBobberEntity;
import net.minecraft.entity.projectile.thrown.EggEntity;
import net.minecraft.entity.projectile.thrown.SnowballEntity;
import net.minecraft.entity.vehicle.BoatEntity;
import net.minecraft.entity.vehicle.MinecartEntity;
import net.minecraft.inventory.SimpleInventory;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.item.MapItem;
import net.minecraft.network.Connection;
import net.minecraft.network.NetworkHandler;
import net.minecraft.network.packet.Packet;
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
import net.minecraft.screen.ScreenHandler;
import net.minecraft.stat.Stats;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.ClientWorld;
import net.minecraft.world.chunk.Chunk;
import net.minecraft.world.explosion.Explosion;
import net.minecraft.world.storage.SavedDataStorage;

@Environment(value=EnvType.CLIENT)
public class ClientNetworkHandler
extends NetworkHandler {
    private boolean disconnected = false;
    private Connection connection;
    public String message;
    private Minecraft minecraft;
    private ClientWorld world;
    private boolean started = false;
    public SavedDataStorage clientPersistentStateManager = new SavedDataStorage(null);
    Random random = new Random();

    public ClientNetworkHandler(Minecraft minecraft, String address, int port) {
        this.minecraft = minecraft;
        Socket socket = new Socket(InetAddress.getByName(address), port);
        this.connection = new Connection(socket, "Client", this);
    }

    public void tick() {
        if (!this.disconnected) {
            this.connection.tick();
        }
        this.connection.interrupt();
    }

    public void onHello(LoginHelloPacket packet) {
        this.minecraft.interactionManager = new MultiplayerInteractionManager(this.minecraft, this);
        this.minecraft.stats.increment(Stats.JOIN_MULTIPLAYER, 1);
        this.world = new ClientWorld(this, packet.worldSeed, packet.dimensionId);
        this.world.isRemote = true;
        this.minecraft.setWorld(this.world);
        this.minecraft.player.dimensionId = packet.dimensionId;
        this.minecraft.setScreen(new DownloadingTerrainScreen(this));
        this.minecraft.player.id = packet.protocolVersion;
    }

    public void onItemEntitySpawn(ItemEntitySpawnS2CPacket packet) {
        double d = (double)packet.x / 32.0;
        double d2 = (double)packet.y / 32.0;
        double d3 = (double)packet.z / 32.0;
        ItemEntity itemEntity = new ItemEntity(this.world, d, d2, d3, new ItemStack(packet.itemRawId, packet.itemCount, packet.itemDamage));
        itemEntity.velocityX = (double)packet.velocityX / 128.0;
        itemEntity.velocityY = (double)packet.velocityY / 128.0;
        itemEntity.velocityZ = (double)packet.velocityZ / 128.0;
        itemEntity.trackedPosX = packet.x;
        itemEntity.trackedPosY = packet.y;
        itemEntity.trackedPosZ = packet.z;
        this.world.forceEntity(packet.id, itemEntity);
    }

    public void onEntitySpawn(EntitySpawnS2CPacket packet) {
        double d = (double)packet.x / 32.0;
        double d2 = (double)packet.y / 32.0;
        double d3 = (double)packet.z / 32.0;
        Entity entity = null;
        if (packet.entityType == 10) {
            entity = new MinecartEntity(this.world, d, d2, d3, 0);
        }
        if (packet.entityType == 11) {
            entity = new MinecartEntity(this.world, d, d2, d3, 1);
        }
        if (packet.entityType == 12) {
            entity = new MinecartEntity(this.world, d, d2, d3, 2);
        }
        if (packet.entityType == 90) {
            entity = new FishingBobberEntity(this.world, d, d2, d3);
        }
        if (packet.entityType == 60) {
            entity = new ArrowEntity(this.world, d, d2, d3);
        }
        if (packet.entityType == 61) {
            entity = new SnowballEntity(this.world, d, d2, d3);
        }
        if (packet.entityType == 63) {
            entity = new FireballEntity(this.world, d, d2, d3, (double)packet.velocityX / 8000.0, (double)packet.velocityY / 8000.0, (double)packet.velocityZ / 8000.0);
            packet.entityData = 0;
        }
        if (packet.entityType == 62) {
            entity = new EggEntity(this.world, d, d2, d3);
        }
        if (packet.entityType == 1) {
            entity = new BoatEntity(this.world, d, d2, d3);
        }
        if (packet.entityType == 50) {
            entity = new TntEntity(this.world, d, d2, d3);
        }
        if (packet.entityType == 70) {
            entity = new FallingBlockEntity(this.world, d, d2, d3, Block.SAND.id);
        }
        if (packet.entityType == 71) {
            entity = new FallingBlockEntity(this.world, d, d2, d3, Block.GRAVEL.id);
        }
        if (entity != null) {
            entity.trackedPosX = packet.x;
            entity.trackedPosY = packet.y;
            entity.trackedPosZ = packet.z;
            entity.yaw = 0.0f;
            entity.pitch = 0.0f;
            entity.id = packet.id;
            this.world.forceEntity(packet.id, entity);
            if (packet.entityData > 0) {
                Entity entity2;
                if (packet.entityType == 60 && (entity2 = this.getEntity(packet.entityData)) instanceof LivingEntity) {
                    ((ArrowEntity)entity).owner = (LivingEntity)entity2;
                }
                entity.setVelocityClient((double)packet.velocityX / 8000.0, (double)packet.velocityY / 8000.0, (double)packet.velocityZ / 8000.0);
            }
        }
    }

    public void onLightningEntitySpawn(GlobalEntitySpawnS2CPacket packet) {
        double d = (double)packet.x / 32.0;
        double d2 = (double)packet.y / 32.0;
        double d3 = (double)packet.z / 32.0;
        LightningEntity lightningEntity = null;
        if (packet.type == 1) {
            lightningEntity = new LightningEntity(this.world, d, d2, d3);
        }
        if (lightningEntity != null) {
            lightningEntity.trackedPosX = packet.x;
            lightningEntity.trackedPosY = packet.y;
            lightningEntity.trackedPosZ = packet.z;
            lightningEntity.yaw = 0.0f;
            lightningEntity.pitch = 0.0f;
            lightningEntity.id = packet.id;
            this.world.spawnGlobalEntity(lightningEntity);
        }
    }

    public void onPaintingEntitySpawn(PaintingEntitySpawnS2CPacket packet) {
        PaintingEntity paintingEntity = new PaintingEntity(this.world, packet.x, packet.y, packet.z, packet.facing, packet.variant);
        this.world.forceEntity(packet.id, paintingEntity);
    }

    public void onEntityVelocityUpdate(EntityVelocityUpdateS2CPacket packet) {
        Entity entity = this.getEntity(packet.id);
        if (entity == null) {
            return;
        }
        entity.setVelocityClient((double)packet.velocityX / 8000.0, (double)packet.velocityY / 8000.0, (double)packet.velocityZ / 8000.0);
    }

    public void onEntityTrackerUpdate(EntityTrackerUpdateS2CPacket packet) {
        Entity entity = this.getEntity(packet.id);
        if (entity != null && packet.getTrackedValues() != null) {
            entity.getDataTracker().writeUpdatedEntries(packet.getTrackedValues());
        }
    }

    public void onPlayerSpawn(PlayerSpawnS2CPacket packet) {
        double d = (double)packet.x / 32.0;
        double d2 = (double)packet.y / 32.0;
        double d3 = (double)packet.z / 32.0;
        float f = (float)(packet.yaw * 360) / 256.0f;
        float f2 = (float)(packet.pitch * 360) / 256.0f;
        OtherPlayerEntity otherPlayerEntity = new OtherPlayerEntity(this.minecraft.world, packet.name);
        otherPlayerEntity.trackedPosX = packet.x;
        otherPlayerEntity.prevX = otherPlayerEntity.lastTickX = (double)otherPlayerEntity.trackedPosX;
        otherPlayerEntity.trackedPosY = packet.y;
        otherPlayerEntity.prevY = otherPlayerEntity.lastTickY = (double)otherPlayerEntity.trackedPosY;
        otherPlayerEntity.trackedPosZ = packet.z;
        otherPlayerEntity.prevZ = otherPlayerEntity.lastTickZ = (double)otherPlayerEntity.trackedPosZ;
        int n = packet.itemRawId;
        otherPlayerEntity.inventory.main[otherPlayerEntity.inventory.selectedSlot] = n == 0 ? null : new ItemStack(n, 1, 0);
        otherPlayerEntity.setPositionAndAngles(d, d2, d3, f, f2);
        this.world.forceEntity(packet.id, otherPlayerEntity);
    }

    public void onEntityPosition(EntityPositionS2CPacket packet) {
        Entity entity = this.getEntity(packet.id);
        if (entity == null) {
            return;
        }
        entity.trackedPosX = packet.x;
        entity.trackedPosY = packet.y;
        entity.trackedPosZ = packet.z;
        double d = (double)entity.trackedPosX / 32.0;
        double d2 = (double)entity.trackedPosY / 32.0 + 0.015625;
        double d3 = (double)entity.trackedPosZ / 32.0;
        float f = (float)(packet.yaw * 360) / 256.0f;
        float f2 = (float)(packet.pitch * 360) / 256.0f;
        entity.setPositionAndAnglesAvoidEntities(d, d2, d3, f, f2, 3);
    }

    public void onEntity(EntityS2CPacket packet) {
        Entity entity = this.getEntity(packet.id);
        if (entity == null) {
            return;
        }
        entity.trackedPosX += packet.deltaX;
        entity.trackedPosY += packet.deltaY;
        entity.trackedPosZ += packet.deltaZ;
        double d = (double)entity.trackedPosX / 32.0;
        double d2 = (double)entity.trackedPosY / 32.0;
        double d3 = (double)entity.trackedPosZ / 32.0;
        float f = packet.rotate ? (float)(packet.yaw * 360) / 256.0f : entity.yaw;
        float f2 = packet.rotate ? (float)(packet.pitch * 360) / 256.0f : entity.pitch;
        entity.setPositionAndAnglesAvoidEntities(d, d2, d3, f, f2, 3);
    }

    public void onEntityDestroy(EntityDestroyS2CPacket packet) {
        this.world.removeEntity(packet.id);
    }

    public void onPlayerMove(PlayerMovePacket packet) {
        ClientPlayerEntity clientPlayerEntity = this.minecraft.player;
        double d = clientPlayerEntity.x;
        double d2 = clientPlayerEntity.y;
        double d3 = clientPlayerEntity.z;
        float f = clientPlayerEntity.yaw;
        float f2 = clientPlayerEntity.pitch;
        if (packet.changePosition) {
            d = packet.x;
            d2 = packet.y;
            d3 = packet.z;
        }
        if (packet.changeLook) {
            f = packet.yaw;
            f2 = packet.pitch;
        }
        clientPlayerEntity.cameraOffset = 0.0f;
        clientPlayerEntity.velocityZ = 0.0;
        clientPlayerEntity.velocityY = 0.0;
        clientPlayerEntity.velocityX = 0.0;
        clientPlayerEntity.setPositionAndAngles(d, d2, d3, f, f2);
        packet.x = clientPlayerEntity.x;
        packet.y = clientPlayerEntity.boundingBox.minY;
        packet.z = clientPlayerEntity.z;
        packet.eyeHeight = clientPlayerEntity.y;
        this.connection.sendPacket(packet);
        if (!this.started) {
            this.minecraft.player.prevX = this.minecraft.player.x;
            this.minecraft.player.prevY = this.minecraft.player.y;
            this.minecraft.player.prevZ = this.minecraft.player.z;
            this.started = true;
            this.minecraft.setScreen(null);
        }
    }

    public void onChunkStatusUpdate(ChunkStatusUpdateS2CPacket packet) {
        this.world.updateChunk(packet.x, packet.z, packet.load);
    }

    public void onChunkDeltaUpdate(ChunkDeltaUpdateS2CPacket packet) {
        Chunk chunk = this.world.getChunk(packet.x, packet.z);
        int n = packet.x * 16;
        int n2 = packet.z * 16;
        for (int i = 0; i < packet.size; ++i) {
            short s = packet.positions[i];
            int n3 = packet.blockRawIds[i] & 0xFF;
            byte by = packet.blockMetadata[i];
            int n4 = s >> 12 & 0xF;
            int n5 = s >> 8 & 0xF;
            int n6 = s & 0xFF;
            chunk.setBlock(n4, n6, n5, n3, by);
            this.world.clearBlockResets(n4 + n, n6, n5 + n2, n4 + n, n6, n5 + n2);
            this.world.setBlocksDirty(n4 + n, n6, n5 + n2, n4 + n, n6, n5 + n2);
        }
    }

    public void handleChunkData(ChunkDataS2CPacket packet) {
        this.world.clearBlockResets(packet.x, packet.y, packet.z, packet.x + packet.sizeX - 1, packet.y + packet.sizeY - 1, packet.z + packet.sizeZ - 1);
        this.world.handleChunkDataUpdate(packet.x, packet.y, packet.z, packet.sizeX, packet.sizeY, packet.sizeZ, packet.chunkData);
    }

    public void onBlockUpdate(BlockUpdateS2CPacket packet) {
        this.world.setBlockWithMetaFromPacket(packet.x, packet.y, packet.z, packet.blockRawId, packet.blockMetadata);
    }

    public void onDisconnect(DisconnectPacket packet) {
        this.connection.disconnect("disconnect.kicked", new Object[0]);
        this.disconnected = true;
        this.minecraft.setWorld(null);
        this.minecraft.setScreen(new DisconnectedScreen("disconnect.disconnected", "disconnect.genericReason", packet.reason));
    }

    public void onDisconnected(String reason, Object[] objects) {
        if (this.disconnected) {
            return;
        }
        this.disconnected = true;
        this.minecraft.setWorld(null);
        this.minecraft.setScreen(new DisconnectedScreen("disconnect.lost", reason, objects));
    }

    public void sendPacketAndDisconnect(Packet packet) {
        if (this.disconnected) {
            return;
        }
        this.connection.sendPacket(packet);
        this.connection.disconnect();
    }

    public void sendPacket(Packet packet) {
        if (this.disconnected) {
            return;
        }
        this.connection.sendPacket(packet);
    }

    public void onItemPickupAnimation(ItemPickupAnimationS2CPacket packet) {
        Entity entity = this.getEntity(packet.entityId);
        LivingEntity livingEntity = (LivingEntity)this.getEntity(packet.collectorEntityId);
        if (livingEntity == null) {
            livingEntity = this.minecraft.player;
        }
        if (entity != null) {
            this.world.playSound(entity, "random.pop", 0.2f, ((this.random.nextFloat() - this.random.nextFloat()) * 0.7f + 1.0f) * 2.0f);
            this.minecraft.particleManager.addParticle(new PickupParticle(this.minecraft.world, entity, livingEntity, -0.5f));
            this.world.removeEntity(packet.entityId);
        }
    }

    public void onChatMessage(ChatMessagePacket packet) {
        this.minecraft.inGameHud.addChatMessage(packet.chatMessage);
    }

    public void onEntityAnimation(EntityAnimationPacket packet) {
        Entity entity = this.getEntity(packet.id);
        if (entity == null) {
            return;
        }
        if (packet.animationId == 1) {
            PlayerEntity playerEntity = (PlayerEntity)entity;
            playerEntity.swingHand();
        } else if (packet.animationId == 2) {
            entity.animateHurt();
        } else if (packet.animationId == 3) {
            PlayerEntity playerEntity = (PlayerEntity)entity;
            playerEntity.wakeUp(false, false, false);
        } else if (packet.animationId == 4) {
            PlayerEntity playerEntity = (PlayerEntity)entity;
            playerEntity.spawn();
        }
    }

    public void onPlayerSleepUpdate(PlayerSleepUpdateS2CPacket packet) {
        Entity entity = this.getEntity(packet.id);
        if (entity == null) {
            return;
        }
        if (packet.status == 0) {
            PlayerEntity playerEntity = (PlayerEntity)entity;
            playerEntity.trySleep(packet.x, packet.y, packet.z);
        }
    }

    public void onHandshake(HandshakePacket packet) {
        if (packet.name.equals("-")) {
            this.sendPacket(new LoginHelloPacket(this.minecraft.session.username, 14));
        } else {
            try {
                URL uRL = new URL("http://www.minecraft.net/game/joinserver.jsp?user=" + this.minecraft.session.username + "&sessionId=" + this.minecraft.session.sessionId + "&serverId=" + packet.name);
                BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(uRL.openStream()));
                String string = bufferedReader.readLine();
                bufferedReader.close();
                if (string.equalsIgnoreCase("ok")) {
                    this.sendPacket(new LoginHelloPacket(this.minecraft.session.username, 14));
                } else {
                    this.connection.disconnect("disconnect.loginFailedInfo", string);
                }
            }
            catch (Exception exception) {
                exception.printStackTrace();
                this.connection.disconnect("disconnect.genericReason", "Internal client error: " + exception.toString());
            }
        }
    }

    public void disconnect() {
        this.disconnected = true;
        this.connection.interrupt();
        this.connection.disconnect("disconnect.closed", new Object[0]);
    }

    public void onLivingEntitySpawn(LivingEntitySpawnS2CPacket packet) {
        double d = (double)packet.x / 32.0;
        double d2 = (double)packet.y / 32.0;
        double d3 = (double)packet.z / 32.0;
        float f = (float)(packet.yaw * 360) / 256.0f;
        float f2 = (float)(packet.pitch * 360) / 256.0f;
        LivingEntity livingEntity = (LivingEntity)EntityRegistry.create(packet.entityType, this.minecraft.world);
        livingEntity.trackedPosX = packet.x;
        livingEntity.trackedPosY = packet.y;
        livingEntity.trackedPosZ = packet.z;
        livingEntity.id = packet.id;
        livingEntity.setPositionAndAngles(d, d2, d3, f, f2);
        livingEntity.interpolateOnly = true;
        this.world.forceEntity(packet.id, livingEntity);
        List list = packet.getTrackedValues();
        if (list != null) {
            livingEntity.getDataTracker().writeUpdatedEntries(list);
        }
    }

    public void onWorldTimeUpdate(WorldTimeUpdateS2CPacket packet) {
        this.minecraft.world.setTime(packet.time);
    }

    public void onPlayerSpawnPosition(PlayerSpawnPositionS2CPacket packet) {
        this.minecraft.player.setSpawnPos(new Vec3i(packet.x, packet.y, packet.z));
        this.minecraft.world.getProperties().setSpawn(packet.x, packet.y, packet.z);
    }

    public void onEntityVehicleSet(EntityVehicleSetS2CPacket packet) {
        Entity entity = this.getEntity(packet.id);
        Entity entity2 = this.getEntity(packet.vehicleId);
        if (packet.id == this.minecraft.player.id) {
            entity = this.minecraft.player;
        }
        if (entity == null) {
            return;
        }
        entity.setVehicle(entity2);
    }

    public void onEntityStatus(EntityStatusS2CPacket packet) {
        Entity entity = this.getEntity(packet.id);
        if (entity != null) {
            entity.processServerEntityStatus(packet.status);
        }
    }

    private Entity getEntity(int id) {
        if (id == this.minecraft.player.id) {
            return this.minecraft.player;
        }
        return this.world.getEntity(id);
    }

    public void onHealthUpdate(HealthUpdateS2CPacket packet) {
        this.minecraft.player.damageTo(packet.health);
    }

    public void onPlayerRespawn(PlayerRespawnPacket packet) {
        if (packet.dimensionRawId != this.minecraft.player.dimensionId) {
            this.started = false;
            this.world = new ClientWorld(this, this.world.getProperties().getSeed(), packet.dimensionRawId);
            this.world.isRemote = true;
            this.minecraft.setWorld(this.world);
            this.minecraft.player.dimensionId = packet.dimensionRawId;
            this.minecraft.setScreen(new DownloadingTerrainScreen(this));
        }
        this.minecraft.respawnPlayer(true, packet.dimensionRawId);
    }

    public void onExplosion(ExplosionS2CPacket packet) {
        Explosion explosion = new Explosion(this.minecraft.world, null, packet.x, packet.y, packet.z, packet.radius);
        explosion.damagedBlocks = packet.affectedBlocks;
        explosion.playExplosionSound(true);
    }

    public void onOpenScreen(OpenScreenS2CPacket packet) {
        if (packet.screenHandlerId == 0) {
            SimpleInventory simpleInventory = new SimpleInventory(packet.name, packet.size);
            this.minecraft.player.openChestScreen(simpleInventory);
            this.minecraft.player.currentScreenHandler.syncId = packet.syncId;
        } else if (packet.screenHandlerId == 2) {
            FurnaceBlockEntity furnaceBlockEntity = new FurnaceBlockEntity();
            this.minecraft.player.openFurnaceScreen(furnaceBlockEntity);
            this.minecraft.player.currentScreenHandler.syncId = packet.syncId;
        } else if (packet.screenHandlerId == 3) {
            DispenserBlockEntity dispenserBlockEntity = new DispenserBlockEntity();
            this.minecraft.player.openDispenserScreen(dispenserBlockEntity);
            this.minecraft.player.currentScreenHandler.syncId = packet.syncId;
        } else if (packet.screenHandlerId == 1) {
            ClientPlayerEntity clientPlayerEntity = this.minecraft.player;
            this.minecraft.player.openCraftingScreen(MathHelper.floor(clientPlayerEntity.x), MathHelper.floor(clientPlayerEntity.y), MathHelper.floor(clientPlayerEntity.z));
            this.minecraft.player.currentScreenHandler.syncId = packet.syncId;
        }
    }

    public void onScreenHandlerSlotUpdate(ScreenHandlerSlotUpdateS2CPacket packet) {
        if (packet.syncId == -1) {
            this.minecraft.player.inventory.setCursorStack(packet.stack);
        } else if (packet.syncId == 0 && packet.slot >= 36 && packet.slot < 45) {
            ItemStack itemStack = this.minecraft.player.playerScreenHandler.getSlot(packet.slot).getStack();
            if (packet.stack != null && (itemStack == null || itemStack.count < packet.stack.count)) {
                packet.stack.bobbingAnimationTime = 5;
            }
            this.minecraft.player.playerScreenHandler.setStackInSlot(packet.slot, packet.stack);
        } else if (packet.syncId == this.minecraft.player.currentScreenHandler.syncId) {
            this.minecraft.player.currentScreenHandler.setStackInSlot(packet.slot, packet.stack);
        }
    }

    public void onScreenHandlerAcknowledgement(ScreenHandlerAcknowledgementPacket packet) {
        ScreenHandler screenHandler = null;
        if (packet.syncId == 0) {
            screenHandler = this.minecraft.player.playerScreenHandler;
        } else if (packet.syncId == this.minecraft.player.currentScreenHandler.syncId) {
            screenHandler = this.minecraft.player.currentScreenHandler;
        }
        if (screenHandler != null) {
            if (packet.accepted) {
                screenHandler.onAcknowledgementAccepted(packet.actionType);
            } else {
                screenHandler.onAcknowledgementDenied(packet.actionType);
                this.sendPacket(new ScreenHandlerAcknowledgementPacket(packet.syncId, packet.actionType, true));
            }
        }
    }

    public void onInventory(InventoryS2CPacket packet) {
        if (packet.syncId == 0) {
            this.minecraft.player.playerScreenHandler.updateSlotStacks(packet.contents);
        } else if (packet.syncId == this.minecraft.player.currentScreenHandler.syncId) {
            this.minecraft.player.currentScreenHandler.updateSlotStacks(packet.contents);
        }
    }

    public void handleUpdateSign(UpdateSignPacket packet) {
        BlockEntity blockEntity;
        if (this.minecraft.world.isPosLoaded(packet.x, packet.y, packet.z) && (blockEntity = this.minecraft.world.getBlockEntity(packet.x, packet.y, packet.z)) instanceof SignBlockEntity) {
            SignBlockEntity signBlockEntity = (SignBlockEntity)blockEntity;
            for (int i = 0; i < 4; ++i) {
                signBlockEntity.texts[i] = packet.text[i];
            }
            signBlockEntity.markDirty();
        }
    }

    public void onScreenHandlerPropertyUpdate(ScreenHandlerPropertyUpdateS2CPacket packet) {
        this.handle(packet);
        if (this.minecraft.player.currentScreenHandler != null && this.minecraft.player.currentScreenHandler.syncId == packet.syncId) {
            this.minecraft.player.currentScreenHandler.setProperty(packet.propertyId, packet.value);
        }
    }

    public void onEntityEquipmentUpdate(EntityEquipmentUpdateS2CPacket packet) {
        Entity entity = this.getEntity(packet.id);
        if (entity != null) {
            entity.setEquipmentStack(packet.slot, packet.itemRawId, packet.itemDamage);
        }
    }

    public void onCloseScreen(CloseScreenS2CPacket packet) {
        this.minecraft.player.closeHandledScreen();
    }

    public void onPlayNoteSound(PlayNoteSoundS2CPacket packet) {
        this.minecraft.world.playNoteBlockActionAt(packet.x, packet.y, packet.z, packet.instrument, packet.pitch);
    }

    public void onGameStateChange(GameStateChangeS2CPacket packet) {
        int n = packet.reason;
        if (n >= 0 && n < GameStateChangeS2CPacket.REASONS.length && GameStateChangeS2CPacket.REASONS[n] != null) {
            this.minecraft.player.sendMessage(GameStateChangeS2CPacket.REASONS[n]);
        }
        if (n == 1) {
            this.world.getProperties().setRaining(true);
            this.world.setRainGradient(1.0f);
        } else if (n == 2) {
            this.world.getProperties().setRaining(false);
            this.world.setRainGradient(0.0f);
        }
    }

    public void onMapUpdate(MapUpdateS2CPacket packet) {
        if (packet.itemRawId == Item.MAP.id) {
            MapItem.getMapState(packet.id, this.minecraft.world).readUpdateData(packet.updateData);
        } else {
            System.out.println("Unknown itemid: " + packet.id);
        }
    }

    public void onWorldEvent(WorldEventS2CPacket packet) {
        this.minecraft.world.worldEvent(packet.eventId, packet.x, packet.y, packet.z, packet.data);
    }

    public void onIncreaseStat(IncreaseStatS2CPacket packet) {
        ((MultiplayerClientPlayerEntity)this.minecraft.player).handleIncreaseStat(Stats.getStatById(packet.statId), packet.amount);
    }

    public boolean isServerSide() {
        return false;
    }
}

