/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.client.InteractionManager;
import net.minecraft.client.Minecraft;
import net.minecraft.client.network.ClientNetworkHandler;
import net.minecraft.client.network.MultiplayerClientPlayerEntity;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.network.packet.c2s.play.ClickSlotC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerActionC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractBlockC2SPacket;
import net.minecraft.network.packet.c2s.play.PlayerInteractEntityC2SPacket;
import net.minecraft.network.packet.c2s.play.UpdateSelectedSlotC2SPacket;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class MultiplayerInteractionManager
extends InteractionManager {
    private int breakingPosX = -1;
    private int breakingPosY = -1;
    private int breakingPosZ = -1;
    private float blockBreakingProgress = 0.0f;
    private float lastBlockBreakingProgress = 0.0f;
    private float breakingSoundDelayTicks = 0.0f;
    private int breakingDelayTicks = 0;
    private boolean breakingBlock = false;
    private ClientNetworkHandler networkHandler;
    private int selectedSlot = 0;

    public MultiplayerInteractionManager(Minecraft minecraft, ClientNetworkHandler networkHandler) {
        super(minecraft);
        this.networkHandler = networkHandler;
    }

    public void preparePlayer(PlayerEntity player) {
        player.yaw = -180.0f;
    }

    public boolean breakBlock(int x, int y, int z, int direction) {
        int n = this.minecraft.world.getBlockId(x, y, z);
        boolean bl = super.breakBlock(x, y, z, direction);
        ItemStack itemStack = this.minecraft.player.getHand();
        if (itemStack != null) {
            itemStack.postMine(n, x, y, z, this.minecraft.player);
            if (itemStack.count == 0) {
                itemStack.onRemoved(this.minecraft.player);
                this.minecraft.player.clearStackInHand();
            }
        }
        return bl;
    }

    public void attackBlock(int x, int y, int z, int direction) {
        if (!this.breakingBlock || x != this.breakingPosX || y != this.breakingPosY || z != this.breakingPosZ) {
            this.networkHandler.sendPacket(new PlayerActionC2SPacket(0, x, y, z, direction));
            int n = this.minecraft.world.getBlockId(x, y, z);
            if (n > 0 && this.blockBreakingProgress == 0.0f) {
                Block.BLOCKS[n].onBlockBreakStart(this.minecraft.world, x, y, z, this.minecraft.player);
            }
            if (n > 0 && Block.BLOCKS[n].getHardness(this.minecraft.player) >= 1.0f) {
                this.breakBlock(x, y, z, direction);
            } else {
                this.breakingBlock = true;
                this.breakingPosX = x;
                this.breakingPosY = y;
                this.breakingPosZ = z;
                this.blockBreakingProgress = 0.0f;
                this.lastBlockBreakingProgress = 0.0f;
                this.breakingSoundDelayTicks = 0.0f;
            }
        }
    }

    public void cancelBlockBreaking() {
        this.blockBreakingProgress = 0.0f;
        this.breakingBlock = false;
    }

    public void processBlockBreakingAction(int x, int y, int z, int side) {
        if (!this.breakingBlock) {
            return;
        }
        this.updateSelectedSlot();
        if (this.breakingDelayTicks > 0) {
            --this.breakingDelayTicks;
            return;
        }
        if (x == this.breakingPosX && y == this.breakingPosY && z == this.breakingPosZ) {
            int n = this.minecraft.world.getBlockId(x, y, z);
            if (n == 0) {
                this.breakingBlock = false;
                return;
            }
            Block block = Block.BLOCKS[n];
            this.blockBreakingProgress += block.getHardness(this.minecraft.player);
            if (this.breakingSoundDelayTicks % 4.0f == 0.0f && block != null) {
                this.minecraft.soundManager.playSound(block.soundGroup.getSound(), (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, (block.soundGroup.getVolume() + 1.0f) / 8.0f, block.soundGroup.getPitch() * 0.5f);
            }
            this.breakingSoundDelayTicks += 1.0f;
            if (this.blockBreakingProgress >= 1.0f) {
                this.breakingBlock = false;
                this.networkHandler.sendPacket(new PlayerActionC2SPacket(2, x, y, z, side));
                this.breakBlock(x, y, z, side);
                this.blockBreakingProgress = 0.0f;
                this.lastBlockBreakingProgress = 0.0f;
                this.breakingSoundDelayTicks = 0.0f;
                this.breakingDelayTicks = 5;
            }
        } else {
            this.attackBlock(x, y, z, side);
        }
    }

    public void update(float f) {
        if (this.blockBreakingProgress <= 0.0f) {
            this.minecraft.inGameHud.progress = 0.0f;
            this.minecraft.worldRenderer.miningProgress = 0.0f;
        } else {
            float f2;
            this.minecraft.inGameHud.progress = f2 = this.lastBlockBreakingProgress + (this.blockBreakingProgress - this.lastBlockBreakingProgress) * f;
            this.minecraft.worldRenderer.miningProgress = f2;
        }
    }

    public float getReachDistance() {
        return 4.0f;
    }

    public void setWorld(World world) {
        super.setWorld(world);
    }

    public void tick() {
        this.updateSelectedSlot();
        this.lastBlockBreakingProgress = this.blockBreakingProgress;
        this.minecraft.soundManager.tick();
    }

    private void updateSelectedSlot() {
        int n = this.minecraft.player.inventory.selectedSlot;
        if (n != this.selectedSlot) {
            this.selectedSlot = n;
            this.networkHandler.sendPacket(new UpdateSelectedSlotC2SPacket(this.selectedSlot));
        }
    }

    public boolean interactBlock(PlayerEntity player, World world, ItemStack item, int x, int y, int z, int side) {
        this.updateSelectedSlot();
        this.networkHandler.sendPacket(new PlayerInteractBlockC2SPacket(x, y, z, side, player.inventory.getSelectedItem()));
        boolean bl = super.interactBlock(player, world, item, x, y, z, side);
        return bl;
    }

    public boolean interactItem(PlayerEntity player, World world, ItemStack item) {
        this.updateSelectedSlot();
        this.networkHandler.sendPacket(new PlayerInteractBlockC2SPacket(-1, -1, -1, 255, player.inventory.getSelectedItem()));
        boolean bl = super.interactItem(player, world, item);
        return bl;
    }

    public PlayerEntity createPlayer(World world) {
        return new MultiplayerClientPlayerEntity(this.minecraft, world, this.minecraft.session, this.networkHandler);
    }

    public void attackEntity(PlayerEntity player, Entity target) {
        this.updateSelectedSlot();
        this.networkHandler.sendPacket(new PlayerInteractEntityC2SPacket(player.id, target.id, 1));
        player.attack(target);
    }

    public void interactEntity(PlayerEntity player, Entity entity) {
        this.updateSelectedSlot();
        this.networkHandler.sendPacket(new PlayerInteractEntityC2SPacket(player.id, entity.id, 0));
        player.interact(entity);
    }

    public ItemStack clickSlot(int syncId, int slotId, int button, boolean shift, PlayerEntity player) {
        short s = player.currentScreenHandler.nextRevision(player.inventory);
        ItemStack itemStack = super.clickSlot(syncId, slotId, button, shift, player);
        this.networkHandler.sendPacket(new ClickSlotC2SPacket(syncId, slotId, button, shift, itemStack, s));
        return itemStack;
    }

    public void onScreenRemoved(int syncId, PlayerEntity player) {
        if (syncId == -9999) {
            return;
        }
    }
}

