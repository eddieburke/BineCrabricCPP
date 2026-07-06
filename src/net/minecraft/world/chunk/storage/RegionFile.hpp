#pragma once
#include "net/minecraft/nbt/BinaryIO.hpp"
#include "net/minecraft/nbt/Compression.hpp"
#include <array>
#include <ctime>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>
namespace net::minecraft {
namespace fs = std::filesystem;
class RegionFile {
public:
  explicit RegionFile(fs::path file) : file_(std::move(file)) {
    fs::create_directories(file_.parent_path());
    openOrCreate();
    initializeHeader();
  }
  ~RegionFile() {
    close();
  }
  RegionFile(const RegionFile&) = delete;
  RegionFile& operator=(const RegionFile&) = delete;
  [[nodiscard]] bool hasChunkData(int chunkX, int chunkZ) const {
    if(isOutsideRegion(chunkX, chunkZ)) {
      return false;
    }
    return chunkBlockInfo_[static_cast<std::size_t>(index(chunkX, chunkZ))] != 0;
  }
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> readChunk(int chunkX, int chunkZ) {
    if(isOutsideRegion(chunkX, chunkZ)) {
      return std::nullopt;
    }
    const std::uint32_t blockInfo = chunkBlockInfo_[static_cast<std::size_t>(index(chunkX, chunkZ))];
    if(blockInfo == 0) {
      return std::nullopt;
    }
    const std::uint32_t sectorOffset = blockInfo >> 8U;
    const std::uint32_t sectorCount = blockInfo & 0xFFU;
    if(sectorOffset + sectorCount > sectorFree_.size()) {
      return std::nullopt;
    }
    stream_.clear();
    stream_.seekg(static_cast<std::streamoff>(sectorOffset * sectorSize));
    const std::uint32_t length = readU32();
    if(length == 0 || length > sectorCount * sectorSize) {
      return std::nullopt;
    }
    const std::uint8_t compression = readU8();
    std::vector<std::uint8_t> compressed(length - 1U);
    if(!compressed.empty()) {
      stream_.read(reinterpret_cast<char*>(compressed.data()), static_cast<std::streamsize>(compressed.size()));
    }
    if(!stream_) {
      return std::nullopt;
    }
    if(compression == 1U) {
      return gzipDecompress(compressed);
    }
    if(compression == 2U) {
      return zlibDecompress(compressed);
    }
    return std::nullopt;
  }
  void writeChunk(int chunkX, int chunkZ, const std::vector<std::uint8_t>& rawChunk) {
    if(isOutsideRegion(chunkX, chunkZ)) {
      return;
    }
    const std::vector<std::uint8_t> compressed = zlibCompress(rawChunk);
    const std::size_t chunkBytes = compressed.size() + 5U;
    const std::size_t sectorsNeeded = (chunkBytes + sectorSize - 1U) / sectorSize;
    if(sectorsNeeded == 0 || sectorsNeeded >= 256U) {
      return;
    }
    const int chunkIndex = index(chunkX, chunkZ);
    const std::uint32_t oldInfo = chunkBlockInfo_[static_cast<std::size_t>(chunkIndex)];
    const std::uint32_t oldOffset = oldInfo >> 8U;
    const std::uint32_t oldCount = oldInfo & 0xFFU;
    std::uint32_t sectorOffset = 0;
    if(oldOffset != 0U && oldCount == sectorsNeeded) {
      sectorOffset = oldOffset;
    } else {
      if(oldOffset != 0U && oldCount > 0U) {
        for(std::uint32_t i = 0; i < oldCount && oldOffset + i < sectorFree_.size(); ++i) {
          sectorFree_[static_cast<std::size_t>(oldOffset + i)] = 1U;
        }
      }
      sectorOffset = findFreeRun(static_cast<std::uint32_t>(sectorsNeeded));
      if(sectorOffset == 0U) {
        sectorOffset = static_cast<std::uint32_t>(sectorFree_.size());
        appendSectors(static_cast<std::uint32_t>(sectorsNeeded));
      }
    }
    for(std::uint32_t i = 0; i < sectorsNeeded && sectorOffset + i < sectorFree_.size(); ++i) {
      sectorFree_[static_cast<std::size_t>(sectorOffset + i)] = 0U;
    }
    writeChunkData(sectorOffset, compressed, 2U);
    writeChunkBlockInfo(chunkIndex, (sectorOffset << 8U) | static_cast<std::uint32_t>(sectorsNeeded));
    writeChunkSaveTime(chunkIndex, static_cast<std::uint32_t>(std::time(nullptr)));
    // Single flush per chunk save instead of one per sub-write (data + 2 header
    // updates). The OS coalesces the buffered writes; durability per chunk is
    // unchanged, matching vanilla RandomAccessFile semantics.
    stream_.flush();
  }
  [[nodiscard]] int resetBytesWritten() {
    const int bytes = bytesWritten_;
    bytesWritten_ = 0;
    return bytes;
  }
  void flush() {
    stream_.flush();
  }
  void close() {
    if(stream_.is_open()) {
      stream_.flush();
      stream_.close();
    }
  }

private:
  static constexpr std::uint32_t sectorSize = 4096U;
  [[nodiscard]] static bool isOutsideRegion(int chunkX, int chunkZ) {
    return chunkX < 0 || chunkX >= 32 || chunkZ < 0 || chunkZ >= 32;
  }
  [[nodiscard]] static int index(int chunkX, int chunkZ) {
    return chunkX + chunkZ * 32;
  }
  void openOrCreate() {
    stream_.open(file_, std::ios::in | std::ios::out | std::ios::binary);
    if(!stream_.is_open()) {
      std::ofstream create(file_, std::ios::binary | std::ios::trunc);
      create.close();
      stream_.clear();
      stream_.open(file_, std::ios::in | std::ios::out | std::ios::binary);
    }
  }
  void initializeHeader() {
    const auto fileSize = fs::exists(file_) ? fs::file_size(file_) : 0ULL;
    if(fileSize < sectorSize * 2U) {
      stream_.seekp(0, std::ios::end);
      const std::vector<std::uint8_t> zeros(sectorSize * 2U, 0U);
      stream_.write(reinterpret_cast<const char*>(zeros.data()), static_cast<std::streamsize>(zeros.size()));
      stream_.flush();
      bytesWritten_ += static_cast<int>(zeros.size());
    } else if(fileSize % sectorSize != 0U) {
      const std::uint64_t remainder = fileSize % sectorSize;
      const std::uint64_t padding = sectorSize - remainder;
      stream_.seekp(0, std::ios::end);
      const std::vector<std::uint8_t> zeros(static_cast<std::size_t>(padding), 0U);
      stream_.write(reinterpret_cast<const char*>(zeros.data()), static_cast<std::streamsize>(zeros.size()));
      stream_.flush();
    }
    const std::uint64_t alignedSize = fs::file_size(file_);
    const std::size_t sectorCount = static_cast<std::size_t>(alignedSize / sectorSize);
    sectorFree_.assign(sectorCount, 1U);
    if(sectorFree_.size() >= 1U) {
      sectorFree_[0] = 0U;
    }
    if(sectorFree_.size() >= 2U) {
      sectorFree_[1] = 0U;
    }
    stream_.clear();
    stream_.seekg(0, std::ios::beg);
    for(std::size_t i = 0; i < chunkBlockInfo_.size(); ++i) {
      chunkBlockInfo_[i] = readU32();
      const std::uint32_t offset = chunkBlockInfo_[i] >> 8U;
      const std::uint32_t count = chunkBlockInfo_[i] & 0xFFU;
      if(offset == 0U || count == 0U) {
        continue;
      }
      if(offset + count > sectorFree_.size()) {
        continue;
      }
      for(std::uint32_t j = 0; j < count; ++j) {
        sectorFree_[static_cast<std::size_t>(offset + j)] = 0U;
      }
    }
    for(std::size_t i = 0; i < chunkSaveTimes_.size(); ++i) {
      chunkSaveTimes_[i] = readU32();
    }
  }
  void appendSectors(std::uint32_t count) {
    stream_.clear();
    stream_.seekp(0, std::ios::end);
    const std::vector<std::uint8_t> zeros(sectorSize, 0U);
    for(std::uint32_t i = 0; i < count; ++i) {
      stream_.write(reinterpret_cast<const char*>(zeros.data()), static_cast<std::streamsize>(zeros.size()));
      sectorFree_.push_back(0U);
    }
    stream_.flush();
    bytesWritten_ += static_cast<int>(sectorSize * count);
  }
  [[nodiscard]] std::uint32_t findFreeRun(std::uint32_t sectorsNeeded) const {
    if(sectorsNeeded == 0U || sectorFree_.size() < sectorsNeeded) {
      return 0U;
    }
    std::uint32_t runStart = 0U;
    std::uint32_t runLength = 0U;
    for(std::uint32_t i = 0U; i < sectorFree_.size(); ++i) {
      if(sectorFree_[static_cast<std::size_t>(i)] != 0U) {
        if(runLength == 0U) {
          runStart = i;
        }
        ++runLength;
        if(runLength >= sectorsNeeded) {
          return runStart;
        }
      } else {
        runLength = 0U;
      }
    }
    return 0U;
  }
  void writeChunkData(std::uint32_t sectorOffset, const std::vector<std::uint8_t>& compressed,
                      std::uint8_t compressionType) {
    stream_.clear();
    stream_.seekp(static_cast<std::streamoff>(sectorOffset * sectorSize));
    writeU32(static_cast<std::uint32_t>(compressed.size() + 1U));
    writeU8(compressionType);
    if(!compressed.empty()) {
      stream_.write(reinterpret_cast<const char*>(compressed.data()),
                    static_cast<std::streamsize>(compressed.size()));
    }
  }
  void writeChunkBlockInfo(int indexValue, std::uint32_t blockInfo) {
    chunkBlockInfo_[static_cast<std::size_t>(indexValue)] = blockInfo;
    writeU32At(static_cast<std::streamoff>(indexValue * 4), blockInfo);
  }
  void writeChunkSaveTime(int indexValue, std::uint32_t saveTime) {
    chunkSaveTimes_[static_cast<std::size_t>(indexValue)] = saveTime;
    writeU32At(static_cast<std::streamoff>(sectorSize + indexValue * 4), saveTime);
  }
  [[nodiscard]] std::uint32_t readU32() {
    std::array<std::uint8_t, 4> bytes{};
    stream_.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if(!stream_) {
      return 0U;
    }
    return (static_cast<std::uint32_t>(bytes[0]) << 24U) | (static_cast<std::uint32_t>(bytes[1]) << 16U) |
           (static_cast<std::uint32_t>(bytes[2]) << 8U) | static_cast<std::uint32_t>(bytes[3]);
  }
  void writeU32(std::uint32_t value) {
    const std::array<std::uint8_t, 4> bytes = {
        static_cast<std::uint8_t>((value >> 24U) & 0xFFU), static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 8U) & 0xFFU), static_cast<std::uint8_t>(value & 0xFFU)};
    stream_.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }
  [[nodiscard]] std::uint8_t readU8() {
    char value = 0;
    stream_.read(&value, 1);
    if(!stream_) {
      return 0U;
    }
    return static_cast<std::uint8_t>(value);
  }
  void writeU8(std::uint8_t value) {
    const char byte = static_cast<char>(value);
    stream_.write(&byte, 1);
  }
  void writeU32At(std::streamoff offset, std::uint32_t value) {
    stream_.clear();
    stream_.seekp(offset);
    writeU32(value);
  }
  fs::path file_;
  std::fstream stream_;
  std::array<std::uint32_t, 1024> chunkBlockInfo_{};
  std::array<std::uint32_t, 1024> chunkSaveTimes_{};
  std::vector<std::uint8_t> sectorFree_;
  int bytesWritten_ = 0;
};
} // namespace net::minecraft
