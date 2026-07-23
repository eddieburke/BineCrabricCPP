/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.passive;

import net.minecraft.entity.passive.AnimalEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.world.World;

public class CowEntity
extends AnimalEntity {
    public CowEntity(World arg) {
        super(arg);
        this.texture = "/mob/cow.png";
        this.setBoundingBoxSpacing(0.9f, 1.3f);
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
    }

    protected String getRandomSound() {
        return "mob.cow";
    }

    protected String getHurtSound() {
        return "mob.cowhurt";
    }

    protected String getDeathSound() {
        return "mob.cowhurt";
    }

    protected float getSoundVolume() {
        return 0.4f;
    }

    protected int getDroppedItemId() {
        return Item.LEATHER.id;
    }

    public boolean interact(PlayerEntity player) {
        ItemStack itemStack = player.inventory.getSelectedItem();
        if (itemStack != null && itemStack.itemId == Item.BUCKET.id) {
            player.inventory.setStack(player.inventory.selectedSlot, new ItemStack(Item.MILK_BUCKET));
            return true;
        }
        return false;
    }
}

