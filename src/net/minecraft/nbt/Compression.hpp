#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>
#ifndef MINECRAFT_DISABLE_ZLIB
#include "net/minecraft/nbt/ZlibShim.hpp"
#endif
namespace net::minecraft {
#ifndef MINECRAFT_DISABLE_ZLIB
class ZlibStream final {
 public:
 enum class Mode {
  Deflate,
  Inflate,
 };
 ZlibStream(Mode mode, int windowBits, int level = zlibshim::Z_DEFAULT_COMPRESSION) : mode_(mode) {
  const int status = mode == Mode::Deflate
                         ? zlibshim::deflateInit2(
                               &stream_, level, zlibshim::Z_DEFLATED, windowBits, 8, zlibshim::Z_DEFAULT_STRATEGY)
                         : zlibshim::inflateInit2(&stream_, windowBits);
  if(status != zlibshim::Z_OK) {
   throw std::runtime_error("Failed to initialize zlib stream");
  }
 }
 ~ZlibStream() {
  if(mode_ == Mode::Deflate) {
   zlibshim::deflateEnd(&stream_);
  } else {
   zlibshim::inflateEnd(&stream_);
  }
 }
 ZlibStream(const ZlibStream&) = delete;
 ZlibStream& operator=(const ZlibStream&) = delete;
 [[nodiscard]] zlibshim::z_stream& get() noexcept {
  return stream_;
 }

 private:
 Mode mode_;
 zlibshim::z_stream stream_{};
};
#endif
inline std::vector<std::uint8_t> compressZlib(const std::vector<std::uint8_t>& input, bool gzip) {
#ifdef MINECRAFT_DISABLE_ZLIB
 (void)gzip;
 return input;
#else
 ZlibStream ownedStream(ZlibStream::Mode::Deflate, gzip ? 31 : 15);
 zlibshim::z_stream& stream = ownedStream.get();
 std::vector<std::uint8_t> output;
 output.resize(4096);
 stream.next_in = const_cast<zlibshim::Bytef*>(reinterpret_cast<const zlibshim::Bytef*>(input.data()));
 stream.avail_in = static_cast<zlibshim::uInt>(input.size());
 int status = zlibshim::Z_OK;
 while(status != zlibshim::Z_STREAM_END) {
  if(stream.total_out >= output.size()) {
   output.resize(output.size() * 2U);
  }
  stream.next_out = reinterpret_cast<zlibshim::Bytef*>(output.data() + stream.total_out);
  stream.avail_out = static_cast<zlibshim::uInt>(output.size() - stream.total_out);
  status = zlibshim::deflate(&stream, stream.avail_in == 0 ? zlibshim::Z_FINISH : zlibshim::Z_NO_FLUSH);
  if(status != zlibshim::Z_OK && status != zlibshim::Z_STREAM_END && status != zlibshim::Z_BUF_ERROR) {
   throw std::runtime_error("Failed to compress buffer");
  }
  if(status == zlibshim::Z_BUF_ERROR && stream.avail_out > 0) {
   break;
  }
 }
 const std::size_t produced = static_cast<std::size_t>(stream.total_out);
 output.resize(produced);
 return output;
#endif
}
inline std::vector<std::uint8_t> decompressZlib(const std::vector<std::uint8_t>& input, bool gzip) {
#ifdef MINECRAFT_DISABLE_ZLIB
 (void)gzip;
 return input;
#else
 const int windowBits = gzip ? (15 + 16) : 15;
 ZlibStream ownedStream(ZlibStream::Mode::Inflate, windowBits);
 zlibshim::z_stream& stream = ownedStream.get();
 std::vector<std::uint8_t> output;
 output.resize(4096);
 stream.next_in = const_cast<zlibshim::Bytef*>(reinterpret_cast<const zlibshim::Bytef*>(input.data()));
 stream.avail_in = static_cast<zlibshim::uInt>(input.size());
 int status = zlibshim::Z_OK;
 while(status != zlibshim::Z_STREAM_END) {
  if(stream.total_out >= output.size()) {
   output.resize(output.size() * 2U);
  }
  stream.next_out = reinterpret_cast<zlibshim::Bytef*>(output.data() + stream.total_out);
  stream.avail_out = static_cast<zlibshim::uInt>(output.size() - stream.total_out);
  status = zlibshim::inflate(&stream, zlibshim::Z_NO_FLUSH);
  if(status != zlibshim::Z_OK && status != zlibshim::Z_STREAM_END) {
   throw std::runtime_error("Failed to decompress buffer");
  }
 }
 const std::size_t produced = static_cast<std::size_t>(stream.total_out);
 output.resize(produced);
 return output;
#endif
}
// Raw DEFLATE (no zlib/gzip wrapper, windowBits -15) used by ZIP/JAR readers.
// Returns an empty vector on failure rather than throwing; expectedSize seeds
// the output buffer (the uncompressed size from the ZIP local header).
inline std::vector<std::uint8_t> decompressRawDeflate(const std::vector<std::uint8_t>& input,
                                                      std::uint32_t expectedSize) {
#ifdef MINECRAFT_DISABLE_ZLIB
 (void)expectedSize;
 return {};
#else
 std::optional<ZlibStream> ownedStream;
 try {
  ownedStream.emplace(ZlibStream::Mode::Inflate, -15);
 } catch(const std::runtime_error&) {
  return {};
 }
 zlibshim::z_stream& stream = ownedStream->get();
 std::vector<std::uint8_t> output;
 output.resize(std::max<std::size_t>(4096, static_cast<std::size_t>(expectedSize)));
 stream.next_in = const_cast<zlibshim::Bytef*>(reinterpret_cast<const zlibshim::Bytef*>(input.data()));
 stream.avail_in = static_cast<zlibshim::uInt>(input.size());
 int status = zlibshim::Z_OK;
 while(status != zlibshim::Z_STREAM_END) {
  if(stream.total_out >= output.size()) {
   output.resize(output.size() * 2U);
  }
  stream.next_out = reinterpret_cast<zlibshim::Bytef*>(output.data() + stream.total_out);
  stream.avail_out = static_cast<zlibshim::uInt>(output.size() - stream.total_out);
  status = zlibshim::inflate(&stream, zlibshim::Z_NO_FLUSH);
  if(status != zlibshim::Z_OK && status != zlibshim::Z_STREAM_END) {
   return {};
  }
 }
 const std::size_t produced = static_cast<std::size_t>(stream.total_out);
 output.resize(produced);
 return output;
#endif
}
inline std::vector<std::uint8_t> gzipCompress(const std::vector<std::uint8_t>& input) {
 return compressZlib(input, true);
}
inline std::vector<std::uint8_t> gzipDecompress(const std::vector<std::uint8_t>& input) {
 return decompressZlib(input, true);
}
inline std::vector<std::uint8_t> zlibCompress(const std::vector<std::uint8_t>& input) {
 return compressZlib(input, false);
}
inline std::vector<std::uint8_t> zlibDecompress(const std::vector<std::uint8_t>& input) {
 return decompressZlib(input, false);
}
} // namespace net::minecraft
