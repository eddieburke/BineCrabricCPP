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
#include <cstring>
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
   // One read for the whole record. The 4-byte length, the compression byte and
   // the payload are contiguous inside the chunk's own sectors, so fetching them
   // separately cost three seek+read round trips per chunk load — two of which
   // were for five bytes.
   std::vector<std::uint8_t> record(static_cast<std::size_t>(sectorCount) * sectorSize);
   if(!readAt(static_cast<std::uint64_t>(sectorOffset) * sectorSize, record.data(), record.size())) {
    return std::nullopt;
   }
   const std::uint32_t length = readU32(record.data());
   // writeChunk sizes the run as ceil((payload + 5) / sectorSize), so a record
   // whose declared length does not fit its own sectors is malformed.
   if(length == 0 || length + 4U > record.size()) {
    return std::nullopt;
   }
   const std::uint8_t compression = record[4];
   record.erase(record.begin(), record.begin() + 5);
   record.resize(length - 1U);
   return CompressedChunk{compression, std::move(record)};
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
    LARGE_INTEGER aligned{};
    aligned.QuadPart = static_cast<LONGLONG>(size + (sectorSize - size % sectorSize));
    if(handle_ != INVALID_HANDLE_VALUE && ::SetFilePointerEx(handle_, aligned, nullptr, FILE_BEGIN) != 0) {
     ::SetEndOfFile(handle_);
    }
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
   if(count == 0U || handle_ == INVALID_HANDLE_VALUE) {
    return;
   }
   // Extend by moving the end of file, not by writing zeros: Windows reads the
   // gap back as zeros, so growing a region by N sectors is one syscall instead
   // of N writes of a freshly allocated 4 KiB zero buffer each.
   LARGE_INTEGER end{};
   end.QuadPart = static_cast<LONGLONG>(fileSize() + static_cast<std::uint64_t>(count) * sectorSize);
   if(::SetFilePointerEx(handle_, end, nullptr, FILE_BEGIN) == 0 || ::SetEndOfFile(handle_) == 0) {
    return;
   }
   sectorFree_.insert(sectorFree_.end(), static_cast<std::size_t>(count), 0U);
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
   // Header and payload go out in one write — they are contiguous, and issuing
   // them as three separate seek+write pairs tripled the syscalls per save.
   std::vector<std::uint8_t> record(compressed.size() + 5U);
   writeU32(record.data(), static_cast<std::uint32_t>(compressed.size() + 1U));
   record[4] = compressionType;
   if(!compressed.empty()) {
    std::memcpy(record.data() + 5, compressed.data(), compressed.size());
   }
   writeAt(static_cast<std::uint64_t>(sectorOffset) * sectorSize, record.data(), record.size());
  }
  void writeChunkBlockInfo(int indexValue, std::uint32_t blockInfo) {
   chunkBlockInfo_[static_cast<std::size_t>(indexValue)] = blockInfo;
   writeU32At(static_cast<std::uint64_t>(indexValue * 4), blockInfo);
  }
  void writeChunkSaveTime(int indexValue, std::uint32_t saveTime) {
   chunkSaveTimes_[static_cast<std::size_t>(indexValue)] = saveTime;
   writeU32At(sectorSize + static_cast<std::uint64_t>(indexValue * 4), saveTime);
  }
  // Positional I/O. An OVERLAPPED carrying the offset works on a synchronous
  // handle and completes before the call returns, so every read/write is one
  // syscall instead of a SetFilePointerEx paired with a ReadFile/WriteFile.
  // It also leaves no shared file pointer to race on.
  [[nodiscard]] static OVERLAPPED positionAt(std::uint64_t offset) noexcept {
   OVERLAPPED overlapped{};
   overlapped.Offset = static_cast<DWORD>(offset & 0xFFFFFFFFULL);
   overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32U);
   return overlapped;
  }
  [[nodiscard]] bool readAt(std::uint64_t offset, void* buffer, std::size_t size) const {
   if(handle_ == INVALID_HANDLE_VALUE) {
    return false;
   }
   OVERLAPPED overlapped = positionAt(offset);
   DWORD read = 0;
   return ::ReadFile(handle_, buffer, static_cast<DWORD>(size), &read, &overlapped) != 0 &&
          read == static_cast<DWORD>(size);
  }
  bool writeAt(std::uint64_t offset, const void* buffer, std::size_t size) {
   if(handle_ == INVALID_HANDLE_VALUE) {
    return false;
   }
   OVERLAPPED overlapped = positionAt(offset);
   DWORD written = 0;
   return ::WriteFile(handle_, buffer, static_cast<DWORD>(size), &written, &overlapped) != 0 &&
          written == static_cast<DWORD>(size);
  }
  [[nodiscard]] static std::uint32_t readU32(const std::uint8_t* bytes) noexcept {
   return (static_cast<std::uint32_t>(bytes[0]) << 24U) | (static_cast<std::uint32_t>(bytes[1]) << 16U) |
          (static_cast<std::uint32_t>(bytes[2]) << 8U) | static_cast<std::uint32_t>(bytes[3]);
  }
  static void writeU32(std::uint8_t* bytes, std::uint32_t value) noexcept {
   bytes[0] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
   bytes[1] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
   bytes[2] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
   bytes[3] = static_cast<std::uint8_t>(value & 0xFFU);
  }
  void writeU32At(std::uint64_t offset, std::uint32_t value) {
   std::array<std::uint8_t, 4> bytes{};
   writeU32(bytes.data(), value);
   writeAt(offset, bytes.data(), bytes.size());
  }
  fs::path file_;
  HANDLE handle_ = INVALID_HANDLE_VALUE;
  std::array<std::uint32_t, 1024> chunkBlockInfo_{};
  std::array<std::uint32_t, 1024> chunkSaveTimes_{};
  std::vector<std::uint8_t> sectorFree_;
  int bytesWritten_ = 0;
};
} // namespace net::minecraft
