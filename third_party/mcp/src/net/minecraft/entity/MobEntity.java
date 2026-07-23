/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity;

import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.ai.pathing.Path;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;

public class MobEntity
extends LivingEntity {
    private Path path;
    protected Entity target;
    protected boolean movementBlocked = false;

    public MobEntity(World arg) {
        super(arg);
    }

    protected boolean isMovementBlocked() {
        return false;
    }

    protected void tickLiving() {
        this.movementBlocked = this.isMovementBlocked();
        float f = 16.0f;
        if (this.target == null) {
            this.target = this.getTargetInRange();
            if (this.target != null) {
                this.path = this.world.findPath(this, this.target, f);
            }
        } else if (!this.target.isAlive()) {
            this.target = null;
        } else {
            float f2 = this.target.getDistance(this);
            if (this.canSee(this.target)) {
                this.attack(this.target, f2);
            } else {
                this.resetAttack(this.target, f2);
            }
        }
        if (!(this.movementBlocked || this.target == null || this.path != null && this.random.nextInt(20) != 0)) {
            this.path = this.world.findPath(this, this.target, f);
        } else if (!this.movementBlocked && (this.path == null && this.random.nextInt(80) == 0 || this.random.nextInt(80) == 0)) {
            this.pathingUpdate();
        }
        int n = MathHelper.floor(this.boundingBox.minY + 0.5);
        boolean bl = this.isSubmergedInWater();
        boolean bl2 = this.isTouchingLava();
        this.pitch = 0.0f;
        if (this.path == null || this.random.nextInt(100) == 0) {
            super.tickLiving();
            this.path = null;
            return;
        }
        Vec3d vec3d = this.path.getNodePosition(this);
        double d = this.width * 2.0f;
        while (vec3d != null && vec3d.squaredDistanceTo(this.x, vec3d.y, this.z) < d * d) {
            this.path.next();
            if (this.path.isFinished()) {
                vec3d = null;
                this.path = null;
                continue;
            }
            vec3d = this.path.getNodePosition(this);
        }
        this.jumping = false;
        if (vec3d != null) {
            float f3;
            double d2 = vec3d.x - this.x;
            double d3 = vec3d.z - this.z;
            double d4 = vec3d.y - (double)n;
            float f4 = (float)(Math.atan2(d3, d2) * 180.0 / 3.1415927410125732) - 90.0f;
            this.forwardSpeed = this.movementSpeed;
            for (f3 = f4 - this.yaw; f3 < -180.0f; f3 += 360.0f) {
            }
            while (f3 >= 180.0f) {
                f3 -= 360.0f;
            }
            if (f3 > 30.0f) {
                f3 = 30.0f;
            }
            if (f3 < -30.0f) {
                f3 = -30.0f;
            }
            this.yaw += f3;
            if (this.movementBlocked && this.target != null) {
                double d5 = this.target.x - this.x;
                double d6 = this.target.z - this.z;
                float f5 = this.yaw;
                this.yaw = (float)(Math.atan2(d6, d5) * 180.0 / 3.1415927410125732) - 90.0f;
                f3 = (f5 - this.yaw + 90.0f) * (float)Math.PI / 180.0f;
                this.sidewaysSpeed = -MathHelper.sin(f3) * this.forwardSpeed * 1.0f;
                this.forwardSpeed = MathHelper.cos(f3) * this.forwardSpeed * 1.0f;
            }
            if (d4 > 0.0) {
                this.jumping = true;
            }
        }
        if (this.target != null) {
            this.lookAt(this.target, 30.0f, 30.0f);
        }
        if (this.horizontalCollision && !this.hasPath()) {
            this.jumping = true;
        }
        if (this.random.nextFloat() < 0.8f && (bl || bl2)) {
            this.jumping = true;
        }
    }

    protected void pathingUpdate() {
        boolean bl = false;
        int n = -1;
        int n2 = -1;
        int n3 = -1;
        float f = -99999.0f;
        for (int i = 0; i < 10; ++i) {
            int n4;
            int n5;
            int n6 = MathHelper.floor(this.x + (double)this.random.nextInt(13) - 6.0);
            float f2 = this.getPathfindingFavor(n6, n5 = MathHelper.floor(this.y + (double)this.random.nextInt(7) - 3.0), n4 = MathHelper.floor(this.z + (double)this.random.nextInt(13) - 6.0));
            if (!(f2 > f)) continue;
            f = f2;
            n = n6;
            n2 = n5;
            n3 = n4;
            bl = true;
        }
        if (bl) {
            this.path = this.world.findPath(this, n, n2, n3, 10.0f);
        }
    }

    protected void attack(Entity other, float distance) {
    }

    protected void resetAttack(Entity other, float distance) {
    }

    protected float getPathfindingFavor(int x, int y, int z) {
        return 0.0f;
    }

    protected Entity getTargetInRange() {
        return null;
    }

    public boolean canSpawn() {
        int n = MathHelper.floor(this.x);
        int n2 = MathHelper.floor(this.boundingBox.minY);
        int n3 = MathHelper.floor(this.z);
        return super.canSpawn() && this.getPathfindingFavor(n, n2, n3) >= 0.0f;
    }

    public boolean hasPath() {
        return this.path != null;
    }

    public void setPath(Path path) {
        this.path = path;
    }

    public Entity getTarget() {
        return this.target;
    }

    public void setTarget(Entity target) {
        this.target = target;
    }
}

