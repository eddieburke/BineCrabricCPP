/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.mob;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.Entity;
import net.minecraft.entity.mob.ZombieEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.world.World;

public class PigZombieEntity
extends ZombieEntity {
    private int anger = 0;
    private int angrySoundDelay = 0;
    private static final ItemStack heldItem = new ItemStack(Item.GOLDEN_SWORD, 1);

    public PigZombieEntity(World arg) {
        super(arg);
        this.texture = "/mob/pigzombie.png";
        this.movementSpeed = 0.5f;
        this.attackDamage = 5;
        this.fireImmune = true;
    }

    public void tick() {
        float f = this.movementSpeed = this.target != null ? 0.95f : 0.5f;
        if (this.angrySoundDelay > 0 && --this.angrySoundDelay == 0) {
            this.world.playSound(this, "mob.zombiepig.zpigangry", this.getSoundVolume() * 2.0f, ((this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f) * 1.8f);
        }
        super.tick();
    }

    public boolean canSpawn() {
        return this.world.difficulty > 0 && this.world.canSpawnEntity(this.boundingBox) && this.world.getEntityCollisions(this, this.boundingBox).size() == 0 && !this.world.isBoxSubmergedInFluid(this.boundingBox);
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        nbt.putShort("Anger", (short)this.anger);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        this.anger = nbt.getShort("Anger");
    }

    protected Entity getTargetInRange() {
        if (this.anger == 0) {
            return null;
        }
        return super.getTargetInRange();
    }

    public void tickMovement() {
        super.tickMovement();
    }

    public boolean damage(Entity damageSource, int amount) {
        if (damageSource instanceof PlayerEntity) {
            List list = this.world.getEntities(this, this.boundingBox.expand(32.0, 32.0, 32.0));
            for (int i = 0; i < list.size(); ++i) {
                Entity entity = (Entity)list.get(i);
                if (!(entity instanceof PigZombieEntity)) continue;
                PigZombieEntity pigZombieEntity = (PigZombieEntity)entity;
                pigZombieEntity.makeAngry(damageSource);
            }
            this.makeAngry(damageSource);
        }
        return super.damage(damageSource, amount);
    }

    private void makeAngry(Entity target) {
        this.target = target;
        this.anger = 400 + this.random.nextInt(400);
        this.angrySoundDelay = this.random.nextInt(40);
    }

    protected String getRandomSound() {
        return "mob.zombiepig.zpig";
    }

    protected String getHurtSound() {
        return "mob.zombiepig.zpighurt";
    }

    protected String getDeathSound() {
        return "mob.zombiepig.zpigdeath";
    }

    protected int getDroppedItemId() {
        return Item.COOKED_PORKCHOP.id;
    }

    @Environment(value=EnvType.CLIENT)
    public ItemStack getHeldItem() {
        return heldItem;
    }
}

