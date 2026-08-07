#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
namespace net::minecraft::client::render {
namespace PackCatalog {
std::string lower(std::string value) {
 for(char& ch : value) {
  ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
 }
 return value;
}
std::string readFile(const std::filesystem::path& path) {
 std::ifstream input(path, std::ios::binary);
 if(!input) {
  return {};
 }
 input.seekg(0, std::ios::end);
 const std::streamsize size = input.tellg();
 if(size <= 0) {
  return {};
 }
 input.seekg(0, std::ios::beg);
 std::string content(static_cast<std::size_t>(size), '\0');
 if(!input.read(content.data(), size)) {
  return {};
 }
 return content;
}
std::vector<std::string> directoryResources(const std::filesystem::path& root) {
 std::vector<std::string> resources;
 std::error_code error;
 std::filesystem::recursive_directory_iterator it(root, error);
 const std::filesystem::recursive_directory_iterator end;
 for(; !error && it != end; it.increment(error)) {
  if(!it->is_regular_file(error)) {
   continue;
  }
  resources.push_back(std::filesystem::relative(it->path(), root, error).generic_string());
 }
 std::sort(resources.begin(), resources.end());
 return resources;
}
std::vector<std::string> zipResources(const resource::pack::ZippedTexturePack& zip) {
 std::vector<std::string> resources;
 resources.reserve(zip.entries().size());
 for(const auto& entry : zip.entries()) {
  resources.push_back(entry.name);
 }
 std::sort(resources.begin(), resources.end());
 return resources;
}
std::uint64_t packDirectoryStamp(const std::filesystem::path& root) {
 std::uint64_t stamp = 1469598103934665603ull;
 std::error_code error;
 if(!std::filesystem::exists(root, error)) {
  return stamp;
 }
 for(std::filesystem::recursive_directory_iterator it(root, error), end; !error && it != end; it.increment(error)) {
  if(!it->is_regular_file(error)) {
   continue;
  }
  const std::string path = std::filesystem::relative(it->path(), root, error).generic_string();
  for(const unsigned char ch : path) {
   stamp = (stamp ^ ch) * 1099511628211ull;
  }
  const auto size = it->file_size(error);
  const auto time = it->last_write_time(error).time_since_epoch().count();
  stamp = (stamp ^ static_cast<std::uint64_t>(size)) * 1099511628211ull;
  stamp = (stamp ^ static_cast<std::uint64_t>(time)) * 1099511628211ull;
 }
 return stamp;
}
}
} // namespace net::minecraft::client::render
