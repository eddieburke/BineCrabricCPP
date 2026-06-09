#pragma once

#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/chunk/storage/RegionIo.hpp"
#include "net/minecraft/world/storage/AlphaWorldStorageSource.hpp"
#include "net/minecraft/world/storage/RegionWorldStorage.hpp"
#include "net/minecraft/world/storage/WorldSaveInfo.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace net::minecraft {

namespace fs = std::filesystem;

class RegionWorldStorageSource : public AlphaWorldStorageSource {
public:
    explicit RegionWorldStorageSource(fs::path file) : AlphaWorldStorageSource(std::move(file)) {}

    [[nodiscard]] std::string getName() const override { return "Scaevolus' McRegion"; }

    [[nodiscard]] std::vector<WorldSaveInfo> getAll() override
    {
        std::vector<WorldSaveInfo> saves;
        const fs::path& dir = savesDirectory();
        if (!fs::exists(dir)) {
            return saves;
        }
        for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
            if (!entry.is_directory()) {
                continue;
            }
            const std::string saveName = entry.path().filename().string();
            const std::optional<WorldProperties> props = getWorldProperties(saveName);
            if (!props.has_value()) {
                continue;
            }
            const bool sameVersion = props->getVersion() != 19132;
            std::string displayName = props->getName();
            if (displayName.empty() || MathHelper::isNullOrEmpty(displayName)) {
                displayName = saveName;
            }
            WorldProperties propsCopy = *props;
            saves.emplace_back(saveName, displayName, propsCopy.setLastPlayed(), propsCopy.getSizeOnDisk(), sameVersion);
        }
        return saves;
    }

    void flush() override { RegionIo::flush(); }

    [[nodiscard]] std::unique_ptr<WorldStorage> getSaveLoader(const std::string& string, bool bl) override
    {
        return std::make_unique<RegionWorldStorage>(savesDirectory(), string, bl);
    }

    [[nodiscard]] bool needsConversion(const std::string& saveName) const override
    {
        const std::optional<WorldProperties> props = const_cast<RegionWorldStorageSource*>(this)->getWorldProperties(saveName);
        return props.has_value() && props->getVersion() == 0;
    }

    bool convert(const std::string& saveName, client::gui::screen::LoadingDisplay* display) override
    {
        (void)saveName;
        (void)display;
        return false;
    }
};

} // namespace net::minecraft
