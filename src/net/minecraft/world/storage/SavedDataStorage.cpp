#include "net/minecraft/world/storage/SavedDataStorage.hpp"

#include "net/minecraft/item/map/MapState.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtIo.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace net::minecraft {

SavedDataStorage::SavedDataStorage(WorldStorage* storage) : storage_(storage)
{
    loadIdCounts();
}

PersistentState* SavedDataStorage::getOrCreate(const std::type_index& stateClass, const std::string& id)
{
    (void)stateClass;
    const auto it = loadedStatesById_.find(id);
    if (it != loadedStatesById_.end()) {
        return it->second.get();
    }

    if (storage_ != nullptr) {
        const std::filesystem::path file = storage_->getWorldPropertiesFile(id);
        if (!file.empty() && std::filesystem::exists(file)) {
            try {
                std::ifstream input(file, std::ios::binary);
                if (input) {
                    const NbtCompound root = NbtIo::readCompressed(input);
                    if (root.contains("data")) {
                        if (stateClass == typeid(map::MapState)) {
                            auto state = std::make_unique<map::MapState>(id);
                            state->readNbt(root.getCompound("data"));
                            auto* pointer = state.get();
                            loadedStates_.push_back(pointer);
                            loadedStatesById_[id] = std::move(state);
                            return pointer;
                        }
                    }
                }
            } catch (const std::exception& exception) {
                std::cerr << "Failed to load saved data '" << id << "': " << exception.what() << '\n';
            }
        }
    }
    return nullptr;
}

void SavedDataStorage::set(const std::string& id, std::unique_ptr<PersistentState> state)
{
    if (state == nullptr) {
        throw std::runtime_error("Can't set null data");
    }
    if (const auto existing = loadedStatesById_.find(id); existing != loadedStatesById_.end()) {
        loadedStates_.erase(
            std::remove(loadedStates_.begin(), loadedStates_.end(), existing->second.get()),
            loadedStates_.end());
        loadedStatesById_.erase(existing);
    }
    loadedStates_.push_back(state.get());
    loadedStatesById_[id] = std::move(state);
}

void SavedDataStorage::save()
{
    for (PersistentState* state : loadedStates_) {
        if (state != nullptr && state->isDirty()) {
            save(*state);
            state->setDirty(false);
        }
    }
}

void SavedDataStorage::save(PersistentState& state)
{
    if (storage_ == nullptr) {
        return;
    }

    try {
        const std::filesystem::path file = storage_->getWorldPropertiesFile(state.id);
        if (file.empty()) {
            return;
        }

        NbtCompound data;
        state.writeNbt(data);
        NbtCompound root;
        root.put("data", data);

        std::ofstream output(file, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Failed to open saved data file");
        }
        NbtIo::writeCompressed(root, output);
    } catch (const std::exception& exception) {
        std::cerr << "Failed to save data '" << state.id << "': " << exception.what() << '\n';
    }
}

void SavedDataStorage::loadIdCounts()
{
    idCounts_.clear();
    if (storage_ == nullptr) {
        return;
    }

    try {
        const std::filesystem::path file = storage_->getWorldPropertiesFile("idcounts");
        if (file.empty() || !std::filesystem::exists(file)) {
            return;
        }

        std::ifstream input(file, std::ios::binary);
        if (!input) {
            return;
        }

        const NbtCompound root = NbtIo::read(input);
        for (const auto& entry : root.storage().asCompound()) {
            if (entry.second.type() == Nbt::Type::Short) {
                idCounts_[entry.first] = static_cast<int>(entry.second.asShort());
            }
        }
    } catch (const std::exception& exception) {
        std::cerr << "Failed to load id counts: " << exception.what() << '\n';
    }
}

int SavedDataStorage::getIdCount(const std::string& id)
{
    int value = 0;
    if (const auto it = idCounts_.find(id); it != idCounts_.end()) {
        value = it->second + 1;
    }
    idCounts_[id] = value;

    if (storage_ == nullptr) {
        return value;
    }

    try {
        const std::filesystem::path file = storage_->getWorldPropertiesFile("idcounts");
        if (file.empty()) {
            return value;
        }

        NbtCompound root;
        for (const auto& entry : idCounts_) {
            root.putShort(entry.first, static_cast<std::int16_t>(entry.second));
        }

        std::ofstream output(file, std::ios::binary | std::ios::trunc);
        if (output) {
            NbtIo::write(root, output);
        }
    } catch (const std::exception& exception) {
        std::cerr << "Failed to save id counts: " << exception.what() << '\n';
    }
    return value;
}

} // namespace net::minecraft
