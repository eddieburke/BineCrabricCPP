/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.entity.ai.pathing;

import net.minecraft.block.Block;
import net.minecraft.block.DoorBlock;
import net.minecraft.block.material.Material;
import net.minecraft.entity.Entity;
import net.minecraft.entity.ai.pathing.Path;
import net.minecraft.entity.ai.pathing.PathMinHeap;
import net.minecraft.entity.ai.pathing.PathNode;
import net.minecraft.util.IntHashMap;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.BlockView;

public class PathNodeNavigator {
    private BlockView blockView;
    private PathMinHeap minHeap = new PathMinHeap();
    private IntHashMap pathNodeCache = new IntHashMap();
    private PathNode[] successors = new PathNode[32];

    public PathNodeNavigator(BlockView blockView) {
        this.blockView = blockView;
    }

    public Path findPath(Entity startEntity, Entity endEntity, float distance) {
        return this.findPath(startEntity, endEntity.x, endEntity.boundingBox.minY, endEntity.z, distance);
    }

    public Path findPath(Entity startEntity, int x, int y, int z, float distance) {
        return this.findPath(startEntity, (float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, distance);
    }

    private Path findPath(Entity startEntity, double x, double y, double z, float distance) {
        this.minHeap.clear();
        this.pathNodeCache.clear();
        PathNode pathNode = this.getNode(MathHelper.floor(startEntity.boundingBox.minX), MathHelper.floor(startEntity.boundingBox.minY), MathHelper.floor(startEntity.boundingBox.minZ));
        PathNode pathNode2 = this.getNode(MathHelper.floor(x - (double)(startEntity.width / 2.0f)), MathHelper.floor(y), MathHelper.floor(z - (double)(startEntity.width / 2.0f)));
        PathNode pathNode3 = new PathNode(MathHelper.floor(startEntity.width + 1.0f), MathHelper.floor(startEntity.height + 1.0f), MathHelper.floor(startEntity.width + 1.0f));
        Path path = this.findPath(startEntity, pathNode, pathNode2, pathNode3, distance);
        return path;
    }

    private Path findPath(Entity startEntity, PathNode startNode, PathNode endNode, PathNode position, float distance) {
        startNode.penalizedPathLength = 0.0f;
        startNode.heapWeight = startNode.distanceToNearestTarget = startNode.getDistance(endNode);
        this.minHeap.clear();
        this.minHeap.push(startNode);
        PathNode pathNode = startNode;
        while (!this.minHeap.isEmpty()) {
            PathNode pathNode2 = this.minHeap.pop();
            if (pathNode2.equals(endNode)) {
                return this.createPath(startNode, endNode);
            }
            if (pathNode2.getDistance(endNode) < pathNode.getDistance(endNode)) {
                pathNode = pathNode2;
            }
            pathNode2.visited = true;
            int n = this.getSuccessors(startEntity, pathNode2, position, endNode, distance);
            for (int i = 0; i < n; ++i) {
                PathNode pathNode3 = this.successors[i];
                float f = pathNode2.penalizedPathLength + pathNode2.getDistance(pathNode3);
                if (pathNode3.isInHeap() && !(f < pathNode3.penalizedPathLength)) continue;
                pathNode3.previous = pathNode2;
                pathNode3.penalizedPathLength = f;
                pathNode3.distanceToNearestTarget = pathNode3.getDistance(endNode);
                if (pathNode3.isInHeap()) {
                    this.minHeap.setNodeWeight(pathNode3, pathNode3.penalizedPathLength + pathNode3.distanceToNearestTarget);
                    continue;
                }
                pathNode3.heapWeight = pathNode3.penalizedPathLength + pathNode3.distanceToNearestTarget;
                this.minHeap.push(pathNode3);
            }
        }
        if (pathNode == startNode) {
            return null;
        }
        return this.createPath(startNode, pathNode);
    }

    private int getSuccessors(Entity startEntity, PathNode startNode, PathNode endNode, PathNode position, float distance) {
        int n = 0;
        int n2 = 0;
        if (this.isPassable(startEntity, startNode.x, startNode.y + 1, startNode.z, endNode) == 1) {
            n2 = 1;
        }
        PathNode pathNode = this.getNode(startEntity, startNode.x, startNode.y, startNode.z + 1, endNode, n2);
        PathNode pathNode2 = this.getNode(startEntity, startNode.x - 1, startNode.y, startNode.z, endNode, n2);
        PathNode pathNode3 = this.getNode(startEntity, startNode.x + 1, startNode.y, startNode.z, endNode, n2);
        PathNode pathNode4 = this.getNode(startEntity, startNode.x, startNode.y, startNode.z - 1, endNode, n2);
        if (pathNode != null && !pathNode.visited && pathNode.getDistance(position) < distance) {
            this.successors[n++] = pathNode;
        }
        if (pathNode2 != null && !pathNode2.visited && pathNode2.getDistance(position) < distance) {
            this.successors[n++] = pathNode2;
        }
        if (pathNode3 != null && !pathNode3.visited && pathNode3.getDistance(position) < distance) {
            this.successors[n++] = pathNode3;
        }
        if (pathNode4 != null && !pathNode4.visited && pathNode4.getDistance(position) < distance) {
            this.successors[n++] = pathNode4;
        }
        return n;
    }

    private PathNode getNode(Entity entity, int x, int y, int z, PathNode endNode, int stepHeight) {
        PathNode pathNode = null;
        if (this.isPassable(entity, x, y, z, endNode) == 1) {
            pathNode = this.getNode(x, y, z);
        }
        if (pathNode == null && stepHeight > 0 && this.isPassable(entity, x, y + stepHeight, z, endNode) == 1) {
            pathNode = this.getNode(x, y + stepHeight, z);
            y += stepHeight;
        }
        if (pathNode != null) {
            int n = 0;
            int n2 = 0;
            while (y > 0 && (n2 = this.isPassable(entity, x, y - 1, z, endNode)) == 1) {
                if (++n >= 4) {
                    return null;
                }
                if (--y <= 0) continue;
                pathNode = this.getNode(x, y, z);
            }
            if (n2 == -2) {
                return null;
            }
        }
        return pathNode;
    }

    private final PathNode getNode(int x, int y, int z) {
        int n = PathNode.hash(x, y, z);
        PathNode pathNode = (PathNode)this.pathNodeCache.get(n);
        if (pathNode == null) {
            pathNode = new PathNode(x, y, z);
            this.pathNodeCache.put(n, pathNode);
        }
        return pathNode;
    }

    private int isPassable(Entity entity, int x, int y, int z, PathNode node) {
        for (int i = x; i < x + node.x; ++i) {
            for (int j = y; j < y + node.y; ++j) {
                for (int k = z; k < z + node.z; ++k) {
                    int n = this.blockView.getBlockId(i, j, k);
                    if (n <= 0) continue;
                    if (n == Block.IRON_DOOR.id || n == Block.DOOR.id) {
                        int n2 = this.blockView.getBlockMeta(i, j, k);
                        if (DoorBlock.getOpen(n2)) continue;
                        return 0;
                    }
                    Material material = Block.BLOCKS[n].material;
                    if (material.blocksMovement()) {
                        return 0;
                    }
                    if (material == Material.WATER) {
                        return -1;
                    }
                    if (material != Material.LAVA) continue;
                    return -2;
                }
            }
        }
        return 1;
    }

    private Path createPath(PathNode unused, PathNode startNode) {
        int n = 1;
        PathNode pathNode = startNode;
        while (pathNode.previous != null) {
            ++n;
            pathNode = pathNode.previous;
        }
        PathNode[] pathNodeArray = new PathNode[n];
        pathNode = startNode;
        pathNodeArray[--n] = pathNode;
        while (pathNode.previous != null) {
            pathNode = pathNode.previous;
            pathNodeArray[--n] = pathNode;
        }
        return new Path(pathNodeArray);
    }
}

