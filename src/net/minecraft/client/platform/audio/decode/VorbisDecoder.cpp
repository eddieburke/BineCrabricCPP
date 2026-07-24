#include "net/minecraft/client/platform/audio/decode/AudioDecoder.hpp"
#if defined(MINECRAFT_HAS_VORBIS)
#include <ogg/ogg.h>
#include "stb_vorbis.h"
#include <cstring>
#include <fstream>
#include <vector>

namespace net::minecraft::client::platform::audio::decode {

bool decodeVorbisMemory(const std::uint8_t* data, std::size_t size, PcmBuffer& out) {
 if(!data || size == 0) {
  return false;
 }
 ogg_sync_state oy;
 ogg_sync_init(&oy);
 char* buffer = ogg_sync_buffer(&oy, static_cast<long>(size));
 if(buffer != nullptr) {
  std::memcpy(buffer, data, size);
  ogg_sync_wrote(&oy, static_cast<long>(size));
 }
 ogg_page og;
 const bool validOgg = (ogg_sync_pageout(&oy, &og) == 1);
 ogg_sync_clear(&oy);
 if(!validOgg) {
  return false;
 }

 int error = 0;
 stb_vorbis* v = stb_vorbis_open_memory(data, static_cast<int>(size), &error, nullptr);
 if(v == nullptr) {
  return false;
 }

 stb_vorbis_info info = stb_vorbis_get_info(v);
 out.sampleRate = info.sample_rate;
 out.channels = static_cast<std::uint16_t>(info.channels);
 out.samples.clear();

 if(out.channels == 0 || out.sampleRate == 0) {
  stb_vorbis_close(v);
  return false;
 }

 int channels = 0;
 float** outputs = nullptr;
 while(true) {
  int samplesPerChannel = stb_vorbis_get_frame_float(v, &channels, &outputs);
  if(samplesPerChannel <= 0) {
   break;
  }
  const std::size_t oldSize = out.samples.size();
  out.samples.resize(oldSize + static_cast<std::size_t>(samplesPerChannel) * out.channels);
  for(int s = 0; s < samplesPerChannel; ++s) {
   for(int c = 0; c < out.channels; ++c) {
    out.samples[oldSize + static_cast<std::size_t>(s) * out.channels + c] = outputs[c][s];
   }
  }
 }

 stb_vorbis_close(v);
 return !out.samples.empty();
}

bool decodeVorbisFile(const std::string& path, PcmBuffer& out) {
 std::ifstream input(path, std::ios::binary | std::ios::ate);
 if(!input) {
  return false;
 }
 const auto fileSize = input.tellg();
 if(fileSize <= 0) {
  return false;
 }
 input.seekg(0, std::ios::beg);

 std::vector<std::uint8_t> buffer(static_cast<std::size_t>(fileSize));
 if(!input.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
  return false;
 }

 return decodeVorbisMemory(buffer.data(), buffer.size(), out);
}

} // namespace net::minecraft::client::platform::audio::decode
#endif
