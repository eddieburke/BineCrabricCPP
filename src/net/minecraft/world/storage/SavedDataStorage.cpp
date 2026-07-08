#include "net/minecraft/world/storage/SavedDataStorage.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtFileIo.hpp"
#include "net/minecraft/nbt/NbtIo.hpp"
#include "net/minecraft/world/PersistentStateRegistry.hpp"

namespace net::minecraft {
SavedDataStorage::SavedDataStorage(WorldStorage* storage) : storage_(storage) {
    loadIdCounts();
}

void SavedDataStorage::shareWith(SavedDataStorage& other) {
    impl_ = other.impl_;
    storage_ = other.storage_;
}

PersistentState* SavedDataStorage::getOrCreate(const std::type_index& stateClass, const std::string& id) {
    (void) stateClass;
    const auto it = impl_->loadedStatesById_.find(id);
    if (it != impl_->loadedStatesById_.end()) {
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
                        if (std::unique_ptr<PersistentState> state =
                                PersistentStateRegistry::instance().create(stateClass, id)) {
                            state->readNbt(root.getCompound("data"));
                            PersistentState* pointer = state.get();
                            impl_->loadedStates_.push_back(pointer);
                            impl_->loadedStatesById_[id] = std::move(state);
                            return pointer;
                        }
                    }
                }
            } catch (const std::exception&) {
            }
        }
    }
    return nullptr;
}

void SavedDataStorage::set(const std::string& id, std::unique_ptr<PersistentState> state) {
    if (state == nullptr) {
        throw std::runtime_error("Can't set null data");
    }
    if (const auto existing = impl_->loadedStatesById_.find(id); existing != impl_->loadedStatesById_.end()) {
        impl_->loadedStates_.erase(
            std::remove(impl_->loadedStates_.begin(), impl_->loadedStates_.end(), existing->second.get()),
            impl_->loadedStates_.end());
        impl_->loadedStatesById_.erase(existing);
    }
    impl_->loadedStates_.push_back(state.get());
    impl_->loadedStatesById_[id] = std::move(state);
}

void SavedDataStorage::save() {
    for (PersistentState* state : impl_->loadedStates_) {
        if (state != nullptr && state->isDirty()) {
            save(*state);
            state->setDirty(false);
        }
    }
}

void SavedDataStorage::save(PersistentState& state) {
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
        writeFileAtomic(file, [&root](std::ostream& output) { NbtIo::writeCompressed(root, output); });
    } catch (const std::exception&) {
    }
}

void SavedDataStorage::loadIdCounts() {
    impl_->idCounts_.clear();
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
                impl_->idCounts_[entry.first] = static_cast<int>(entry.second.asShort());
            }
        }
    } catch (const std::exception&) {
    }
}

int SavedDataStorage::getIdCount(const std::string& id) {
    int value = 0;
    if (const auto it = impl_->idCounts_.find(id); it != impl_->idCounts_.end()) {
        value = it->second + 1;
    }
    impl_->idCounts_[id] = value;
    if (storage_ == nullptr) {
        return value;
    }
    try {
        const std::filesystem::path file = storage_->getWorldPropertiesFile("idcounts");
        if (file.empty()) {
            return value;
        }
        NbtCompound root;
        for (const auto& entry : impl_->idCounts_) {
            root.putShort(entry.first, static_cast<std::int16_t>(entry.second));
        }
        // Uncompressed, matching loadIdCounts()'s NbtIo::read.
        writeFileAtomic(file, [&root](std::ostream& output) { NbtIo::write(root, output); });
    } catch (const std::exception&) {
    }
    return value;
}
}  // namespace net::minecraft
