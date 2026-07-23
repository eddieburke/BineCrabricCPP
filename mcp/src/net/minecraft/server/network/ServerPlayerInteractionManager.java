/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.server.network;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.player.ServerPlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.network.packet.s2c.play.BlockUpdateS2CPacket;
import net.minecraft.world.ServerWorld;
import net.minecraft.world.World;

@Environment(value=EnvType.SERVER)
public class ServerPlayerInteractionManager {
    private ServerWorld world;
    public PlayerEntity player;
    private float blockBreakProgress = 0.0f;
    private int failedMiningStartTime;
    private int failedMiningX;
    private int failedMiningY;
    private int failedMiningZ;
    private int tickCounter;
    private boolean mining;
    private int miningX;
    private int miningY;
    private int miningZ;
    private int startMiningTime;

    public ServerPlayerInteractionManager(ServerWorld world) {
        this.world = world;
    }

    public void update() {
        ++this.tickCounter;
        if (this.mining) {
            int n = this.tickCounter - this.startMiningTime;
            int n2 = this.world.getBlockId(this.miningX, this.miningY, this.miningZ);
            if (n2 != 0) {
                Block block = Block.BLOCKS[n2];
                float f = block.getHardness(this.player) * (float)(n + 1);
                if (f >= 1.0f) {
                    this.mining = false;
                    this.tryBreakBlock(this.miningX, this.miningY, this.miningZ);
                }
            } else {
                this.mining = false;
            }
        }
    }

    public void onBlockBreakingAction(int x, int y, int z, int direction) {
        this.world.extinguishFire(null, x, y, z, direction);
        this.failedMiningStartTime = this.tickCounter;
        int n = this.world.getBlockId(x, y, z);
        if (n > 0) {
            Block.BLOCKS[n].onBlockBreakStart(this.world, x, y, z, this.player);
        }
        if (n > 0 && Block.BLOCKS[n].getHardness(this.player) >= 1.0f) {
            this.tryBreakBlock(x, y, z);
        } else {
            this.failedMiningX = x;
            this.failedMiningY = y;
            this.failedMiningZ = z;
        }
    }

    public void continueMining(int x, int y, int z) {
        if (x == this.failedMiningX && y == this.failedMiningY && z == this.failedMiningZ) {
            int n = this.tickCounter - this.failedMiningStartTime;
            int n2 = this.world.getBlockId(x, y, z);
            if (n2 != 0) {
                Block block = Block.BLOCKS[n2];
                float f = block.getHardness(this.player) * (float)(n + 1);
                if (f >= 0.7f) {
                    this.tryBreakBlock(x, y, z);
                } else if (!this.mining) {
                    this.mining = true;
                    this.miningX = x;
                    this.miningY = y;
                    this.miningZ = z;
                    this.startMiningTime = this.failedMiningStartTime;
                }
            }
        }
        this.blockBreakProgress = 0.0f;
    }

    public boolean finishMining(int x, int y, int z) {
        Block block = Block.BLOCKS[this.world.getBlockId(x, y, z)];
        int n = this.world.getBlockMeta(x, y, z);
        boolean bl = this.world.setBlock(x, y, z, 0);
        if (block != null && bl) {
            block.onMetadataChange(this.world, x, y, z, n);
        }
        return bl;
    }

    public boolean tryBreakBlock(int x, int y, int z) {
        int n = this.world.getBlockId(x, y, z);
        int n2 = this.world.getBlockMeta(x, y, z);
        this.world.worldEvent(this.player, 2001, x, y, z, n + this.world.getBlockMeta(x, y, z) * 256);
        boolean bl = this.finishMining(x, y, z);
        ItemStack itemStack = this.player.getHand();
        if (itemStack != null) {
            itemStack.postMine(n, x, y, z, this.player);
            if (itemStack.count == 0) {
                itemStack.onRemoved(this.player);
                this.player.clearStackInHand();
            }
        }
        if (bl && this.player.canHarvest(Block.BLOCKS[n])) {
            Block.BLOCKS[n].afterBreak(this.world, this.player, x, y, z, n2);
            ((ServerPlayerEntity)this.player).networkHandler.sendPacket(new BlockUpdateS2CPacket(x, y, z, this.world));
        }
        return bl;
    }

    public boolean interactItem(PlayerEntity player, World world, ItemStack stack) {
        int n = stack.count;
        ItemStack itemStack = stack.use(world, player);
        if (itemStack != stack || itemStack != null && itemStack.count != n) {
            player.inventory.main[player.inventory.selectedSlot] = itemStack;
            if (itemStack.count == 0) {
                player.inventory.main[player.inventory.selectedSlot] = null;
            }
            return true;
        }
        return false;
    }

    public boolean interactBlock(PlayerEntity player, World world, ItemStack stack, int x, int y, int z, int side) {
        int n = world.getBlockId(x, y, z);
        if (n > 0 && Block.BLOCKS[n].onUse(world, x, y, z, player)) {
            return true;
        }
        if (stack == null) {
            return false;
        }
        return stack.useOnBlock(player, world, x, y, z, side);
    }
}

