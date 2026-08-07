#include "net/minecraft/client/render/shaderpack/VanillaPackEmbed.hpp"
#include <algorithm>
namespace net::minecraft::client::render {
std::vector<std::string> VanillaPackEmbed::resources() {
 std::vector<std::string> result;
 result.reserve(embeddedResources().size());
 for(const auto& [path, ignored] : embeddedResources()) result.push_back(path);
 std::sort(result.begin(), result.end());
 return result;
}
std::string VanillaPackEmbed::get(std::string_view path) {
 const auto& embedded = embeddedResources();
 const auto found = embedded.find(std::string(path));
 return found == embedded.end() ? std::string{} : found->second;
}
bool VanillaPackEmbed::has(std::string_view path) {
 return embeddedResources().contains(std::string(path));
}
} // namespace net::minecraft::client::render
