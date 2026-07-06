#include "net/minecraft/world/dimension/DimensionType.hpp"
#include <algorithm>
#include <deque>
#include <mutex>
namespace net::minecraft::dimension {
namespace {
// deque: element addresses stay stable across registration, so the const
// DimensionType* a Dimension caches for its lifetime never dangles.
std::deque<DimensionType>& storage() {
  static std::deque<DimensionType> value;
  return value;
}
std::vector<const DimensionType*>& pointerCache() {
  static std::vector<const DimensionType*> value;
  return value;
}
void rebuildPointerCache() {
  pointerCache().clear();
  for(const DimensionType& type : storage()) {
    pointerCache().push_back(&type);
  }
}
void ensureBuiltins() {
  static std::once_flag flag;
  std::call_once(flag, [] { registerBuiltinDimensions(); });
}
} // namespace
void registerDimension(DimensionType type) {
  for(DimensionType& existing : storage()) {
    if(existing.id == type.id) {
      existing = std::move(type);
      rebuildPointerCache();
      return;
    }
  }
  storage().push_back(std::move(type));
  rebuildPointerCache();
}
const DimensionType* dimensionById(int id) {
  ensureBuiltins();
  for(const DimensionType& type : storage()) {
    if(type.id == id) {
      return &type;
    }
  }
  return nullptr;
}
const std::vector<const DimensionType*>& allDimensions() {
  ensureBuiltins();
  return pointerCache();
}
} // namespace net::minecraft::dimension
