#pragma once
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_set>
#include "net/minecraft/client/gl/ShaderProgram.hpp"
namespace net::minecraft::client::gl {
// Synchronous shader binary cache — no background writer thread. All disk I/O
// happens on the calling (render) thread, eliminating thread synchronization
// overhead and race conditions. The render thread already owns all GL operations,
// so blocking writes here are at controlled points (after compile/link).
class ShaderBinaryCache {
 public:
 explicit ShaderBinaryCache(std::filesystem::path root = {});
 ~ShaderBinaryCache() = default;
 ShaderBinaryCache(const ShaderBinaryCache&) = delete;
 ShaderBinaryCache& operator=(const ShaderBinaryCache&) = delete;
 void setRoot(std::filesystem::path root);
 [[nodiscard]] const std::filesystem::path& root() const noexcept { return root_; }
 [[nodiscard]] std::optional<ProgramBinaryBlob> tryLoad(std::uint64_t contentHash) const;
 bool store(const ProgramBinaryBlob& blob);
 // Deprecated: kept for API compatibility, now just calls store() synchronously.
 void storeAsync(ProgramBinaryBlob blob) { store(blob); }
 void remove(std::uint64_t contentHash);

 private:
  void scanDirectory();
  [[nodiscard]] std::wstring nativePath(std::uint64_t contentHash) const;
  std::filesystem::path root_;
  // Guarded by knownMutex_: mutations happen on render thread, but we keep the
  // mutex for potential future use or if callers mutate from other threads.
  mutable std::mutex knownMutex_;
  std::unordered_set<std::uint64_t> knownHashes_;
};
} // namespace net::minecraft::client::gl
