/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity;

import net.minecraft.block.Block;
import net.minecraft.entity.LivingEntity;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

public class FlyingEntity
extends LivingEntity {
    public FlyingEntity(World arg) {
        super(arg);
    }

    protected void onLanding(float fallDistance) {
    }

    public void travel(float x, float z) {
        if (this.isSubmergedInWater()) {
            this.moveNonSolid(x, z, 0.02f);
            this.move(this.velocityX, this.velocityY, this.velocityZ);
            this.velocityX *= (double)0.8f;
            this.velocityY *= (double)0.8f;
            this.velocityZ *= (double)0.8f;
        } else if (this.isTouchingLava()) {
            this.moveNonSolid(x, z, 0.02f);
            this.move(this.velocityX, this.velocityY, this.velocityZ);
            this.velocityX *= 0.5;
            this.velocityY *= 0.5;
            this.velocityZ *= 0.5;
        } else {
            float f = 0.91f;
            if (this.onGround) {
                f = 0.54600006f;
                int n = this.world.getBlockId(MathHelper.floor(this.x), MathHelper.floor(this.boundingBox.minY) - 1, MathHelper.floor(this.z));
                if (n > 0) {
                    f = Block.BLOCKS[n].slipperiness * 0.91f;
                }
            }
            float f2 = 0.16277136f / (f * f * f);
            this.moveNonSolid(x, z, this.onGround ? 0.1f * f2 : 0.02f);
            f = 0.91f;
            if (this.onGround) {
                f = 0.54600006f;
                int n = this.world.getBlockId(MathHelper.floor(this.x), MathHelper.floor(this.boundingBox.minY) - 1, MathHelper.floor(this.z));
                if (n > 0) {
                    f = Block.BLOCKS[n].slipperiness * 0.91f;
                }
            }
            this.move(this.velocityX, this.velocityY, this.velocityZ);
            this.velocityX *= (double)f;
            this.velocityY *= (double)f;
            this.velocityZ *= (double)f;
        }
        this.lastWalkAnimationSpeed = this.walkAnimationSpeed;
        double d = this.x - this.prevX;
        double d2 = this.z - this.prevZ;
        float f = MathHelper.sqrt(d * d + d2 * d2) * 4.0f;
        if (f > 1.0f) {
            f = 1.0f;
        }
        this.walkAnimationSpeed += (f - this.walkAnimationSpeed) * 0.4f;
        this.walkAnimationProgress += this.walkAnimationSpeed;
    }

    public boolean isOnLadder() {
        return false;
    }
}

