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
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.item.Item;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class DoorBlock
extends Block {
    protected DoorBlock(int i, Material arg) {
        super(i, arg);
        this.textureId = 97;
        if (arg == Material.METAL) {
            ++this.textureId;
        }
        float f = 0.5f;
        float f2 = 1.0f;
        this.setBoundingBox(0.5f - f, 0.0f, 0.5f - f, 0.5f + f, f2, 0.5f + f);
    }

    public int getTexture(int side, int meta) {
        if (side == 0 || side == 1) {
            return this.textureId;
        }
        int n = this.setOpen(meta);
        if ((n == 0 || n == 2) ^ side <= 3) {
            return this.textureId;
        }
        int n2 = n / 2 + (side & 1 ^ n);
        int n3 = this.textureId - (meta & 8) * 2;
        if (((n2 += (meta & 4) / 4) & 1) != 0) {
            n3 = -n3;
        }
        return n3;
    }

    public boolean isOpaque() {
        return false;
    }

    public boolean isFullCube() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 7;
    }

    @Environment(value=EnvType.CLIENT)
    public Box getBoundingBox(World world, int x, int y, int z) {
        this.updateBoundingBox(world, x, y, z);
        return super.getBoundingBox(world, x, y, z);
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        this.updateBoundingBox(world, x, y, z);
        return super.getCollisionShape(world, x, y, z);
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        this.rotate(this.setOpen(blockView.getBlockMeta(x, y, z)));
    }

    public void rotate(int meta) {
        float f = 0.1875f;
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f);
        if (meta == 0) {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, f);
        }
        if (meta == 1) {
            this.setBoundingBox(1.0f - f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
        }
        if (meta == 2) {
            this.setBoundingBox(0.0f, 0.0f, 1.0f - f, 1.0f, 1.0f, 1.0f);
        }
        if (meta == 3) {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, f, 1.0f, 1.0f);
        }
    }

    public void onBlockBreakStart(World world, int x, int y, int z, PlayerEntity player) {
        this.onUse(world, x, y, z, player);
    }

    public boolean onUse(World world, int x, int y, int z, PlayerEntity player) {
        if (this.material == Material.METAL) {
            return true;
        }
        int n = world.getBlockMeta(x, y, z);
        if ((n & 8) != 0) {
            if (world.getBlockId(x, y - 1, z) == this.id) {
                this.onUse(world, x, y - 1, z, player);
            }
            return true;
        }
        if (world.getBlockId(x, y + 1, z) == this.id) {
            world.setBlockMeta(x, y + 1, z, (n ^ 4) + 8);
        }
        world.setBlockMeta(x, y, z, n ^ 4);
        world.setBlocksDirty(x, y - 1, z, x, y, z);
        world.worldEvent(player, 1003, x, y, z, 0);
        return true;
    }

    public void setOpen(World world, int x, int y, int z, boolean open) {
        boolean bl;
        int n = world.getBlockMeta(x, y, z);
        if ((n & 8) != 0) {
            if (world.getBlockId(x, y - 1, z) == this.id) {
                this.setOpen(world, x, y - 1, z, open);
            }
            return;
        }
        boolean bl2 = bl = (world.getBlockMeta(x, y, z) & 4) > 0;
        if (bl == open) {
            return;
        }
        if (world.getBlockId(x, y + 1, z) == this.id) {
            world.setBlockMeta(x, y + 1, z, (n ^ 4) + 8);
        }
        world.setBlockMeta(x, y, z, n ^ 4);
        world.setBlocksDirty(x, y - 1, z, x, y, z);
        world.worldEvent(null, 1003, x, y, z, 0);
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        int n = world.getBlockMeta(x, y, z);
        if ((n & 8) != 0) {
            if (world.getBlockId(x, y - 1, z) != this.id) {
                world.setBlock(x, y, z, 0);
            }
            if (id > 0 && Block.BLOCKS[id].canEmitRedstonePower()) {
                this.neighborUpdate(world, x, y - 1, z, id);
            }
        } else {
            boolean bl = false;
            if (world.getBlockId(x, y + 1, z) != this.id) {
                world.setBlock(x, y, z, 0);
                bl = true;
            }
            if (!world.shouldSuffocate(x, y - 1, z)) {
                world.setBlock(x, y, z, 0);
                bl = true;
                if (world.getBlockId(x, y + 1, z) == this.id) {
                    world.setBlock(x, y + 1, z, 0);
                }
            }
            if (bl) {
                if (!world.isRemote) {
                    this.dropStacks(world, x, y, z, n);
                }
            } else if (id > 0 && Block.BLOCKS[id].canEmitRedstonePower()) {
                boolean bl2 = world.isEmittingRedstonePower(x, y, z) || world.isEmittingRedstonePower(x, y + 1, z);
                this.setOpen(world, x, y, z, bl2);
            }
        }
    }

    public int getDroppedItemId(int blockMeta, Random random) {
        if ((blockMeta & 8) != 0) {
            return 0;
        }
        if (this.material == Material.METAL) {
            return Item.IRON_DOOR.id;
        }
        return Item.WOODEN_DOOR.id;
    }

    public HitResult raycast(World world, int x, int y, int z, Vec3d startPos, Vec3d endPos) {
        this.updateBoundingBox(world, x, y, z);
        return super.raycast(world, x, y, z, startPos, endPos);
    }

    public int setOpen(int meta) {
        if ((meta & 4) == 0) {
            return meta - 1 & 3;
        }
        return meta & 3;
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        if (y >= 127) {
            return false;
        }
        return world.shouldSuffocate(x, y - 1, z) && super.canPlaceAt(world, x, y, z) && super.canPlaceAt(world, x, y + 1, z);
    }

    public static boolean getOpen(int meta) {
        return (meta & 4) != 0;
    }

    public int getPistonBehavior() {
        return 1;
    }
}

