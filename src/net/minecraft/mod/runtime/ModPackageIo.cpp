#include "net/minecraft/mod/runtime/ModPackageIo.hpp"
#include <algorithm>
#include <optional>
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
#include "net/minecraft/nbt/Compression.hpp"
#include "net/minecraft/util/json/JsonFields.hpp"
namespace net::minecraft::mod::runtime {
namespace {
std::uint16_t readU16(const std::vector<std::uint8_t>& data, std::size_t offset) {
  if(offset + 2 > data.size()) {
    return 0;
  }
  return static_cast<std::uint16_t>(data[offset]) | (static_cast<std::uint16_t>(data[offset + 1]) << 8U);
}
std::uint32_t readU32(const std::vector<std::uint8_t>& data, std::size_t offset) {
  if(offset + 4 > data.size()) {
    return 0;
  }
  return static_cast<std::uint32_t>(data[offset]) | (static_cast<std::uint32_t>(data[offset + 1]) << 8U) |
         (static_cast<std::uint32_t>(data[offset + 2]) << 16U) |
         (static_cast<std::uint32_t>(data[offset + 3]) << 24U);
}
} // namespace
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
  if(entryCount > kMaxModArchiveEntries) {
    return false;
  }
  std::uint64_t extractedBytes = 0;
  std::size_t offset = centralDirOffset;
  for(std::uint16_t i = 0; i < entryCount; ++i) {
    if(offset + 46 > archive.size() || readU32(archive, offset) != 0x02014b50U) {
      return false;
    }
    ZipEntry entry;
    entry.compressionMethod = readU16(archive, offset + 10);
    entry.compressedSize = readU32(archive, offset + 20);
    entry.uncompressedSize = readU32(archive, offset + 24);
    extractedBytes += entry.uncompressedSize;
    if(entry.uncompressedSize > kMaxModEntryBytes || extractedBytes > kMaxModExtractedBytes) {
      return false;
    }
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
const ZipEntry* findZipEntry(const std::vector<ZipEntry>& entries, std::string_view path) {
  const std::string normalized = normalizeRelativePath(path);
  for(const ZipEntry& entry : entries) {
    if(normalizeRelativePath(entry.name) == normalized) {
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
      return zlibDecompress(compressed);
    } catch(...) {
      return {};
    }
  }
  return {};
}
ModPackage makeBrokenPackage(ModPackageSource source,
                             const std::filesystem::path& sourcePath,
                             std::string id,
                             std::string error) {
  ModPackage info;
  info.id = std::move(id);
  info.name = sourcePath.filename().string();
  info.source = source;
  info.enabledByDefault = false;
  info.configuredEnabled = false;
  info.error = std::move(error);
  info.sourcePath = sourcePath;
  return info;
}
bool parseManifestJson(const std::string& manifestTextRaw,
                       ModPackage& out,
                       const std::filesystem::path& sourcePath,
                       ModPackageSource source,
                       std::string errorPrefix) {
  std::string manifestText = manifestTextRaw;
  if(manifestText.size() >= 3 && static_cast<unsigned char>(manifestText[0]) == 0xEF &&
     static_cast<unsigned char>(manifestText[1]) == 0xBB && static_cast<unsigned char>(manifestText[2]) == 0xBF) {
    manifestText.erase(0, 3);
  }
  const std::optional<std::string> id = util::json::stringField(manifestText, "id");
  if(!id.has_value() || trimCopy(*id).empty()) {
    out =
        makeBrokenPackage(source, sourcePath, sanitizeName(sourcePath.stem().string()), errorPrefix + "missing id");
    return false;
  }
  out.id = trimCopy(*id);
  if(!isSafeModId(out.id)) {
    out = makeBrokenPackage(source,
                            sourcePath,
                            sanitizeName(out.id),
                            errorPrefix + "id may contain only letters, digits, '.', '_' and '-'");
    return false;
  }
  const std::optional<std::string> name = util::json::stringField(manifestText, "name");
  out.name = name.has_value() && !trimCopy(*name).empty() ? trimCopy(*name) : out.id;
  if(const std::optional<std::string> version = util::json::stringField(manifestText, "version")) {
    out.version = trimCopy(*version);
  }
  if(const std::optional<std::string> description = util::json::stringField(manifestText, "description")) {
    out.description = trimCopy(*description);
  }
  if(const std::optional<std::string> downloadUrl = util::json::stringField(manifestText, "download_url")) {
    out.downloadUrl = trimCopy(*downloadUrl);
  }
  if(const std::optional<std::string> entry = util::json::stringField(manifestText, "entry")) {
    out.entry = normalizeRelativePath(trimCopy(*entry));
  }
  if(out.entry.empty()) {
    if(const std::optional<std::string> script = util::json::stringField(manifestText, "script")) {
      out.entry = normalizeRelativePath(trimCopy(*script));
    }
  }
  out.source = source;
  out.sourcePath = sourcePath;
  out.enabledByDefault = util::json::boolField(manifestText, "enabled").value_or(true);
  out.configuredEnabled = out.enabledByDefault;
  out.runtimeScript = !out.entry.empty() && toLowerCopy(out.entry).ends_with(".lua");
  if(!out.entry.empty() && !isSafeRelativePath(out.entry)) {
    out.error = "Unsafe Lua script path";
    out.enabledByDefault = false;
    out.configuredEnabled = false;
  } else if(!out.entry.empty() && !out.runtimeScript) {
    out.error = "Only Lua script entries are supported; use entry/script ending in .lua";
    out.enabledByDefault = false;
    out.configuredEnabled = false;
  }
  return true;
}
void sortMods(std::vector<ModPackage>& mods) {
  std::stable_sort(mods.begin(), mods.end(), [](const ModPackage& lhs, const ModPackage& rhs) {
    const std::string lhsName = toLowerCopy(lhs.name.empty() ? lhs.id : lhs.name);
    const std::string rhsName = toLowerCopy(rhs.name.empty() ? rhs.id : rhs.name);
    if(lhsName != rhsName) {
      return lhsName < rhsName;
    }
    return lhs.id < rhs.id;
  });
}
} // namespace net::minecraft::mod::runtime
