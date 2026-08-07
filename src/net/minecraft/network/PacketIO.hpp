#pragma once
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "net/minecraft/nbt/BinaryIO.hpp"
namespace net::minecraft::packetio {
class PacketUnderflow : public std::runtime_error {
 public:
  PacketUnderflow() : std::runtime_error("Unexpected end of packet buffer") {
  }
};
inline void requireAvailable(const std::uint8_t* src, const std::uint8_t* end, std::size_t count) {
  if(end - src < static_cast<std::ptrdiff_t>(count)) {
   throw PacketUnderflow();
  }
}
inline std::uint8_t readU8(const std::uint8_t*& src, const std::uint8_t* end) {
  requireAvailable(src, end, 1);
  return *src++;
}
inline std::int8_t readI8(const std::uint8_t*& src, const std::uint8_t* end) {
  return static_cast<std::int8_t>(readU8(src, end));
}
inline std::uint16_t readU16BE(const std::uint8_t*& src, const std::uint8_t* end) {
  requireAvailable(src, end, 2);
  const std::uint16_t value = static_cast<std::uint16_t>((static_cast<std::uint16_t>(src[0]) << 8U) | src[1]);
  src += 2;
  return value;
}
inline std::int16_t readI16BE(const std::uint8_t*& src, const std::uint8_t* end) {
  return static_cast<std::int16_t>(readU16BE(src, end));
}
inline std::uint32_t readU32BE(const std::uint8_t*& src, const std::uint8_t* end) {
  requireAvailable(src, end, 4);
  const std::uint32_t value = (static_cast<std::uint32_t>(src[0]) << 24U) |
                              (static_cast<std::uint32_t>(src[1]) << 16U) |
                              (static_cast<std::uint32_t>(src[2]) << 8U) | src[3];
  src += 4;
  return value;
}
inline std::int32_t readI32BE(const std::uint8_t*& src, const std::uint8_t* end) {
  return static_cast<std::int32_t>(readU32BE(src, end));
}
inline std::uint64_t readU64BE(const std::uint8_t*& src, const std::uint8_t* end) {
  requireAvailable(src, end, 8);
  std::uint64_t value = 0;
  for(int i = 0; i < 8; ++i) {
   value = (value << 8U) | src[i];
  }
  src += 8;
  return value;
}
inline std::int64_t readI64BE(const std::uint8_t*& src, const std::uint8_t* end) {
  return static_cast<std::int64_t>(readU64BE(src, end));
}
inline float readFloatBE(const std::uint8_t*& src, const std::uint8_t* end) {
  const std::uint32_t raw = readU32BE(src, end);
  float value = 0.0f;
  std::memcpy(&value, &raw, sizeof(value));
  return value;
}
inline double readDoubleBE(const std::uint8_t*& src, const std::uint8_t* end) {
  const std::uint64_t raw = readU64BE(src, end);
  double value = 0.0;
  std::memcpy(&value, &raw, sizeof(value));
  return value;
}
inline std::vector<std::uint8_t> readBytes(const std::uint8_t*& src, const std::uint8_t* end, std::size_t count) {
  requireAvailable(src, end, count);
  std::vector<std::uint8_t> bytes(src, src + count);
  src += count;
  return bytes;
}
inline bool readBoolean(const std::uint8_t*& src, const std::uint8_t* end) {
  return readU8(src, end) != 0;
}
inline void writeU8(std::uint8_t*& dest, std::uint8_t* end, std::uint8_t value) {
  if(dest >= end) {
   throw std::runtime_error("Packet write buffer overflow");
  }
  *dest++ = value;
}
inline void writeI8(std::uint8_t*& dest, std::uint8_t* end, std::int8_t value) {
  writeU8(dest, end, static_cast<std::uint8_t>(value));
}
inline void writeU16BE(std::uint8_t*& dest, std::uint8_t* end, std::uint16_t value) {
  writeU8(dest, end, static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
  writeU8(dest, end, static_cast<std::uint8_t>(value & 0xFFU));
}
inline void writeI16BE(std::uint8_t*& dest, std::uint8_t* end, std::int16_t value) {
  writeU16BE(dest, end, static_cast<std::uint16_t>(value));
}
inline void writeU32BE(std::uint8_t*& dest, std::uint8_t* end, std::uint32_t value) {
  writeU8(dest, end, static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
  writeU8(dest, end, static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
  writeU8(dest, end, static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
  writeU8(dest, end, static_cast<std::uint8_t>(value & 0xFFU));
}
inline void writeI32BE(std::uint8_t*& dest, std::uint8_t* end, std::int32_t value) {
  writeU32BE(dest, end, static_cast<std::uint32_t>(value));
}
inline void writeU64BE(std::uint8_t*& dest, std::uint8_t* end, std::uint64_t value) {
  for(int shift = 56; shift >= 0; shift -= 8) {
   writeU8(dest, end, static_cast<std::uint8_t>((value >> shift) & 0xFFU));
  }
}
inline void writeI64BE(std::uint8_t*& dest, std::uint8_t* end, std::int64_t value) {
  writeU64BE(dest, end, static_cast<std::uint64_t>(value));
}
inline void writeFloatBE(std::uint8_t*& dest, std::uint8_t* end, float value) {
  std::uint32_t raw = 0;
  std::memcpy(&raw, &value, sizeof(value));
  writeU32BE(dest, end, raw);
}
inline void writeDoubleBE(std::uint8_t*& dest, std::uint8_t* end, double value) {
  std::uint64_t raw = 0;
  std::memcpy(&raw, &value, sizeof(value));
  writeU64BE(dest, end, raw);
}
inline void writeBytes(std::uint8_t*& dest, std::uint8_t* end, const std::uint8_t* data, std::size_t count) {
  if(count == 0) {
   return;
  }
  if(end - dest < static_cast<std::ptrdiff_t>(count)) {
   throw std::runtime_error("Packet write buffer overflow");
  }
  std::memcpy(dest, data, count);
  dest += count;
}
inline void writeBytes(std::uint8_t*& dest, std::uint8_t* end, const std::vector<std::uint8_t>& bytes) {
  writeBytes(dest, end, bytes.data(), bytes.size());
}
inline void writeBoolean(std::uint8_t*& dest, std::uint8_t* end, bool value) {
  writeU8(dest, end, value ? 1U : 0U);
}
inline std::size_t javaStringLength(std::string_view value) {
  std::size_t length = 0;
  for(std::size_t i = 0; i < value.size();) {
   length += binary::decodeUtf8At(value, i) <= 0xFFFFU ? 1 : 2;
  }
  return length;
}
inline std::size_t javaStringSize(std::string_view value) {
  return 2U + 2U * javaStringLength(value);
}
inline std::size_t utfStringSize(std::string_view value) {
  std::size_t bytes = 2U;
  for(std::size_t i = 0; i < value.size();) {
   const std::uint32_t codePoint = binary::decodeUtf8At(value, i);
   if(codePoint == 0) {
    bytes += 2;
   } else if(codePoint <= 0x7FU) {
    bytes += 1;
   } else if(codePoint <= 0x7FFU) {
    bytes += 2;
   } else if(codePoint <= 0xFFFFU) {
    bytes += 3;
   } else {
    bytes += 6;
   }
  }
  return bytes;
}
inline void writeJavaString(std::uint8_t*& dest, std::uint8_t* end, std::string_view value) {
  const std::size_t length = javaStringLength(value);
  if(length > 0x7FFFU) {
   throw std::runtime_error("Packet string is too long");
  }
  writeU16BE(dest, end, static_cast<std::uint16_t>(length));
  for(std::size_t i = 0; i < value.size();) {
   const std::uint32_t codePoint = binary::decodeUtf8At(value, i);
   if(codePoint <= 0xFFFFU) {
    writeU16BE(dest, end, static_cast<std::uint16_t>(codePoint));
   } else {
    const std::uint32_t offset = codePoint - 0x10000U;
    writeU16BE(dest, end, static_cast<std::uint16_t>(0xD800U + (offset >> 10U)));
    writeU16BE(dest, end, static_cast<std::uint16_t>(0xDC00U + (offset & 0x3FFU)));
   }
  }
}
inline std::string readJavaString(const std::uint8_t*& src, const std::uint8_t* end, std::uint16_t maxChars) {
  const std::uint16_t length = readU16BE(src, end);
  if(length > maxChars) {
   throw std::runtime_error("Received packet string longer than allowed length");
  }
  std::string result;
  result.reserve(length);
  std::uint32_t pendingHigh = 0;
  bool hasPendingHigh = false;
  for(std::uint16_t i = 0; i < length; ++i) {
   const std::uint32_t unit = readU16BE(src, end);
   if(hasPendingHigh) {
    if(unit >= 0xDC00U && unit <= 0xDFFFU) {
     binary::appendCodePointToUtf8(
         result, 0x10000U + (((pendingHigh - 0xD800U) << 10U) | (unit - 0xDC00U)));
     hasPendingHigh = false;
    } else {
     throw std::runtime_error("Malformed UTF-16 surrogate pair");
    }
   } else if(unit >= 0xD800U && unit <= 0xDBFFU) {
    pendingHigh = unit;
    hasPendingHigh = true;
   } else if(unit >= 0xDC00U && unit <= 0xDFFFU) {
    throw std::runtime_error("Unexpected UTF-16 low surrogate");
   } else {
    binary::appendCodePointToUtf8(result, unit);
   }
  }
  if(hasPendingHigh) {
   throw std::runtime_error("Truncated UTF-16 surrogate pair");
  }
  return result;
}
inline void writeUtfString(std::uint8_t*& dest, std::uint8_t* end, std::string_view value) {
  const std::size_t byteLength = utfStringSize(value) - 2U;
  if(byteLength > 0xFFFFU) {
   throw std::runtime_error("UTF packet string is too long");
  }
  writeU16BE(dest, end, static_cast<std::uint16_t>(byteLength));
  for(std::size_t i = 0; i < value.size();) {
   const std::uint32_t codePoint = binary::decodeUtf8At(value, i);
   if(codePoint == 0) {
    writeU8(dest, end, 0xC0U);
    writeU8(dest, end, 0x80U);
   } else if(codePoint <= 0x7FU) {
    writeU8(dest, end, static_cast<std::uint8_t>(codePoint));
   } else if(codePoint <= 0x7FFU) {
    writeU8(dest, end, static_cast<std::uint8_t>(0xC0U | ((codePoint >> 6U) & 0x1FU)));
    writeU8(dest, end, static_cast<std::uint8_t>(0x80U | (codePoint & 0x3FU)));
   } else if(codePoint <= 0xFFFFU) {
    writeU8(dest, end, static_cast<std::uint8_t>(0xE0U | ((codePoint >> 12U) & 0x0FU)));
    writeU8(dest, end, static_cast<std::uint8_t>(0x80U | ((codePoint >> 6U) & 0x3FU)));
    writeU8(dest, end, static_cast<std::uint8_t>(0x80U | (codePoint & 0x3FU)));
   } else {
    const std::uint32_t offset = codePoint - 0x10000U;
    const std::uint16_t units[2] = {
        static_cast<std::uint16_t>(0xD800U + (offset >> 10U)),
        static_cast<std::uint16_t>(0xDC00U + (offset & 0x3FFU))};
    for(const std::uint16_t unit : units) {
     writeU8(dest, end, static_cast<std::uint8_t>(0xE0U | ((unit >> 12U) & 0x0FU)));
     writeU8(dest, end, static_cast<std::uint8_t>(0x80U | ((unit >> 6U) & 0x3FU)));
     writeU8(dest, end, static_cast<std::uint8_t>(0x80U | (unit & 0x3FU)));
    }
   }
  }
}
inline std::string readUtfString(const std::uint8_t*& src, const std::uint8_t* end) {
  const std::uint16_t byteLength = readU16BE(src, end);
  requireAvailable(src, end, byteLength);
  std::string result = binary::decodeModifiedUtf8(src, byteLength);
  src += byteLength;
  return result;
}
} // namespace net::minecraft::packetio
