#include "net/minecraft/client/option/OptionRegistry.hpp"
#include <array>
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>
namespace net::minecraft::client::gui::screen::option {
namespace options_screen {
extern std::array<net::minecraft::client::option::OptionSpec, 8> kSpecs;
}
namespace quality_screen {
extern std::array<net::minecraft::client::option::OptionSpec, 9> kSpecs;
}
namespace performance_screen {
extern std::array<net::minecraft::client::option::OptionSpec, 10> kSpecs;
}
namespace detail_screen {
extern std::array<net::minecraft::client::option::OptionSpec, 8> kSpecs;
}
namespace animation_screen {
extern std::array<net::minecraft::client::option::OptionSpec, 8> kSpecs;
}
namespace world_screen {
extern std::array<net::minecraft::client::option::OptionSpec, 4> kSpecs;
}
} // namespace net::minecraft::client::gui::screen::option
namespace net::minecraft::client::option {
namespace {
std::vector<OptionSpec> gRegistry;
std::unordered_map<std::string_view, std::size_t> gKeyIndex;
bool gRegistered = false;
std::string normalizeLegacyOptionKey(std::string_view persistKey) {
 if(persistKey.size() < 3 || persistKey[0] != 'o' || persistKey[1] != 'f' ||
    !std::isupper(static_cast<unsigned char>(persistKey[2]))) {
  return {};
 }
 std::string normalized(persistKey.substr(2));
 std::size_t uppercasePrefix = 0;
 while(uppercasePrefix < normalized.size() &&
       std::isupper(static_cast<unsigned char>(normalized[uppercasePrefix]))) {
  ++uppercasePrefix;
 }
 if(uppercasePrefix == normalized.size()) {
  for(char& ch : normalized) {
   ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return normalized;
 }
 if(uppercasePrefix <= 1) {
  normalized[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(normalized[0])));
  return normalized;
 }
 for(std::size_t i = 0; i + 1 < uppercasePrefix; ++i) {
  normalized[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(normalized[i])));
 }
 return normalized;
}
} // namespace
void OptionRegistry::registerGroup(std::span<const OptionSpec> specs) {
 for(const OptionSpec& spec : specs) {
  gKeyIndex[spec.persistKey] = gRegistry.size();
  gRegistry.push_back(spec);
 }
}
void OptionRegistry::registerAll() {
 if(gRegistered) {
  return;
 }
 gRegistry.clear();
 gKeyIndex.clear();
 registerGroup(gui::screen::option::options_screen::kSpecs);
 registerGroup(gui::screen::option::quality_screen::kSpecs);
 registerGroup(gui::screen::option::performance_screen::kSpecs);
 registerGroup(gui::screen::option::detail_screen::kSpecs);
 registerGroup(gui::screen::option::animation_screen::kSpecs);
 registerGroup(gui::screen::option::world_screen::kSpecs);
 gRegistered = true;
}
std::span<const OptionSpec> OptionRegistry::all() {
 registerAll();
 return gRegistry;
}
std::optional<std::size_t> OptionRegistry::indexOf(std::string_view persistKey) {
 registerAll();
 const auto it = gKeyIndex.find(persistKey);
 if(it != gKeyIndex.end()) {
  return it->second;
 }
 const std::string normalized = normalizeLegacyOptionKey(persistKey);
 if(normalized.empty()) {
  return std::nullopt;
 }
 const auto normalizedIt = gKeyIndex.find(normalized);
 if(normalizedIt == gKeyIndex.end()) {
  return std::nullopt;
 }
 return normalizedIt->second;
}
const OptionSpec& OptionRegistry::at(std::size_t index) {
 registerAll();
 return gRegistry.at(index);
}
std::optional<const OptionSpec*> OptionRegistry::byKey(std::string_view persistKey) {
 registerAll();
 const std::optional<std::size_t> idx = indexOf(persistKey);
 if(!idx) {
  return std::nullopt;
 }
 return &gRegistry[*idx];
}
} // namespace net::minecraft::client::option
