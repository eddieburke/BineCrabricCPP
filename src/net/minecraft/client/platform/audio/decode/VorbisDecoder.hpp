#pragma once

#include "net/minecraft/client/platform/audio/decode/PcmBuffer.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace net::minecraft::client::platform::audio::decode {

#if defined(MINECRAFT_HAS_VORBIS)

[[nodiscard]] bool decodeVorbisFile(const std::string& path, PcmBuffer& out);
[[nodiscard]] bool decodeVorbisMemory(const std::uint8_t* data, std::size_t size, PcmBuffer& out);

#else

[[nodiscard]] inline bool decodeVorbisFile(const std::string& /*path*/, PcmBuffer& /*out*/)
{
    return false;
}

[[nodiscard]] inline bool decodeVorbisMemory(const std::uint8_t* /*data*/, std::size_t /*size*/, PcmBuffer& /*out*/)
{
    return false;
}

#endif

} // namespace net::minecraft::client::platform::audio::decode
