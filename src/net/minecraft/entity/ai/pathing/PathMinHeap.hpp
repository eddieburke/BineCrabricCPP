#pragma once
#include "net/minecraft/entity/ai/pathing/PathNode.hpp"
#include <limits>
#include <stdexcept>
#include <vector>
namespace net::minecraft::entity::ai::pathing {
class PathMinHeap {
public:
  PathNode* push(PathNode* node) {
    if(node == nullptr) {
      return nullptr;
    }
    if(node->heapIndex >= 0) {
      throw std::logic_error("OW KNOWS!");
    }
    if(count_ == static_cast<int>(pathNodes_.size())) {
      pathNodes_.resize(static_cast<std::size_t>(count_ << 1));
    }
    pathNodes_[static_cast<std::size_t>(count_)] = node;
    node->heapIndex = count_;
    shiftUp(count_++);
    return node;
  }
  void clear() {
    count_ = 0;
  }
  PathNode* pop() {
    if(count_ <= 0) {
      return nullptr;
    }
    PathNode* node = pathNodes_[0];
    pathNodes_[0] = pathNodes_[static_cast<std::size_t>(--count_)];
    pathNodes_[static_cast<std::size_t>(count_)] = nullptr;
    if(count_ > 0) {
      shiftDown(0);
    }
    node->heapIndex = -1;
    return node;
  }
  void setNodeWeight(PathNode* node, float weight) {
    if(node == nullptr) {
      return;
    }
    const float previousWeight = node->heapWeight;
    node->heapWeight = weight;
    if(weight < previousWeight) {
      shiftUp(node->heapIndex);
    } else {
      shiftDown(node->heapIndex);
    }
  }
  [[nodiscard]] bool isEmpty() const {
    return count_ <= 0;
  }

private:
  void shiftUp(int index) {
    PathNode* node = pathNodes_[static_cast<std::size_t>(index)];
    const float weight = node->heapWeight;
    while(index > 0) {
      const int parent = (index - 1) >> 1;
      PathNode* parentNode = pathNodes_[static_cast<std::size_t>(parent)];
      if(!(weight < parentNode->heapWeight)) {
        break;
      }
      pathNodes_[static_cast<std::size_t>(index)] = parentNode;
      parentNode->heapIndex = index;
      index = parent;
    }
    pathNodes_[static_cast<std::size_t>(index)] = node;
    node->heapIndex = index;
  }
  void shiftDown(int index) {
    PathNode* node = pathNodes_[static_cast<std::size_t>(index)];
    const float weight = node->heapWeight;
    while(true) {
      const int left = 1 + (index << 1);
      const int right = left + 1;
      if(left >= count_) {
        break;
      }
      PathNode* leftNode = pathNodes_[static_cast<std::size_t>(left)];
      const float leftWeight = leftNode->heapWeight;
      PathNode* rightNode = nullptr;
      float rightWeight = std::numeric_limits<float>::infinity();
      if(right < count_) {
        rightNode = pathNodes_[static_cast<std::size_t>(right)];
        rightWeight = rightNode->heapWeight;
      }
      if(leftWeight < rightWeight) {
        if(!(leftWeight < weight)) {
          break;
        }
        pathNodes_[static_cast<std::size_t>(index)] = leftNode;
        leftNode->heapIndex = index;
        index = left;
        continue;
      }
      if(!(rightWeight < weight)) {
        break;
      }
      pathNodes_[static_cast<std::size_t>(index)] = rightNode;
      rightNode->heapIndex = index;
      index = right;
    }
    pathNodes_[static_cast<std::size_t>(index)] = node;
    node->heapIndex = index;
  }
  std::vector<PathNode*> pathNodes_{1024, nullptr};
  int count_ = 0;
};
} // namespace net::minecraft::entity::ai::pathing
