#pragma once
#include <cstdint>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <optional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/gl/ShaderProgram.hpp"
namespace net::minecraft::client::gl {
class ShaderBinaryCache {
 public:
 explicit ShaderBinaryCache(std::filesystem::path root);
 ~ShaderBinaryCache();
 ShaderBinaryCache(const ShaderBinaryCache&) = delete;
 ShaderBinaryCache& operator=(const ShaderBinaryCache&) = delete;
 [[nodiscard]] const std::filesystem::path& root() const noexcept { return root_; }
 [[nodiscard]] std::optional<ProgramBinaryBlob> tryLoad(std::uint64_t contentHash);
 bool store(const ProgramBinaryBlob& blob);
 void storeAsync(ProgramBinaryBlob blob);
 void remove(std::uint64_t contentHash);

 private:
  [[nodiscard]] std::wstring nativePath(std::uint64_t contentHash) const;
  bool storeOnDisk(const ProgramBinaryBlob& blob);
  void pruneDiskCache();
  void writerLoop();
  std::filesystem::path root_;
  std::thread writer_;
  mutable std::mutex writerMutex_;
  mutable std::mutex diskMutex_;
  std::condition_variable writerCv_;
  std::vector<std::shared_ptr<const ProgramBinaryBlob>> writeQueue_;
  // Only blobs whose disk write has not landed yet. Bounded by the write queue,
  // not by a byte budget: once a blob is on disk the linked program lives in
  // ProgramCache and re-reading it is a file read, not a recompile.
  std::unordered_map<std::uint64_t, std::shared_ptr<const ProgramBinaryBlob>> pendingWrites_;
  bool writerStop_ = false;
  std::uint64_t writesSincePrune_ = 0;
};
} // namespace net::minecraft::client::gl
