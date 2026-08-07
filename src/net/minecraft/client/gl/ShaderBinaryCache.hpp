#pragma once
#include <cstdint>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include "net/minecraft/client/gl/ShaderProgram.hpp"
namespace net::minecraft::client::gl {
class ShaderBinaryCache {
 public:
 explicit ShaderBinaryCache(std::filesystem::path root);
 ~ShaderBinaryCache();
 ShaderBinaryCache(const ShaderBinaryCache&) = delete;
 ShaderBinaryCache& operator=(const ShaderBinaryCache&) = delete;
 void setRoot(std::filesystem::path root);
 [[nodiscard]] const std::filesystem::path& root() const noexcept { return root_; }
 [[nodiscard]] std::optional<ProgramBinaryBlob> tryLoad(std::uint64_t contentHash) const;
 bool store(const ProgramBinaryBlob& blob);
 void storeAsync(ProgramBinaryBlob blob);
 void remove(std::uint64_t contentHash);

 private:
  [[nodiscard]] std::wstring nativePath(std::uint64_t contentHash) const;
  void writerLoop();
  std::filesystem::path root_;
  std::thread writer_;
  std::mutex writerMutex_;
  std::condition_variable writerCv_;
  std::vector<ProgramBinaryBlob> writeQueue_;
  bool writerStop_ = false;
};
} // namespace net::minecraft::client::gl
