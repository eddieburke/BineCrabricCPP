#include "net/minecraft/entity/ai/pathing/PathNodeNavigator.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/DoorBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

#include <vector>

namespace net::minecraft::entity::ai::pathing {

namespace {

bool nodesEqual(PathNode* left, PathNode* right)
{
    return left != nullptr && right != nullptr && left->equals(*right);
}

} // namespace

void PathNodeNavigator::resetSearch()
{
    minHeap_.clear();
    pathNodeCache_.clear();
    ownedNodes_.clear();
}

Path PathNodeNavigator::findPath(Entity* startEntity, Entity* endEntity, float distance)
{
    if (startEntity == nullptr || endEntity == nullptr) {
        return Path {{}};
    }
    return findPath(startEntity, endEntity->x, endEntity->boundingBox.minY, endEntity->z, distance);
}

Path PathNodeNavigator::findPath(Entity* startEntity, int x, int y, int z, float distance)
{
    return findPath(startEntity, static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5, static_cast<double>(z) + 0.5,
        distance);
}

Path PathNodeNavigator::findPath(Entity* startEntity, double x, double y, double z, float distance)
{
    if (startEntity == nullptr || blockView_ == nullptr) {
        return Path {{}};
    }

    resetSearch();

    PathNode* startNode = getNode(MathHelper::floor(startEntity->boundingBox.minX),
        MathHelper::floor(startEntity->boundingBox.minY), MathHelper::floor(startEntity->boundingBox.minZ));
    PathNode* endNode = getNode(MathHelper::floor(x - static_cast<double>(startEntity->width / 2.0f)),
        MathHelper::floor(y), MathHelper::floor(z - static_cast<double>(startEntity->width / 2.0f)));
    PathNode sizeNode {MathHelper::floor(startEntity->width + 1.0f), MathHelper::floor(startEntity->height + 1.0f),
        MathHelper::floor(startEntity->width + 1.0f)};
    return findPath(startEntity, startNode, endNode, sizeNode, distance);
}

Path PathNodeNavigator::findPath(Entity* startEntity, PathNode* startNode, PathNode* endNode, PathNode& sizeNode,
    float distance)
{
    if (startEntity == nullptr || startNode == nullptr || endNode == nullptr) {
        return Path {{}};
    }

    startNode->penalizedPathLength = 0.0f;
    startNode->heapWeight = startNode->distanceToNearestTarget = startNode->getDistance(*endNode);
    minHeap_.clear();
    minHeap_.push(startNode);
    PathNode* closestNode = startNode;

    while (!minHeap_.isEmpty()) {
        PathNode* current = minHeap_.pop();
        if (nodesEqual(current, endNode)) {
            return createPath(startNode, endNode);
        }
        if (current->getDistance(*endNode) < closestNode->getDistance(*endNode)) {
            closestNode = current;
        }
        current->visited = true;
        const int successorCount = getSuccessors(startEntity, current, sizeNode, endNode, distance);
        for (int i = 0; i < successorCount; ++i) {
            PathNode* successor = getNode(successors_[i].x, successors_[i].y, successors_[i].z);
            if (successor == nullptr) {
                continue;
            }
            const float pathLength = current->penalizedPathLength + current->getDistance(*successor);
            if (successor->isInHeap() && !(pathLength < successor->penalizedPathLength)) {
                continue;
            }
            successor->previous = current;
            successor->penalizedPathLength = pathLength;
            successor->distanceToNearestTarget = successor->getDistance(*endNode);
            if (successor->isInHeap()) {
                minHeap_.setNodeWeight(successor, successor->penalizedPathLength + successor->distanceToNearestTarget);
            } else {
                successor->heapWeight = successor->penalizedPathLength + successor->distanceToNearestTarget;
                minHeap_.push(successor);
            }
        }
    }

    if (closestNode == startNode) {
        return Path {{}};
    }
    return createPath(startNode, closestNode);
}

int PathNodeNavigator::getSuccessors(Entity* startEntity, PathNode* startNode, PathNode& sizeNode, PathNode* endNode,
    float distance)
{
    int count = 0;
    int stepHeight = 0;
    if (isPassable(startEntity, startNode->x, startNode->y + 1, startNode->z, sizeNode) == 1) {
        stepHeight = 1;
    }

    PathNode* north = getNode(startEntity, startNode->x, startNode->y, startNode->z + 1, sizeNode, stepHeight);
    PathNode* west = getNode(startEntity, startNode->x - 1, startNode->y, startNode->z, sizeNode, stepHeight);
    PathNode* east = getNode(startEntity, startNode->x + 1, startNode->y, startNode->z, sizeNode, stepHeight);
    PathNode* south = getNode(startEntity, startNode->x, startNode->y, startNode->z - 1, sizeNode, stepHeight);

    if (north != nullptr && !north->visited && north->getDistance(*endNode) < distance) {
        successors_[count++] = *north;
    }
    if (west != nullptr && !west->visited && west->getDistance(*endNode) < distance) {
        successors_[count++] = *west;
    }
    if (east != nullptr && !east->visited && east->getDistance(*endNode) < distance) {
        successors_[count++] = *east;
    }
    if (south != nullptr && !south->visited && south->getDistance(*endNode) < distance) {
        successors_[count++] = *south;
    }
    return count;
}

PathNode* PathNodeNavigator::getNode(Entity* entity, int x, int y, int z, PathNode& sizeNode, int stepHeight)
{
    PathNode* node = nullptr;
    if (isPassable(entity, x, y, z, sizeNode) == 1) {
        node = getNode(x, y, z);
    }
    if (node == nullptr && stepHeight > 0 && isPassable(entity, x, y + stepHeight, z, sizeNode) == 1) {
        node = getNode(x, y + stepHeight, z);
        y += stepHeight;
    }
    if (node != nullptr) {
        int dropCount = 0;
        int passableBelow = 0;
        while (y > 0 && (passableBelow = isPassable(entity, x, y - 1, z, sizeNode)) == 1) {
            if (++dropCount >= 4) {
                return nullptr;
            }
            if (--y <= 0) {
                continue;
            }
            node = getNode(x, y, z);
        }
        if (passableBelow == -2) {
            return nullptr;
        }
    }
    return node;
}

PathNode* PathNodeNavigator::getNode(int x, int y, int z)
{
    const int hash = PathNode::hash(x, y, z);
    PathNode* cached = pathNodeCache_.get(hash);
    if (cached != nullptr) {
        return cached;
    }

    auto* node = new PathNode(x, y, z);
    ownedNodes_.push_back(node);
    pathNodeCache_.put(hash, node);
    return node;
}

int PathNodeNavigator::isPassable(Entity* /*entity*/, int x, int y, int z, PathNode& node)
{
    if (blockView_ == nullptr) {
        return 0;
    }

    for (int ix = x; ix < x + node.x; ++ix) {
        for (int iy = y; iy < y + node.y; ++iy) {
            for (int iz = z; iz < z + node.z; ++iz) {
                const int blockId = blockView_->getBlockId(ix, iy, iz);
                if (blockId <= 0) {
                    continue;
                }
                if (block::Block::DOOR != nullptr && block::Block::IRON_DOOR != nullptr
                    && (blockId == block::Block::IRON_DOOR->id || blockId == block::Block::DOOR->id)) {
                    const int meta = blockView_->getBlockMeta(ix, iy, iz);
                    if (block::DoorBlock::getOpen(meta)) {
                        continue;
                    }
                    return 0;
                }
                if (blockId >= block::Block::BLOCK_COUNT || block::Block::BLOCKS[blockId] == nullptr) {
                    continue;
                }
                block::material::Material& material = block::Block::BLOCKS[blockId]->material;
                if (material.blocksMovement()) {
                    return 0;
                }
                if (&material == &block::material::Material::WATER) {
                    return -1;
                }
                if (&material == &block::material::Material::LAVA) {
                    return -2;
                }
            }
        }
    }
    return 1;
}

Path PathNodeNavigator::createPath(PathNode* /*unused*/, PathNode* endNode)
{
    int count = 1;
    for (PathNode* node = endNode; node != nullptr && node->previous != nullptr; node = node->previous) {
        ++count;
    }

    std::vector<PathNode> nodes(static_cast<std::size_t>(count));
    PathNode* node = endNode;
    for (int index = count - 1; index >= 0 && node != nullptr; --index) {
        nodes[static_cast<std::size_t>(index)] = PathNode {node->x, node->y, node->z};
        node = node->previous;
    }
    return Path(std::move(nodes));
}

} // namespace net::minecraft::entity::ai::pathing
