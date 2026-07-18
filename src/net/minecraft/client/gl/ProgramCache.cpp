#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/resource/ResourceRoot.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
namespace net::minecraft::client::gl {
ProgramCache::ProgramCache(std::string resourceRoot) : resourceRoot_(std::move(resourceRoot)) {}
std::string ProgramCache::makeKey(const std::string& a,
                                  const std::string& b,
                                  const std::string& versionPreamble,
                                  const std::vector<ShaderDefine>& defines) {
  // Defines are order-independent for identity purposes, so sort a copy.
  std::vector<ShaderDefine> sorted = defines;
  std::sort(sorted.begin(), sorted.end(), [](const ShaderDefine& lhs, const ShaderDefine& rhs) {
    return lhs.name < rhs.name;
  });
  std::string key;
  key.reserve(a.size() + b.size() + versionPreamble.size() + 32 * sorted.size() + 16);
  key += a;
  key += '|';
  key += b;
  key += '|';
  key += versionPreamble;
  key += '|';
  for(const ShaderDefine& def : sorted) {
    key += def.name;
    key += '=';
    key += def.value;
    key += ';';
  }
  return key;
}
std::string ProgramCache::loadText(const std::string& relPath) const {
  const std::filesystem::path root = resourceRoot_.empty()
                                         ? resource::resourceRoot()
                                         : std::filesystem::path(resourceRoot_);
  std::ifstream in(root / relPath, std::ios::binary);
  if(!in) {
    return {};
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}
ShaderProgram* ProgramCache::get(const std::string& vshPath,
                                 const std::string& fshPath,
                                 const std::string& versionPreamble,
                                 const std::vector<ShaderDefine>& defines) {
  const std::string key = makeKey(vshPath, fshPath, versionPreamble, defines);
  const auto found = cache_.find(key);
  if(found != cache_.end()) {
    return found->second.failed ? nullptr : found->second.program.get();
  }
  const std::string vsrc = loadText(vshPath);
  const std::string fsrc = loadText(fshPath);
  Entry entry;
  if(vsrc.empty() || fsrc.empty()) {
    entry.failed = true;
    cache_.emplace(key, std::move(entry));
    return nullptr;
  }
  return getFromSource(key, vsrc, fsrc, versionPreamble, defines);
}
ShaderProgram* ProgramCache::getFromSource(const std::string& key,
                                           const std::string& vertexSource,
                                           const std::string& fragmentSource,
                                           const std::string& versionPreamble,
                                           const std::vector<ShaderDefine>& defines) {
  const auto found = cache_.find(key);
  if(found != cache_.end()) {
    return found->second.failed ? nullptr : found->second.program.get();
  }
  Entry entry;
  entry.program = std::make_unique<ShaderProgram>();
  if(!entry.program->compile(vertexSource, fragmentSource, versionPreamble, defines)) {
    entry.program.reset();
    entry.failed = true;
    cache_.emplace(key, std::move(entry));
    return nullptr;
  }
  ShaderProgram* raw = entry.program.get();
  cache_.emplace(key, std::move(entry));
  return raw;
}
void ProgramCache::clear() {
  cache_.clear();
}
} // namespace net::minecraft::client::gl
