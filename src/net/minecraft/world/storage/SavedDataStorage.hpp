#pragma once

#include "net/minecraft/world/PersistentState.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace net::minecraft {

class SavedDataStorage {
public:
    explicit SavedDataStorage(WorldStorage* storage);

    PersistentState* getOrCreate(const std::type_index& stateClass, const std::string& id);
    void set(const std::string& id, std::unique_ptr<PersistentState> state);
    void save();
    [[nodiscard]] int getIdCount(const std::string& id);

private:
    void save(PersistentState& state);
    void loadIdCounts();

    WorldStorage* storage_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<PersistentState>> loadedStatesById_;
    std::vector<PersistentState*> loadedStates_;
    std::unordered_map<std::string, int> idCounts_;
};

} // namespace net::minecraft
