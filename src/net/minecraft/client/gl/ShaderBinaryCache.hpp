#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include "net/minecraft/client/gl/ShaderProgram.hpp"
namespace net::minecraft::client::gl {
class ShaderBinaryCache {
 public:
 explicit ShaderBinaryCache(std::filesystem::path root);
 void setRoot(std::filesystem::path root);
 [[nodiscard]] const std::filesystem::path& root() const noexcept {
  return root_;
 }
 [[nodiscard]] std::optional<ProgramBinaryBlob> tryLoad(std::uint64_t contentHash) const;
 bool store(const ProgramBinaryBlob& blob) const;
 void remove(std::uint64_t contentHash) const;

 private:
 [[nodiscard]] std::filesystem::path pathFor(std::uint64_t contentHash) const;
 std::filesystem::path root_;
};
} // namespace net::minecraft::client::gl
