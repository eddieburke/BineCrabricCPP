/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.passive;

import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.ai.pathing.Path;
import net.minecraft.entity.passive.AnimalEntity;
import net.minecraft.entity.passive.SheepEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.projectile.ArrowEntity;
import net.minecraft.item.FoodItem;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class WolfEntity
extends AnimalEntity {
    private boolean begging = false;
    private float begAnimationProgress;
    private float lastBegAnimationProcess;
    private boolean furWet;
    private boolean shakingWaterOff;
    private float shakeProgress;
    private float lastShakeProgress;

    public WolfEntity(World arg) {
        super(arg);
        this.texture = "/mob/wolf.png";
        this.setBoundingBoxSpacing(0.8f, 0.8f);
        this.movementSpeed = 1.1f;
        this.health = 8;
    }

    protected void initDataTracker() {
        super.initDataTracker();
        this.dataTracker.startTracking(16, (byte)0);
        this.dataTracker.startTracking(17, "");
        this.dataTracker.startTracking(18, new Integer(this.health));
    }

    protected boolean bypassesSteppingEffects() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public String getTexture() {
        if (this.isTamed()) {
            return "/mob/wolf_tame.png";
        }
        if (this.isAngry()) {
            return "/mob/wolf_angry.png";
        }
        return super.getTexture();
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        nbt.putBoolean("Angry", this.isAngry());
        nbt.putBoolean("Sitting", this.isInSittingPose());
        if (this.getOwnerName() == null) {
            nbt.putString("Owner", "");
        } else {
            nbt.putString("Owner", this.getOwnerName());
        }
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        this.setAngry(nbt.getBoolean("Angry"));
        this.setSitting(nbt.getBoolean("Sitting"));
        String string = nbt.getString("Owner");
        if (string.length() > 0) {
            this.setOwnerName(string);
            this.setTamed(true);
        }
    }

    protected boolean canDespawn() {
        return !this.isTamed();
    }

    protected String getRandomSound() {
        if (this.isAngry()) {
            return "mob.wolf.growl";
        }
        if (this.random.nextInt(3) == 0) {
            if (this.isTamed() && this.dataTracker.getInt(18) < 10) {
                return "mob.wolf.whine";
            }
            return "mob.wolf.panting";
        }
        return "mob.wolf.bark";
    }

    protected String getHurtSound() {
        return "mob.wolf.hurt";
    }

    protected String getDeathSound() {
        return "mob.wolf.death";
    }

    protected float getSoundVolume() {
        return 0.4f;
    }

    protected int getDroppedItemId() {
        return -1;
    }

    protected void tickLiving() {
        List list;
        super.tickLiving();
        if (!this.movementBlocked && !this.hasPath() && this.isTamed() && this.vehicle == null) {
            PlayerEntity playerEntity = this.world.getPlayer(this.getOwnerName());
            if (playerEntity != null) {
                float f = playerEntity.getDistance(this);
                if (f > 5.0f) {
                    this.damage((Entity)playerEntity, f);
                }
            } else if (!this.isSubmergedInWater()) {
                this.setSitting(true);
            }
        } else if (!(this.target != null || this.hasPath() || this.isTamed() || this.world.random.nextInt(100) != 0 || (list = this.world.collectEntitiesByClass(SheepEntity.class, Box.createCached(this.x, this.y, this.z, this.x + 1.0, this.y + 1.0, this.z + 1.0).expand(16.0, 4.0, 16.0))).isEmpty())) {
            this.setTarget((Entity)list.get(this.world.random.nextInt(list.size())));
        }
        if (this.isSubmergedInWater()) {
            this.setSitting(false);
        }
        if (!this.world.isRemote) {
            this.dataTracker.set(18, this.health);
        }
    }

    public void tickMovement() {
        Entity entity;
        super.tickMovement();
        this.begging = false;
        if (this.hasLookTarget() && !this.hasPath() && !this.isAngry() && (entity = this.getLookTarget()) instanceof PlayerEntity) {
            PlayerEntity playerEntity = (PlayerEntity)entity;
            ItemStack itemStack = playerEntity.inventory.getSelectedItem();
            if (itemStack != null) {
                if (!this.isTamed() && itemStack.itemId == Item.BONE.id) {
                    this.begging = true;
                } else if (this.isTamed() && Item.ITEMS[itemStack.itemId] instanceof FoodItem) {
                    this.begging = ((FoodItem)Item.ITEMS[itemStack.itemId]).isMeat();
                }
            }
        }
        if (!this.interpolateOnly && this.furWet && !this.shakingWaterOff && !this.hasPath() && this.onGround) {
            this.shakingWaterOff = true;
            this.shakeProgress = 0.0f;
            this.lastShakeProgress = 0.0f;
            this.world.broadcastEntityEvent(this, (byte)8);
        }
    }

    public void tick() {
        super.tick();
        this.lastBegAnimationProcess = this.begAnimationProgress;
        this.begAnimationProgress = this.begging ? (this.begAnimationProgress += (1.0f - this.begAnimationProgress) * 0.4f) : (this.begAnimationProgress += (0.0f - this.begAnimationProgress) * 0.4f);
        if (this.begging) {
            this.lookTimer = 10;
        }
        if (this.isWet()) {
            this.furWet = true;
            this.shakingWaterOff = false;
            this.shakeProgress = 0.0f;
            this.lastShakeProgress = 0.0f;
        } else if ((this.furWet || this.shakingWaterOff) && this.shakingWaterOff) {
            if (this.shakeProgress == 0.0f) {
                this.world.playSound(this, "mob.wolf.shake", this.getSoundVolume(), (this.random.nextFloat() - this.random.nextFloat()) * 0.2f + 1.0f);
            }
            this.lastShakeProgress = this.shakeProgress;
            this.shakeProgress += 0.05f;
            if (this.lastShakeProgress >= 2.0f) {
                this.furWet = false;
                this.shakingWaterOff = false;
                this.lastShakeProgress = 0.0f;
                this.shakeProgress = 0.0f;
            }
            if (this.shakeProgress > 0.4f) {
                float f = (float)this.boundingBox.minY;
                int n = (int)(MathHelper.sin((this.shakeProgress - 0.4f) * (float)Math.PI) * 7.0f);
                for (int i = 0; i < n; ++i) {
                    float f2 = (this.random.nextFloat() * 2.0f - 1.0f) * this.width * 0.5f;
                    float f3 = (this.random.nextFloat() * 2.0f - 1.0f) * this.width * 0.5f;
                    this.world.addParticle("splash", this.x + (double)f2, f + 0.8f, this.z + (double)f3, this.velocityX, this.velocityY, this.velocityZ);
                }
            }
        }
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isFurWet() {
        return this.furWet;
    }

    @Environment(value=EnvType.CLIENT)
    public float getFurBrightnessMultiplier(float tickDelta) {
        return 0.75f + (this.lastShakeProgress + (this.shakeProgress - this.lastShakeProgress) * tickDelta) / 2.0f * 0.25f;
    }

    @Environment(value=EnvType.CLIENT)
    public float getShakeAnimationProgress(float tickDelta, float offset) {
        float f = (this.lastShakeProgress + (this.shakeProgress - this.lastShakeProgress) * tickDelta + offset) / 1.8f;
        if (f < 0.0f) {
            f = 0.0f;
        } else if (f > 1.0f) {
            f = 1.0f;
        }
        return MathHelper.sin(f * (float)Math.PI) * MathHelper.sin(f * (float)Math.PI * 11.0f) * 0.15f * (float)Math.PI;
    }

    @Environment(value=EnvType.CLIENT)
    public float getBegAnimationProgress(float tickDelta) {
        return (this.lastBegAnimationProcess + (this.begAnimationProgress - this.lastBegAnimationProcess) * tickDelta) * 0.15f * (float)Math.PI;
    }

    public float getEyeHeight() {
        return this.height * 0.8f;
    }

    protected int getMaxLookPitchChange() {
        if (this.isInSittingPose()) {
            return 20;
        }
        return super.getMaxLookPitchChange();
    }

    private void damage(Entity source, float amount) {
        Path path = this.world.findPath(this, source, 16.0f);
        if (path == null && amount > 12.0f) {
            int n = MathHelper.floor(source.x) - 2;
            int n2 = MathHelper.floor(source.z) - 2;
            int n3 = MathHelper.floor(source.boundingBox.minY);
            for (int i = 0; i <= 4; ++i) {
                for (int j = 0; j <= 4; ++j) {
                    if (i >= 1 && j >= 1 && i <= 3 && j <= 3 || !this.world.shouldSuffocate(n + i, n3 - 1, n2 + j) || this.world.shouldSuffocate(n + i, n3, n2 + j) || this.world.shouldSuffocate(n + i, n3 + 1, n2 + j)) continue;
                    this.setPositionAndAnglesKeepPrevAngles((float)(n + i) + 0.5f, n3, (float)(n2 + j) + 0.5f, this.yaw, this.pitch);
                    return;
                }
            }
        } else {
            this.setPath(path);
        }
    }

    protected boolean isMovementBlocked() {
        return this.isInSittingPose() || this.shakingWaterOff;
    }

    public boolean damage(Entity damageSource, int amount) {
        this.setSitting(false);
        if (damageSource != null && !(damageSource instanceof PlayerEntity) && !(damageSource instanceof ArrowEntity)) {
            amount = (amount + 1) / 2;
        }
        if (super.damage(damageSource, amount)) {
            if (!this.isTamed() && !this.isAngry()) {
                if (damageSource instanceof PlayerEntity) {
                    this.setAngry(true);
                    this.target = damageSource;
                }
                if (damageSource instanceof ArrowEntity && ((ArrowEntity)damageSource).owner != null) {
                    damageSource = ((ArrowEntity)damageSource).owner;
                }
                if (damageSource instanceof LivingEntity) {
                    List list = this.world.collectEntitiesByClass(WolfEntity.class, Box.createCached(this.x, this.y, this.z, this.x + 1.0, this.y + 1.0, this.z + 1.0).expand(16.0, 4.0, 16.0));
                    for (Entity entity : list) {
                        WolfEntity wolfEntity = (WolfEntity)entity;
                        if (wolfEntity.isTamed() || wolfEntity.target != null) continue;
                        wolfEntity.target = damageSource;
                        if (!(damageSource instanceof PlayerEntity)) continue;
                        wolfEntity.setAngry(true);
                    }
                }
            } else if (damageSource != this && damageSource != null) {
                if (this.isTamed() && damageSource instanceof PlayerEntity && ((PlayerEntity)damageSource).name.equalsIgnoreCase(this.getOwnerName())) {
                    return true;
                }
                this.target = damageSource;
            }
            return true;
        }
        return false;
    }

    protected Entity getTargetInRange() {
        if (this.isAngry()) {
            return this.world.getClosestPlayer(this, 16.0);
        }
        return null;
    }

    protected void attack(Entity other, float distance) {
        if (distance > 2.0f && distance < 6.0f && this.random.nextInt(10) == 0) {
            if (this.onGround) {
                double d = other.x - this.x;
                double d2 = other.z - this.z;
                float f = MathHelper.sqrt(d * d + d2 * d2);
                this.velocityX = d / (double)f * 0.5 * (double)0.8f + this.velocityX * (double)0.2f;
                this.velocityZ = d2 / (double)f * 0.5 * (double)0.8f + this.velocityZ * (double)0.2f;
                this.velocityY = 0.4f;
            }
        } else if ((double)distance < 1.5 && other.boundingBox.maxY > this.boundingBox.minY && other.boundingBox.minY < this.boundingBox.maxY) {
            this.attackCooldown = 20;
            int n = 2;
            if (this.isTamed()) {
                n = 4;
            }
            other.damage(this, n);
        }
    }

    public boolean interact(PlayerEntity player) {
        ItemStack itemStack = player.inventory.getSelectedItem();
        if (!this.isTamed()) {
            if (itemStack != null && itemStack.itemId == Item.BONE.id && !this.isAngry()) {
                --itemStack.count;
                if (itemStack.count <= 0) {
                    player.inventory.setStack(player.inventory.selectedSlot, null);
                }
                if (!this.world.isRemote) {
                    if (this.random.nextInt(3) == 0) {
                        this.setTamed(true);
                        this.setPath(null);
                        this.setSitting(true);
                        this.health = 20;
                        this.setOwnerName(player.name);
                        this.showFeedParticles(true);
                        this.world.broadcastEntityEvent(this, (byte)7);
                    } else {
                        this.showFeedParticles(false);
                        this.world.broadcastEntityEvent(this, (byte)6);
                    }
                }
                return true;
            }
        } else {
            FoodItem foodItem;
            if (itemStack != null && Item.ITEMS[itemStack.itemId] instanceof FoodItem && (foodItem = (FoodItem)Item.ITEMS[itemStack.itemId]).isMeat() && this.dataTracker.getInt(18) < 20) {
                --itemStack.count;
                if (itemStack.count <= 0) {
                    player.inventory.setStack(player.inventory.selectedSlot, null);
                }
                this.heal(((FoodItem)Item.RAW_PORKCHOP).getHealthRestored());
                return true;
            }
            if (player.name.equalsIgnoreCase(this.getOwnerName())) {
                if (!this.world.isRemote) {
                    this.setSitting(!this.isInSittingPose());
                    this.jumping = false;
                    this.setPath(null);
                }
                return true;
            }
        }
        return false;
    }

    void showFeedParticles(boolean hearts) {
        String string = "heart";
        if (!hearts) {
            string = "smoke";
        }
        for (int i = 0; i < 7; ++i) {
            double d = this.random.nextGaussian() * 0.02;
            double d2 = this.random.nextGaussian() * 0.02;
            double d3 = this.random.nextGaussian() * 0.02;
            this.world.addParticle(string, this.x + (double)(this.random.nextFloat() * this.width * 2.0f) - (double)this.width, this.y + 0.5 + (double)(this.random.nextFloat() * this.height), this.z + (double)(this.random.nextFloat() * this.width * 2.0f) - (double)this.width, d, d2, d3);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public void processServerEntityStatus(byte status) {
        if (status == 7) {
            this.showFeedParticles(true);
        } else if (status == 6) {
            this.showFeedParticles(false);
        } else if (status == 8) {
            this.shakingWaterOff = true;
            this.shakeProgress = 0.0f;
            this.lastShakeProgress = 0.0f;
        } else {
            super.processServerEntityStatus(status);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public float getTailAngle() {
        if (this.isAngry()) {
            return 1.5393804f;
        }
        if (this.isTamed()) {
            return (0.55f - (float)(20 - this.dataTracker.getInt(18)) * 0.02f) * (float)Math.PI;
        }
        return 0.62831855f;
    }

    public int getLimitPerChunk() {
        return 8;
    }

    public String getOwnerName() {
        return this.dataTracker.getString(17);
    }

    public void setOwnerName(String owner) {
        this.dataTracker.set(17, owner);
    }

    public boolean isInSittingPose() {
        return (this.dataTracker.getByte(16) & 1) != 0;
    }

    public void setSitting(boolean inSittingPose) {
        byte by = this.dataTracker.getByte(16);
        if (inSittingPose) {
            this.dataTracker.set(16, (byte)(by | 1));
        } else {
            this.dataTracker.set(16, (byte)(by & 0xFFFFFFFE));
        }
    }

    public boolean isAngry() {
        return (this.dataTracker.getByte(16) & 2) != 0;
    }

    public void setAngry(boolean angry) {
        byte by = this.dataTracker.getByte(16);
        if (angry) {
            this.dataTracker.set(16, (byte)(by | 2));
        } else {
            this.dataTracker.set(16, (byte)(by & 0xFFFFFFFD));
        }
    }

    public boolean isTamed() {
        return (this.dataTracker.getByte(16) & 4) != 0;
    }

    public void setTamed(boolean tamed) {
        byte by = this.dataTracker.getByte(16);
        if (tamed) {
            this.dataTracker.set(16, (byte)(by | 4));
        } else {
            this.dataTracker.set(16, (byte)(by & 0xFFFFFFFB));
        }
    }
}

