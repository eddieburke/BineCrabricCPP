/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.mob;

import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.Monster;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;
import net.minecraft.world.chunk.Chunk;

public class SlimeEntity
extends LivingEntity
implements Monster {
    public float stretch;
    public float lastStretch;
    private int ticksUntilJump = 0;

    public SlimeEntity(World arg) {
        super(arg);
        this.texture = "/mob/slime.png";
        int n = 1 << this.random.nextInt(3);
        this.standingEyeHeight = 0.0f;
        this.ticksUntilJump = this.random.nextInt(20) + 10;
        this.setSize(n);
    }

    protected void initDataTracker() {
        super.initDataTracker();
        this.dataTracker.startTracking(16, new Byte(1));
    }

    public void setSize(int size) {
        this.dataTracker.set(16, new Byte((byte)size));
        this.setBoundingBoxSpacing(0.6f * (float)size, 0.6f * (float)size);
        this.health = size * size;
        this.setPosition(this.x, this.y, this.z);
    }

    public int getSize() {
        return this.dataTracker.getByte(16);
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        nbt.putInt("Size", this.getSize() - 1);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        this.setSize(nbt.getInt("Size") + 1);
    }

    public void tick() {
        this.lastStretch = this.stretch;
        boolean bl = this.onGround;
        super.tick();
        if (this.onGround && !bl) {
            int n = this.getSize();
            for (int i = 0; i < n * 8; ++i) {
                float f = this.random.nextFloat() * (float)Math.PI * 2.0f;
                float f2 = this.random.nextFloat() * 0.5f + 0.5f;
                float f3 = MathHelper.sin(f) * (float)n * 0.5f * f2;
                float f4 = MathHelper.cos(f) * (float)n * 0.5f * f2;
                this.world.addParticle("slime", this.x + (double)f3, this.boundingBox.minY, this.z + (double)f4, 0.0, 0.0, 0.0);
            }
            if (n > 2) {
                this.world.playSound(this, "mob.slime", this.getSoundVolume(), ((this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f) / 0.8f);
            }
            this.stretch = -0.5f;
        }
        this.stretch *= 0.6f;
    }

    protected void tickLiving() {
        this.tryDespawn();
        PlayerEntity playerEntity = this.world.getClosestPlayer(this, 16.0);
        if (playerEntity != null) {
            this.lookAt(playerEntity, 10.0f, 20.0f);
        }
        if (this.onGround && this.ticksUntilJump-- <= 0) {
            this.ticksUntilJump = this.random.nextInt(20) + 10;
            if (playerEntity != null) {
                this.ticksUntilJump /= 3;
            }
            this.jumping = true;
            if (this.getSize() > 1) {
                this.world.playSound(this, "mob.slime", this.getSoundVolume(), ((this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f) * 0.8f);
            }
            this.stretch = 1.0f;
            this.sidewaysSpeed = 1.0f - this.random.nextFloat() * 2.0f;
            this.forwardSpeed = 1 * this.getSize();
        } else {
            this.jumping = false;
            if (this.onGround) {
                this.forwardSpeed = 0.0f;
                this.sidewaysSpeed = 0.0f;
            }
        }
    }

    public void markDead() {
        int n = this.getSize();
        if (!this.world.isRemote && n > 1 && this.health == 0) {
            for (int i = 0; i < 4; ++i) {
                float f = ((float)(i % 2) - 0.5f) * (float)n / 4.0f;
                float f2 = ((float)(i / 2) - 0.5f) * (float)n / 4.0f;
                SlimeEntity slimeEntity = new SlimeEntity(this.world);
                slimeEntity.setSize(n / 2);
                slimeEntity.setPositionAndAnglesKeepPrevAngles(this.x + (double)f, this.y + 0.5, this.z + (double)f2, this.random.nextFloat() * 360.0f, 0.0f);
                this.world.spawnEntity(slimeEntity);
            }
        }
        super.markDead();
    }

    public void onPlayerInteraction(PlayerEntity player) {
        int n = this.getSize();
        if (n > 1 && this.canSee(player) && (double)this.getDistance(player) < 0.6 * (double)n && player.damage(this, n)) {
            this.world.playSound(this, "mob.slimeattack", 1.0f, (this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f);
        }
    }

    protected String getHurtSound() {
        return "mob.slime";
    }

    protected String getDeathSound() {
        return "mob.slime";
    }

    protected int getDroppedItemId() {
        if (this.getSize() == 1) {
            return Item.SLIMEBALL.id;
        }
        return 0;
    }

    public boolean canSpawn() {
        Chunk chunk = this.world.getChunkFromPos(MathHelper.floor(this.x), MathHelper.floor(this.z));
        return (this.getSize() == 1 || this.world.difficulty > 0) && this.random.nextInt(10) == 0 && chunk.getSlimeRandom(987234911L).nextInt(10) == 0 && this.y < 16.0;
    }

    protected float getSoundVolume() {
        return 0.6f;
    }
}

