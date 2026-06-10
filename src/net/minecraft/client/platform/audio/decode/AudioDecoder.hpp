#pragma once

#include "net/minecraft/client/platform/audio/decode/MusFile.hpp"
#include "net/minecraft/client/platform/audio/decode/PcmBuffer.hpp"
#include "net/minecraft/client/platform/audio/decode/VorbisDecoder.hpp"
#include "net/minecraft/client/platform/audio/decode/WavDecoder.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace net::minecraft::client::platform::audio::decode {

[[nodiscard]] inline std::string fileExtensionLower(const std::string& path)
{
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= path.size()) {
        return {};
    }
    std::string ext = path.substr(dot + 1);
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return ext;
}

[[nodiscard]] inline bool decodeAudioFile(const std::string& path, PcmBuffer& out)
{
    if (isMusPath(path)) {
        std::vector<std::uint8_t> decrypted;
        if (!loadDecryptedMus(path, decrypted)) {
            return false;
        }
        return decodeVorbisMemory(decrypted.data(), decrypted.size(), out);
    }

    const std::string ext = fileExtensionLower(path);
    if (ext == "wav") {
        return decodeWavFile(path, out);
    }
    if (ext == "ogg") {
        return decodeVorbisFile(path, out);
    }
    return decodeVorbisFile(path, out) || decodeWavFile(path, out);
}

[[nodiscard]] inline bool decodeAudioMemory(
    const std::uint8_t* data, std::size_t size, const std::string& pathHint, PcmBuffer& out)
{
    (void)pathHint;
    return decodeVorbisMemory(data, size, out);
}

} // namespace net::minecraft::client::platform::audio::decode
