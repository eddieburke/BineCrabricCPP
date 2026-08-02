#include "net/minecraft/client/gl/ShaderBinaryCache.hpp"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <system_error>
namespace net::minecraft::client::gl {
namespace {
constexpr char kMagic[8] = {'M', 'C', 'S', 'P', 'B', 'I', 'N', '1'};
constexpr std::uint32_t kFileVersion = 4;

template <typename T>
bool writePod(std::ostream& out, const T& value) {
 out.write(reinterpret_cast<const char*>(&value), sizeof(T));
 return static_cast<bool>(out);
}
template <typename T>
bool readPod(std::istream& in, T& value) {
 in.read(reinterpret_cast<char*>(&value), sizeof(T));
 return static_cast<bool>(in);
}
} // namespace

ShaderBinaryCache::ShaderBinaryCache(std::filesystem::path root) : root_(std::move(root)) {}

void ShaderBinaryCache::setRoot(std::filesystem::path root) {
 root_ = std::move(root);
}

std::filesystem::path ShaderBinaryCache::pathFor(std::uint64_t contentHash) const {
 char name[32]{};
 std::snprintf(name, sizeof(name), "%016llx.bin", static_cast<unsigned long long>(contentHash));
 return root_ / name;
}

std::optional<ProgramBinaryBlob> ShaderBinaryCache::tryLoad(std::uint64_t contentHash) const {
 if(root_.empty()) return std::nullopt;
 const std::filesystem::path path = pathFor(contentHash);
 std::ifstream in(path, std::ios::binary);
 if(!in) return std::nullopt;
 char magic[8]{};
 if(!in.read(magic, 8) || std::memcmp(magic, kMagic, 8) != 0) return std::nullopt;
 std::uint32_t version = 0;
 std::uint64_t hash = 0;
 unsigned int format = 0;
 std::uint32_t flags = 0;
 std::uint32_t size = 0;
 if(!readPod(in, version) || version != kFileVersion || !readPod(in, hash) || hash != contentHash ||
    !readPod(in, format) || !readPod(in, flags) || !readPod(in, size) || size == 0 || size > 64u * 1024u * 1024u) {
  return std::nullopt;
 }
 ProgramBinaryBlob blob;
 blob.contentHash = hash;
 blob.binaryFormat = format;
 blob.flags = flags;
  blob.compute = (flags & ShaderProgram::kFlagCompute) != 0;
  blob.tessellation = (flags & ShaderProgram::kFlagTessellation) != 0;
 blob.bytes.resize(size);
 if(!in.read(reinterpret_cast<char*>(blob.bytes.data()), static_cast<std::streamsize>(size))) {
  return std::nullopt;
 }
 return blob;
}

bool ShaderBinaryCache::store(const ProgramBinaryBlob& blob) const {
 if(root_.empty() || blob.bytes.empty() || blob.contentHash == 0 || blob.binaryFormat == 0) return false;
 std::error_code ec;
 std::filesystem::create_directories(root_, ec);
 const std::filesystem::path path = pathFor(blob.contentHash);
 const std::filesystem::path tmp = path.string() + ".tmp";
 {
  std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
  if(!out) return false;
  out.write(kMagic, 8);
  const std::uint32_t version = kFileVersion;
  const std::uint32_t size = static_cast<std::uint32_t>(blob.bytes.size());
  if(!writePod(out, version) || !writePod(out, blob.contentHash) || !writePod(out, blob.binaryFormat) ||
     !writePod(out, blob.flags) || !writePod(out, size) ||
     !out.write(reinterpret_cast<const char*>(blob.bytes.data()),
                static_cast<std::streamsize>(blob.bytes.size()))) {
   out.close();
   std::filesystem::remove(tmp, ec);
   return false;
  }
 }
 std::filesystem::rename(tmp, path, ec);
 if(ec) {
  std::filesystem::remove(path, ec);
  std::filesystem::rename(tmp, path, ec);
 }
 return !ec;
}

void ShaderBinaryCache::remove(std::uint64_t contentHash) const {
 if(root_.empty()) return;
 std::error_code ec;
 std::filesystem::remove(pathFor(contentHash), ec);
}
} // namespace net::minecraft::client::gl
