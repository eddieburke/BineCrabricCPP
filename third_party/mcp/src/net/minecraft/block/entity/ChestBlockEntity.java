/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.block.entity;

import net.minecraft.block.entity.BlockEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtList;

public class ChestBlockEntity
extends BlockEntity
implements Inventory {
    private ItemStack[] inventory = new ItemStack[36];

    public int size() {
        return 27;
    }

    public ItemStack getStack(int slot) {
        return this.inventory[slot];
    }

    public ItemStack removeStack(int slot, int amount) {
        if (this.inventory[slot] != null) {
            if (this.inventory[slot].count <= amount) {
                ItemStack itemStack = this.inventory[slot];
                this.inventory[slot] = null;
                this.markDirty();
                return itemStack;
            }
            ItemStack itemStack = this.inventory[slot].split(amount);
            if (this.inventory[slot].count == 0) {
                this.inventory[slot] = null;
            }
            this.markDirty();
            return itemStack;
        }
        return null;
    }

    public void setStack(int slot, ItemStack stack) {
        this.inventory[slot] = stack;
        if (stack != null && stack.count > this.getMaxCountPerStack()) {
            stack.count = this.getMaxCountPerStack();
        }
        this.markDirty();
    }

    public String getName() {
        return "Chest";
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        NbtList nbtList = nbt.getList("Items");
        this.inventory = new ItemStack[this.size()];
        for (int i = 0; i < nbtList.size(); ++i) {
            NbtCompound nbtCompound = (NbtCompound)nbtList.get(i);
            int n = nbtCompound.getByte("Slot") & 0xFF;
            if (n < 0 || n >= this.inventory.length) continue;
            this.inventory[n] = new ItemStack(nbtCompound);
        }
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        NbtList nbtList = new NbtList();
        for (int i = 0; i < this.inventory.length; ++i) {
            if (this.inventory[i] == null) continue;
            NbtCompound nbtCompound = new NbtCompound();
            nbtCompound.putByte("Slot", (byte)i);
            this.inventory[i].writeNbt(nbtCompound);
            nbtList.add(nbtCompound);
        }
        nbt.put("Items", nbtList);
    }

    public int getMaxCountPerStack() {
        return 64;
    }

    public boolean canPlayerUse(PlayerEntity player) {
        if (this.world.getBlockEntity(this.x, this.y, this.z) != this) {
            return false;
        }
        return !(player.getSquaredDistance((double)this.x + 0.5, (double)this.y + 0.5, (double)this.z + 0.5) > 64.0);
    }
}

