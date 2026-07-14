#include "net/minecraft/stat/StatFile.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <vector>
namespace net::minecraft::stat {
namespace {
constexpr std::array<char, 4> kMagic{'M', 'N', 'S', 'T'};
constexpr std::uint32_t kVersion = 1;
void writeU32(std::ostream& out, std::uint32_t value) {
  const auto bytes = std::array<char, 4>{
      static_cast<char>((value >> 0) & 0xFF),
      static_cast<char>((value >> 8) & 0xFF),
      static_cast<char>((value >> 16) & 0xFF),
      static_cast<char>((value >> 24) & 0xFF),
  };
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}
void writeI32(std::ostream& out, std::int32_t value) {
  writeU32(out, static_cast<std::uint32_t>(value));
}
[[nodiscard]] bool readU32(std::istream& in, std::uint32_t& value) {
  std::array<char, 4> bytes{};
  if(!in.read(bytes.data(), static_cast<std::streamsize>(bytes.size()))) {
    return false;
  }
  value = static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[0])) |
          (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[1])) << 8) |
          (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[2])) << 16) |
          (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[3])) << 24);
  return true;
}
[[nodiscard]] bool readI32(std::istream& in, std::int32_t& value) {
  std::uint32_t raw = 0;
  if(!readU32(in, raw)) {
    return false;
  }
  value = static_cast<std::int32_t>(raw);
  return true;
}
} // namespace
bool readStatFile(const std::filesystem::path& path, std::unordered_map<int, int>& out) {
  std::ifstream input(path, std::ios::binary);
  if(!input) {
    return false;
  }
  std::array<char, 4> magic{};
  if(!input.read(magic.data(), static_cast<std::streamsize>(magic.size())) || magic != kMagic) {
    return false;
  }
  std::uint32_t version = 0;
  std::uint32_t count = 0;
  if(!readU32(input, version) || version != kVersion || !readU32(input, count)) {
    return false;
  }
  out.clear();
  out.reserve(static_cast<std::size_t>(count));
  for(std::uint32_t i = 0; i < count; ++i) {
    std::int32_t statId = 0;
    std::int32_t amount = 0;
    if(!readI32(input, statId) || !readI32(input, amount)) {
      out.clear();
      return false;
    }
    if(amount != 0) {
      out[statId] = amount;
    }
  }
  return true;
}
bool writeStatFile(const std::filesystem::path& path, const std::unordered_map<int, int>& stats) {
  const std::filesystem::path tempPath = path.string() + ".tmp";
  {
    std::ofstream output(tempPath, std::ios::binary | std::ios::trunc);
    if(!output) {
      return false;
    }
    output.write(kMagic.data(), static_cast<std::streamsize>(kMagic.size()));
    writeU32(output, kVersion);
    writeU32(output, static_cast<std::uint32_t>(stats.size()));
    for(const auto& [statId, amount] : stats) {
      writeI32(output, statId);
      writeI32(output, amount);
    }
    if(!output) {
      return false;
    }
  }
  std::error_code error;
  std::filesystem::rename(tempPath, path, error);
  if(error) {
    std::filesystem::remove(path, error);
    error.clear();
    std::filesystem::rename(tempPath, path, error);
  }
  return !error;
}
} // namespace net::minecraft::stat
