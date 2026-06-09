#pragma once

#include "net/minecraft/world/chunk/storage/RegionFile.hpp"

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace net::minecraft {

namespace fs = std::filesystem;

class RegionIo {
public:
    static RegionFile& getRegionFile(const fs::path& worldDir, int chunkX, int chunkZ)
    {
        const fs::path regionDir = worldDir / "region";
        const fs::path regionFile = regionDir / ("r." + std::to_string(chunkX >> 5) + "." + std::to_string(chunkZ >> 5) + ".mcr");
        const std::string key = regionFile.lexically_normal().generic_string();

        std::lock_guard<std::mutex> lock(mutex_());
        auto& slot = files_()[key];
        if (!slot) {
            slot = std::make_shared<RegionFile>(regionFile);
        }
        return *slot;
    }

    static std::optional<std::vector<std::uint8_t>> readChunkData(const fs::path& worldDir, int chunkX, int chunkZ)
    {
        return getRegionFile(worldDir, chunkX, chunkZ).readChunk(chunkX & 0x1F, chunkZ & 0x1F);
    }

    static void writeChunkData(const fs::path& worldDir, int chunkX, int chunkZ, const std::vector<std::uint8_t>& data)
    {
        getRegionFile(worldDir, chunkX, chunkZ).writeChunk(chunkX & 0x1F, chunkZ & 0x1F, data);
    }

    static int getChunkSize(const fs::path& worldDir, int chunkX, int chunkZ)
    {
        return getRegionFile(worldDir, chunkX, chunkZ).resetBytesWritten();
    }

    static void flush()
    {
        std::lock_guard<std::mutex> lock(mutex_());
        for (auto& [key, file] : files_()) {
            (void)key;
            if (file) {
                file->flush();
            }
        }
        files_().clear();
    }

private:
    static std::unordered_map<std::string, std::shared_ptr<RegionFile>>& files_()
    {
        static std::unordered_map<std::string, std::shared_ptr<RegionFile>> files;
        return files;
    }

    static std::mutex& mutex_()
    {
        static std::mutex mutex;
        return mutex;
    }
};

} // namespace net::minecraft

