/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.BlockWithEntity;
import net.minecraft.block.entity.BlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.item.Item;
import net.minecraft.util.math.Box;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class SignBlock
extends BlockWithEntity {
    private Class blockEntityClazz;
    private boolean standing;

    protected SignBlock(int id, Class blockEntityClazz, boolean standing) {
        super(id, Material.WOOD);
        this.standing = standing;
        this.textureId = 4;
        this.blockEntityClazz = blockEntityClazz;
        float f = 0.25f;
        float f2 = 1.0f;
        this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f2, 0.5f + f);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return null;
    }

    @Environment(value=EnvType.CLIENT)
    public Box getBoundingBox(World world, int x, int y, int z) {
        this.updateBoundingBox(world, x, y, z);
        return super.getBoundingBox(world, x, y, z);
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        if (this.standing) {
            return;
        }
        int n = blockView.getBlockMeta(x, y, z);
        float f = 0.28125f;
        float f2 = 0.78125f;
        float f3 = 0.0f;
        float f4 = 1.0f;
        float f5 = 0.125f;
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        if (n == 2) {
            this.setBoundingBox(f3, f, 1.0f - f5, f4, f2, 1.0f);
        }
        if (n == 3) {
            this.setBoundingBox(f3, f, 0.0f, f4, f2, f5);
        }
        if (n == 4) {
            this.setBoundingBox(1.0f - f5, f, f3, 1.0f, f2, f4);
        }
        if (n == 5) {
            this.setBoundingBox(0.0f, f, f3, f5, f2, f4);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return -1;
    }

    public boolean isFullCube() {
        return false;
    }

    public boolean isOpaque() {
        return false;
    }

    protected BlockEntity createBlockEntity() {
        try {
            return (BlockEntity)this.blockEntityClazz.newInstance();
        }
        catch (Exception exception) {
            throw new RuntimeException(exception);
        }
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        return Item.SIGN.id;
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        boolean bl = false;
        if (this.standing) {
            if (!world.getMaterial(x, y - 1, z).isSolid()) {
                bl = true;
            }
        } else {
            int n = world.getBlockMeta(x, y, z);
            bl = true;
            if (n == 2 && world.getMaterial(x, y, z + 1).isSolid()) {
                bl = false;
            }
            if (n == 3 && world.getMaterial(x, y, z - 1).isSolid()) {
                bl = false;
            }
            if (n == 4 && world.getMaterial(x + 1, y, z).isSolid()) {
                bl = false;
            }
            if (n == 5 && world.getMaterial(x - 1, y, z).isSolid()) {
                bl = false;
            }
        }
        if (bl) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, 0);
        }
        super.neighborUpdate(world, x, y, z, id);
    }
}

