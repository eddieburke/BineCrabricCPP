#pragma once

#include "net/minecraft/client/platform/audio/decode/PcmBuffer.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace net::minecraft::client::platform::audio::decode {

[[nodiscard]] inline bool decodeWavFile(const std::string& path, PcmBuffer& out)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }

    auto readU32 = [&](std::uint32_t& value) -> bool {
        unsigned char bytes[4] = {};
        if (!input.read(reinterpret_cast<char*>(bytes), 4)) {
            return false;
        }
        value = static_cast<std::uint32_t>(bytes[0])
            | (static_cast<std::uint32_t>(bytes[1]) << 8)
            | (static_cast<std::uint32_t>(bytes[2]) << 16)
            | (static_cast<std::uint32_t>(bytes[3]) << 24);
        return true;
    };

    auto readU16 = [&](std::uint16_t& value) -> bool {
        unsigned char bytes[2] = {};
        if (!input.read(reinterpret_cast<char*>(bytes), 2)) {
            return false;
        }
        value = static_cast<std::uint16_t>(bytes[0])
            | (static_cast<std::uint16_t>(bytes[1]) << 8);
        return true;
    };

    char riff[4] = {};
    if (!input.read(riff, 4) || riff[0] != 'R' || riff[1] != 'I' || riff[2] != 'F' || riff[3] != 'F') {
        return false;
    }

    std::uint32_t riffSize = 0;
    if (!readU32(riffSize)) {
        return false;
    }
    (void)riffSize;

    char wave[4] = {};
    if (!input.read(wave, 4) || wave[0] != 'W' || wave[1] != 'A' || wave[2] != 'V' || wave[3] != 'E') {
        return false;
    }

    std::uint16_t audioFormat = 0;
    std::uint16_t channels = 0;
    std::uint32_t sampleRate = 0;
    std::uint16_t bitsPerSample = 0;
    std::vector<std::uint8_t> pcmBytes;

    while (input) {
        char chunkId[4] = {};
        if (!input.read(chunkId, 4)) {
            break;
        }
        std::uint32_t chunkSize = 0;
        if (!readU32(chunkSize)) {
            return false;
        }

        if (chunkId[0] == 'f' && chunkId[1] == 'm' && chunkId[2] == 't' && chunkId[3] == ' ') {
            if (!readU16(audioFormat) || !readU16(channels) || !readU32(sampleRate)) {
                return false;
            }
            std::uint32_t byteRate = 0;
            std::uint16_t blockAlign = 0;
            if (!readU32(byteRate) || !readU16(blockAlign) || !readU16(bitsPerSample)) {
                return false;
            }
            (void)byteRate;
            (void)blockAlign;
            if (chunkSize > 16) {
                input.seekg(static_cast<std::streamoff>(chunkSize - 16), std::ios::cur);
            }
        } else if (chunkId[0] == 'd' && chunkId[1] == 'a' && chunkId[2] == 't' && chunkId[3] == 'a') {
            pcmBytes.resize(chunkSize);
            if (!input.read(reinterpret_cast<char*>(pcmBytes.data()), static_cast<std::streamsize>(chunkSize))) {
                return false;
            }
        } else {
            input.seekg(static_cast<std::streamoff>(chunkSize), std::ios::cur);
        }
    }

    if (pcmBytes.empty() || channels == 0 || sampleRate == 0 || bitsPerSample == 0) {
        return false;
    }
    if (audioFormat != 1 && audioFormat != 3) {
        return false; // PCM or IEEE float only
    }

    const std::size_t frameCount = pcmBytes.size() / (static_cast<std::size_t>(channels) * (bitsPerSample / 8));
    out.sampleRate = sampleRate;
    out.channels = channels;
    out.samples.resize(frameCount * channels);

    if (audioFormat == 1 && bitsPerSample == 16) {
        const auto* src = reinterpret_cast<const std::int16_t*>(pcmBytes.data());
        for (std::size_t i = 0; i < frameCount * channels; ++i) {
            out.samples[i] = static_cast<float>(src[i]) / 32768.0f;
        }
        return true;
    }
    if (audioFormat == 1 && bitsPerSample == 8) {
        for (std::size_t i = 0; i < frameCount * channels; ++i) {
            out.samples[i] = (static_cast<float>(pcmBytes[i]) - 128.0f) / 128.0f;
        }
        return true;
    }
    if (audioFormat == 3 && bitsPerSample == 32) {
        const auto* src = reinterpret_cast<const float*>(pcmBytes.data());
        out.samples.assign(src, src + frameCount * channels);
        return true;
    }

    return false;
}

} // namespace net::minecraft::client::platform::audio::decode
