#include "net/minecraft/client/gl/ShaderBinaryCache.hpp"
#include <cstdio>
#include <cstring>
#include <system_error>
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
namespace net::minecraft::client::gl {
namespace diagnostics = net::minecraft::client::diagnostics;
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

ShaderBinaryCache::ShaderBinaryCache(std::filesystem::path root) : root_(std::move(root)) {}

ShaderBinaryCache::~ShaderBinaryCache() {
 {
  std::lock_guard lock(writerMutex_);
  writerStop_ = true;
 }
 writerCv_.notify_one();
 if(writer_.joinable()) writer_.join();
}

void ShaderBinaryCache::setRoot(std::filesystem::path root) {
 root_ = std::move(root);
 scanDirectory();
 writerStop_ = false;
 if(!root_.empty() && !writer_.joinable()) {
  writer_ = std::thread(&ShaderBinaryCache::writerLoop, this);
 }
}

void ShaderBinaryCache::scanDirectory() {
 knownHashes_.clear();
 if(root_.empty()) return;
 std::error_code ec;
 for(const auto& entry : std::filesystem::directory_iterator(root_, ec)) {
  if(!entry.is_regular_file()) continue;
  const std::string name = entry.path().stem().string();
  if(name.size() != 16) continue;
  char* end = nullptr;
  const std::uint64_t hash = std::strtoull(name.c_str(), &end, 16);
  if(end == name.c_str() + 16 && hash != 0) knownHashes_.insert(hash);
 }
}

std::wstring ShaderBinaryCache::nativePath(std::uint64_t contentHash) const {
 wchar_t name[32]{};
 std::swprintf(name, sizeof(name) / sizeof(wchar_t), L"%016llx.bin", static_cast<unsigned long long>(contentHash));
 return root_.native() + L"\\" + name;
}

std::optional<ProgramBinaryBlob> ShaderBinaryCache::tryLoad(std::uint64_t contentHash) const {
 diagnostics::WorkSpan span("io.shader.disk.read");
 if(root_.empty()) return std::nullopt;
 if(knownHashes_.find(contentHash) == knownHashes_.end()) return std::nullopt;
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
 diagnostics::WorkSpan span("io.shader.disk.write");
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
 knownHashes_.insert(blob.contentHash);
 return true;
}

void ShaderBinaryCache::storeAsync(ProgramBinaryBlob blob) {
 if(root_.empty() || blob.bytes.empty() || blob.contentHash == 0 || blob.binaryFormat == 0) return;
 knownHashes_.insert(blob.contentHash);
 {
  std::lock_guard lock(writerMutex_);
  writeQueue_.push_back(std::move(blob));
 }
 writerCv_.notify_one();
}

void ShaderBinaryCache::remove(std::uint64_t contentHash) {
 if(root_.empty()) return;
 knownHashes_.erase(contentHash);
 const std::wstring path = nativePath(contentHash);
 DeleteFileW(path.c_str());
}

void ShaderBinaryCache::writerLoop() {
 std::vector<ProgramBinaryBlob> local;
 for(;;) {
  {
   std::unique_lock lock(writerMutex_);
   writerCv_.wait(lock, [this] { return writerStop_ || !writeQueue_.empty(); });
   if(writerStop_ && writeQueue_.empty()) return;
   local.swap(writeQueue_);
  }
  for(auto& blob : local) store(blob);
  local.clear();
 }
}
} // namespace net::minecraft::client::gl
