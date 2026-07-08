#include "net/minecraft/world/storage/RegionWorldStorageSource.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

#include "net/minecraft/nbt/BinaryIO.hpp"
#include "net/minecraft/nbt/Compression.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtFileIo.hpp"
#include "net/minecraft/nbt/NbtIo.hpp"
#include "net/minecraft/world/chunk/storage/RegionIo.hpp"

namespace net::minecraft {
namespace {
// Alpha and McRegion store the *same* chunk NBT; only the container differs (alpha = one
// gzip file per chunk, region = zlib entry in r.X.Z.mcr). Conversion is therefore a repack,
// not a deserialize/re-serialize — read the alpha file, decompress to the raw NBT, and hand
// it to RegionIo (which zlib-compresses it into the region file).
[[nodiscard]] bool parseChunkFileName(const std::string& name, int& chunkX, int& chunkZ) {
    // c.<base36 x>.<base36 z>.dat
    std::array<std::string, 4> tokens;
    std::size_t token = 0;
    for (const char ch : name) {
        if (ch == '.') {
            if (++token >= tokens.size()) {
                return false;
            }
            continue;
        }
        tokens[token].push_back(ch);
    }
    if (token != 3 || tokens[0] != "c" || tokens[3] != "dat") {
        return false;
    }
    const auto fromBase36 = [](const std::string& text, int& out) {
        if (text.empty()) {
            return false;
        }
        bool negative = false;
        std::size_t i = 0;
        if (text[0] == '-') {
            negative = true;
            i = 1;
            if (text.size() == 1) {
                return false;
            }
        }
        long value = 0;
        for (; i < text.size(); ++i) {
            const char ch = text[i];
            int digit = 0;
            if (ch >= '0' && ch <= '9') {
                digit = ch - '0';
            } else if (ch >= 'a' && ch <= 'z') {
                digit = ch - 'a' + 10;
            } else if (ch >= 'A' && ch <= 'Z') {
                digit = ch - 'A' + 10;
            } else {
                return false;
            }
            value = value * 36 + digit;
        }
        out = static_cast<int>(negative ? -value : value);
        return true;
    };
    return fromBase36(tokens[1], chunkX) && fromBase36(tokens[2], chunkZ);
}

void convertDimension(const fs::path& dimDir) {
    if (!fs::exists(dimDir)) {
        return;
    }
    // Collect first, then write: writing creates dimDir/region, so we must not be iterating
    // dimDir while RegionIo mutates it.
    std::vector<std::tuple<fs::path, int, int>> chunkFiles;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(dimDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        int chunkX = 0;
        int chunkZ = 0;
        if (parseChunkFileName(entry.path().filename().string(), chunkX, chunkZ)) {
            chunkFiles.emplace_back(entry.path(), chunkX, chunkZ);
        }
    }
    for (const auto& [file, chunkX, chunkZ] : chunkFiles) {
        try {
            std::ifstream input(file, std::ios::binary);
            if (!input) {
                continue;
            }
            const std::vector<std::uint8_t> compressed = binary::readAllBytes(input);
            if (compressed.empty()) {
                continue;
            }
            const std::vector<std::uint8_t> raw = gzipDecompress(compressed);
            if (raw.empty()) {
                continue;
            }
            RegionIo::writeChunkData(dimDir, chunkX, chunkZ, raw);
        } catch (const std::exception&) {
        }
    }
}

// Mark the save as McRegion (version 19132) so it loads region-native and is not re-converted
// on the next open (needsConversion treats version 0 as alpha).
[[nodiscard]] bool markConverted(const fs::path& saveDir) {
    const fs::path levelDat = saveDir / "level.dat";
    if (!fs::exists(levelDat)) {
        return false;
    }
    NbtCompound root;
    try {
        std::ifstream input(levelDat, std::ios::binary);
        if (!input) {
            return false;
        }
        root = NbtIo::readCompressed(input);
    } catch (const std::exception&) {
        return false;
    }
    if (!root.contains("Data")) {
        return false;
    }
    NbtCompound data = root.getCompound("Data");
    data.putInt("version", 19132);
    root.put("Data", data);
    try {
        AtomicWriteOptions options;
        options.keepBackup = true;
        writeFileAtomic(levelDat, [&root](std::ostream& output) { NbtIo::writeCompressed(root, output); }, options);
    } catch (const std::exception&) {
        return false;
    }
    return true;
}
}  // namespace

bool RegionWorldStorageSource::convert(const std::string& saveName, client::gui::screen::LoadingDisplay* display) {
    (void) display;
    const fs::path saveDir = savesDirectory() / saveName;
    if (!fs::exists(saveDir)) {
        return false;
    }
    // Overworld chunks live in the save root; the Nether (if present) in DIM-1. Both use the
    // alpha base36 c.x.z.dat layout.
    convertDimension(saveDir);
    convertDimension(saveDir / "DIM-1");
    RegionIo::flush();
    return markConverted(saveDir);
}
}  // namespace net::minecraft
