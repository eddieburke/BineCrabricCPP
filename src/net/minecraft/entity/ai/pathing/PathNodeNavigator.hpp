#pragma once
#include <array>
#include <deque>
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/ai/pathing/Path.hpp"
#include "net/minecraft/entity/ai/pathing/PathMinHeap.hpp"
#include "net/minecraft/util/IntHashMap.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace net::minecraft::entity::ai::pathing {
class PathNodeNavigator {
 public:
 explicit PathNodeNavigator(BlockView* blockView) : blockView_(blockView) {
 }
 [[nodiscard]] Path findPath(Entity* startEntity, Entity* endEntity, float distance);
 [[nodiscard]] Path findPath(Entity* startEntity, int x, int y, int z, float distance);

 private:
 [[nodiscard]] Path findPath(Entity* startEntity, double x, double y, double z, float distance);
 [[nodiscard]] Path findPath(
     Entity* startEntity, PathNode* startNode, PathNode* endNode, PathNode& sizeNode, float distance);
 int getSuccessors(Entity* startEntity, PathNode* startNode, PathNode& sizeNode, PathNode* endNode, float distance);
 PathNode* getNode(Entity* entity, int x, int y, int z, PathNode& sizeNode, int stepHeight);
 PathNode* getNode(int x, int y, int z);
 int isPassable(Entity* entity, int x, int y, int z, PathNode& node);
 [[nodiscard]] Path createPath(PathNode* endNode);
 void resetSearch();
 BlockView* blockView_ = nullptr;
 PathMinHeap minHeap_{};
 util::IntHashMap<PathNode*> pathNodeCache_{};
 std::array<PathNode*, 4> successors_{};
 std::deque<PathNode> ownedNodes_{};
};
} // namespace net::minecraft::entity::ai::pathing
