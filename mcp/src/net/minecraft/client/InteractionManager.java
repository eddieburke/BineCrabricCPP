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
import net.minecraft.client.Minecraft;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.ClientPlayerEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class InteractionManager {
    protected final Minecraft minecraft;
    public boolean noTick = false;

    public InteractionManager(Minecraft minecraft) {
        this.minecraft = minecraft;
    }

    public void setWorld(World world) {
    }

    public void attackBlock(int x, int y, int z, int direction) {
        this.minecraft.world.extinguishFire(this.minecraft.player, x, y, z, direction);
        this.breakBlock(x, y, z, direction);
    }

    public boolean breakBlock(int x, int y, int z, int direction) {
        World world = this.minecraft.world;
        Block block = Block.BLOCKS[world.getBlockId(x, y, z)];
        world.worldEvent(2001, x, y, z, block.id + world.getBlockMeta(x, y, z) * 256);
        int n = world.getBlockMeta(x, y, z);
        boolean bl = world.setBlock(x, y, z, 0);
        if (block != null && bl) {
            block.onMetadataChange(world, x, y, z, n);
        }
        return bl;
    }

    public void processBlockBreakingAction(int x, int y, int z, int side) {
    }

    public void cancelBlockBreaking() {
    }

    public void update(float f) {
    }

    public float getReachDistance() {
        return 5.0f;
    }

    public boolean interactItem(PlayerEntity player, World world, ItemStack item) {
        int n = item.count;
        ItemStack itemStack = item.use(world, player);
        if (itemStack != item || itemStack != null && itemStack.count != n) {
            player.inventory.main[player.inventory.selectedSlot] = itemStack;
            if (itemStack.count == 0) {
                player.inventory.main[player.inventory.selectedSlot] = null;
            }
            return true;
        }
        return false;
    }

    public void preparePlayer(PlayerEntity player) {
    }

    public void tick() {
    }

    public boolean canBeRendered() {
        return true;
    }

    public void preparePlayerRespawn(PlayerEntity player) {
    }

    public boolean interactBlock(PlayerEntity player, World world, ItemStack item, int x, int y, int z, int side) {
        int n = world.getBlockId(x, y, z);
        if (n > 0 && Block.BLOCKS[n].onUse(world, x, y, z, player)) {
            return true;
        }
        if (item == null) {
            return false;
        }
        return item.useOnBlock(player, world, x, y, z, side);
    }

    public PlayerEntity createPlayer(World world) {
        return new ClientPlayerEntity(this.minecraft, world, this.minecraft.session, world.dimension.id);
    }

    public void interactEntity(PlayerEntity player, Entity entity) {
        player.interact(entity);
    }

    public void attackEntity(PlayerEntity player, Entity target) {
        player.attack(target);
    }

    public ItemStack clickSlot(int syncId, int slotId, int button, boolean shift, PlayerEntity player) {
        return player.currentScreenHandler.onSlotClick(slotId, button, shift, player);
    }

    public void onScreenRemoved(int syncId, PlayerEntity player) {
        player.currentScreenHandler.onClosed(player);
        player.currentScreenHandler = player.playerScreenHandler;
    }
}

