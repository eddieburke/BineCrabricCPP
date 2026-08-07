#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <array>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "net/minecraft/nbt/BinaryIO.hpp"
#include "net/minecraft/nbt/Compression.hpp"
namespace net::minecraft {
namespace fs = std::filesystem;
class RegionFile {
 public:
  struct CompressedChunk {
   std::uint8_t compression = 0;
   std::vector<std::uint8_t> bytes;
  };
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
  [[nodiscard]] std::optional<CompressedChunk> readCompressedChunk(int chunkX, int chunkZ) {
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
   const std::uint64_t base = static_cast<std::uint64_t>(sectorOffset) * sectorSize;
   const std::uint32_t length = readU32At(base);
   if(length == 0 || length > sectorCount * sectorSize) {
    return std::nullopt;
   }
   const std::uint8_t compression = readU8At(base + 4U);
   std::vector<std::uint8_t> compressed(length - 1U);
   if(!compressed.empty() && !readAt(base + 5U, compressed.data(), compressed.size())) {
    return std::nullopt;
   }
   return CompressedChunk{compression, std::move(compressed)};
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
  }
  [[nodiscard]] int resetBytesWritten() {
   const int bytes = bytesWritten_;
   bytesWritten_ = 0;
   return bytes;
  }
  void flush() {
   if(handle_ != INVALID_HANDLE_VALUE) {
    ::FlushFileBuffers(handle_);
   }
  }
  void close() {
   if(handle_ != INVALID_HANDLE_VALUE) {
    ::FlushFileBuffers(handle_);
    ::CloseHandle(handle_);
    handle_ = INVALID_HANDLE_VALUE;
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
   handle_ = ::CreateFileW(file_.wstring().c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           nullptr);
  }
  [[nodiscard]] std::uint64_t fileSize() const {
   LARGE_INTEGER size{};
   if(handle_ == INVALID_HANDLE_VALUE || !::GetFileSizeEx(handle_, &size)) {
    return 0ULL;
   }
   return static_cast<std::uint64_t>(size.QuadPart);
  }
  void initializeHeader() {
   const std::uint64_t size = fileSize();
   if(size < sectorSize * 2U) {
    appendSectors(2U);
   } else if(size % sectorSize != 0U) {
    const std::uint64_t padding = sectorSize - size % sectorSize;
    const std::vector<std::uint8_t> zeros(static_cast<std::size_t>(padding), 0U);
    writeAtEnd(zeros.data(), zeros.size());
   }
   const std::uint64_t alignedSize = fileSize();
   const std::size_t sectorCount = static_cast<std::size_t>(alignedSize / sectorSize);
   sectorFree_.assign(sectorCount, 1U);
   if(sectorFree_.size() >= 1U) {
    sectorFree_[0] = 0U;
   }
   if(sectorFree_.size() >= 2U) {
    sectorFree_[1] = 0U;
   }
   std::array<std::uint8_t, sectorSize * 2U> header{};
   if(!readAt(0, header.data(), header.size())) {
    return;
   }
   for(std::size_t i = 0; i < chunkBlockInfo_.size(); ++i) {
    chunkBlockInfo_[i] = (static_cast<std::uint32_t>(header[i * 4U]) << 24U) |
                         (static_cast<std::uint32_t>(header[i * 4U + 1U]) << 16U) |
                         (static_cast<std::uint32_t>(header[i * 4U + 2U]) << 8U) |
                         static_cast<std::uint32_t>(header[i * 4U + 3U]);
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
    chunkSaveTimes_[i] = (static_cast<std::uint32_t>(header[sectorSize + i * 4U]) << 24U) |
                         (static_cast<std::uint32_t>(header[sectorSize + i * 4U + 1U]) << 16U) |
                         (static_cast<std::uint32_t>(header[sectorSize + i * 4U + 2U]) << 8U) |
                         static_cast<std::uint32_t>(header[sectorSize + i * 4U + 3U]);
   }
  }
  void appendSectors(std::uint32_t count) {
   const std::vector<std::uint8_t> zeros(sectorSize, 0U);
   for(std::uint32_t i = 0; i < count; ++i) {
    writeAtEnd(zeros.data(), zeros.size());
    sectorFree_.push_back(0U);
   }
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
  void writeChunkData(std::uint32_t sectorOffset,
                      const std::vector<std::uint8_t>& compressed,
                      std::uint8_t compressionType) {
   const std::uint64_t base = static_cast<std::uint64_t>(sectorOffset) * sectorSize;
   writeU32At(base, static_cast<std::uint32_t>(compressed.size() + 1U));
   writeU8At(base + 4U, compressionType);
   if(!compressed.empty()) {
    writeAt(base + 5U, compressed.data(), compressed.size());
   }
  }
  void writeChunkBlockInfo(int indexValue, std::uint32_t blockInfo) {
   chunkBlockInfo_[static_cast<std::size_t>(indexValue)] = blockInfo;
   writeU32At(static_cast<std::uint64_t>(indexValue * 4), blockInfo);
  }
  void writeChunkSaveTime(int indexValue, std::uint32_t saveTime) {
   chunkSaveTimes_[static_cast<std::size_t>(indexValue)] = saveTime;
   writeU32At(sectorSize + static_cast<std::uint64_t>(indexValue * 4), saveTime);
  }
  [[nodiscard]] bool setPosition(std::uint64_t offset) const {
   if(handle_ == INVALID_HANDLE_VALUE) {
    return false;
   }
   LARGE_INTEGER position{};
   position.QuadPart = static_cast<LONGLONG>(offset);
   return ::SetFilePointerEx(handle_, position, nullptr, FILE_BEGIN) != 0;
  }
  [[nodiscard]] bool readAt(std::uint64_t offset, void* buffer, std::size_t size) const {
   if(!setPosition(offset)) {
    return false;
   }
   DWORD read = 0;
   return ::ReadFile(handle_, buffer, static_cast<DWORD>(size), &read, nullptr) != 0 &&
          read == static_cast<DWORD>(size);
  }
  bool writeAt(std::uint64_t offset, const void* buffer, std::size_t size) {
   if(!setPosition(offset)) {
    return false;
   }
   DWORD written = 0;
   return ::WriteFile(handle_, buffer, static_cast<DWORD>(size), &written, nullptr) != 0 &&
          written == static_cast<DWORD>(size);
  }
  bool writeAtEnd(const void* buffer, std::size_t size) {
   if(handle_ == INVALID_HANDLE_VALUE) {
    return false;
   }
   LARGE_INTEGER end{};
   end.QuadPart = 0;
   if(!::SetFilePointerEx(handle_, end, nullptr, FILE_END)) {
    return false;
   }
   DWORD written = 0;
   return ::WriteFile(handle_, buffer, static_cast<DWORD>(size), &written, nullptr) != 0 &&
          written == static_cast<DWORD>(size);
  }
  [[nodiscard]] std::uint32_t readU32At(std::uint64_t offset) const {
   std::array<std::uint8_t, 4> bytes{};
   if(!readAt(offset, bytes.data(), bytes.size())) {
    return 0U;
   }
   return (static_cast<std::uint32_t>(bytes[0]) << 24U) | (static_cast<std::uint32_t>(bytes[1]) << 16U) |
          (static_cast<std::uint32_t>(bytes[2]) << 8U) | static_cast<std::uint32_t>(bytes[3]);
  }
  void writeU32At(std::uint64_t offset, std::uint32_t value) {
   const std::array<std::uint8_t, 4> bytes = {static_cast<std::uint8_t>((value >> 24U) & 0xFFU),
                                              static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
                                              static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
                                              static_cast<std::uint8_t>(value & 0xFFU)};
   writeAt(offset, bytes.data(), bytes.size());
  }
  [[nodiscard]] std::uint8_t readU8At(std::uint64_t offset) const {
   std::uint8_t value = 0;
   if(!readAt(offset, &value, 1)) {
    return 0U;
   }
   return value;
  }
  void writeU8At(std::uint64_t offset, std::uint8_t value) {
   writeAt(offset, &value, 1);
  }
  fs::path file_;
  HANDLE handle_ = INVALID_HANDLE_VALUE;
  std::array<std::uint32_t, 1024> chunkBlockInfo_{};
  std::array<std::uint32_t, 1024> chunkSaveTimes_{};
  std::vector<std::uint8_t> sectorFree_;
  int bytesWritten_ = 0;
};
} // namespace net::minecraft
