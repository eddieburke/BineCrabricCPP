#include "net/minecraft/network/HandshakeMetadata.hpp"
#include "net/minecraft/mod/runtime/ModHostUtil.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
#include <cctype>
#include <sstream>
namespace net::minecraft::network {
namespace {
std::string percentEncode(std::string_view value) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  std::string out;
  out.reserve(value.size() * 3U);
  for(const unsigned char ch : value) {
    if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' ||
       ch == '.' || ch == '~') {
      out.push_back(static_cast<char>(ch));
      continue;
    }
    out.push_back('%');
    out.push_back(kHex[ch >> 4U]);
    out.push_back(kHex[ch & 0x0FU]);
  }
  return out;
}
int fromHex(char ch) {
  if(ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  if(ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  }
  return -1;
}
std::string percentDecode(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for(std::size_t i = 0; i < value.size(); ++i) {
    if(value[i] == '%' && i + 2 < value.size()) {
      const int hi = fromHex(value[i + 1]);
      const int lo = fromHex(value[i + 2]);
      if(hi >= 0 && lo >= 0) {
        out.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }
    out.push_back(value[i] == '+' ? ' ' : value[i]);
  }
  return out;
}
} // namespace
std::string appendHandshakeMetadata(const std::string& serverId, bool nativeCppMods, bool luaModsEnabled,
                                    const std::vector<std::string>& requiredMods,
                                    const std::unordered_map<std::string, std::string>& downloadUrls) {
  std::ostringstream out;
  out << serverId << ";omega=kind=" << (nativeCppMods ? "cpp" : "java") << "&lua=" << (luaModsEnabled ? "1" : "0");
  if(!requiredMods.empty()) {
    out << "&mods=" << percentEncode(mod::runtime::WorldRequiredMods::joinCsv(requiredMods));
  }
  for(const auto& [modId, url] : downloadUrls) {
    if(mod::runtime::isSafeModId(modId) && !url.empty()) {
      out << "&dl." << modId << '=' << percentEncode(url);
    }
  }
  return out.str();
}
HandshakeMetadata parseHandshakeMetadata(const std::string& raw) {
  HandshakeMetadata out;
  const std::size_t omegaMarker = raw.find(";omega=");
  const std::size_t legacyModsMarker = raw.find(";mods=");
  if(omegaMarker == std::string::npos) {
    if(legacyModsMarker == std::string::npos) {
      out.serverId = raw;
      return out;
    }
    out.serverId = raw.substr(0, legacyModsMarker);
    out.requiredMods = mod::runtime::WorldRequiredMods::splitCsv(raw.substr(legacyModsMarker + 6));
    out.nativeCppMods = !out.requiredMods.empty();
    out.luaModsEnabled = out.nativeCppMods;
    return out;
  }
  out.serverId = raw.substr(0, omegaMarker);
  out.hasOmegaMetadata = true;
  const std::string query = raw.substr(omegaMarker + 7);
  std::size_t cursor = 0;
  while(cursor <= query.size()) {
    const std::size_t next = query.find('&', cursor);
    const std::string_view pair(query.data() + cursor, (next == std::string::npos ? query.size() : next) - cursor);
    if(!pair.empty()) {
      const std::size_t equals = pair.find('=');
      const std::string key(pair.substr(0, equals));
      const std::string value = equals == std::string::npos ? std::string{} : percentDecode(pair.substr(equals + 1));
      if(key == "kind") {
        out.nativeCppMods = value == "cpp";
      } else if(key == "lua") {
        out.luaModsEnabled = value == "1" || mod::runtime::toLowerCopy(value) == "true";
      } else if(key == "mods") {
        out.requiredMods = mod::runtime::WorldRequiredMods::splitCsv(value);
      } else if(key.rfind("dl.", 0) == 0) {
        const std::string modId = key.substr(3);
        if(mod::runtime::isSafeModId(modId) && !value.empty()) {
          out.downloadUrls[modId] = value;
        }
      }
    }
    if(next == std::string::npos) {
      break;
    }
    cursor = next + 1;
  }
  return out;
}
} // namespace net::minecraft::network
