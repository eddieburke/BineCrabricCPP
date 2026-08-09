#include "net/minecraft/client/gl/ShaderBinaryCache.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <system_error>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
namespace net::minecraft::client::gl {
namespace {
constexpr char kMagic[8] = {'M', 'C', 'S', 'P', 'B', 'I', 'N', '1'};
constexpr std::uint32_t kFileVersion = 5;
#pragma pack(push, 1)
struct FileHeader {
 char magic[8];
 std::uint32_t version;
 std::uint64_t contentHash;
 unsigned int binaryFormat;
 std::uint32_t flags;
 std::uint32_t size;
};
#pragma pack(pop)
static_assert(sizeof(FileHeader) == 32);
} // namespace

ShaderBinaryCache::ShaderBinaryCache(std::filesystem::path root) : root_(std::move(root)) {
 if(!root_.empty()) writer_ = std::thread(&ShaderBinaryCache::writerLoop, this);
}

ShaderBinaryCache::~ShaderBinaryCache() {
 {
  std::lock_guard lock(writerMutex_);
  writerStop_ = true;
 }
 writerCv_.notify_one();
 if(writer_.joinable()) writer_.join();
}

std::wstring ShaderBinaryCache::nativePath(std::uint64_t contentHash) const {
 wchar_t name[32]{};
 std::swprintf(name, sizeof(name) / sizeof(wchar_t), L"%016llx.bin", static_cast<unsigned long long>(contentHash));
 return root_.native() + L"\\" + name;
}

std::optional<ProgramBinaryBlob> ShaderBinaryCache::tryLoad(std::uint64_t contentHash) {
 if(root_.empty()) return std::nullopt;
 {
  // A blob queued by storeAsync is not on disk yet. A pack reload drops the
  // linked programs and re-resolves every hash, so without this the reload
  // would race the writer thread and fall back to a source recompile.
  const std::lock_guard lock(writerMutex_);
  if(const auto pending = pendingWrites_.find(contentHash); pending != pendingWrites_.end()) {
   return *pending->second;
  }
 }
 const std::wstring path = nativePath(contentHash);
 HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
 if(file == INVALID_HANDLE_VALUE) return std::nullopt;
 LARGE_INTEGER fileSize;
 if(!GetFileSizeEx(file, &fileSize) || fileSize.QuadPart < static_cast<LONGLONG>(sizeof(FileHeader))) {
  CloseHandle(file);
  return std::nullopt;
 }
 FileHeader header{};
 DWORD bytesRead = 0;
 if(!ReadFile(file, &header, sizeof(header), &bytesRead, nullptr) || bytesRead != sizeof(header)) {
  CloseHandle(file);
  return std::nullopt;
 }
 if(std::memcmp(header.magic, kMagic, 8) != 0 || header.version != kFileVersion ||
    header.contentHash != contentHash || header.size == 0 || header.size > 64u * 1024u * 1024u ||
    header.binaryFormat == 0) {
  CloseHandle(file);
  return std::nullopt;
 }
 ProgramBinaryBlob blob;
 blob.contentHash = header.contentHash;
 blob.binaryFormat = header.binaryFormat;
 blob.flags = header.flags;
 blob.compute = (header.flags & ShaderProgram::kFlagCompute) != 0;
 blob.tessellation = (header.flags & ShaderProgram::kFlagTessellation) != 0;
 blob.bytes.resize(header.size);
 if(!ReadFile(file, blob.bytes.data(), header.size, &bytesRead, nullptr) || bytesRead != header.size) {
  CloseHandle(file);
  return std::nullopt;
 }
 CloseHandle(file);
 return blob;
}

bool ShaderBinaryCache::store(const ProgramBinaryBlob& blob) {
 if(root_.empty() || blob.bytes.empty() || blob.contentHash == 0 || blob.binaryFormat == 0) return false;
 std::error_code ec;
 std::filesystem::create_directories(root_, ec);
 const std::wstring path = nativePath(blob.contentHash);
 const std::wstring tmp = path + L".tmp";
 HANDLE file = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
 if(file == INVALID_HANDLE_VALUE) return false;
 FileHeader header{};
 std::memcpy(header.magic, kMagic, 8);
 header.version = kFileVersion;
 header.contentHash = blob.contentHash;
 header.binaryFormat = blob.binaryFormat;
 header.flags = blob.flags;
 header.size = static_cast<std::uint32_t>(blob.bytes.size());
 DWORD written = 0;
 bool ok = WriteFile(file, &header, sizeof(header), &written, nullptr) && written == sizeof(header);
 if(ok) {
  ok = WriteFile(file, blob.bytes.data(), header.size, &written, nullptr) && written == header.size;
 }
 CloseHandle(file);
 if(!ok) {
  DeleteFileW(tmp.c_str());
  return false;
 }
  if(!MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) {
   DeleteFileW(path.c_str());
   MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
  }
  return true;
}

void ShaderBinaryCache::storeAsync(ProgramBinaryBlob blob) {
 if(root_.empty() || blob.bytes.empty() || blob.contentHash == 0 || blob.binaryFormat == 0) {
  return;
 }
 {
  std::lock_guard lock(writerMutex_);
  if(pendingWrites_.contains(blob.contentHash)) return;
  auto pending = std::make_shared<const ProgramBinaryBlob>(std::move(blob));
  pendingWrites_.emplace(pending->contentHash, pending);
  writeQueue_.push_back(std::move(pending));
 }
 writerCv_.notify_one();
}

void ShaderBinaryCache::remove(std::uint64_t contentHash) {
 if(root_.empty()) return;
 {
  const std::lock_guard lock(writerMutex_);
  pendingWrites_.erase(contentHash);
  std::erase_if(writeQueue_, [contentHash](const auto& blob) {
   return blob->contentHash == contentHash;
  });
 }
 const std::wstring path = nativePath(contentHash);
 DeleteFileW(path.c_str());
}

void ShaderBinaryCache::writerLoop() {
 std::vector<std::shared_ptr<const ProgramBinaryBlob>> local;
 for(;;) {
  {
   std::unique_lock lock(writerMutex_);
   writerCv_.wait(lock, [this] { return writerStop_ || !writeQueue_.empty(); });
   if(writerStop_ && writeQueue_.empty()) return;
   local.swap(writeQueue_);
  }
  for(const auto& blob : local) {
   const std::uint64_t contentHash = blob->contentHash;
   store(*blob);
   const std::lock_guard lock(writerMutex_);
   const auto pending = pendingWrites_.find(contentHash);
   if(pending != pendingWrites_.end() && pending->second == blob) pendingWrites_.erase(pending);
  }
  local.clear();
 }
}
} // namespace net::minecraft::client::gl
