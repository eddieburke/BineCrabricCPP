#include "net/minecraft/client/platform/audio/decode/AudioDecoder.hpp"
#if defined(MINECRAFT_HAS_VORBIS)
#define OV_EXCLUDE_STATIC_CALLBACKS 1
#include <vorbis/vorbisfile.h>
#include <cstring>
#include <vector>
namespace net::minecraft::client::platform::audio::decode {
namespace {
struct MemorySource {
 const std::uint8_t* data = nullptr;
 std::size_t size = 0;
 std::size_t offset = 0;
};
std::size_t memoryRead(void* dst, std::size_t size1, std::size_t size2, void* source) {
 auto* mem = static_cast<MemorySource*>(source);
 const std::size_t requested = size1 * size2;
 const std::size_t available = mem->size - mem->offset;
 const std::size_t toCopy = requested < available ? requested : available;
 if(toCopy > 0) {
  std::memcpy(dst, mem->data + mem->offset, toCopy);
  mem->offset += toCopy;
 }
 return size1 == 0 ? 0 : toCopy / size1;
}
int memorySeek(void* source, ogg_int64_t offset, int whence) {
 auto* mem = static_cast<MemorySource*>(source);
 switch(whence) {
 case SEEK_SET:
  mem->offset = static_cast<std::size_t>(offset);
  break;
 case SEEK_CUR:
  mem->offset = static_cast<std::size_t>(static_cast<ogg_int64_t>(mem->offset) + offset);
  break;
 case SEEK_END:
  mem->offset = static_cast<std::size_t>(static_cast<ogg_int64_t>(mem->size) + offset);
  break;
 default:
  return -1;
 }
 if(mem->offset > mem->size) {
  mem->offset = mem->size;
 }
 return 0;
}
long memoryTell(void* source) {
 return static_cast<long>(static_cast<MemorySource*>(source)->offset);
}
[[nodiscard]] bool decodeVorbisHandle(OggVorbis_File& vf, PcmBuffer& out) {
 const vorbis_info* info = ov_info(&vf, -1);
 if(info == nullptr || info->channels <= 0 || info->rate <= 0) {
  ov_clear(&vf);
  return false;
 }
 out.sampleRate = static_cast<std::uint32_t>(info->rate);
 out.channels = static_cast<std::uint16_t>(info->channels);
 out.samples.clear();
 float** pcmChannels = nullptr;
 int currentSection = 0;
 while(true) {
  const long samplesPerChannel = ov_read_float(&vf, &pcmChannels, 4096, &currentSection);
  if(samplesPerChannel == 0) {
   break;
  }
  if(samplesPerChannel < 0) {
   ov_clear(&vf);
   return false;
  }
  const std::size_t oldSize = out.samples.size();
  out.samples.resize(oldSize + static_cast<std::size_t>(samplesPerChannel) * out.channels);
  for(long sample = 0; sample < samplesPerChannel; ++sample) {
   for(int channel = 0; channel < info->channels; ++channel) {
    out.samples[oldSize + static_cast<std::size_t>(sample) * out.channels +
                static_cast<std::size_t>(channel)] = pcmChannels[channel][sample];
   }
  }
 }
 ov_clear(&vf);
 return !out.samples.empty();
}
} // namespace
bool decodeVorbisFile(const std::string& path, PcmBuffer& out) {
 OggVorbis_File vf{};
 if(ov_fopen(path.c_str(), &vf) != 0) {
  return false;
 }
 return decodeVorbisHandle(vf, out);
}
bool decodeVorbisMemory(const std::uint8_t* data, std::size_t size, PcmBuffer& out) {
 MemorySource source{data, size, 0};
 ov_callbacks callbacks{};
 callbacks.read_func = memoryRead;
 callbacks.seek_func = memorySeek;
 callbacks.close_func = nullptr;
 callbacks.tell_func = memoryTell;
 OggVorbis_File vf{};
 if(ov_open_callbacks(&source, &vf, nullptr, 0, callbacks) != 0) {
  return false;
 }
 return decodeVorbisHandle(vf, out);
}
} // namespace net::minecraft::client::platform::audio::decode
#endif
