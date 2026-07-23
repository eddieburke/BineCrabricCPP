/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.player;

import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.entity.DispenserBlockEntity;
import net.minecraft.block.entity.FurnaceBlockEntity;
import net.minecraft.client.resource.language.TranslationStorage;
import net.minecraft.entity.Entity;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.player.SleepAttemptResult;
import net.minecraft.entity.projectile.ArrowEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.item.NetworkSyncedItem;
import net.minecraft.network.packet.Packet;
import net.minecraft.network.packet.play.ChatMessagePacket;
import net.minecraft.network.packet.play.EntityAnimationPacket;
import net.minecraft.network.packet.s2c.play.ChunkDataS2CPacket;
import net.minecraft.network.packet.s2c.play.CloseScreenS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityEquipmentUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.EntityVehicleSetS2CPacket;
import net.minecraft.network.packet.s2c.play.HealthUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.IncreaseStatS2CPacket;
import net.minecraft.network.packet.s2c.play.InventoryS2CPacket;
import net.minecraft.network.packet.s2c.play.ItemPickupAnimationS2CPacket;
import net.minecraft.network.packet.s2c.play.OpenScreenS2CPacket;
import net.minecraft.network.packet.s2c.play.PlayerSleepUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ScreenHandlerPropertyUpdateS2CPacket;
import net.minecraft.network.packet.s2c.play.ScreenHandlerSlotUpdateS2CPacket;
import net.minecraft.screen.CraftingScreenHandler;
import net.minecraft.screen.DispenserScreenHandler;
import net.minecraft.screen.FurnaceScreenHandler;
import net.minecraft.screen.GenericContainerScreenHandler;
import net.minecraft.screen.ScreenHandler;
import net.minecraft.screen.ScreenHandlerListener;
import net.minecraft.screen.slot.CraftingResultSlot;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.entity.EntityTracker;
import net.minecraft.server.network.ServerPlayNetworkHandler;
import net.minecraft.server.network.ServerPlayerInteractionManager;
import net.minecraft.stat.Stat;
import net.minecraft.util.math.ChunkPos;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.ServerWorld;
import net.minecraft.world.World;

@Environment(value=EnvType.SERVER)
public class ServerPlayerEntity
extends PlayerEntity
implements ScreenHandlerListener {
    public ServerPlayNetworkHandler networkHandler;
    public MinecraftServer server;
    public ServerPlayerInteractionManager interactionManager;
    public double lastX;
    public double lastZ;
    public List pendingChunkUpdates = new LinkedList();
    public Set activeChunks = new HashSet();
    private int lastHealthScore = -99999999;
    private int joinInvulnerabilityTicks = 60;
    private ItemStack[] equipment = new ItemStack[]{null, null, null, null, null};
    private int screenHandlerSyncId = 0;
    public boolean skipPacketSlotUpdates;

    public ServerPlayerEntity(MinecraftServer server, World world, String name, ServerPlayerInteractionManager interactionManager) {
        super(world);
        interactionManager.player = this;
        this.interactionManager = interactionManager;
        Vec3i vec3i = world.getSpawnPos();
        int n = vec3i.x;
        int n2 = vec3i.z;
        int n3 = vec3i.y;
        if (!world.dimension.hasCeiling) {
            n3 = world.getSpawnPositionValidityY(n += this.random.nextInt(20) - 10, n2);
            n2 += this.random.nextInt(20) - 10;
        }
        this.setPositionAndAnglesKeepPrevAngles((double)n + 0.5, n3, (double)n2 + 0.5, 0.0f, 0.0f);
        this.server = server;
        this.stepHeight = 0.0f;
        this.name = name;
        this.standingEyeHeight = 0.0f;
    }

    public void setWorld(World world) {
        super.setWorld(world);
        this.interactionManager = new ServerPlayerInteractionManager((ServerWorld)world);
        this.interactionManager.player = this;
    }

    public void initScreenHandler() {
        this.currentScreenHandler.addListener(this);
    }

    public ItemStack[] getEquipment() {
        return this.equipment;
    }

    protected void resetEyeHeight() {
        this.standingEyeHeight = 0.0f;
    }

    public float getEyeHeight() {
        return 1.62f;
    }

    public void tick() {
        this.interactionManager.update();
        --this.joinInvulnerabilityTicks;
        this.currentScreenHandler.sendContentUpdates();
        for (int i = 0; i < 5; ++i) {
            ItemStack itemStack = this.getEquipment(i);
            if (itemStack == this.equipment[i]) continue;
            this.server.getEntityTracker(this.dimensionId).sendToListeners(this, new EntityEquipmentUpdateS2CPacket(this.id, i, itemStack));
            this.equipment[i] = itemStack;
        }
    }

    public ItemStack getEquipment(int slot) {
        if (slot == 0) {
            return this.inventory.getSelectedItem();
        }
        return this.inventory.armor[slot - 1];
    }

    public void onKilledBy(Entity adversary) {
        this.inventory.dropInventory();
    }

    public boolean damage(Entity damageSource, int amount) {
        if (this.joinInvulnerabilityTicks > 0) {
            return false;
        }
        if (!this.server.pvpEnabled) {
            if (damageSource instanceof PlayerEntity) {
                return false;
            }
            if (damageSource instanceof ArrowEntity) {
                ArrowEntity arrowEntity = (ArrowEntity)damageSource;
                if (arrowEntity.owner instanceof PlayerEntity) {
                    return false;
                }
            }
        }
        return super.damage(damageSource, amount);
    }

    protected boolean isPvpEnabled() {
        return this.server.pvpEnabled;
    }

    public void heal(int amount) {
        super.heal(amount);
    }

    public void playerTick(boolean shouldSendChunkUpdates) {
        ChunkPos chunkPos;
        Object object;
        super.tick();
        for (int i = 0; i < this.inventory.size(); ++i) {
            ItemStack itemStack = this.inventory.getStack(i);
            if (itemStack == null || !Item.ITEMS[itemStack.itemId].isNetworkSynced() || this.networkHandler.getBlockDataSendQueueSize() > 2 || (object = ((NetworkSyncedItem)Item.ITEMS[itemStack.itemId]).getUpdatePacket(itemStack, this.world, this)) == null) continue;
            this.networkHandler.sendPacket((Packet)object);
        }
        if (shouldSendChunkUpdates && !this.pendingChunkUpdates.isEmpty() && (chunkPos = (ChunkPos)this.pendingChunkUpdates.get(0)) != null) {
            boolean bl = false;
            if (this.networkHandler.getBlockDataSendQueueSize() < 4) {
                bl = true;
            }
            if (bl) {
                object = this.server.getWorld(this.dimensionId);
                this.pendingChunkUpdates.remove(chunkPos);
                this.networkHandler.sendPacket(new ChunkDataS2CPacket(chunkPos.x * 16, 0, chunkPos.z * 16, 16, 128, 16, (World)object));
                List list = ((ServerWorld)object).getBlockEntities(chunkPos.x * 16, 0, chunkPos.z * 16, chunkPos.x * 16 + 16, 128, chunkPos.z * 16 + 16);
                for (int i = 0; i < list.size(); ++i) {
                    this.updateBlockEntity((BlockEntity)list.get(i));
                }
            }
        }
        if (this.inTeleportationState) {
            if (this.server.properties.getProperty("allow-nether", true)) {
                if (this.currentScreenHandler != this.playerScreenHandler) {
                    this.closeHandledScreen();
                }
                if (this.vehicle != null) {
                    this.setVehicle(this.vehicle);
                } else {
                    this.changeDimensionCooldown += 0.0125f;
                    if (this.changeDimensionCooldown >= 1.0f) {
                        this.changeDimensionCooldown = 1.0f;
                        this.portalCooldown = 10;
                        this.server.playerManager.changePlayerDimension(this);
                    }
                }
                this.inTeleportationState = false;
            }
        } else {
            if (this.changeDimensionCooldown > 0.0f) {
                this.changeDimensionCooldown -= 0.05f;
            }
            if (this.changeDimensionCooldown < 0.0f) {
                this.changeDimensionCooldown = 0.0f;
            }
        }
        if (this.portalCooldown > 0) {
            --this.portalCooldown;
        }
        if (this.health != this.lastHealthScore) {
            this.networkHandler.sendPacket(new HealthUpdateS2CPacket(this.health));
            this.lastHealthScore = this.health;
        }
    }

    private void updateBlockEntity(BlockEntity blockentity) {
        Packet packet;
        if (blockentity != null && (packet = blockentity.createUpdatePacket()) != null) {
            this.networkHandler.sendPacket(packet);
        }
    }

    public void tickMovement() {
        super.tickMovement();
    }

    public void sendPickup(Entity item, int count) {
        if (!item.dead) {
            EntityTracker entityTracker = this.server.getEntityTracker(this.dimensionId);
            if (item instanceof ItemEntity) {
                entityTracker.sendToListeners(item, new ItemPickupAnimationS2CPacket(item.id, this.id));
            }
            if (item instanceof ArrowEntity) {
                entityTracker.sendToListeners(item, new ItemPickupAnimationS2CPacket(item.id, this.id));
            }
        }
        super.sendPickup(item, count);
        this.currentScreenHandler.sendContentUpdates();
    }

    public void swingHand() {
        if (!this.handSwinging) {
            this.handSwingTicks = -1;
            this.handSwinging = true;
            EntityTracker entityTracker = this.server.getEntityTracker(this.dimensionId);
            entityTracker.sendToListeners(this, new EntityAnimationPacket(this, 1));
        }
    }

    public void clearWakePosition() {
    }

    public SleepAttemptResult trySleep(int x, int y, int z) {
        SleepAttemptResult sleepAttemptResult = super.trySleep(x, y, z);
        if (sleepAttemptResult == SleepAttemptResult.OK) {
            EntityTracker entityTracker = this.server.getEntityTracker(this.dimensionId);
            PlayerSleepUpdateS2CPacket playerSleepUpdateS2CPacket = new PlayerSleepUpdateS2CPacket(this, 0, x, y, z);
            entityTracker.sendToListeners(this, playerSleepUpdateS2CPacket);
            this.networkHandler.teleport(this.x, this.y, this.z, this.yaw, this.pitch);
            this.networkHandler.sendPacket(playerSleepUpdateS2CPacket);
        }
        return sleepAttemptResult;
    }

    public void wakeUp(boolean resetSleepTimer, boolean updateSleepingPlayers, boolean setSpawnPos) {
        if (this.isSleeping()) {
            EntityTracker entityTracker = this.server.getEntityTracker(this.dimensionId);
            entityTracker.sendToAround(this, new EntityAnimationPacket(this, 3));
        }
        super.wakeUp(resetSleepTimer, updateSleepingPlayers, setSpawnPos);
        if (this.networkHandler != null) {
            this.networkHandler.teleport(this.x, this.y, this.z, this.yaw, this.pitch);
        }
    }

    public void setVehicle(Entity entity) {
        super.setVehicle(entity);
        this.networkHandler.sendPacket(new EntityVehicleSetS2CPacket(this, this.vehicle));
        this.networkHandler.teleport(this.x, this.y, this.z, this.yaw, this.pitch);
    }

    protected void fall(double heightDifference, boolean onGround) {
    }

    public void handleFall(double heightDifference, boolean onGround) {
        super.fall(heightDifference, onGround);
    }

    private void incrementScreenHandlerSyncId() {
        this.screenHandlerSyncId = this.screenHandlerSyncId % 100 + 1;
    }

    public void openCraftingScreen(int x, int y, int z) {
        this.incrementScreenHandlerSyncId();
        this.networkHandler.sendPacket(new OpenScreenS2CPacket(this.screenHandlerSyncId, 1, "Crafting", 9));
        this.currentScreenHandler = new CraftingScreenHandler(this.inventory, this.world, x, y, z);
        this.currentScreenHandler.syncId = this.screenHandlerSyncId;
        this.currentScreenHandler.addListener(this);
    }

    public void openChestScreen(Inventory inventory) {
        this.incrementScreenHandlerSyncId();
        this.networkHandler.sendPacket(new OpenScreenS2CPacket(this.screenHandlerSyncId, 0, inventory.getName(), inventory.size()));
        this.currentScreenHandler = new GenericContainerScreenHandler(this.inventory, inventory);
        this.currentScreenHandler.syncId = this.screenHandlerSyncId;
        this.currentScreenHandler.addListener(this);
    }

    public void openFurnaceScreen(FurnaceBlockEntity furnace) {
        this.incrementScreenHandlerSyncId();
        this.networkHandler.sendPacket(new OpenScreenS2CPacket(this.screenHandlerSyncId, 2, furnace.getName(), furnace.size()));
        this.currentScreenHandler = new FurnaceScreenHandler(this.inventory, furnace);
        this.currentScreenHandler.syncId = this.screenHandlerSyncId;
        this.currentScreenHandler.addListener(this);
    }

    public void openDispenserScreen(DispenserBlockEntity dispenser) {
        this.incrementScreenHandlerSyncId();
        this.networkHandler.sendPacket(new OpenScreenS2CPacket(this.screenHandlerSyncId, 3, dispenser.getName(), dispenser.size()));
        this.currentScreenHandler = new DispenserScreenHandler(this.inventory, dispenser);
        this.currentScreenHandler.syncId = this.screenHandlerSyncId;
        this.currentScreenHandler.addListener(this);
    }

    public void onSlotUpdate(ScreenHandler handler, int slot, ItemStack stack) {
        if (handler.getSlot(slot) instanceof CraftingResultSlot) {
            return;
        }
        if (this.skipPacketSlotUpdates) {
            return;
        }
        this.networkHandler.sendPacket(new ScreenHandlerSlotUpdateS2CPacket(handler.syncId, slot, stack));
    }

    public void onContentsUpdate(ScreenHandler screenHandler) {
        this.onContentsUpdate(screenHandler, screenHandler.getStacks());
    }

    public void onContentsUpdate(ScreenHandler handler, List stacks) {
        this.networkHandler.sendPacket(new InventoryS2CPacket(handler.syncId, stacks));
        this.networkHandler.sendPacket(new ScreenHandlerSlotUpdateS2CPacket(-1, -1, this.inventory.getCursorStack()));
    }

    public void onPropertyUpdate(ScreenHandler handler, int syncId, int trackedValue) {
        this.networkHandler.sendPacket(new ScreenHandlerPropertyUpdateS2CPacket(handler.syncId, syncId, trackedValue));
    }

    public void onCursorStackChanged(ItemStack stack) {
    }

    public void closeHandledScreen() {
        this.networkHandler.sendPacket(new CloseScreenS2CPacket(this.currentScreenHandler.syncId));
        this.onHandledScreenClosed();
    }

    public void updateCursorStack() {
        if (this.skipPacketSlotUpdates) {
            return;
        }
        this.networkHandler.sendPacket(new ScreenHandlerSlotUpdateS2CPacket(-1, -1, this.inventory.getCursorStack()));
    }

    public void onHandledScreenClosed() {
        this.currentScreenHandler.onClosed(this);
        this.currentScreenHandler = this.playerScreenHandler;
    }

    public void updateInput(float sidewaysSpeed, float forwardSpeed, boolean jumping, boolean sneaking, float pitch, float yaw) {
        this.sidewaysSpeed = sidewaysSpeed;
        this.forwardSpeed = forwardSpeed;
        this.jumping = jumping;
        this.setSneaking(sneaking);
        this.pitch = pitch;
        this.yaw = yaw;
    }

    public void increaseStat(Stat stat, int amount) {
        if (stat == null) {
            return;
        }
        if (!stat.localOnly) {
            while (amount > 100) {
                this.networkHandler.sendPacket(new IncreaseStatS2CPacket(stat.id, 100));
                amount -= 100;
            }
            this.networkHandler.sendPacket(new IncreaseStatS2CPacket(stat.id, amount));
        }
    }

    public void onDisconnect() {
        if (this.vehicle != null) {
            this.setVehicle(this.vehicle);
        }
        if (this.passenger != null) {
            this.passenger.setVehicle(this);
        }
        if (this.sleeping) {
            this.wakeUp(true, false, false);
        }
    }

    public void markHealthDirty() {
        this.lastHealthScore = -99999999;
    }

    public void sendMessage(String message) {
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        String string = translationStorage.get(message);
        this.networkHandler.sendPacket(new ChatMessagePacket(string));
    }
}

