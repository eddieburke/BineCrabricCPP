#pragma once
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
namespace net::minecraft::client::platform::audio::decode {
struct PcmBuffer {
  std::uint32_t sampleRate = 44100;
  std::uint16_t channels = 2;
  std::vector<float> samples;
};
#if defined(MINECRAFT_HAS_VORBIS)
[[nodiscard]] bool decodeVorbisFile(const std::string& path, PcmBuffer& out);
[[nodiscard]] bool decodeVorbisMemory(const std::uint8_t* data, std::size_t size, PcmBuffer& out);
#else
[[nodiscard]] inline bool decodeVorbisFile(const std::string& /*path*/, PcmBuffer& /*out*/) {
  return false;
}
[[nodiscard]] inline bool decodeVorbisMemory(const std::uint8_t* /*data*/, std::size_t /*size*/, PcmBuffer& /*out*/) {
  return false;
}
#endif
[[nodiscard]] inline std::string fileExtensionLower(const std::string& path) {
  const auto dot = path.find_last_of('.');
  if(dot == std::string::npos || dot + 1 >= path.size()) {
    return {};
  }
  std::string ext = path.substr(dot + 1);
  for(char& c : ext) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return ext;
}
[[nodiscard]] inline bool isMusPath(const std::string& path) {
  if(path.size() < 4) {
    return false;
  }
  const auto lower = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
  return path[path.size() - 4] == '.' && lower(path[path.size() - 3]) == 'm' && lower(path[path.size() - 2]) == 'u' &&
         lower(path[path.size() - 1]) == 's';
}
[[nodiscard]] inline int javaFilenameHash(const std::filesystem::path& path) {
  const std::string name = path.filename().string();
  int hash = 0;
  for(const unsigned char c : name) {
    hash = 31 * hash + static_cast<int>(c);
  }
  return hash;
}
inline void decryptMusInPlace(std::uint8_t* data, int length, int hash) {
  for(int i = 0; i < length; ++i) {
    const auto decrypted = static_cast<std::uint8_t>(data[i] ^ static_cast<std::uint8_t>(hash >> 8));
    data[i] = decrypted;
    hash = hash * 498729871 + 85731 * static_cast<int>(static_cast<std::int8_t>(decrypted));
  }
}
[[nodiscard]] inline bool loadDecryptedMus(const std::string& path, std::vector<std::uint8_t>& out) {
  std::ifstream input(path, std::ios::binary);
  if(!input) {
    return false;
  }
  input.seekg(0, std::ios::end);
  const std::streamoff size = input.tellg();
  if(size <= 0) {
    return false;
  }
  input.seekg(0, std::ios::beg);
  out.resize(static_cast<std::size_t>(size));
  if(!input.read(reinterpret_cast<char*>(out.data()), size)) {
    return false;
  }
  int hash = javaFilenameHash(std::filesystem::path(path));
  decryptMusInPlace(out.data(), static_cast<int>(out.size()), hash);
  return true;
}
[[nodiscard]] inline bool decodeWavFile(const std::string& path, PcmBuffer& out) {
  std::ifstream input(path, std::ios::binary);
  if(!input) {
    return false;
  }
  auto readU32 = [&](std::uint32_t& value) -> bool {
    unsigned char bytes[4] = {};
    if(!input.read(reinterpret_cast<char*>(bytes), 4)) {
      return false;
    }
    value = static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8) |
            (static_cast<std::uint32_t>(bytes[2]) << 16) | (static_cast<std::uint32_t>(bytes[3]) << 24);
    return true;
  };
  auto readU16 = [&](std::uint16_t& value) -> bool {
    unsigned char bytes[2] = {};
    if(!input.read(reinterpret_cast<char*>(bytes), 2)) {
      return false;
    }
    value = static_cast<std::uint16_t>(bytes[0]) | (static_cast<std::uint16_t>(bytes[1]) << 8);
    return true;
  };
  char riff[4] = {};
  if(!input.read(riff, 4) || riff[0] != 'R' || riff[1] != 'I' || riff[2] != 'F' || riff[3] != 'F') {
    return false;
  }
  std::uint32_t riffSize = 0;
  if(!readU32(riffSize)) {
    return false;
  }
  (void)riffSize;
  char wave[4] = {};
  if(!input.read(wave, 4) || wave[0] != 'W' || wave[1] != 'A' || wave[2] != 'V' || wave[3] != 'E') {
    return false;
  }
  std::uint16_t audioFormat = 0;
  std::uint16_t channels = 0;
  std::uint32_t sampleRate = 0;
  std::uint16_t bitsPerSample = 0;
  std::vector<std::uint8_t> pcmBytes;
  while(input) {
    char chunkId[4] = {};
    if(!input.read(chunkId, 4)) {
      break;
    }
    std::uint32_t chunkSize = 0;
    if(!readU32(chunkSize)) {
      return false;
    }
    if(chunkId[0] == 'f' && chunkId[1] == 'm' && chunkId[2] == 't' && chunkId[3] == ' ') {
      if(!readU16(audioFormat) || !readU16(channels) || !readU32(sampleRate)) {
        return false;
      }
      std::uint32_t byteRate = 0;
      std::uint16_t blockAlign = 0;
      if(!readU32(byteRate) || !readU16(blockAlign) || !readU16(bitsPerSample)) {
        return false;
      }
      (void)byteRate;
      (void)blockAlign;
      if(chunkSize > 16) {
        input.seekg(static_cast<std::streamoff>(chunkSize - 16), std::ios::cur);
      }
    } else if(chunkId[0] == 'd' && chunkId[1] == 'a' && chunkId[2] == 't' && chunkId[3] == 'a') {
      pcmBytes.resize(chunkSize);
      if(!input.read(reinterpret_cast<char*>(pcmBytes.data()), static_cast<std::streamsize>(chunkSize))) {
        return false;
      }
    } else {
      input.seekg(static_cast<std::streamoff>(chunkSize), std::ios::cur);
    }
  }
  if(pcmBytes.empty() || channels == 0 || sampleRate == 0 || bitsPerSample == 0) {
    return false;
  }
  if(audioFormat != 1 && audioFormat != 3) {
    return false;
  }
  const std::size_t frameCount = pcmBytes.size() / (static_cast<std::size_t>(channels) * (bitsPerSample / 8));
  out.sampleRate = sampleRate;
  out.channels = channels;
  out.samples.resize(frameCount * channels);
  if(audioFormat == 1 && bitsPerSample == 16) {
    const auto* src = reinterpret_cast<const std::int16_t*>(pcmBytes.data());
    for(std::size_t i = 0; i < frameCount * channels; ++i) {
      out.samples[i] = static_cast<float>(src[i]) / 32768.0f;
    }
    return true;
  }
  if(audioFormat == 1 && bitsPerSample == 8) {
    for(std::size_t i = 0; i < frameCount * channels; ++i) {
      out.samples[i] = (static_cast<float>(pcmBytes[i]) - 128.0f) / 128.0f;
    }
    return true;
  }
  if(audioFormat == 3 && bitsPerSample == 32) {
    const auto* src = reinterpret_cast<const float*>(pcmBytes.data());
    out.samples.assign(src, src + frameCount * channels);
    return true;
  }
  return false;
}
[[nodiscard]] inline bool decodeAudioFile(const std::string& path, PcmBuffer& out) {
  if(isMusPath(path)) {
    std::vector<std::uint8_t> decrypted;
    if(!loadDecryptedMus(path, decrypted)) {
      return false;
    }
    return decodeVorbisMemory(decrypted.data(), decrypted.size(), out);
  }
  const std::string ext = fileExtensionLower(path);
  if(ext == "wav") {
    return decodeWavFile(path, out);
  }
  if(ext == "ogg") {
    return decodeVorbisFile(path, out);
  }
  return decodeVorbisFile(path, out) || decodeWavFile(path, out);
}
[[nodiscard]] inline bool decodeAudioMemory(const std::uint8_t* data,
                                            std::size_t size,
                                            const std::string& /*pathHint*/,
                                            PcmBuffer& out) {
  return decodeVorbisMemory(data, size, out);
}
} // namespace net::minecraft::client::platform::audio::decode
