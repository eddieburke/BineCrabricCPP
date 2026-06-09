#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

#ifndef MINECRAFT_DISABLE_ZLIB
#include "net/minecraft/nbt/ZlibShim.hpp"
#endif

namespace net::minecraft {

inline std::vector<std::uint8_t> compressZlib(const std::vector<std::uint8_t>& input, bool gzip)
{
#ifdef MINECRAFT_DISABLE_ZLIB
    (void)gzip;
    return input;
#else
    zlibshim::z_stream stream{};
    const int windowBits = gzip ? (15 + 16) : 15;
    if (zlibshim::deflateInit2(&stream, zlibshim::Z_BEST_COMPRESSION, zlibshim::Z_DEFLATED, windowBits, 8, zlibshim::Z_DEFAULT_STRATEGY) != zlibshim::Z_OK) {
        throw std::runtime_error("Failed to initialize zlib compressor");
    }

    std::vector<std::uint8_t> output;
    output.resize(4096);
    stream.next_in = const_cast<zlibshim::Bytef*>(reinterpret_cast<const zlibshim::Bytef*>(input.data()));
    stream.avail_in = static_cast<zlibshim::uInt>(input.size());

    int status = zlibshim::Z_OK;
    while (status != zlibshim::Z_STREAM_END) {
        if (stream.total_out >= output.size()) {
            output.resize(output.size() * 2U);
        }
        stream.next_out = reinterpret_cast<zlibshim::Bytef*>(output.data() + stream.total_out);
        stream.avail_out = static_cast<zlibshim::uInt>(output.size() - stream.total_out);
        status = zlibshim::deflate(&stream, stream.avail_in == 0 ? zlibshim::Z_FINISH : zlibshim::Z_NO_FLUSH);
        if (status != zlibshim::Z_OK && status != zlibshim::Z_STREAM_END && status != zlibshim::Z_BUF_ERROR) {
            zlibshim::deflateEnd(&stream);
            throw std::runtime_error("Failed to compress buffer");
        }
        if (status == zlibshim::Z_BUF_ERROR && stream.avail_out > 0) {
            break;
        }
    }

    const std::size_t produced = static_cast<std::size_t>(stream.total_out);
    zlibshim::deflateEnd(&stream);
    output.resize(produced);
    return output;
#endif
}

inline std::vector<std::uint8_t> decompressZlib(const std::vector<std::uint8_t>& input, bool gzip)
{
#ifdef MINECRAFT_DISABLE_ZLIB
    (void)gzip;
    return input;
#else
    zlibshim::z_stream stream{};
    const int windowBits = gzip ? (15 + 16) : 15;
    if (zlibshim::inflateInit2(&stream, windowBits) != zlibshim::Z_OK) {
        throw std::runtime_error("Failed to initialize zlib decompressor");
    }

    std::vector<std::uint8_t> output;
    output.resize(4096);
    stream.next_in = const_cast<zlibshim::Bytef*>(reinterpret_cast<const zlibshim::Bytef*>(input.data()));
    stream.avail_in = static_cast<zlibshim::uInt>(input.size());

    int status = zlibshim::Z_OK;
    while (status != zlibshim::Z_STREAM_END) {
        if (stream.total_out >= output.size()) {
            output.resize(output.size() * 2U);
        }
        stream.next_out = reinterpret_cast<zlibshim::Bytef*>(output.data() + stream.total_out);
        stream.avail_out = static_cast<zlibshim::uInt>(output.size() - stream.total_out);
        status = zlibshim::inflate(&stream, zlibshim::Z_NO_FLUSH);
        if (status != zlibshim::Z_OK && status != zlibshim::Z_STREAM_END) {
            zlibshim::inflateEnd(&stream);
            throw std::runtime_error("Failed to decompress buffer");
        }
    }

    const std::size_t produced = static_cast<std::size_t>(stream.total_out);
    zlibshim::inflateEnd(&stream);
    output.resize(produced);
    return output;
#endif
}

inline std::vector<std::uint8_t> gzipCompress(const std::vector<std::uint8_t>& input)
{
    return compressZlib(input, true);
}

inline std::vector<std::uint8_t> gzipDecompress(const std::vector<std::uint8_t>& input)
{
    return decompressZlib(input, true);
}

inline std::vector<std::uint8_t> zlibCompress(const std::vector<std::uint8_t>& input)
{
    return compressZlib(input, false);
}

inline std::vector<std::uint8_t> zlibDecompress(const std::vector<std::uint8_t>& input)
{
    return decompressZlib(input, false);
}

} // namespace net::minecraft
