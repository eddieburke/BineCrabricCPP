#pragma once
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "net/minecraft/world/chunk/storage/RegionFile.hpp"

namespace net::minecraft {
namespace fs = std::filesystem;

// Serialized access to .mcr region files. One mutex covers map lookup and all
// per-file I/O (matches Java's synchronized RegionIo + RegionFile methods).
class RegionIo {
   public:
    static std::optional<std::vector<std::uint8_t>> readChunkData(const fs::path& worldDir, int chunkX, int chunkZ) {
        std::lock_guard<std::mutex> lock(mutex());
        return regionFile(worldDir, chunkX, chunkZ).readChunk(chunkX & 0x1F, chunkZ & 0x1F);
    }

    static void writeChunkData(const fs::path& worldDir,
                               int chunkX,
                               int chunkZ,
                               const std::vector<std::uint8_t>& data) {
        std::lock_guard<std::mutex> lock(mutex());
        regionFile(worldDir, chunkX, chunkZ).writeChunk(chunkX & 0x1F, chunkZ & 0x1F, data);
    }

    static int getChunkSize(const fs::path& worldDir, int chunkX, int chunkZ) {
        std::lock_guard<std::mutex> lock(mutex());
        return regionFile(worldDir, chunkX, chunkZ).resetBytesWritten();
    }

    static void flush() {
        std::lock_guard<std::mutex> lock(mutex());
        for (auto& [key, file] : openFiles()) {
            (void) key;
            if (file) {
                file->flush();
            }
        }
        openFiles().clear();
    }

   private:
    static RegionFile& regionFile(const fs::path& worldDir, int chunkX, int chunkZ) {
        const fs::path regionDir = worldDir / "region";
        const fs::path regionPath =
            regionDir / ("r." + std::to_string(chunkX >> 5) + "." + std::to_string(chunkZ >> 5) + ".mcr");
        const std::string key = regionPath.lexically_normal().generic_string();
        auto& slot = openFiles()[key];
        if (!slot) {
            slot = std::make_shared<RegionFile>(regionPath);
        }
        return *slot;
    }

    static std::unordered_map<std::string, std::shared_ptr<RegionFile>>& openFiles() {
        static std::unordered_map<std::string, std::shared_ptr<RegionFile>> files;
        return files;
    }

    static std::mutex& mutex() {
        static std::mutex mutex;
        return mutex;
    }
};
}  // namespace net::minecraft
