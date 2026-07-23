/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.item;

import net.minecraft.block.Block;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.vehicle.BoatEntity;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.hit.HitResultType;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.World;

public class BoatItem
extends Item {
    public BoatItem(int i) {
        super(i);
        this.maxCount = 1;
    }

    public ItemStack use(ItemStack stack, World world, PlayerEntity user) {
        float f;
        float f2;
        float f3;
        double d;
        float f4;
        float f5 = 1.0f;
        float f6 = user.prevPitch + (user.pitch - user.prevPitch) * f5;
        float f7 = user.prevYaw + (user.yaw - user.prevYaw) * f5;
        double d2 = user.prevX + (user.x - user.prevX) * (double)f5;
        double d3 = user.prevY + (user.y - user.prevY) * (double)f5 + 1.62 - (double)user.standingEyeHeight;
        double d4 = user.prevZ + (user.z - user.prevZ) * (double)f5;
        Vec3d vec3d = Vec3d.createCached(d2, d3, d4);
        float f8 = MathHelper.cos(-f7 * ((float)Math.PI / 180) - (float)Math.PI);
        float f9 = MathHelper.sin(-f7 * ((float)Math.PI / 180) - (float)Math.PI);
        float f10 = f9 * (f4 = -MathHelper.cos(-f6 * ((float)Math.PI / 180)));
        Vec3d vec3d2 = vec3d.add((double)f10 * (d = 5.0), (double)(f3 = (f2 = MathHelper.sin(-f6 * ((float)Math.PI / 180)))) * d, (double)(f = f8 * f4) * d);
        HitResult hitResult = world.raycast(vec3d, vec3d2, true);
        if (hitResult == null) {
            return stack;
        }
        if (hitResult.type == HitResultType.BLOCK) {
            int n = hitResult.blockX;
            int n2 = hitResult.blockY;
            int n3 = hitResult.blockZ;
            if (!world.isRemote) {
                if (world.getBlockId(n, n2, n3) == Block.SNOW.id) {
                    --n2;
                }
                world.spawnEntity(new BoatEntity(world, (float)n + 0.5f, (float)n2 + 1.0f, (float)n3 + 0.5f));
            }
            --stack.count;
        }
        return stack;
    }
}

