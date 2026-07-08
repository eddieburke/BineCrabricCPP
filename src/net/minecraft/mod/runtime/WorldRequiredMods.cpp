#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>

#include "net/minecraft/mod/runtime/ModHost.hpp"

namespace net::minecraft::mod::runtime {
namespace {
constexpr int kMaxBlockId = 256;
constexpr const char* kFileName = "required-mods.txt";

std::array<std::atomic<bool>, kMaxBlockId>& modBlockIds() {
    static std::array<std::atomic<bool>, kMaxBlockId> ids{};
    return ids;
}

std::mutex& stateMutex() {
    static std::mutex m;
    return m;
}

std::map<int, std::string>& blockIdToMod() {
    static std::map<int, std::string> m;
    return m;
}

std::map<const World*, std::set<std::string>>& placedByWorld() {
    static std::map<const World*, std::set<std::string>> m;
    return m;
}

std::string trimCopy(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}
}  // namespace

void WorldRequiredMods::registerContentBlock(const std::string& modId, int blockId) {
    if (modId.empty() || blockId <= 0 || blockId >= kMaxBlockId) {
        return;
    }
    const std::lock_guard lock(stateMutex());
    blockIdToMod()[blockId] = modId;
    modBlockIds()[static_cast<std::size_t>(blockId)].store(true, std::memory_order_relaxed);
}

bool WorldRequiredMods::isModBlockId(int blockId) noexcept {
    if (blockId <= 0 || blockId >= kMaxBlockId) {
        return false;
    }
    return modBlockIds()[static_cast<std::size_t>(blockId)].load(std::memory_order_relaxed);
}

void WorldRequiredMods::notePlaced(const World* world, int blockId) {
    if (world == nullptr || !isModBlockId(blockId)) {
        return;
    }
    const std::lock_guard lock(stateMutex());
    const auto it = blockIdToMod().find(blockId);
    if (it != blockIdToMod().end()) {
        placedByWorld()[world].insert(it->second);
    }
}

void WorldRequiredMods::forgetWorld(const World* world) {
    const std::lock_guard lock(stateMutex());
    placedByWorld().erase(world);
}

std::vector<std::string> WorldRequiredMods::sessionMods(const World* world) {
    const std::lock_guard lock(stateMutex());
    const auto it = placedByWorld().find(world);
    if (it == placedByWorld().end()) {
        return {};
    }
    return {it->second.begin(), it->second.end()};
}

std::vector<std::string> WorldRequiredMods::readWorldFile(const std::filesystem::path& worldDirectory) {
    std::vector<std::string> mods;
    std::ifstream input(worldDirectory / kFileName);
    if (!input.is_open()) {
        return mods;
    }
    std::string line;
    while (std::getline(input, line)) {
        line = trimCopy(line);
        if (!line.empty() && !line.starts_with('#')) {
            mods.push_back(line);
        }
    }
    std::sort(mods.begin(), mods.end());
    mods.erase(std::unique(mods.begin(), mods.end()), mods.end());
    return mods;
}

void WorldRequiredMods::writeWorldFile(const std::filesystem::path& worldDirectory, const World* world) {
    std::set<std::string> merged;
    for (const std::string& mod : readWorldFile(worldDirectory)) {
        merged.insert(mod);
    }
    for (const std::string& mod : sessionMods(world)) {
        merged.insert(mod);
    }
    if (merged.empty()) {
        return;
    }
    std::error_code ec;
    std::filesystem::create_directories(worldDirectory, ec);
    std::ofstream output(worldDirectory / kFileName, std::ios::trunc);
    if (!output.is_open()) {
        return;
    }
    output << "# Lua content mods this world requires (block ids placed by these mods)\n";
    for (const std::string& mod : merged) {
        output << mod << '\n';
    }
}

std::vector<std::string> WorldRequiredMods::requiredForWorld(const std::filesystem::path& worldDirectory,
                                                             const World* world) {
    std::vector<std::string> required = readWorldFile(worldDirectory);
    for (const std::string& modId : sessionMods(world)) {
        required.push_back(modId);
    }
    std::sort(required.begin(), required.end());
    required.erase(std::unique(required.begin(), required.end()), required.end());
    return required;
}

std::vector<std::string> WorldRequiredMods::missingForDirectory(const std::filesystem::path& worldDirectory) {
    return missingMods(readWorldFile(worldDirectory));
}

std::vector<std::string> WorldRequiredMods::missingFrom(const std::vector<std::string>& required,
                                                        const std::vector<std::string>& available) {
    std::vector<std::string> missing;
    for (const std::string& modId : required) {
        if (std::find(available.begin(), available.end(), modId) == available.end()) {
            missing.push_back(modId);
        }
    }
    return missing;
}

std::string WorldRequiredMods::requirementMessage(const std::vector<std::string>& missing) {
    return "This world requires Lua mods: " + joinCsv(missing);
}

std::vector<std::string> WorldRequiredMods::missingMods(const std::vector<std::string>& required) {
    std::vector<std::string> missing;
    const ModHost& modHost = host();
    for (const std::string& modId : required) {
        const ModPackage* pkg = modHost.findPackageMod(modId);
        if (pkg == nullptr || !pkg->active) {
            missing.push_back(modId);
        }
    }
    return missing;
}

std::string WorldRequiredMods::joinCsv(const std::vector<std::string>& mods) {
    std::string out;
    for (const std::string& mod : mods) {
        if (!out.empty()) {
            out += ',';
        }
        out += mod;
    }
    return out;
}

std::vector<std::string> WorldRequiredMods::splitCsv(const std::string& csv) {
    std::vector<std::string> mods;
    std::stringstream stream(csv);
    std::string item;
    while (std::getline(stream, item, ',')) {
        item = trimCopy(item);
        if (!item.empty()) {
            mods.push_back(item);
        }
    }
    return mods;
}
}  // namespace net::minecraft::mod::runtime
