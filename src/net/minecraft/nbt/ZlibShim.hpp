#pragma once
#include <cstddef>
#include <cstdint>
namespace net::minecraft::zlibshim {
using Byte = unsigned char;
using Bytef = Byte;
using uInt = unsigned int;
using uLong = unsigned long;
using voidpf = void*;
using alloc_func = voidpf (*)(voidpf opaque, uInt items, uInt size);
using free_func = void (*)(voidpf opaque, voidpf address);
struct z_stream {
  Bytef* next_in = nullptr;
  uInt avail_in = 0;
  uLong total_in = 0;
  Bytef* next_out = nullptr;
  uInt avail_out = 0;
  uLong total_out = 0;
  char* msg = nullptr;
  void* state = nullptr;
  alloc_func zalloc = nullptr;
  free_func zfree = nullptr;
  voidpf opaque = nullptr;
  int data_type = 0;
  uLong adler = 0;
  uLong reserved = 0;
};
using z_streamp = z_stream*;
constexpr int Z_NO_FLUSH = 0;
constexpr int Z_FINISH = 4;
constexpr int Z_OK = 0;
constexpr int Z_STREAM_END = 1;
constexpr int Z_NEED_DICT = 2;
constexpr int Z_ERRNO = -1;
constexpr int Z_STREAM_ERROR = -2;
constexpr int Z_DATA_ERROR = -3;
constexpr int Z_MEM_ERROR = -4;
constexpr int Z_BUF_ERROR = -5;
constexpr int Z_VERSION_ERROR = -6;
constexpr int Z_DEFLATED = 8;
constexpr int Z_DEFAULT_COMPRESSION = -1;
constexpr int Z_BEST_COMPRESSION = 9;
constexpr int Z_DEFAULT_STRATEGY = 0;
extern "C" {
const char* zlibVersion();
int deflateInit2_(z_streamp strm, int level, int method, int windowBits, int memLevel, int strategy,
                  const char* version, int stream_size);
int deflate(z_streamp strm, int flush);
int deflateEnd(z_streamp strm);
int inflateInit2_(z_streamp strm, int windowBits, const char* version, int stream_size);
int inflate(z_streamp strm, int flush);
int inflateEnd(z_streamp strm);
}
inline int deflateInit2(z_streamp strm, int level, int method, int windowBits, int memLevel, int strategy) {
  return deflateInit2_(strm, level, method, windowBits, memLevel, strategy, "1.3.1",
                       static_cast<int>(sizeof(z_stream)));
}
inline int inflateInit2(z_streamp strm, int windowBits) {
  return inflateInit2_(strm, windowBits, "1.3.1", static_cast<int>(sizeof(z_stream)));
}
} // namespace net::minecraft::zlibshim
