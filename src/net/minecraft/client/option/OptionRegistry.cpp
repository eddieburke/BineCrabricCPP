#include "net/minecraft/client/option/OptionRegistry.hpp"

#include <array>
#include <unordered_map>
#include <vector>

namespace net::minecraft::client::gui::screen::option {
namespace options_screen { extern std::array<net::minecraft::client::option::OptionSpec, 8> kSpecs; }
namespace quality_screen { extern std::array<net::minecraft::client::option::OptionSpec, 9> kSpecs; }
namespace performance_screen { extern std::array<net::minecraft::client::option::OptionSpec, 10> kSpecs; }
namespace detail_screen { extern std::array<net::minecraft::client::option::OptionSpec, 8> kSpecs; }
namespace animation_screen { extern std::array<net::minecraft::client::option::OptionSpec, 8> kSpecs; }
namespace stereo_screen { extern std::array<net::minecraft::client::option::OptionSpec, 7> kSpecs; }
namespace fog_screen { extern std::array<net::minecraft::client::option::OptionSpec, 10> kSpecs; }
namespace world_screen { extern std::array<net::minecraft::client::option::OptionSpec, 4> kSpecs; }
} // namespace

namespace net::minecraft::client::option {
namespace {

std::vector<OptionSpec> gRegistry;
std::unordered_map<std::string_view, std::size_t> gKeyIndex;
bool gRegistered = false;

} // namespace

void OptionRegistry::registerGroup(std::span<const OptionSpec> specs)
{
    for (const OptionSpec& spec : specs) {
        gKeyIndex[spec.persistKey] = gRegistry.size();
        gRegistry.push_back(spec);
    }
}

void OptionRegistry::registerAll()
{
    if (gRegistered) {
        return;
    }
    gRegistry.clear();
    gKeyIndex.clear();

    registerGroup(gui::screen::option::options_screen::kSpecs);
    registerGroup(gui::screen::option::quality_screen::kSpecs);
    registerGroup(gui::screen::option::performance_screen::kSpecs);
    registerGroup(gui::screen::option::detail_screen::kSpecs);
    registerGroup(gui::screen::option::animation_screen::kSpecs);
    registerGroup(gui::screen::option::stereo_screen::kSpecs);
    registerGroup(gui::screen::option::fog_screen::kSpecs);
    registerGroup(gui::screen::option::world_screen::kSpecs);

    gRegistered = true;
}

std::span<const OptionSpec> OptionRegistry::all()
{
    return gRegistry;
}

std::optional<std::size_t> OptionRegistry::indexOf(std::string_view persistKey)
{
    const auto it = gKeyIndex.find(persistKey);
    if (it == gKeyIndex.end()) {
        return std::nullopt;
    }
    return it->second;
}

const OptionSpec& OptionRegistry::at(std::size_t index)
{
    return gRegistry.at(index);
}

std::optional<const OptionSpec*> OptionRegistry::byKey(std::string_view persistKey)
{
    const std::optional<std::size_t> idx = indexOf(persistKey);
    if (!idx) {
        return std::nullopt;
    }
    return &gRegistry[*idx];
}

} // namespace net::minecraft::client::option
