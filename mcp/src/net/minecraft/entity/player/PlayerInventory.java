/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.player;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.ArmorItem;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtList;

public class PlayerInventory
implements Inventory {
    public ItemStack[] main = new ItemStack[36];
    public ItemStack[] armor = new ItemStack[4];
    public int selectedSlot = 0;
    public PlayerEntity player;
    private ItemStack cursorStack;
    public boolean dirty = false;

    public PlayerInventory(PlayerEntity player) {
        this.player = player;
    }

    public ItemStack getSelectedItem() {
        if (this.selectedSlot < 9 && this.selectedSlot >= 0) {
            return this.main[this.selectedSlot];
        }
        return null;
    }

    @Environment(value=EnvType.SERVER)
    public static int getHotbarSize() {
        return 9;
    }

    private int indexOf(int itemId) {
        for (int i = 0; i < this.main.length; ++i) {
            if (this.main[i] == null || this.main[i].itemId != itemId) continue;
            return i;
        }
        return -1;
    }

    private int indexOf(ItemStack stack) {
        for (int i = 0; i < this.main.length; ++i) {
            if (this.main[i] == null || this.main[i].itemId != stack.itemId || !this.main[i].isStackable() || this.main[i].count >= this.main[i].getMaxCount() || this.main[i].count >= this.getMaxCountPerStack() || this.main[i].hasSubtypes() && this.main[i].getDamage() != stack.getDamage()) continue;
            return i;
        }
        return -1;
    }

    private int getEmptySlot() {
        for (int i = 0; i < this.main.length; ++i) {
            if (this.main[i] != null) continue;
            return i;
        }
        return -1;
    }

    @Environment(value=EnvType.CLIENT)
    public void setHeldItem(int itemId, boolean testInteractionManager) {
        int n = this.indexOf(itemId);
        if (n >= 0 && n < 9) {
            this.selectedSlot = n;
            return;
        }
    }

    @Environment(value=EnvType.CLIENT)
    public void scrollInHotbar(int direction) {
        if (direction > 0) {
            direction = 1;
        }
        if (direction < 0) {
            direction = -1;
        }
        this.selectedSlot -= direction;
        while (this.selectedSlot < 0) {
            this.selectedSlot += 9;
        }
        while (this.selectedSlot >= 9) {
            this.selectedSlot -= 9;
        }
    }

    private int combineStacks(ItemStack stack) {
        int n;
        int n2 = stack.itemId;
        int n3 = stack.count;
        int n4 = this.indexOf(stack);
        if (n4 < 0) {
            n4 = this.getEmptySlot();
        }
        if (n4 < 0) {
            return n3;
        }
        if (this.main[n4] == null) {
            this.main[n4] = new ItemStack(n2, 0, stack.getDamage());
        }
        if ((n = n3) > this.main[n4].getMaxCount() - this.main[n4].count) {
            n = this.main[n4].getMaxCount() - this.main[n4].count;
        }
        if (n > this.getMaxCountPerStack() - this.main[n4].count) {
            n = this.getMaxCountPerStack() - this.main[n4].count;
        }
        if (n == 0) {
            return n3;
        }
        this.main[n4].count += n;
        this.main[n4].bobbingAnimationTime = 5;
        return n3 -= n;
    }

    public void inventoryTick() {
        for (int i = 0; i < this.main.length; ++i) {
            if (this.main[i] == null) continue;
            this.main[i].inventoryTick(this.player.world, this.player, i, this.selectedSlot == i);
        }
    }

    public boolean remove(int itemId) {
        int n = this.indexOf(itemId);
        if (n < 0) {
            return false;
        }
        if (--this.main[n].count <= 0) {
            this.main[n] = null;
        }
        return true;
    }

    public boolean addStack(ItemStack stack) {
        if (!stack.isDamaged()) {
            int n;
            do {
                n = stack.count;
                stack.count = this.combineStacks(stack);
            } while (stack.count > 0 && stack.count < n);
            return stack.count < n;
        }
        int n = this.getEmptySlot();
        if (n >= 0) {
            this.main[n] = ItemStack.clone(stack);
            this.main[n].bobbingAnimationTime = 5;
            stack.count = 0;
            return true;
        }
        return false;
    }

    public ItemStack removeStack(int slot, int amount) {
        ItemStack[] itemStackArray = this.main;
        if (slot >= this.main.length) {
            itemStackArray = this.armor;
            slot -= this.main.length;
        }
        if (itemStackArray[slot] != null) {
            if (itemStackArray[slot].count <= amount) {
                ItemStack itemStack = itemStackArray[slot];
                itemStackArray[slot] = null;
                return itemStack;
            }
            ItemStack itemStack = itemStackArray[slot].split(amount);
            if (itemStackArray[slot].count == 0) {
                itemStackArray[slot] = null;
            }
            return itemStack;
        }
        return null;
    }

    public void setStack(int slot, ItemStack stack) {
        ItemStack[] itemStackArray = this.main;
        if (slot >= itemStackArray.length) {
            slot -= itemStackArray.length;
            itemStackArray = this.armor;
        }
        itemStackArray[slot] = stack;
    }

    public float getStrengthOnBlock(Block block) {
        float f = 1.0f;
        if (this.main[this.selectedSlot] != null) {
            f *= this.main[this.selectedSlot].getMiningSpeedMultiplier(block);
        }
        return f;
    }

    public NbtList writeNbt(NbtList nbt) {
        NbtCompound nbtCompound;
        int n;
        for (n = 0; n < this.main.length; ++n) {
            if (this.main[n] == null) continue;
            nbtCompound = new NbtCompound();
            nbtCompound.putByte("Slot", (byte)n);
            this.main[n].writeNbt(nbtCompound);
            nbt.add(nbtCompound);
        }
        for (n = 0; n < this.armor.length; ++n) {
            if (this.armor[n] == null) continue;
            nbtCompound = new NbtCompound();
            nbtCompound.putByte("Slot", (byte)(n + 100));
            this.armor[n].writeNbt(nbtCompound);
            nbt.add(nbtCompound);
        }
        return nbt;
    }

    public void readNbt(NbtList nbt) {
        this.main = new ItemStack[36];
        this.armor = new ItemStack[4];
        for (int i = 0; i < nbt.size(); ++i) {
            NbtCompound nbtCompound = (NbtCompound)nbt.get(i);
            int n = nbtCompound.getByte("Slot") & 0xFF;
            ItemStack itemStack = new ItemStack(nbtCompound);
            if (itemStack.getItem() == null) continue;
            if (n >= 0 && n < this.main.length) {
                this.main[n] = itemStack;
            }
            if (n < 100 || n >= this.armor.length + 100) continue;
            this.armor[n - 100] = itemStack;
        }
    }

    public int size() {
        return this.main.length + 4;
    }

    public ItemStack getStack(int slot) {
        ItemStack[] itemStackArray = this.main;
        if (slot >= itemStackArray.length) {
            slot -= itemStackArray.length;
            itemStackArray = this.armor;
        }
        return itemStackArray[slot];
    }

    public String getName() {
        return "Inventory";
    }

    public int getMaxCountPerStack() {
        return 64;
    }

    public int getAttackDamage(Entity entity) {
        ItemStack itemStack = this.getStack(this.selectedSlot);
        if (itemStack != null) {
            return itemStack.getAttackDamage(entity);
        }
        return 1;
    }

    public boolean isUsingEffectiveTool(Block block) {
        if (block.material.isHandHarvestable()) {
            return true;
        }
        ItemStack itemStack = this.getStack(this.selectedSlot);
        if (itemStack != null) {
            return itemStack.isSuitableFor(block);
        }
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public ItemStack getArmorStack(int index) {
        return this.armor[index];
    }

    public int getTotalArmorDurability() {
        int n = 0;
        int n2 = 0;
        int n3 = 0;
        for (int i = 0; i < this.armor.length; ++i) {
            if (this.armor[i] == null || !(this.armor[i].getItem() instanceof ArmorItem)) continue;
            int n4 = this.armor[i].getMaxDamage();
            int n5 = this.armor[i].getDamage2();
            int n6 = n4 - n5;
            n2 += n6;
            n3 += n4;
            int n7 = ((ArmorItem)this.armor[i].getItem()).maxProtection;
            n += n7;
        }
        if (n3 == 0) {
            return 0;
        }
        return (n - 1) * n2 / n3 + 1;
    }

    public void damageArmor(int amount) {
        for (int i = 0; i < this.armor.length; ++i) {
            if (this.armor[i] == null || !(this.armor[i].getItem() instanceof ArmorItem)) continue;
            this.armor[i].damage(amount, this.player);
            if (this.armor[i].count != 0) continue;
            this.armor[i].onRemoved(this.player);
            this.armor[i] = null;
        }
    }

    public void dropInventory() {
        int n;
        for (n = 0; n < this.main.length; ++n) {
            if (this.main[n] == null) continue;
            this.player.dropItem(this.main[n], true);
            this.main[n] = null;
        }
        for (n = 0; n < this.armor.length; ++n) {
            if (this.armor[n] == null) continue;
            this.player.dropItem(this.armor[n], true);
            this.armor[n] = null;
        }
    }

    public void markDirty() {
        this.dirty = true;
    }

    public void setCursorStack(ItemStack cursorStack) {
        this.cursorStack = cursorStack;
        this.player.onCursorStackChanged(cursorStack);
    }

    public ItemStack getCursorStack() {
        return this.cursorStack;
    }

    public boolean canPlayerUse(PlayerEntity player) {
        if (this.player.dead) {
            return false;
        }
        return !(player.getSquaredDistance(this.player) > 64.0);
    }

    public boolean contains(ItemStack stack) {
        int n;
        for (n = 0; n < this.armor.length; ++n) {
            if (this.armor[n] == null || !this.armor[n].equals(stack)) continue;
            return true;
        }
        for (n = 0; n < this.main.length; ++n) {
            if (this.main[n] == null || !this.main[n].equals(stack)) continue;
            return true;
        }
        return false;
    }
}

