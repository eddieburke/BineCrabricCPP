/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.item;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.stat.Stats;
import net.minecraft.world.World;

public final class ItemStack {
    public int count = 0;
    public int bobbingAnimationTime;
    public int itemId;
    private int damage;

    public ItemStack(Block block) {
        this(block, 1);
    }

    public ItemStack(Block block, int count) {
        this(block.id, count, 0);
    }

    public ItemStack(Block block, int count, int damage) {
        this(block.id, count, damage);
    }

    public ItemStack(Item item) {
        this(item.id, 1, 0);
    }

    public ItemStack(Item item, int count) {
        this(item.id, count, 0);
    }

    public ItemStack(Item item, int count, int damage) {
        this(item.id, count, damage);
    }

    public ItemStack(int id, int count, int damage) {
        this.itemId = id;
        this.count = count;
        this.damage = damage;
    }

    public ItemStack(NbtCompound nbt) {
        this.readNbt(nbt);
    }

    public ItemStack split(int count) {
        this.count -= count;
        return new ItemStack(this.itemId, count, this.damage);
    }

    public Item getItem() {
        return Item.ITEMS[this.itemId];
    }

    @Environment(value=EnvType.CLIENT)
    public int getTextureId() {
        return this.getItem().getTextureId(this);
    }

    public boolean useOnBlock(PlayerEntity player, World world, int x, int y, int z, int side) {
        boolean bl = this.getItem().useOnBlock(this, player, world, x, y, z, side);
        if (bl) {
            player.increaseStat(Stats.USED[this.itemId], 1);
        }
        return bl;
    }

    public float getMiningSpeedMultiplier(Block block) {
        return this.getItem().getMiningSpeedMultiplier(this, block);
    }

    public ItemStack use(World world, PlayerEntity user) {
        return this.getItem().use(this, world, user);
    }

    public NbtCompound writeNbt(NbtCompound nbt) {
        nbt.putShort("id", (short)this.itemId);
        nbt.putByte("Count", (byte)this.count);
        nbt.putShort("Damage", (short)this.damage);
        return nbt;
    }

    public void readNbt(NbtCompound nbt) {
        this.itemId = nbt.getShort("id");
        this.count = nbt.getByte("Count");
        this.damage = nbt.getShort("Damage");
    }

    public int getMaxCount() {
        return this.getItem().getMaxCount();
    }

    public boolean isStackable() {
        return this.getMaxCount() > 1 && (!this.isDamageable() || !this.isDamaged());
    }

    public boolean isDamageable() {
        return Item.ITEMS[this.itemId].getMaxDamage() > 0;
    }

    public boolean hasSubtypes() {
        return Item.ITEMS[this.itemId].hasSubtypes();
    }

    public boolean isDamaged() {
        return this.isDamageable() && this.damage > 0;
    }

    public int getDamage2() {
        return this.damage;
    }

    public int getDamage() {
        return this.damage;
    }

    public void setDamage(int damage) {
        this.damage = damage;
    }

    public int getMaxDamage() {
        return Item.ITEMS[this.itemId].getMaxDamage();
    }

    public void damage(int amount, Entity entity) {
        if (!this.isDamageable()) {
            return;
        }
        this.damage += amount;
        if (this.damage > this.getMaxDamage()) {
            if (entity instanceof PlayerEntity) {
                ((PlayerEntity)entity).increaseStat(Stats.BROKEN[this.itemId], 1);
            }
            --this.count;
            if (this.count < 0) {
                this.count = 0;
            }
            this.damage = 0;
        }
    }

    public void postHit(LivingEntity target, PlayerEntity attacker) {
        boolean bl = Item.ITEMS[this.itemId].postHit(this, target, attacker);
        if (bl) {
            attacker.increaseStat(Stats.USED[this.itemId], 1);
        }
    }

    public void postMine(int blockId, int x, int y, int z, PlayerEntity miner) {
        boolean bl = Item.ITEMS[this.itemId].postMine(this, blockId, x, y, z, miner);
        if (bl) {
            miner.increaseStat(Stats.USED[this.itemId], 1);
        }
    }

    public int getAttackDamage(Entity attackedEntity) {
        return Item.ITEMS[this.itemId].getAttackDamage(attackedEntity);
    }

    public boolean isSuitableFor(Block block) {
        return Item.ITEMS[this.itemId].isSuitableFor(block);
    }

    public void onRemoved(PlayerEntity entity) {
    }

    public void useOnEntity(LivingEntity entity) {
        Item.ITEMS[this.itemId].useOnEntity(this, entity);
    }

    public ItemStack copy() {
        return new ItemStack(this.itemId, this.count, this.damage);
    }

    public static boolean areEqual(ItemStack left, ItemStack right) {
        if (left == null && right == null) {
            return true;
        }
        if (left == null || right == null) {
            return false;
        }
        return left.equals2(right);
    }

    private boolean equals2(ItemStack stack) {
        if (this.count != stack.count) {
            return false;
        }
        if (this.itemId != stack.itemId) {
            return false;
        }
        return this.damage == stack.damage;
    }

    public boolean isItemEqual(ItemStack stack) {
        return this.itemId == stack.itemId && this.damage == stack.damage;
    }

    @Environment(value=EnvType.CLIENT)
    public String getTranslationKey() {
        return Item.ITEMS[this.itemId].getTranslationKey(this);
    }

    public static ItemStack clone(ItemStack stack) {
        return stack == null ? null : stack.copy();
    }

    public String toString() {
        return this.count + "x" + Item.ITEMS[this.itemId].getTranslationKey() + "@" + this.damage;
    }

    public void inventoryTick(World world, Entity entity, int slot, boolean selected) {
        if (this.bobbingAnimationTime > 0) {
            --this.bobbingAnimationTime;
        }
        Item.ITEMS[this.itemId].inventoryTick(this, world, entity, slot, selected);
    }

    public void onCraft(World world, PlayerEntity player) {
        player.increaseStat(Stats.CRAFTED[this.itemId], this.count);
        Item.ITEMS[this.itemId].onCraft(this, world, player);
    }

    public boolean equals(ItemStack stack) {
        return this.itemId == stack.itemId && this.count == stack.count && this.damage == stack.damage;
    }
}

