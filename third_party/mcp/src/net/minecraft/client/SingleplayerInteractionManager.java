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
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class SingleplayerInteractionManager
extends InteractionManager {
    private int breakingPosX = -1;
    private int breakingPosY = -1;
    private int breakingPosZ = -1;
    private float blockBreakingProgress = 0.0f;
    private float lastBlockBreakingProgress = 0.0f;
    private float breakingSoundDelayTicks = 0.0f;
    private int breakingDelayTicks = 0;

    public SingleplayerInteractionManager(Minecraft minecraft) {
        super(minecraft);
    }

    public void preparePlayer(PlayerEntity player) {
        player.yaw = -180.0f;
    }

    public boolean breakBlock(int x, int y, int z, int direction) {
        int n = this.minecraft.world.getBlockId(x, y, z);
        int n2 = this.minecraft.world.getBlockMeta(x, y, z);
        boolean bl = super.breakBlock(x, y, z, direction);
        ItemStack itemStack = this.minecraft.player.getHand();
        boolean bl2 = this.minecraft.player.canHarvest(Block.BLOCKS[n]);
        if (itemStack != null) {
            itemStack.postMine(n, x, y, z, this.minecraft.player);
            if (itemStack.count == 0) {
                itemStack.onRemoved(this.minecraft.player);
                this.minecraft.player.clearStackInHand();
            }
        }
        if (bl && bl2) {
            Block.BLOCKS[n].afterBreak(this.minecraft.world, this.minecraft.player, x, y, z, n2);
        }
        return bl;
    }

    public void attackBlock(int x, int y, int z, int direction) {
        this.minecraft.world.extinguishFire(this.minecraft.player, x, y, z, direction);
        int n = this.minecraft.world.getBlockId(x, y, z);
        if (n > 0 && this.blockBreakingProgress == 0.0f) {
            Block.BLOCKS[n].onBlockBreakStart(this.minecraft.world, x, y, z, this.minecraft.player);
        }
        if (n > 0 && Block.BLOCKS[n].getHardness(this.minecraft.player) >= 1.0f) {
            this.breakBlock(x, y, z, direction);
        }
    }

    public void cancelBlockBreaking() {
        this.blockBreakingProgress = 0.0f;
        this.breakingDelayTicks = 0;
    }

    public void processBlockBreakingAction(int x, int y, int z, int side) {
        if (this.breakingDelayTicks > 0) {
            --this.breakingDelayTicks;
            return;
        }
        if (x == this.breakingPosX && y == this.breakingPosY && z == this.breakingPosZ) {
            int n = this.minecraft.world.getBlockId(x, y, z);
            if (n == 0) {
                return;
            }
            Block block = Block.BLOCKS[n];
            this.blockBreakingProgress += block.getHardness(this.minecraft.player);
            if (this.breakingSoundDelayTicks % 4.0f == 0.0f && block != null) {
                this.minecraft.soundManager.playSound(block.soundGroup.getSound(), (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, (block.soundGroup.getVolume() + 1.0f) / 8.0f, block.soundGroup.getPitch() * 0.5f);
            }
            this.breakingSoundDelayTicks += 1.0f;
            if (this.blockBreakingProgress >= 1.0f) {
                this.breakBlock(x, y, z, side);
                this.blockBreakingProgress = 0.0f;
                this.lastBlockBreakingProgress = 0.0f;
                this.breakingSoundDelayTicks = 0.0f;
                this.breakingDelayTicks = 5;
            }
        } else {
            this.blockBreakingProgress = 0.0f;
            this.lastBlockBreakingProgress = 0.0f;
            this.breakingSoundDelayTicks = 0.0f;
            this.breakingPosX = x;
            this.breakingPosY = y;
            this.breakingPosZ = z;
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
        this.lastBlockBreakingProgress = this.blockBreakingProgress;
        this.minecraft.soundManager.tick();
    }
}

