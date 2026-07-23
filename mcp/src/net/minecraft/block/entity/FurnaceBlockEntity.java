/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block.entity;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.FurnaceBlock;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtList;
import net.minecraft.recipe.SmeltingRecipeManager;

public class FurnaceBlockEntity
extends BlockEntity
implements Inventory {
    private ItemStack[] inventory = new ItemStack[3];
    public int burnTime = 0;
    public int fuelTime = 0;
    public int cookTime = 0;

    public int size() {
        return this.inventory.length;
    }

    public ItemStack getStack(int slot) {
        return this.inventory[slot];
    }

    public ItemStack removeStack(int slot, int amount) {
        if (this.inventory[slot] != null) {
            if (this.inventory[slot].count <= amount) {
                ItemStack itemStack = this.inventory[slot];
                this.inventory[slot] = null;
                return itemStack;
            }
            ItemStack itemStack = this.inventory[slot].split(amount);
            if (this.inventory[slot].count == 0) {
                this.inventory[slot] = null;
            }
            return itemStack;
        }
        return null;
    }

    public void setStack(int slot, ItemStack stack) {
        this.inventory[slot] = stack;
        if (stack != null && stack.count > this.getMaxCountPerStack()) {
            stack.count = this.getMaxCountPerStack();
        }
    }

    public String getName() {
        return "Furnace";
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        NbtList nbtList = nbt.getList("Items");
        this.inventory = new ItemStack[this.size()];
        for (int i = 0; i < nbtList.size(); ++i) {
            NbtCompound nbtCompound = (NbtCompound)nbtList.get(i);
            byte by = nbtCompound.getByte("Slot");
            if (by < 0 || by >= this.inventory.length) continue;
            this.inventory[by] = new ItemStack(nbtCompound);
        }
        this.burnTime = nbt.getShort("BurnTime");
        this.cookTime = nbt.getShort("CookTime");
        this.fuelTime = this.getFuelTime(this.inventory[1]);
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        nbt.putShort("BurnTime", (short)this.burnTime);
        nbt.putShort("CookTime", (short)this.cookTime);
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

    @Environment(value=EnvType.CLIENT)
    public int getCookTimeDelta(int multiplier) {
        return this.cookTime * multiplier / 200;
    }

    @Environment(value=EnvType.CLIENT)
    public int getFuelTimeDelta(int multiplier) {
        if (this.fuelTime == 0) {
            this.fuelTime = 200;
        }
        return this.burnTime * multiplier / this.fuelTime;
    }

    public boolean isBurning() {
        return this.burnTime > 0;
    }

    public void tick() {
        boolean bl = this.burnTime > 0;
        boolean bl2 = false;
        if (this.burnTime > 0) {
            --this.burnTime;
        }
        if (!this.world.isRemote) {
            if (this.burnTime == 0 && this.canAcceptRecipeOutput()) {
                this.fuelTime = this.burnTime = this.getFuelTime(this.inventory[1]);
                if (this.burnTime > 0) {
                    bl2 = true;
                    if (this.inventory[1] != null) {
                        --this.inventory[1].count;
                        if (this.inventory[1].count == 0) {
                            this.inventory[1] = null;
                        }
                    }
                }
            }
            if (this.isBurning() && this.canAcceptRecipeOutput()) {
                ++this.cookTime;
                if (this.cookTime == 200) {
                    this.cookTime = 0;
                    this.craftRecipe();
                    bl2 = true;
                }
            } else {
                this.cookTime = 0;
            }
            if (bl != this.burnTime > 0) {
                bl2 = true;
                FurnaceBlock.updateLitState(this.burnTime > 0, this.world, this.x, this.y, this.z);
            }
        }
        if (bl2) {
            this.markDirty();
        }
    }

    private boolean canAcceptRecipeOutput() {
        if (this.inventory[0] == null) {
            return false;
        }
        ItemStack itemStack = SmeltingRecipeManager.getInstance().craft(this.inventory[0].getItem().id);
        if (itemStack == null) {
            return false;
        }
        if (this.inventory[2] == null) {
            return true;
        }
        if (!this.inventory[2].isItemEqual(itemStack)) {
            return false;
        }
        if (this.inventory[2].count < this.getMaxCountPerStack() && this.inventory[2].count < this.inventory[2].getMaxCount()) {
            return true;
        }
        return this.inventory[2].count < itemStack.getMaxCount();
    }

    public void craftRecipe() {
        if (!this.canAcceptRecipeOutput()) {
            return;
        }
        ItemStack itemStack = SmeltingRecipeManager.getInstance().craft(this.inventory[0].getItem().id);
        if (this.inventory[2] == null) {
            this.inventory[2] = itemStack.copy();
        } else if (this.inventory[2].itemId == itemStack.itemId) {
            ++this.inventory[2].count;
        }
        --this.inventory[0].count;
        if (this.inventory[0].count <= 0) {
            this.inventory[0] = null;
        }
    }

    private int getFuelTime(ItemStack itemStack) {
        if (itemStack == null) {
            return 0;
        }
        int n = itemStack.getItem().id;
        if (n < 256 && Block.BLOCKS[n].material == Material.WOOD) {
            return 300;
        }
        if (n == Item.STICK.id) {
            return 100;
        }
        if (n == Item.COAL.id) {
            return 1600;
        }
        if (n == Item.LAVA_BUCKET.id) {
            return 20000;
        }
        if (n == Block.SAPLING.id) {
            return 100;
        }
        return 0;
    }

    public boolean canPlayerUse(PlayerEntity player) {
        if (this.world.getBlockEntity(this.x, this.y, this.z) != this) {
            return false;
        }
        return !(player.getSquaredDistance((double)this.x + 0.5, (double)this.y + 0.5, (double)this.z + 0.5) > 64.0);
    }
}

