/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.block;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.block.material.Material;
import net.minecraft.util.hit.HitResult;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Vec3d;
import net.minecraft.world.BlockView;
import net.minecraft.world.World;

public class RailBlock
extends Block {
    private final boolean alwaysStraight;

    public static final boolean isRail(World world, int x, int y, int z) {
        int n = world.getBlockId(x, y, z);
        return n == Block.RAIL.id || n == Block.POWERED_RAIL.id || n == Block.DETECTOR_RAIL.id;
    }

    public static final boolean isRail(int id) {
        return id == Block.RAIL.id || id == Block.POWERED_RAIL.id || id == Block.DETECTOR_RAIL.id;
    }

    protected RailBlock(int id, int textureId, boolean alwaysStraight) {
        super(id, textureId, Material.PISTON_BREAKABLE);
        this.alwaysStraight = alwaysStraight;
        this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
    }

    public boolean isAlwaysStraight() {
        return this.alwaysStraight;
    }

    public Box getCollisionShape(World world, int x, int y, int z) {
        return null;
    }

    public boolean isOpaque() {
        return false;
    }

    public HitResult raycast(World world, int x, int y, int z, Vec3d startPos, Vec3d endPos) {
        this.updateBoundingBox(world, x, y, z);
        return super.raycast(world, x, y, z, startPos, endPos);
    }

    public void updateBoundingBox(BlockView blockView, int x, int y, int z) {
        int n = blockView.getBlockMeta(x, y, z);
        if (n >= 2 && n <= 5) {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.625f, 1.0f);
        } else {
            this.setBoundingBox(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
        }
    }

    public int getTexture(int side, int meta) {
        if (this.alwaysStraight ? this.id == Block.POWERED_RAIL.id && (meta & 8) == 0 : meta >= 6) {
            return this.textureId - 16;
        }
        return this.textureId;
    }

    public boolean isFullCube() {
        return false;
    }

    @Environment(value=EnvType.CLIENT)
    public int getRenderType() {
        return 9;
    }

    public int getDroppedItemCount(Random random) {
        return 1;
    }

    public boolean canPlaceAt(World world, int x, int y, int z) {
        return world.shouldSuffocate(x, y - 1, z);
    }

    public void onPlaced(World world, int x, int y, int z) {
        if (!world.isRemote) {
            this.updateShape(world, x, y, z, true);
        }
    }

    public void neighborUpdate(World world, int x, int y, int z, int id) {
        int n;
        if (world.isRemote) {
            return;
        }
        int n2 = n = world.getBlockMeta(x, y, z);
        if (this.alwaysStraight) {
            n2 &= 7;
        }
        boolean bl = false;
        if (!world.shouldSuffocate(x, y - 1, z)) {
            bl = true;
        }
        if (n2 == 2 && !world.shouldSuffocate(x + 1, y, z)) {
            bl = true;
        }
        if (n2 == 3 && !world.shouldSuffocate(x - 1, y, z)) {
            bl = true;
        }
        if (n2 == 4 && !world.shouldSuffocate(x, y, z - 1)) {
            bl = true;
        }
        if (n2 == 5 && !world.shouldSuffocate(x, y, z + 1)) {
            bl = true;
        }
        if (bl) {
            this.dropStacks(world, x, y, z, world.getBlockMeta(x, y, z));
            world.setBlock(x, y, z, 0);
        } else if (this.id == Block.POWERED_RAIL.id) {
            boolean bl2 = world.isEmittingRedstonePower(x, y, z) || world.isEmittingRedstonePower(x, y + 1, z);
            bl2 = bl2 || this.isPoweredByConnectedRails(world, x, y, z, n, true, 0) || this.isPoweredByConnectedRails(world, x, y, z, n, false, 0);
            boolean bl3 = false;
            if (bl2 && (n & 8) == 0) {
                world.setBlockMeta(x, y, z, n2 | 8);
                bl3 = true;
            } else if (!bl2 && (n & 8) != 0) {
                world.setBlockMeta(x, y, z, n2);
                bl3 = true;
            }
            if (bl3) {
                world.notifyNeighbors(x, y - 1, z, this.id);
                if (n2 == 2 || n2 == 3 || n2 == 4 || n2 == 5) {
                    world.notifyNeighbors(x, y + 1, z, this.id);
                }
            }
        } else if (id > 0 && Block.BLOCKS[id].canEmitRedstonePower() && !this.alwaysStraight && new RailNode(world, x, y, z).countConnections() == 3) {
            this.updateShape(world, x, y, z, false);
        }
    }

    private void updateShape(World world, int x, int y, int z, boolean force) {
        if (world.isRemote) {
            return;
        }
        new RailNode(world, x, y, z).updateState(world.isEmittingRedstonePower(x, y, z), force);
    }

    private boolean isPoweredByConnectedRails(World world, int x, int y, int z, int meta, boolean towardsNegative, int depth) {
        if (depth >= 8) {
            return false;
        }
        int n = meta & 7;
        boolean bl = true;
        switch (n) {
            case 0: {
                if (towardsNegative) {
                    ++z;
                    break;
                }
                --z;
                break;
            }
            case 1: {
                if (towardsNegative) {
                    --x;
                    break;
                }
                ++x;
                break;
            }
            case 2: {
                if (towardsNegative) {
                    --x;
                } else {
                    ++x;
                    ++y;
                    bl = false;
                }
                n = 1;
                break;
            }
            case 3: {
                if (towardsNegative) {
                    --x;
                    ++y;
                    bl = false;
                } else {
                    ++x;
                }
                n = 1;
                break;
            }
            case 4: {
                if (towardsNegative) {
                    ++z;
                } else {
                    --z;
                    ++y;
                    bl = false;
                }
                n = 0;
                break;
            }
            case 5: {
                if (towardsNegative) {
                    ++z;
                    ++y;
                    bl = false;
                } else {
                    --z;
                }
                n = 0;
            }
        }
        if (this.isPoweredByRail(world, x, y, z, towardsNegative, depth, n)) {
            return true;
        }
        return bl && this.isPoweredByRail(world, x, y - 1, z, towardsNegative, depth, n);
    }

    private boolean isPoweredByRail(World world, int x, int y, int z, boolean towardsNegative, int depth, int shape) {
        int n = world.getBlockId(x, y, z);
        if (n == Block.POWERED_RAIL.id) {
            int n2 = world.getBlockMeta(x, y, z);
            int n3 = n2 & 7;
            if (shape == 1 && (n3 == 0 || n3 == 4 || n3 == 5)) {
                return false;
            }
            if (shape == 0 && (n3 == 1 || n3 == 2 || n3 == 3)) {
                return false;
            }
            if ((n2 & 8) != 0) {
                if (world.isEmittingRedstonePower(x, y, z) || world.isEmittingRedstonePower(x, y + 1, z)) {
                    return true;
                }
                return this.isPoweredByConnectedRails(world, x, y, z, n2, towardsNegative, depth + 1);
            }
        }
        return false;
    }

    public int getPistonBehavior() {
        return 0;
    }

    class RailNode {
        private World world;
        private int x;
        private int y;
        private int z;
        private final boolean alwaysStraight;
        private List connections = new ArrayList();

        public RailNode(World world, int x, int y, int z) {
            this.world = world;
            this.x = x;
            this.y = y;
            this.z = z;
            int n = world.getBlockId(x, y, z);
            int n2 = world.getBlockMeta(x, y, z);
            if (((RailBlock)Block.BLOCKS[n]).alwaysStraight) {
                this.alwaysStraight = true;
                n2 &= 0xFFFFFFF7;
            } else {
                this.alwaysStraight = false;
            }
            this.updateConnections(n2);
        }

        private void updateConnections(int meta) {
            this.connections.clear();
            if (meta == 0) {
                this.connections.add(new BlockPos(this.x, this.y, this.z - 1));
                this.connections.add(new BlockPos(this.x, this.y, this.z + 1));
            } else if (meta == 1) {
                this.connections.add(new BlockPos(this.x - 1, this.y, this.z));
                this.connections.add(new BlockPos(this.x + 1, this.y, this.z));
            } else if (meta == 2) {
                this.connections.add(new BlockPos(this.x - 1, this.y, this.z));
                this.connections.add(new BlockPos(this.x + 1, this.y + 1, this.z));
            } else if (meta == 3) {
                this.connections.add(new BlockPos(this.x - 1, this.y + 1, this.z));
                this.connections.add(new BlockPos(this.x + 1, this.y, this.z));
            } else if (meta == 4) {
                this.connections.add(new BlockPos(this.x, this.y + 1, this.z - 1));
                this.connections.add(new BlockPos(this.x, this.y, this.z + 1));
            } else if (meta == 5) {
                this.connections.add(new BlockPos(this.x, this.y, this.z - 1));
                this.connections.add(new BlockPos(this.x, this.y + 1, this.z + 1));
            } else if (meta == 6) {
                this.connections.add(new BlockPos(this.x + 1, this.y, this.z));
                this.connections.add(new BlockPos(this.x, this.y, this.z + 1));
            } else if (meta == 7) {
                this.connections.add(new BlockPos(this.x - 1, this.y, this.z));
                this.connections.add(new BlockPos(this.x, this.y, this.z + 1));
            } else if (meta == 8) {
                this.connections.add(new BlockPos(this.x - 1, this.y, this.z));
                this.connections.add(new BlockPos(this.x, this.y, this.z - 1));
            } else if (meta == 9) {
                this.connections.add(new BlockPos(this.x + 1, this.y, this.z));
                this.connections.add(new BlockPos(this.x, this.y, this.z - 1));
            }
        }

        private void removeSoftConnections() {
            for (int i = 0; i < this.connections.size(); ++i) {
                RailNode railNode = this.getNeighborRail((BlockPos)this.connections.get(i));
                if (railNode == null || !railNode.connectsTo(this)) {
                    this.connections.remove(i--);
                    continue;
                }
                this.connections.set(i, new BlockPos(railNode.x, railNode.y, railNode.z));
            }
        }

        private boolean couldConnectTo(int x, int y, int z) {
            if (RailBlock.isRail(this.world, x, y, z)) {
                return true;
            }
            if (RailBlock.isRail(this.world, x, y + 1, z)) {
                return true;
            }
            return RailBlock.isRail(this.world, x, y - 1, z);
        }

        private RailNode getNeighborRail(BlockPos pos) {
            if (RailBlock.isRail(this.world, pos.x, pos.y, pos.z)) {
                return new RailNode(this.world, pos.x, pos.y, pos.z);
            }
            if (RailBlock.isRail(this.world, pos.x, pos.y + 1, pos.z)) {
                return new RailNode(this.world, pos.x, pos.y + 1, pos.z);
            }
            if (RailBlock.isRail(this.world, pos.x, pos.y - 1, pos.z)) {
                return new RailNode(this.world, pos.x, pos.y - 1, pos.z);
            }
            return null;
        }

        private boolean connectsTo(RailNode railNode) {
            for (int i = 0; i < this.connections.size(); ++i) {
                BlockPos blockPos = (BlockPos)this.connections.get(i);
                if (blockPos.x != railNode.x || blockPos.z != railNode.z) continue;
                return true;
            }
            return false;
        }

        private boolean hasConnection(int x, int y, int z) {
            for (int i = 0; i < this.connections.size(); ++i) {
                BlockPos blockPos = (BlockPos)this.connections.get(i);
                if (blockPos.x != x || blockPos.z != z) continue;
                return true;
            }
            return false;
        }

        private int countConnections() {
            int n = 0;
            if (this.couldConnectTo(this.x, this.y, this.z - 1)) {
                ++n;
            }
            if (this.couldConnectTo(this.x, this.y, this.z + 1)) {
                ++n;
            }
            if (this.couldConnectTo(this.x - 1, this.y, this.z)) {
                ++n;
            }
            if (this.couldConnectTo(this.x + 1, this.y, this.z)) {
                ++n;
            }
            return n;
        }

        private boolean canConnectTo(RailNode railNode) {
            if (this.connectsTo(railNode)) {
                return true;
            }
            if (this.connections.size() == 2) {
                return false;
            }
            if (this.connections.size() == 0) {
                return true;
            }
            BlockPos blockPos = (BlockPos)this.connections.get(0);
            if (railNode.y == this.y && blockPos.y == this.y) {
                return true;
            }
            return true;
        }

        private void addConnection(RailNode railNode) {
            this.connections.add(new BlockPos(railNode.x, railNode.y, railNode.z));
            boolean bl = this.hasConnection(this.x, this.y, this.z - 1);
            boolean bl2 = this.hasConnection(this.x, this.y, this.z + 1);
            boolean bl3 = this.hasConnection(this.x - 1, this.y, this.z);
            boolean bl4 = this.hasConnection(this.x + 1, this.y, this.z);
            int n = -1;
            if (bl || bl2) {
                n = 0;
            }
            if (bl3 || bl4) {
                n = 1;
            }
            if (!this.alwaysStraight) {
                if (bl2 && bl4 && !bl && !bl3) {
                    n = 6;
                }
                if (bl2 && bl3 && !bl && !bl4) {
                    n = 7;
                }
                if (bl && bl3 && !bl2 && !bl4) {
                    n = 8;
                }
                if (bl && bl4 && !bl2 && !bl3) {
                    n = 9;
                }
            }
            if (n == 0) {
                if (RailBlock.isRail(this.world, this.x, this.y + 1, this.z - 1)) {
                    n = 4;
                }
                if (RailBlock.isRail(this.world, this.x, this.y + 1, this.z + 1)) {
                    n = 5;
                }
            }
            if (n == 1) {
                if (RailBlock.isRail(this.world, this.x + 1, this.y + 1, this.z)) {
                    n = 2;
                }
                if (RailBlock.isRail(this.world, this.x - 1, this.y + 1, this.z)) {
                    n = 3;
                }
            }
            if (n < 0) {
                n = 0;
            }
            int n2 = n;
            if (this.alwaysStraight) {
                n2 = this.world.getBlockMeta(this.x, this.y, this.z) & 8 | n;
            }
            this.world.setBlockMeta(this.x, this.y, this.z, n2);
        }

        private boolean hasNeighborRail(int x, int y, int z) {
            RailNode railNode = this.getNeighborRail(new BlockPos(x, y, z));
            if (railNode == null) {
                return false;
            }
            railNode.removeSoftConnections();
            return railNode.canConnectTo(this);
        }

        public void updateState(boolean powered, boolean force) {
            boolean bl = this.hasNeighborRail(this.x, this.y, this.z - 1);
            boolean bl2 = this.hasNeighborRail(this.x, this.y, this.z + 1);
            boolean bl3 = this.hasNeighborRail(this.x - 1, this.y, this.z);
            boolean bl4 = this.hasNeighborRail(this.x + 1, this.y, this.z);
            int n = -1;
            if ((bl || bl2) && !bl3 && !bl4) {
                n = 0;
            }
            if ((bl3 || bl4) && !bl && !bl2) {
                n = 1;
            }
            if (!this.alwaysStraight) {
                if (bl2 && bl4 && !bl && !bl3) {
                    n = 6;
                }
                if (bl2 && bl3 && !bl && !bl4) {
                    n = 7;
                }
                if (bl && bl3 && !bl2 && !bl4) {
                    n = 8;
                }
                if (bl && bl4 && !bl2 && !bl3) {
                    n = 9;
                }
            }
            if (n == -1) {
                if (bl || bl2) {
                    n = 0;
                }
                if (bl3 || bl4) {
                    n = 1;
                }
                if (!this.alwaysStraight) {
                    if (powered) {
                        if (bl2 && bl4) {
                            n = 6;
                        }
                        if (bl3 && bl2) {
                            n = 7;
                        }
                        if (bl4 && bl) {
                            n = 9;
                        }
                        if (bl && bl3) {
                            n = 8;
                        }
                    } else {
                        if (bl && bl3) {
                            n = 8;
                        }
                        if (bl4 && bl) {
                            n = 9;
                        }
                        if (bl3 && bl2) {
                            n = 7;
                        }
                        if (bl2 && bl4) {
                            n = 6;
                        }
                    }
                }
            }
            if (n == 0) {
                if (RailBlock.isRail(this.world, this.x, this.y + 1, this.z - 1)) {
                    n = 4;
                }
                if (RailBlock.isRail(this.world, this.x, this.y + 1, this.z + 1)) {
                    n = 5;
                }
            }
            if (n == 1) {
                if (RailBlock.isRail(this.world, this.x + 1, this.y + 1, this.z)) {
                    n = 2;
                }
                if (RailBlock.isRail(this.world, this.x - 1, this.y + 1, this.z)) {
                    n = 3;
                }
            }
            if (n < 0) {
                n = 0;
            }
            this.updateConnections(n);
            int n2 = n;
            if (this.alwaysStraight) {
                n2 = this.world.getBlockMeta(this.x, this.y, this.z) & 8 | n;
            }
            if (force || this.world.getBlockMeta(this.x, this.y, this.z) != n2) {
                this.world.setBlockMeta(this.x, this.y, this.z, n2);
                for (int i = 0; i < this.connections.size(); ++i) {
                    RailNode railNode = this.getNeighborRail((BlockPos)this.connections.get(i));
                    if (railNode == null) continue;
                    railNode.removeSoftConnections();
                    if (!railNode.canConnectTo(this)) continue;
                    railNode.addConnection(this);
                }
            }
        }
    }
}

