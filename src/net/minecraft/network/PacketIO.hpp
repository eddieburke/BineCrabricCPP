#pragma once
#include <cstdint>
#include <cstring>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "net/minecraft/nbt/BinaryIO.hpp"
namespace net::minecraft::packetio {
inline std::uint8_t readU8(std::istream& input) {
 const int value = input.get();
 if(value == std::char_traits<char>::eof()) {
  throw std::runtime_error("Unexpected end of packet stream");
 }
 return static_cast<std::uint8_t>(value);
}
inline std::int8_t readI8(std::istream& input) {
 return static_cast<std::int8_t>(readU8(input));
}
inline std::uint16_t readU16BE(std::istream& input) {
 const std::uint16_t high = readU8(input);
 const std::uint16_t low = readU8(input);
 return static_cast<std::uint16_t>((high << 8U) | low);
}
inline std::int16_t readI16BE(std::istream& input) {
 return static_cast<std::int16_t>(readU16BE(input));
}
inline std::uint32_t readU32BE(std::istream& input) {
 const std::uint32_t b0 = readU8(input);
 const std::uint32_t b1 = readU8(input);
 const std::uint32_t b2 = readU8(input);
 const std::uint32_t b3 = readU8(input);
 return (b0 << 24U) | (b1 << 16U) | (b2 << 8U) | b3;
}
inline std::int32_t readI32BE(std::istream& input) {
 return static_cast<std::int32_t>(readU32BE(input));
}
inline std::uint64_t readU64BE(std::istream& input) {
 std::uint64_t value = 0;
 for(int i = 0; i < 8; ++i) {
  value = (value << 8U) | readU8(input);
 }
 return value;
}
inline std::int64_t readI64BE(std::istream& input) {
 return static_cast<std::int64_t>(readU64BE(input));
}
inline float readFloatBE(std::istream& input) {
 const std::uint32_t raw = readU32BE(input);
 float value = 0.0f;
 std::memcpy(&value, &raw, sizeof(value));
 return value;
}
inline double readDoubleBE(std::istream& input) {
 const std::uint64_t raw = readU64BE(input);
 double value = 0.0;
 std::memcpy(&value, &raw, sizeof(value));
 return value;
}
inline void writeU8(std::ostream& output, std::uint8_t value) {
 output.put(static_cast<char>(value));
 if(!output) {
  throw std::runtime_error("Failed to write packet byte");
 }
}
inline void writeI8(std::ostream& output, std::int8_t value) {
 writeU8(output, static_cast<std::uint8_t>(value));
}
inline void writeU16BE(std::ostream& output, std::uint16_t value) {
 writeU8(output, static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
 writeU8(output, static_cast<std::uint8_t>(value & 0xFFU));
}
inline void writeI16BE(std::ostream& output, std::int16_t value) {
 writeU16BE(output, static_cast<std::uint16_t>(value));
}
inline void writeU32BE(std::ostream& output, std::uint32_t value) {
 writeU8(output, static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
 writeU8(output, static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
 writeU8(output, static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
 writeU8(output, static_cast<std::uint8_t>(value & 0xFFU));
}
inline void writeI32BE(std::ostream& output, std::int32_t value) {
 writeU32BE(output, static_cast<std::uint32_t>(value));
}
inline void writeU64BE(std::ostream& output, std::uint64_t value) {
 for(int shift = 56; shift >= 0; shift -= 8) {
  writeU8(output, static_cast<std::uint8_t>((value >> shift) & 0xFFU));
 }
}
inline void writeI64BE(std::ostream& output, std::int64_t value) {
 writeU64BE(output, static_cast<std::uint64_t>(value));
}
inline void writeFloatBE(std::ostream& output, float value) {
 std::uint32_t raw = 0;
 std::memcpy(&raw, &value, sizeof(value));
 writeU32BE(output, raw);
}
inline void writeDoubleBE(std::ostream& output, double value) {
 std::uint64_t raw = 0;
 std::memcpy(&raw, &value, sizeof(value));
 writeU64BE(output, raw);
}
inline std::vector<std::uint8_t> readBytes(std::istream& input, std::size_t count) {
 std::vector<std::uint8_t> bytes(count);
 if(count == 0) {
  return bytes;
 }
 input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(count));
 if(input.gcount() != static_cast<std::streamsize>(count)) {
  throw std::runtime_error("Unexpected end of packet stream");
 }
 return bytes;
}
inline void writeBytes(std::ostream& output, const std::vector<std::uint8_t>& bytes) {
 if(!bytes.empty()) {
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if(!output) {
   throw std::runtime_error("Failed to write packet bytes");
  }
 }
}
inline std::u16string toUtf16(std::string_view value) {
 return binary::utf8ToUtf16(value);
}
inline std::string fromUtf16(const std::u16string& value) {
 return binary::utf16ToUtf8(value);
}
inline void writeJavaString(std::ostream& output, std::string_view value) {
 const std::u16string utf16 = toUtf16(value);
 // Java Packet.writeString rejects length > Short.MAX_VALUE (32767); the peer's
 // readString reads a signed short and throws on a negative length. Mirror that
 // bound exactly so we never emit a string a vanilla peer would reject.
 if(utf16.size() > 0x7FFFU) {
  throw std::runtime_error("Packet string is too long");
 }
 writeU16BE(output, static_cast<std::uint16_t>(utf16.size()));
 for(const char16_t ch : utf16) {
  writeU16BE(output, static_cast<std::uint16_t>(ch));
 }
}
inline std::string readJavaString(std::istream& input, std::uint16_t maxChars) {
 const std::uint16_t length = readU16BE(input);
 if(length > maxChars) {
  throw std::runtime_error("Received packet string longer than allowed length");
 }
 std::u16string utf16;
 utf16.reserve(length);
 for(std::uint16_t i = 0; i < length; ++i) {
  utf16.push_back(static_cast<char16_t>(readU16BE(input)));
 }
 return fromUtf16(utf16);
}
inline void writeUtfString(std::ostream& output, std::string_view value) {
 const std::vector<std::uint8_t> encoded = binary::encodeModifiedUtf8(value);
 if(encoded.size() > 0xFFFFU) {
  throw std::runtime_error("UTF packet string is too long");
 }
 writeU16BE(output, static_cast<std::uint16_t>(encoded.size()));
 writeBytes(output, encoded);
}
inline std::string readUtfString(std::istream& input) {
 const std::uint16_t byteLength = readU16BE(input);
 const std::vector<std::uint8_t> bytes = readBytes(input, byteLength);
 std::size_t pos = 0;
 return binary::decodeModifiedUtf8(bytes, pos, byteLength);
}
inline bool readBoolean(std::istream& input) {
 return readU8(input) != 0;
}
inline void writeBoolean(std::ostream& output, bool value) {
 writeU8(output, value ? 1U : 0U);
}
} // namespace net::minecraft::packetio
