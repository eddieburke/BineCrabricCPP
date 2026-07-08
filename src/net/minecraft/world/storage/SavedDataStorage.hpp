#pragma once
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "net/minecraft/world/PersistentState.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"

namespace net::minecraft {
class SavedDataStorage {
   public:
    explicit SavedDataStorage(WorldStorage* storage);
    void shareWith(SavedDataStorage& other);
    PersistentState* getOrCreate(const std::type_index& stateClass, const std::string& id);
    void set(const std::string& id, std::unique_ptr<PersistentState> state);
    void save();
    [[nodiscard]] int getIdCount(const std::string& id);

   private:
    struct Impl {
        std::unordered_map<std::string, std::unique_ptr<PersistentState>> loadedStatesById_;
        std::vector<PersistentState*> loadedStates_;
        std::unordered_map<std::string, int> idCounts_;
    };

    void save(PersistentState& state);
    void loadIdCounts();
    std::shared_ptr<Impl> impl_ = std::make_shared<Impl>();
    WorldStorage* storage_ = nullptr;
};
}  // namespace net::minecraft
