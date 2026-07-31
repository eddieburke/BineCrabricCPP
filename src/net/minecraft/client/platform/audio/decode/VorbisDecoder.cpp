#include "net/minecraft/client/platform/audio/decode/AudioDecoder.hpp"
#if defined(MINECRAFT_HAS_VORBIS)
#include <vorbis/vorbisfile.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <vector>
namespace net::minecraft::client::platform::audio::decode {
struct MemoryVorbis {
 const std::uint8_t* data;
 std::size_t size;
 std::size_t pos;
};
std::size_t readVorbis(void* p, std::size_t s, std::size_t n, void* d) {
 auto& m = *static_cast<MemoryVorbis*>(d);
 auto z = std::min(s * n, m.size - m.pos);
 std::memcpy(p, m.data + m.pos, z);
 m.pos += z;
 return s ? z / s : 0;
}
int seekVorbis(void* d, ogg_int64_t o, int w) {
 auto& m = *static_cast<MemoryVorbis*>(d);
 ogg_int64_t b = w == SEEK_SET ? 0 : w == SEEK_CUR ? (ogg_int64_t)m.pos
                                                   : (ogg_int64_t)m.size;
 auto p = b + o;
 if(p < 0 || (std::size_t)p > m.size) return -1;
 m.pos = (std::size_t)p;
 return 0;
}
long tellVorbis(void* d) { return (long)static_cast<MemoryVorbis*>(d)->pos; }
bool decodeVorbisMemory(const std::uint8_t* data, std::size_t size, PcmBuffer& out) {
 if(!data || !size) return false;
 MemoryVorbis memory{data, size, 0};
 OggVorbis_File file{};
 ov_callbacks callbacks{readVorbis, seekVorbis, nullptr, tellVorbis};
 if(ov_open_callbacks(&memory, &file, nullptr, 0, callbacks) < 0) return false;
 vorbis_info* info = ov_info(&file, -1);
 if(!info || info->channels < 1 || info->rate < 1) {
  ov_clear(&file);
  return false;
 }
 out.sampleRate = (std::uint32_t)info->rate;
 out.channels = (std::uint16_t)info->channels;
 out.samples.clear();
 float** pcm = nullptr;
 for(;;) {
  int section = 0;
  long frames = ov_read_float(&file, &pcm, 4096, &section);
  if(frames <= 0) break;
  auto start = out.samples.size();
  out.samples.resize(start + (std::size_t)frames * out.channels);
  for(long f = 0; f < frames; ++f)
   for(int c = 0; c < info->channels; ++c) out.samples[start + (std::size_t)f * out.channels + c] = pcm[c][f];
 }
 ov_clear(&file);
 return !out.samples.empty();
}
bool decodeVorbisFile(const std::string& path, PcmBuffer& out) {
 std::ifstream input(path, std::ios::binary | std::ios::ate);
 if(!input) return false;
 auto n = input.tellg();
 if(n <= 0) return false;
 input.seekg(0);
 std::vector<std::uint8_t> data((std::size_t)n);
 return input.read((char*)data.data(), n) && decodeVorbisMemory(data.data(), data.size(), out);
}
} // namespace net::minecraft::client::platform::audio::decode
#endif
