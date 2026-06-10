#pragma once

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace net::minecraft::client::platform::audio::decode {

// Beta 1.7.3 .mus files are OGG data XOR-scrambled with a filename hash seed.

[[nodiscard]] inline int javaFilenameHash(const std::filesystem::path& path)
{
    const std::string name = path.filename().string();
    int hash = 0;
    for (const unsigned char c : name) {
        hash = 31 * hash + static_cast<int>(c);
    }
    return hash;
}

inline void decryptMusInPlace(std::uint8_t* data, int length, int hash)
{
    for (int i = 0; i < length; ++i) {
        const auto decrypted = static_cast<std::uint8_t>(data[i] ^ static_cast<std::uint8_t>(hash >> 8));
        data[i] = decrypted;
        hash = hash * 498729871 + 85731 * static_cast<int>(static_cast<std::int8_t>(decrypted));
    }
}

[[nodiscard]] inline bool loadDecryptedMus(const std::string& path, std::vector<std::uint8_t>& out)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    input.seekg(0, std::ios::end);
    const std::streamoff size = input.tellg();
    if (size <= 0) {
        return false;
    }
    input.seekg(0, std::ios::beg);
    out.resize(static_cast<std::size_t>(size));
    if (!input.read(reinterpret_cast<char*>(out.data()), size)) {
        return false;
    }
    int hash = javaFilenameHash(std::filesystem::path(path));
    decryptMusInPlace(out.data(), static_cast<int>(out.size()), hash);
    return true;
}

[[nodiscard]] inline bool isMusPath(const std::string& path)
{
    if (path.size() < 4) {
        return false;
    }
    const auto lower = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
    return path[path.size() - 4] == '.'
        && lower(path[path.size() - 3]) == 'm'
        && lower(path[path.size() - 2]) == 'u'
        && lower(path[path.size() - 1]) == 's';
}

} // namespace net::minecraft::client::platform::audio::decode
