#pragma once

#include <cstdint>
#include <vector>

namespace net::minecraft::client::platform::audio::decode {

struct PcmBuffer {
    std::uint32_t sampleRate = 44100;
    std::uint16_t channels = 2;
    std::vector<float> samples; // interleaved stereo/mono float [-1, 1]
};

} // namespace net::minecraft::client::platform::audio::decode
