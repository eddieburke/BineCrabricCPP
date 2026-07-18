#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/nbt/Compression.hpp"
namespace net::minecraft::client::resource::pack {
namespace {
bool texturePackTraceEnabled() {
  static const bool enabled = [] {
    const char* value = std::getenv("MINECRAFT_TRACE_TEXTURE_PACKS");
    if(value == nullptr || *value == '\0') {
      return false;
    }
    std::string normalized(value);
    for(char& ch : normalized) {
      ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
  }();
  return enabled;
}
void texturePackTrace(std::string message) {
  if(!texturePackTraceEnabled()) {
    return;
  }
  ClientLog::LOGGER.log(LogLevel::Fine, "[texture-pack-trace] " + message);
}
std::uint32_t readU32(const std::vector<std::uint8_t>& data, std::size_t offset) {
  if(offset + 4 > data.size()) {
    return 0;
  }
  return static_cast<std::uint32_t>(data[offset]) | (static_cast<std::uint32_t>(data[offset + 1]) << 8U) |
         (static_cast<std::uint32_t>(data[offset + 2]) << 16U) |
         (static_cast<std::uint32_t>(data[offset + 3]) << 24U);
}
std::uint16_t readU16(const std::vector<std::uint8_t>& data, std::size_t offset) {
  if(offset + 2 > data.size()) {
    return 0;
  }
  return static_cast<std::uint16_t>(data[offset]) | (static_cast<std::uint16_t>(data[offset + 1]) << 8U);
}
std::string normalizeZipPath(std::string_view path) {
  std::string normalized(path);
  while(!normalized.empty() && (normalized.front() == '/' || normalized.front() == '\\')) {
    normalized.erase(normalized.begin());
  }
  std::replace(normalized.begin(), normalized.end(), '\\', '/');
  return normalized;
}
std::string trimLine(std::string line) {
  if(line.size() > 34) {
    line.resize(34);
  }
  return line;
}
using ZipEntry = ZippedTexturePack::ZipEntry;
bool buildZipIndex(const std::vector<std::uint8_t>& archive, std::vector<ZipEntry>& entries) {
  if(archive.size() < 22) {
    return false;
  }
  std::size_t eocdOffset = std::string::npos;
  for(std::size_t i = archive.size() - 22; i != std::string::npos; --i) {
    if(readU32(archive, i) == 0x06054b50U) {
      eocdOffset = i;
      break;
    }
    if(i == 0) {
      break;
    }
  }
  if(eocdOffset == std::string::npos) {
    return false;
  }
  const std::uint32_t centralDirOffset = readU32(archive, eocdOffset + 16);
  const std::uint16_t entryCount = readU16(archive, eocdOffset + 10);
  std::size_t offset = centralDirOffset;
  for(std::uint16_t i = 0; i < entryCount; ++i) {
    if(offset + 46 > archive.size() || readU32(archive, offset) != 0x02014b50U) {
      return false;
    }
    ZipEntry entry;
    entry.compressionMethod = readU16(archive, offset + 10);
    entry.compressedSize = readU32(archive, offset + 20);
    entry.uncompressedSize = readU32(archive, offset + 24);
    const std::uint16_t nameLength = readU16(archive, offset + 28);
    const std::uint16_t extraLength = readU16(archive, offset + 30);
    const std::uint16_t commentLength = readU16(archive, offset + 32);
    entry.localHeaderOffset = readU32(archive, offset + 42);
    if(offset + 46 + nameLength > archive.size()) {
      return false;
    }
    entry.name.assign(reinterpret_cast<const char*>(archive.data() + offset + 46), nameLength);
    entries.push_back(std::move(entry));
    offset += 46 + nameLength + extraLength + commentLength;
  }
  return true;
}
const ZipEntry* findEntry(const std::vector<ZipEntry>& entries, std::string_view path) {
  const std::string normalized = normalizeZipPath(path);
  for(const ZipEntry& entry : entries) {
    if(entry.name == normalized) {
      return &entry;
    }
  }
  return nullptr;
}
std::vector<std::uint8_t> readZipEntryData(const std::vector<std::uint8_t>& archive, const ZipEntry& entry) {
  const std::size_t offset = entry.localHeaderOffset;
  if(offset + 30 > archive.size() || readU32(archive, offset) != 0x04034b50U) {
    return {};
  }
  const std::uint16_t nameLength = readU16(archive, offset + 26);
  const std::uint16_t extraLength = readU16(archive, offset + 28);
  const std::size_t dataOffset = offset + 30 + nameLength + extraLength;
  if(dataOffset + entry.compressedSize > archive.size()) {
    return {};
  }
  std::vector<std::uint8_t> compressed(
      archive.begin() + static_cast<std::ptrdiff_t>(dataOffset),
      archive.begin() + static_cast<std::ptrdiff_t>(dataOffset + entry.compressedSize));
  if(entry.compressionMethod == 0) {
    return compressed;
  }
  if(entry.compressionMethod == 8) {
    if(std::vector<std::uint8_t> inflated = decompressRawDeflate(compressed, entry.uncompressedSize);
       !inflated.empty()) {
      return inflated;
    }
    try {
      // Fallback for non-standard zip writers that embed zlib-wrapped deflate.
      return zlibDecompress(compressed);
    } catch(...) {
      texturePackTrace("failed to inflate entry '" + entry.name + "' method=8");
      return {};
    }
  }
  texturePackTrace("unsupported compression method for entry '" + entry.name +
                   "' method=" + std::to_string(entry.compressionMethod));
  return {};
}
} // namespace
ZippedTexturePack::ZippedTexturePack(std::filesystem::path file, const TexturePack* fallbackResources)
    : file_(std::move(file)), fallbackResources_(fallbackResources) {
  name = file_.filename().string();
  descriptionLine1 = name;
  key = name;
}
ZippedTexturePack::~ZippedTexturePack() {
  close();
}
void ZippedTexturePack::open() {
  archive_.clear();
  entries_.clear();
  std::ifstream input(file_, std::ios::binary);
  if(!input) {
    return;
  }
  input.seekg(0, std::ios::end);
  const std::streamsize size = input.tellg();
  if(size <= 0) {
    return;
  }
  input.seekg(0, std::ios::beg);
  archive_.resize(static_cast<std::size_t>(size));
  if(!input.read(reinterpret_cast<char*>(archive_.data()), size)) {
    archive_.clear();
    return;
  }
  if(!buildZipIndex(archive_, entries_)) {
    texturePackTrace("failed to index texture pack zip '" + file_.string() + "'");
    archive_.clear();
    entries_.clear();
    return;
  }
  texturePackTrace("opened texture pack zip '" + file_.string() + "' entries=" + std::to_string(entries_.size()));
}
void ZippedTexturePack::close() {
  archive_.clear();
  entries_.clear();
}
void ZippedTexturePack::load() {
  descriptionLine1 = name;
  descriptionLine2.clear();
  icon_.reset();
  iconId_ = -1;
  std::ifstream input(file_, std::ios::binary);
  if(!input) {
    return;
  }
  input.seekg(0, std::ios::end);
  const std::streamsize size = input.tellg();
  if(size <= 0) {
    return;
  }
  input.seekg(0, std::ios::beg);
  std::vector<std::uint8_t> tempArchive(static_cast<std::size_t>(size));
  if(!input.read(reinterpret_cast<char*>(tempArchive.data()), size)) {
    return;
  }
  std::vector<ZipEntry> tempEntries;
  if(!buildZipIndex(tempArchive, tempEntries)) {
    return;
  }
  const auto readEntry = [&](std::string_view path) -> std::vector<std::uint8_t> {
    const ZipEntry* entry = findEntry(tempEntries, path);
    if(entry == nullptr) {
      return {};
    }
    return readZipEntryData(tempArchive, *entry);
  };
  if(const std::vector<std::uint8_t> packText = readEntry("pack.txt"); !packText.empty()) {
    std::string content(packText.begin(), packText.end());
    std::istringstream stream(content);
    std::string line1;
    std::string line2;
    if(std::getline(stream, line1)) {
      descriptionLine1 = trimLine(line1);
    }
    if(std::getline(stream, line2)) {
      descriptionLine2 = trimLine(line2);
    }
  }
  if(const std::vector<std::uint8_t> iconBytes = readEntry("pack.png"); !iconBytes.empty()) {
    const texture::RasterImage raster = texture::TextureManager::loadRasterFromBytes(iconBytes);
    if(raster.width > 0 && raster.height > 0) {
      icon_ = raster;
    }
  }
}
void ZippedTexturePack::unload(texture::TextureManager& textureManager) {
  if(iconId_ >= 0) {
    textureManager.deleteTexture(iconId_);
    iconId_ = -1;
  }
  close();
}
void ZippedTexturePack::bindIcon(texture::TextureManager& textureManager) {
  if(icon_.has_value() && iconId_ < 0) {
    iconId_ = textureManager.load(*icon_);
  }
  if(icon_.has_value() && iconId_ >= 0) {
    textureManager.bindTexture(iconId_);
    return;
  }
  gl::bindTexture(gl::cap::Texture2D, textureManager.getTextureId("/gui/unknown_pack.png"));
}
std::vector<std::uint8_t> ZippedTexturePack::getResource(std::string_view path) const {
  if(!archive_.empty()) {
    const ZipEntry* entry = findEntry(entries_, path);
    if(entry != nullptr) {
      return readZipEntryData(archive_, *entry);
    }
  }
  if(fallbackResources_ != nullptr) {
    return fallbackResources_->getResource(path);
  }
  return {};
}
} // namespace net::minecraft::client::resource::pack
