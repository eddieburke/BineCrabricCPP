/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity;

import java.util.List;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.LiquidBlock;
import net.minecraft.block.material.Material;
import net.minecraft.entity.EntityRegistry;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.LightningEntity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.data.DataTracker;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtDouble;
import net.minecraft.nbt.NbtFloat;
import net.minecraft.nbt.NbtList;
import net.minecraft.sound.BlockSoundGroup;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;

public abstract class Entity {
    private static int nextId = 0;
    public int id = nextId++;
    public double renderDistanceMultiplier = 1.0;
    public boolean blocksSameBlockSpawning = false;
    public Entity passenger;
    public Entity vehicle;
    public World world;
    public double prevX;
    public double prevY;
    public double prevZ;
    public double x;
    public double y;
    public double z;
    public double velocityX;
    public double velocityY;
    public double velocityZ;
    public float yaw;
    public float pitch;
    public float prevYaw;
    public float prevPitch;
    public final Box boundingBox = Box.create(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    public boolean onGround = false;
    public boolean horizontalCollision;
    public boolean verticalCollision;
    public boolean hasCollided = false;
    public boolean velocityModified = false;
    public boolean slowed;
    public boolean keepVelocityOnCollision = true;
    public boolean dead = false;
    public float standingEyeHeight = 0.0f;
    public float width = 0.6f;
    public float height = 1.8f;
    public float prevHorizontalSpeed = 0.0f;
    public float horizontalSpeed = 0.0f;
    protected float fallDistance = 0.0f;
    private int nextStepSoundDistance = 1;
    public double lastTickX;
    public double lastTickY;
    public double lastTickZ;
    public float cameraOffset = 0.0f;
    public float stepHeight = 0.0f;
    public boolean noClip = false;
    public float pushSpeedReduction = 0.0f;
    protected Random random = new Random();
    public int age = 0;
    public int fireImmunityTicks = 1;
    public int fireTicks = 0;
    protected int maxAir = 300;
    protected boolean submergedInWater = false;
    public int hearts = 0;
    public int air = 300;
    private boolean firstTick = true;
    @Environment(value=EnvType.CLIENT)
    public String skinUrl;
    @Environment(value=EnvType.CLIENT)
    public String capeUrl;
    protected boolean fireImmune = false;
    protected DataTracker dataTracker = new DataTracker();
    public float minBrightness = 0.0f;
    private double vehiclePitchDelta;
    private double vehicleYawDelta;
    public boolean isPersistent = false;
    public int chunkX;
    public int chunkSlice;
    public int chunkZ;
    @Environment(value=EnvType.CLIENT)
    public int trackedPosX;
    @Environment(value=EnvType.CLIENT)
    public int trackedPosY;
    @Environment(value=EnvType.CLIENT)
    public int trackedPosZ;
    public boolean ignoreFrustumCull;

    public Entity(World world) {
        this.world = world;
        this.setPosition(0.0, 0.0, 0.0);
        this.dataTracker.startTracking(0, (byte)0);
        this.initDataTracker();
    }

    protected abstract void initDataTracker();

    public DataTracker getDataTracker() {
        return this.dataTracker;
    }

    public boolean equals(Object object) {
        if (object instanceof Entity) {
            return ((Entity)object).id == this.id;
        }
        return false;
    }

    public int hashCode() {
        return this.id;
    }

    @Environment(value=EnvType.CLIENT)
    protected void teleportTop() {
        if (this.world == null) {
            return;
        }
        while (this.y > 0.0) {
            this.setPosition(this.x, this.y, this.z);
            if (this.world.getEntityCollisions(this, this.boundingBox).size() == 0) break;
            this.y += 1.0;
        }
        this.velocityZ = 0.0;
        this.velocityY = 0.0;
        this.velocityX = 0.0;
        this.pitch = 0.0f;
    }

    public void markDead() {
        this.dead = true;
    }

    protected void setBoundingBoxSpacing(float spacingXZ, float spacingY) {
        this.width = spacingXZ;
        this.height = spacingY;
    }

    protected void setRotation(float yaw, float pitch) {
        this.yaw = yaw % 360.0f;
        this.pitch = pitch % 360.0f;
    }

    public void setPosition(double x, double y, double z) {
        this.x = x;
        this.y = y;
        this.z = z;
        float f = this.width / 2.0f;
        float f2 = this.height;
        this.boundingBox.set(x - (double)f, y - (double)this.standingEyeHeight + (double)this.cameraOffset, z - (double)f, x + (double)f, y - (double)this.standingEyeHeight + (double)this.cameraOffset + (double)f2, z + (double)f);
    }

    @Environment(value=EnvType.CLIENT)
    public void changeLookDirection(float cursorDeltaX, float cursorDeltaY) {
        float f = this.pitch;
        float f2 = this.yaw;
        this.yaw = (float)((double)this.yaw + (double)cursorDeltaX * 0.15);
        this.pitch = (float)((double)this.pitch - (double)cursorDeltaY * 0.15);
        if (this.pitch < -90.0f) {
            this.pitch = -90.0f;
        }
        if (this.pitch > 90.0f) {
            this.pitch = 90.0f;
        }
        this.prevPitch += this.pitch - f;
        this.prevYaw += this.yaw - f2;
    }

    public void tick() {
        this.baseTick();
    }

    public void baseTick() {
        if (this.vehicle != null && this.vehicle.dead) {
            this.vehicle = null;
        }
        ++this.age;
        this.prevHorizontalSpeed = this.horizontalSpeed;
        this.prevX = this.x;
        this.prevY = this.y;
        this.prevZ = this.z;
        this.prevPitch = this.pitch;
        this.prevYaw = this.yaw;
        if (this.checkWaterCollisions()) {
            if (!this.submergedInWater && !this.firstTick) {
                float f;
                float f2;
                float f3 = MathHelper.sqrt(this.velocityX * this.velocityX * (double)0.2f + this.velocityY * this.velocityY + this.velocityZ * this.velocityZ * (double)0.2f) * 0.2f;
                if (f3 > 1.0f) {
                    f3 = 1.0f;
                }
                this.world.playSound(this, "random.splash", f3, 1.0f + (this.random.nextFloat() - this.random.nextFloat()) * 0.4f);
                float f4 = MathHelper.floor(this.boundingBox.minY);
                int n = 0;
                while ((float)n < 1.0f + this.width * 20.0f) {
                    f2 = (this.random.nextFloat() * 2.0f - 1.0f) * this.width;
                    f = (this.random.nextFloat() * 2.0f - 1.0f) * this.width;
                    this.world.addParticle("bubble", this.x + (double)f2, f4 + 1.0f, this.z + (double)f, this.velocityX, this.velocityY - (double)(this.random.nextFloat() * 0.2f), this.velocityZ);
                    ++n;
                }
                n = 0;
                while ((float)n < 1.0f + this.width * 20.0f) {
                    f2 = (this.random.nextFloat() * 2.0f - 1.0f) * this.width;
                    f = (this.random.nextFloat() * 2.0f - 1.0f) * this.width;
                    this.world.addParticle("splash", this.x + (double)f2, f4 + 1.0f, this.z + (double)f, this.velocityX, this.velocityY, this.velocityZ);
                    ++n;
                }
            }
            this.fallDistance = 0.0f;
            this.submergedInWater = true;
            this.fireTicks = 0;
        } else {
            this.submergedInWater = false;
        }
        if (this.world.isRemote) {
            this.fireTicks = 0;
        } else if (this.fireTicks > 0) {
            if (this.fireImmune) {
                this.fireTicks -= 4;
                if (this.fireTicks < 0) {
                    this.fireTicks = 0;
                }
            } else {
                if (this.fireTicks % 20 == 0) {
                    this.damage(null, 1);
                }
                --this.fireTicks;
            }
        }
        if (this.isTouchingLava()) {
            this.setOnFire();
        }
        if (this.y < -64.0) {
            this.tickInVoid();
        }
        if (!this.world.isRemote) {
            this.setFlag(0, this.fireTicks > 0);
            this.setFlag(2, this.vehicle != null);
        }
        this.firstTick = false;
    }

    protected void setOnFire() {
        if (!this.fireImmune) {
            this.damage(null, 4);
            this.fireTicks = 600;
        }
    }

    protected void tickInVoid() {
        this.markDead();
    }

    public boolean getEntitiesInside(double offsetX, double offsetY, double offsetZ) {
        Box box = this.boundingBox.offset(offsetX, offsetY, offsetZ);
        List list = this.world.getEntityCollisions(this, box);
        if (list.size() > 0) {
            return false;
        }
        return !this.world.isBoxSubmergedInFluid(box);
    }

    public void move(double dx, double dy, double dz) {
        int n;
        int n2;
        int n3;
        int n4;
        int n5;
        int n6;
        int n7;
        double d;
        int n8;
        int n9;
        boolean bl;
        if (this.noClip) {
            this.boundingBox.translate(dx, dy, dz);
            this.x = (this.boundingBox.minX + this.boundingBox.maxX) / 2.0;
            this.y = this.boundingBox.minY + (double)this.standingEyeHeight - (double)this.cameraOffset;
            this.z = (this.boundingBox.minZ + this.boundingBox.maxZ) / 2.0;
            return;
        }
        this.cameraOffset *= 0.4f;
        double d2 = this.x;
        double d3 = this.z;
        if (this.slowed) {
            this.slowed = false;
            dx *= 0.25;
            dy *= (double)0.05f;
            dz *= 0.25;
            this.velocityX = 0.0;
            this.velocityY = 0.0;
            this.velocityZ = 0.0;
        }
        double d4 = dx;
        double d5 = dy;
        double d6 = dz;
        Box box = this.boundingBox.copy();
        boolean bl2 = bl = this.onGround && this.isSneaking();
        if (bl) {
            double d7 = 0.05;
            while (dx != 0.0 && this.world.getEntityCollisions(this, this.boundingBox.offset(dx, -1.0, 0.0)).size() == 0) {
                dx = dx < d7 && dx >= -d7 ? 0.0 : (dx > 0.0 ? (dx -= d7) : (dx += d7));
                d4 = dx;
            }
            while (dz != 0.0 && this.world.getEntityCollisions(this, this.boundingBox.offset(0.0, -1.0, dz)).size() == 0) {
                dz = dz < d7 && dz >= -d7 ? 0.0 : (dz > 0.0 ? (dz -= d7) : (dz += d7));
                d6 = dz;
            }
        }
        List list = this.world.getEntityCollisions(this, this.boundingBox.stretch(dx, dy, dz));
        for (n9 = 0; n9 < list.size(); ++n9) {
            dy = ((Box)list.get(n9)).getYOffset(this.boundingBox, dy);
        }
        this.boundingBox.translate(0.0, dy, 0.0);
        if (!this.keepVelocityOnCollision && d5 != dy) {
            dz = 0.0;
            dy = 0.0;
            dx = 0.0;
        }
        n9 = this.onGround || d5 != dy && d5 < 0.0 ? 1 : 0;
        for (n8 = 0; n8 < list.size(); ++n8) {
            dx = ((Box)list.get(n8)).getXOffset(this.boundingBox, dx);
        }
        this.boundingBox.translate(dx, 0.0, 0.0);
        if (!this.keepVelocityOnCollision && d4 != dx) {
            dz = 0.0;
            dy = 0.0;
            dx = 0.0;
        }
        for (n8 = 0; n8 < list.size(); ++n8) {
            dz = ((Box)list.get(n8)).getZOffset(this.boundingBox, dz);
        }
        this.boundingBox.translate(0.0, 0.0, dz);
        if (!this.keepVelocityOnCollision && d6 != dz) {
            dz = 0.0;
            dy = 0.0;
            dx = 0.0;
        }
        if (this.stepHeight > 0.0f && n9 != 0 && (bl || this.cameraOffset < 0.05f) && (d4 != dx || d6 != dz)) {
            int n10;
            double d8 = dx;
            d = dy;
            double d9 = dz;
            dx = d4;
            dy = this.stepHeight;
            dz = d6;
            Box box2 = this.boundingBox.copy();
            this.boundingBox.clone(box);
            list = this.world.getEntityCollisions(this, this.boundingBox.stretch(dx, dy, dz));
            for (n10 = 0; n10 < list.size(); ++n10) {
                dy = ((Box)list.get(n10)).getYOffset(this.boundingBox, dy);
            }
            this.boundingBox.translate(0.0, dy, 0.0);
            if (!this.keepVelocityOnCollision && d5 != dy) {
                dz = 0.0;
                dy = 0.0;
                dx = 0.0;
            }
            for (n10 = 0; n10 < list.size(); ++n10) {
                dx = ((Box)list.get(n10)).getXOffset(this.boundingBox, dx);
            }
            this.boundingBox.translate(dx, 0.0, 0.0);
            if (!this.keepVelocityOnCollision && d4 != dx) {
                dz = 0.0;
                dy = 0.0;
                dx = 0.0;
            }
            for (n10 = 0; n10 < list.size(); ++n10) {
                dz = ((Box)list.get(n10)).getZOffset(this.boundingBox, dz);
            }
            this.boundingBox.translate(0.0, 0.0, dz);
            if (!this.keepVelocityOnCollision && d6 != dz) {
                dz = 0.0;
                dy = 0.0;
                dx = 0.0;
            }
            if (!this.keepVelocityOnCollision && d5 != dy) {
                dz = 0.0;
                dy = 0.0;
                dx = 0.0;
            } else {
                dy = -this.stepHeight;
                for (n10 = 0; n10 < list.size(); ++n10) {
                    dy = ((Box)list.get(n10)).getYOffset(this.boundingBox, dy);
                }
                this.boundingBox.translate(0.0, dy, 0.0);
            }
            if (d8 * d8 + d9 * d9 >= dx * dx + dz * dz) {
                dx = d8;
                dy = d;
                dz = d9;
                this.boundingBox.clone(box2);
            } else {
                double d10 = this.boundingBox.minY - (double)((int)this.boundingBox.minY);
                if (d10 > 0.0) {
                    this.cameraOffset = (float)((double)this.cameraOffset + (d10 + 0.01));
                }
            }
        }
        this.x = (this.boundingBox.minX + this.boundingBox.maxX) / 2.0;
        this.y = this.boundingBox.minY + (double)this.standingEyeHeight - (double)this.cameraOffset;
        this.z = (this.boundingBox.minZ + this.boundingBox.maxZ) / 2.0;
        this.horizontalCollision = d4 != dx || d6 != dz;
        this.verticalCollision = d5 != dy;
        this.onGround = d5 != dy && d5 < 0.0;
        this.hasCollided = this.horizontalCollision || this.verticalCollision;
        this.fall(dy, this.onGround);
        if (d4 != dx) {
            this.velocityX = 0.0;
        }
        if (d5 != dy) {
            this.velocityY = 0.0;
        }
        if (d6 != dz) {
            this.velocityZ = 0.0;
        }
        double d11 = this.x - d2;
        d = this.z - d3;
        if (this.bypassesSteppingEffects() && !bl && this.vehicle == null) {
            this.horizontalSpeed = (float)((double)this.horizontalSpeed + (double)MathHelper.sqrt(d11 * d11 + d * d) * 0.6);
            int n11 = MathHelper.floor(this.x);
            n7 = MathHelper.floor(this.y - (double)0.2f - (double)this.standingEyeHeight);
            int n12 = MathHelper.floor(this.z);
            int n13 = this.world.getBlockId(n11, n7, n12);
            if (this.world.getBlockId(n11, n7 - 1, n12) == Block.FENCE.id) {
                n13 = this.world.getBlockId(n11, n7 - 1, n12);
            }
            if (this.horizontalSpeed > (float)this.nextStepSoundDistance && n13 > 0) {
                ++this.nextStepSoundDistance;
                BlockSoundGroup blockSoundGroup = Block.BLOCKS[n13].soundGroup;
                if (this.world.getBlockId(n11, n7 + 1, n12) == Block.SNOW.id) {
                    blockSoundGroup = Block.SNOW.soundGroup;
                    this.world.playSound(this, blockSoundGroup.getSound(), blockSoundGroup.getVolume() * 0.15f, blockSoundGroup.getPitch());
                } else if (!Block.BLOCKS[n13].material.isFluid()) {
                    this.world.playSound(this, blockSoundGroup.getSound(), blockSoundGroup.getVolume() * 0.15f, blockSoundGroup.getPitch());
                }
                Block.BLOCKS[n13].onSteppedOn(this.world, n11, n7, n12, this);
            }
        }
        if (this.world.isRegionLoaded(n6 = MathHelper.floor(this.boundingBox.minX + 0.001), n7 = MathHelper.floor(this.boundingBox.minY + 0.001), n5 = MathHelper.floor(this.boundingBox.minZ + 0.001), n4 = MathHelper.floor(this.boundingBox.maxX - 0.001), n3 = MathHelper.floor(this.boundingBox.maxY - 0.001), n2 = MathHelper.floor(this.boundingBox.maxZ - 0.001))) {
            for (n = n6; n <= n4; ++n) {
                for (int i = n7; i <= n3; ++i) {
                    for (int j = n5; j <= n2; ++j) {
                        int n14 = this.world.getBlockId(n, i, j);
                        if (n14 <= 0) continue;
                        Block.BLOCKS[n14].onEntityCollision(this.world, n, i, j, this);
                    }
                }
            }
        }
        n = this.isWet() ? 1 : 0;
        if (this.world.isFireOrLavaInBox(this.boundingBox.contract(0.001, 0.001, 0.001))) {
            this.damage(1);
            if (n == 0) {
                ++this.fireTicks;
                if (this.fireTicks == 0) {
                    this.fireTicks = 300;
                }
            }
        } else if (this.fireTicks <= 0) {
            this.fireTicks = -this.fireImmunityTicks;
        }
        if (n != 0 && this.fireTicks > 0) {
            this.world.playSound(this, "random.fizz", 0.7f, 1.6f + (this.random.nextFloat() - this.random.nextFloat()) * 0.4f);
            this.fireTicks = -this.fireImmunityTicks;
        }
    }

    protected boolean bypassesSteppingEffects() {
        return true;
    }

    protected void fall(double heightDifference, boolean onGround) {
        if (onGround) {
            if (this.fallDistance > 0.0f) {
                this.onLanding(this.fallDistance);
                this.fallDistance = 0.0f;
            }
        } else if (heightDifference < 0.0) {
            this.fallDistance = (float)((double)this.fallDistance - heightDifference);
        }
    }

    public Box getBoundingBox() {
        return null;
    }

    protected void damage(int amount) {
        if (!this.fireImmune) {
            this.damage(null, amount);
        }
    }

    protected void onLanding(float fallDistance) {
        if (this.passenger != null) {
            this.passenger.onLanding(fallDistance);
        }
    }

    public boolean isWet() {
        return this.submergedInWater || this.world.isRaining(MathHelper.floor(this.x), MathHelper.floor(this.y), MathHelper.floor(this.z));
    }

    public boolean isSubmergedInWater() {
        return this.submergedInWater;
    }

    public boolean checkWaterCollisions() {
        return this.world.updateMovementInFluid(this.boundingBox.expand(0.0, -0.4f, 0.0).contract(0.001, 0.001, 0.001), Material.WATER, this);
    }

    public boolean isInFluid(Material material) {
        int n;
        int n2;
        double d = this.y + (double)this.getEyeHeight();
        int n3 = MathHelper.floor(this.x);
        int n4 = this.world.getBlockId(n3, n2 = MathHelper.floor(MathHelper.floor(d)), n = MathHelper.floor(this.z));
        if (n4 != 0 && Block.BLOCKS[n4].material == material) {
            float f = LiquidBlock.getFluidHeightFromMeta(this.world.getBlockMeta(n3, n2, n)) - 0.11111111f;
            float f2 = (float)(n2 + 1) - f;
            return d < (double)f2;
        }
        return false;
    }

    public float getEyeHeight() {
        return 0.0f;
    }

    public boolean isTouchingLava() {
        return this.world.isMaterialInBox(this.boundingBox.expand(-0.1f, -0.4f, -0.1f), Material.LAVA);
    }

    public void moveNonSolid(float f, float g, float h) {
        float f2 = MathHelper.sqrt(f * f + g * g);
        if (f2 < 0.01f) {
            return;
        }
        if (f2 < 1.0f) {
            f2 = 1.0f;
        }
        f2 = h / f2;
        float f3 = MathHelper.sin(this.yaw * (float)Math.PI / 180.0f);
        float f4 = MathHelper.cos(this.yaw * (float)Math.PI / 180.0f);
        this.velocityX += (double)((f *= f2) * f4 - (g *= f2) * f3);
        this.velocityZ += (double)(g * f4 + f * f3);
    }

    public float getBrightnessAtEyes(float tickDelta) {
        int n = MathHelper.floor(this.x);
        double d = (this.boundingBox.maxY - this.boundingBox.minY) * 0.66;
        int n2 = MathHelper.floor(this.y - (double)this.standingEyeHeight + d);
        int n3 = MathHelper.floor(this.z);
        if (this.world.isRegionLoaded(MathHelper.floor(this.boundingBox.minX), MathHelper.floor(this.boundingBox.minY), MathHelper.floor(this.boundingBox.minZ), MathHelper.floor(this.boundingBox.maxX), MathHelper.floor(this.boundingBox.maxY), MathHelper.floor(this.boundingBox.maxZ))) {
            float f = this.world.getLightBrightness(n, n2, n3);
            if (f < this.minBrightness) {
                f = this.minBrightness;
            }
            return f;
        }
        return this.minBrightness;
    }

    public void setWorld(World world) {
        this.world = world;
    }

    public void setPositionAndAngles(double x, double y, double z, float yaw, float pitch) {
        this.prevX = this.x = x;
        this.prevY = this.y = y;
        this.prevZ = this.z = z;
        this.prevYaw = this.yaw = yaw;
        this.prevPitch = this.pitch = pitch;
        this.cameraOffset = 0.0f;
        double d = this.prevYaw - yaw;
        if (d < -180.0) {
            this.prevYaw += 360.0f;
        }
        if (d >= 180.0) {
            this.prevYaw -= 360.0f;
        }
        this.setPosition(this.x, this.y, this.z);
        this.setRotation(yaw, pitch);
    }

    public void setPositionAndAnglesKeepPrevAngles(double x, double y, double z, float yaw, float pitch) {
        this.prevX = this.x = x;
        this.lastTickX = this.x;
        this.prevY = this.y = y + (double)this.standingEyeHeight;
        this.lastTickY = this.y;
        this.prevZ = this.z = z;
        this.lastTickZ = this.z;
        this.yaw = yaw;
        this.pitch = pitch;
        this.setPosition(this.x, this.y, this.z);
    }

    public float getDistance(Entity entity) {
        float f = (float)(this.x - entity.x);
        float f2 = (float)(this.y - entity.y);
        float f3 = (float)(this.z - entity.z);
        return MathHelper.sqrt(f * f + f2 * f2 + f3 * f3);
    }

    public double getSquaredDistance(double x, double y, double z) {
        double d = this.x - x;
        double d2 = this.y - y;
        double d3 = this.z - z;
        return d * d + d2 * d2 + d3 * d3;
    }

    public double getDistance(double x, double y, double z) {
        double d = this.x - x;
        double d2 = this.y - y;
        double d3 = this.z - z;
        return MathHelper.sqrt(d * d + d2 * d2 + d3 * d3);
    }

    public double getSquaredDistance(Entity entity) {
        double d = this.x - entity.x;
        double d2 = this.y - entity.y;
        double d3 = this.z - entity.z;
        return d * d + d2 * d2 + d3 * d3;
    }

    public void onPlayerInteraction(PlayerEntity player) {
    }

    public void onCollision(Entity otherEntity) {
        if (otherEntity.passenger == this || otherEntity.vehicle == this) {
            return;
        }
        double d = otherEntity.x - this.x;
        double d2 = otherEntity.z - this.z;
        double d3 = MathHelper.absMax(d, d2);
        if (d3 >= (double)0.01f) {
            d3 = MathHelper.sqrt(d3);
            d /= d3;
            d2 /= d3;
            double d4 = 1.0 / d3;
            if (d4 > 1.0) {
                d4 = 1.0;
            }
            d *= d4;
            d2 *= d4;
            d *= (double)0.05f;
            d2 *= (double)0.05f;
            this.addVelocity(-(d *= (double)(1.0f - this.pushSpeedReduction)), 0.0, -(d2 *= (double)(1.0f - this.pushSpeedReduction)));
            otherEntity.addVelocity(d, 0.0, d2);
        }
    }

    public void addVelocity(double x, double y, double z) {
        this.velocityX += x;
        this.velocityY += y;
        this.velocityZ += z;
    }

    protected void scheduleVelocityUpdate() {
        this.velocityModified = true;
    }

    public boolean damage(Entity damageSource, int amount) {
        this.scheduleVelocityUpdate();
        return false;
    }

    public boolean isCollidable() {
        return false;
    }

    public boolean isPushable() {
        return false;
    }

    public void updateKilledAchievement(Entity entityKilled, int score) {
    }

    @Environment(value=EnvType.CLIENT)
    public boolean shouldRender(Vec3d pos) {
        double d = this.x - pos.x;
        double d2 = this.y - pos.y;
        double d3 = this.z - pos.z;
        double d4 = d * d + d2 * d2 + d3 * d3;
        return this.shouldRender(d4);
    }

    @Environment(value=EnvType.CLIENT)
    public boolean shouldRender(double distance) {
        double d = this.boundingBox.getAverageSideLength();
        return distance < (d *= 64.0 * this.renderDistanceMultiplier) * d;
    }

    @Environment(value=EnvType.CLIENT)
    public String getTexture() {
        return null;
    }

    public boolean saveSelfNbt(NbtCompound nbt) {
        String string = this.getRegistryEntry();
        if (this.dead || string == null) {
            return false;
        }
        nbt.putString("id", string);
        this.write(nbt);
        return true;
    }

    public void write(NbtCompound nbt) {
        nbt.put("Pos", this.toNbtList(this.x, this.y + (double)this.cameraOffset, this.z));
        nbt.put("Motion", this.toNbtList(this.velocityX, this.velocityY, this.velocityZ));
        nbt.put("Rotation", this.toNbtList(this.yaw, this.pitch));
        nbt.putFloat("FallDistance", this.fallDistance);
        nbt.putShort("Fire", (short)this.fireTicks);
        nbt.putShort("Air", (short)this.air);
        nbt.putBoolean("OnGround", this.onGround);
        this.writeNbt(nbt);
    }

    public void read(NbtCompound nbt) {
        NbtList nbtList = nbt.getList("Pos");
        NbtList nbtList2 = nbt.getList("Motion");
        NbtList nbtList3 = nbt.getList("Rotation");
        this.velocityX = ((NbtDouble)nbtList2.get((int)0)).value;
        this.velocityY = ((NbtDouble)nbtList2.get((int)1)).value;
        this.velocityZ = ((NbtDouble)nbtList2.get((int)2)).value;
        if (Math.abs(this.velocityX) > 10.0) {
            this.velocityX = 0.0;
        }
        if (Math.abs(this.velocityY) > 10.0) {
            this.velocityY = 0.0;
        }
        if (Math.abs(this.velocityZ) > 10.0) {
            this.velocityZ = 0.0;
        }
        this.lastTickX = this.x = ((NbtDouble)nbtList.get((int)0)).value;
        this.prevX = this.x;
        this.lastTickY = this.y = ((NbtDouble)nbtList.get((int)1)).value;
        this.prevY = this.y;
        this.lastTickZ = this.z = ((NbtDouble)nbtList.get((int)2)).value;
        this.prevZ = this.z;
        this.prevYaw = this.yaw = ((NbtFloat)nbtList3.get((int)0)).value;
        this.prevPitch = this.pitch = ((NbtFloat)nbtList3.get((int)1)).value;
        this.fallDistance = nbt.getFloat("FallDistance");
        this.fireTicks = nbt.getShort("Fire");
        this.air = nbt.getShort("Air");
        this.onGround = nbt.getBoolean("OnGround");
        this.setPosition(this.x, this.y, this.z);
        this.setRotation(this.yaw, this.pitch);
        this.readNbt(nbt);
    }

    protected final String getRegistryEntry() {
        return EntityRegistry.getId(this);
    }

    protected abstract void readNbt(NbtCompound var1);

    protected abstract void writeNbt(NbtCompound var1);

    protected NbtList toNbtList(double ... values) {
        NbtList nbtList = new NbtList();
        for (double d : values) {
            nbtList.add(new NbtDouble(d));
        }
        return nbtList;
    }

    protected NbtList toNbtList(float ... values) {
        NbtList nbtList = new NbtList();
        for (float f : values) {
            nbtList.add(new NbtFloat(f));
        }
        return nbtList;
    }

    @Environment(value=EnvType.CLIENT)
    public float getShadowRadius() {
        return this.height / 2.0f;
    }

    public ItemEntity dropItem(int id, int amount) {
        return this.dropItem(id, amount, 0.0f);
    }

    public ItemEntity dropItem(int id, int amount, float yOffset) {
        return this.dropItem(new ItemStack(id, amount, 0), yOffset);
    }

    public ItemEntity dropItem(ItemStack itemStack, float yOffset) {
        ItemEntity itemEntity = new ItemEntity(this.world, this.x, this.y + (double)yOffset, this.z, itemStack);
        itemEntity.pickupDelay = 10;
        this.world.spawnEntity(itemEntity);
        return itemEntity;
    }

    public boolean isAlive() {
        return !this.dead;
    }

    public boolean isInsideWall() {
        for (int i = 0; i < 8; ++i) {
            int n;
            int n2;
            float f = ((float)((i >> 0) % 2) - 0.5f) * this.width * 0.9f;
            float f2 = ((float)((i >> 1) % 2) - 0.5f) * 0.1f;
            float f3 = ((float)((i >> 2) % 2) - 0.5f) * this.width * 0.9f;
            int n3 = MathHelper.floor(this.x + (double)f);
            if (!this.world.shouldSuffocate(n3, n2 = MathHelper.floor(this.y + (double)this.getEyeHeight() + (double)f2), n = MathHelper.floor(this.z + (double)f3))) continue;
            return true;
        }
        return false;
    }

    public boolean interact(PlayerEntity player) {
        return false;
    }

    public Box getCollisionAgainstShape(Entity other) {
        return null;
    }

    public void tickRiding() {
        if (this.vehicle.dead) {
            this.vehicle = null;
            return;
        }
        this.velocityX = 0.0;
        this.velocityY = 0.0;
        this.velocityZ = 0.0;
        this.tick();
        if (this.vehicle == null) {
            return;
        }
        this.vehicle.updatePassengerPosition();
        this.vehicleYawDelta += (double)(this.vehicle.yaw - this.vehicle.prevYaw);
        this.vehiclePitchDelta += (double)(this.vehicle.pitch - this.vehicle.prevPitch);
        while (this.vehicleYawDelta >= 180.0) {
            this.vehicleYawDelta -= 360.0;
        }
        while (this.vehicleYawDelta < -180.0) {
            this.vehicleYawDelta += 360.0;
        }
        while (this.vehiclePitchDelta >= 180.0) {
            this.vehiclePitchDelta -= 360.0;
        }
        while (this.vehiclePitchDelta < -180.0) {
            this.vehiclePitchDelta += 360.0;
        }
        double d = this.vehicleYawDelta * 0.5;
        double d2 = this.vehiclePitchDelta * 0.5;
        float f = 10.0f;
        if (d > (double)f) {
            d = f;
        }
        if (d < (double)(-f)) {
            d = -f;
        }
        if (d2 > (double)f) {
            d2 = f;
        }
        if (d2 < (double)(-f)) {
            d2 = -f;
        }
        this.vehicleYawDelta -= d;
        this.vehiclePitchDelta -= d2;
        this.yaw = (float)((double)this.yaw + d);
        this.pitch = (float)((double)this.pitch + d2);
    }

    public void updatePassengerPosition() {
        this.passenger.setPosition(this.x, this.y + this.getPassengerRidingHeight() + this.passenger.getStandingEyeHeight(), this.z);
    }

    public double getStandingEyeHeight() {
        return this.standingEyeHeight;
    }

    public double getPassengerRidingHeight() {
        return (double)this.height * 0.75;
    }

    public void setVehicle(Entity entity) {
        this.vehiclePitchDelta = 0.0;
        this.vehicleYawDelta = 0.0;
        if (entity == null) {
            if (this.vehicle != null) {
                this.setPositionAndAnglesKeepPrevAngles(this.vehicle.x, this.vehicle.boundingBox.minY + (double)this.vehicle.height, this.vehicle.z, this.yaw, this.pitch);
                this.vehicle.passenger = null;
            }
            this.vehicle = null;
            return;
        }
        if (this.vehicle == entity) {
            this.vehicle.passenger = null;
            this.vehicle = null;
            this.setPositionAndAnglesKeepPrevAngles(entity.x, entity.boundingBox.minY + (double)entity.height, entity.z, this.yaw, this.pitch);
            return;
        }
        if (this.vehicle != null) {
            this.vehicle.passenger = null;
        }
        if (entity.passenger != null) {
            entity.passenger.vehicle = null;
        }
        this.vehicle = entity;
        entity.passenger = this;
    }

    @Environment(value=EnvType.CLIENT)
    public void setPositionAndAnglesAvoidEntities(double x, double y, double z, float pitch, float yaw, int interpolationSteps) {
        this.setPosition(x, y, z);
        this.setRotation(pitch, yaw);
        List list = this.world.getEntityCollisions(this, this.boundingBox.contract(0.03125, 0.0, 0.03125));
        if (list.size() > 0) {
            double d = 0.0;
            for (int i = 0; i < list.size(); ++i) {
                Box box = (Box)list.get(i);
                if (!(box.maxY > d)) continue;
                d = box.maxY;
            }
            this.setPosition(x, y += d - this.boundingBox.minY, z);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public float getTargetingMargin() {
        return 0.1f;
    }

    public Vec3d getLookVector() {
        return null;
    }

    public void tickPortalCooldown() {
    }

    @Environment(value=EnvType.CLIENT)
    public void setVelocityClient(double x, double y, double z) {
        this.velocityX = x;
        this.velocityY = y;
        this.velocityZ = z;
    }

    @Environment(value=EnvType.CLIENT)
    public void processServerEntityStatus(byte status) {
    }

    @Environment(value=EnvType.CLIENT)
    public void animateHurt() {
    }

    @Environment(value=EnvType.CLIENT)
    public void updateCapeUrl() {
    }

    @Environment(value=EnvType.CLIENT)
    public void setEquipmentStack(int armorSlot, int itemId, int meta) {
    }

    @Environment(value=EnvType.CLIENT)
    public boolean isOnFire() {
        return this.fireTicks > 0 || this.getFlag(0);
    }

    @Environment(value=EnvType.CLIENT)
    public boolean hasVehicle() {
        return this.vehicle != null || this.getFlag(2);
    }

    @Environment(value=EnvType.SERVER)
    public ItemStack[] getEquipment() {
        return null;
    }

    public boolean isSneaking() {
        return this.getFlag(1);
    }

    @Environment(value=EnvType.SERVER)
    public void setSneaking(boolean sneaking) {
        this.setFlag(1, sneaking);
    }

    protected boolean getFlag(int index) {
        return (this.dataTracker.getByte(0) & 1 << index) != 0;
    }

    protected void setFlag(int index, boolean value) {
        byte by = this.dataTracker.getByte(0);
        if (value) {
            this.dataTracker.set(0, (byte)(by | 1 << index));
        } else {
            this.dataTracker.set(0, (byte)(by & ~(1 << index)));
        }
    }

    public void onStruckByLightning(LightningEntity lightning) {
        this.damage(5);
        ++this.fireTicks;
        if (this.fireTicks == 0) {
            this.fireTicks = 300;
        }
    }

    public void onKilledOther(LivingEntity other) {
    }

    protected boolean pushOutOfBlock(double x, double y, double z) {
        int n = MathHelper.floor(x);
        int n2 = MathHelper.floor(y);
        int n3 = MathHelper.floor(z);
        double d = x - (double)n;
        double d2 = y - (double)n2;
        double d3 = z - (double)n3;
        if (this.world.shouldSuffocate(n, n2, n3)) {
            boolean bl = !this.world.shouldSuffocate(n - 1, n2, n3);
            boolean bl2 = !this.world.shouldSuffocate(n + 1, n2, n3);
            boolean bl3 = !this.world.shouldSuffocate(n, n2 - 1, n3);
            boolean bl4 = !this.world.shouldSuffocate(n, n2 + 1, n3);
            boolean bl5 = !this.world.shouldSuffocate(n, n2, n3 - 1);
            boolean bl6 = !this.world.shouldSuffocate(n, n2, n3 + 1);
            int n4 = -1;
            double d4 = 9999.0;
            if (bl && d < d4) {
                d4 = d;
                n4 = 0;
            }
            if (bl2 && 1.0 - d < d4) {
                d4 = 1.0 - d;
                n4 = 1;
            }
            if (bl3 && d2 < d4) {
                d4 = d2;
                n4 = 2;
            }
            if (bl4 && 1.0 - d2 < d4) {
                d4 = 1.0 - d2;
                n4 = 3;
            }
            if (bl5 && d3 < d4) {
                d4 = d3;
                n4 = 4;
            }
            if (bl6 && 1.0 - d3 < d4) {
                d4 = 1.0 - d3;
                n4 = 5;
            }
            float f = this.random.nextFloat() * 0.2f + 0.1f;
            if (n4 == 0) {
                this.velocityX = -f;
            }
            if (n4 == 1) {
                this.velocityX = f;
            }
            if (n4 == 2) {
                this.velocityY = -f;
            }
            if (n4 == 3) {
                this.velocityY = f;
            }
            if (n4 == 4) {
                this.velocityZ = -f;
            }
            if (n4 == 5) {
                this.velocityZ = f;
            }
        }
        return false;
    }
}

