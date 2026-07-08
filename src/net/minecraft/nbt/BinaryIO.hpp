#pragma once
#include <cstdint>
#include <cstring>
#include <istream>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace net::minecraft::binary {
inline std::u16string utf8ToUtf16(std::string_view value) {
    std::u16string result;
    result.reserve(value.size());
    for (std::size_t i = 0; i < value.size();) {
        const std::uint8_t lead = static_cast<std::uint8_t>(value[i]);
        std::uint32_t codePoint = 0;
        std::size_t width = 0;
        if ((lead & 0x80U) == 0U) {
            codePoint = lead;
            width = 1;
        } else if ((lead & 0xE0U) == 0xC0U) {
            codePoint = lead & 0x1FU;
            width = 2;
        } else if ((lead & 0xF0U) == 0xE0U) {
            codePoint = lead & 0x0FU;
            width = 3;
        } else if ((lead & 0xF8U) == 0xF0U) {
            codePoint = lead & 0x07U;
            width = 4;
        } else {
            throw std::runtime_error("Malformed UTF-8 sequence");
        }
        if (i + width > value.size()) {
            throw std::runtime_error("Truncated UTF-8 sequence");
        }
        for (std::size_t j = 1; j < width; ++j) {
            const std::uint8_t tail = static_cast<std::uint8_t>(value[i + j]);
            if ((tail & 0xC0U) != 0x80U) {
                throw std::runtime_error("Malformed UTF-8 continuation byte");
            }
            codePoint = (codePoint << 6U) | (tail & 0x3FU);
        }
        if ((width == 2 && codePoint < 0x80U) || (width == 3 && codePoint < 0x800U) ||
            (width == 4 && codePoint < 0x10000U) || codePoint > 0x10FFFFU ||
            (codePoint >= 0xD800U && codePoint <= 0xDFFFU)) {
            throw std::runtime_error("Invalid UTF-8 code point");
        }
        if (codePoint <= 0xFFFFU) {
            result.push_back(static_cast<char16_t>(codePoint));
        } else {
            codePoint -= 0x10000U;
            result.push_back(static_cast<char16_t>(0xD800U + (codePoint >> 10U)));
            result.push_back(static_cast<char16_t>(0xDC00U + (codePoint & 0x3FFU)));
        }
        i += width;
    }
    return result;
}

inline std::string utf16ToUtf8(std::u16string_view value) {
    std::string result;
    result.reserve(value.size() * 3U);
    for (std::size_t i = 0; i < value.size(); ++i) {
        std::uint32_t codePoint = value[i];
        if (codePoint >= 0xD800U && codePoint <= 0xDBFFU) {
            if (i + 1 >= value.size()) {
                throw std::runtime_error("Truncated UTF-16 surrogate pair");
            }
            const std::uint32_t low = value[++i];
            if (low < 0xDC00U || low > 0xDFFFU) {
                throw std::runtime_error("Malformed UTF-16 surrogate pair");
            }
            codePoint = 0x10000U + (((codePoint - 0xD800U) << 10U) | (low - 0xDC00U));
        } else if (codePoint >= 0xDC00U && codePoint <= 0xDFFFU) {
            throw std::runtime_error("Unexpected UTF-16 low surrogate");
        }
        if (codePoint <= 0x7FU) {
            result.push_back(static_cast<char>(codePoint));
        } else if (codePoint <= 0x7FFU) {
            result.push_back(static_cast<char>(0xC0U | (codePoint >> 6U)));
            result.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
        } else if (codePoint <= 0xFFFFU) {
            result.push_back(static_cast<char>(0xE0U | (codePoint >> 12U)));
            result.push_back(static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU)));
            result.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
        } else {
            result.push_back(static_cast<char>(0xF0U | (codePoint >> 18U)));
            result.push_back(static_cast<char>(0x80U | ((codePoint >> 12U) & 0x3FU)));
            result.push_back(static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU)));
            result.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
        }
    }
    return result;
}

inline bool isAsciiUtf8(std::string_view value) noexcept {
    for (const unsigned char ch : value) {
        if (ch >= 0x80U) {
            return false;
        }
    }
    return true;
}

inline bool isAsciiModifiedUtf8(const std::uint8_t* data, std::size_t byteLength) noexcept {
    for (std::size_t i = 0; i < byteLength; ++i) {
        if (data[i] >= 0x80U) {
            return false;
        }
    }
    return true;
}

inline void appendBytes(std::vector<std::uint8_t>& out, const std::uint8_t* data, std::size_t size) {
    if (size == 0U) {
        return;
    }
    const std::size_t oldSize = out.size();
    out.resize(oldSize + size);
    std::memcpy(out.data() + oldSize, data, size);
}

inline void appendU8(std::vector<std::uint8_t>& out, std::uint8_t value) {
    out.push_back(value);
}

inline void appendU16BE(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

inline void appendU32BE(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

inline void appendU64BE(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
}

inline void appendI16BE(std::vector<std::uint8_t>& out, std::int16_t value) {
    appendU16BE(out, static_cast<std::uint16_t>(value));
}

inline void appendI32BE(std::vector<std::uint8_t>& out, std::int32_t value) {
    appendU32BE(out, static_cast<std::uint32_t>(value));
}

inline void appendI64BE(std::vector<std::uint8_t>& out, std::int64_t value) {
    appendU64BE(out, static_cast<std::uint64_t>(value));
}

inline void appendI32BEArray(std::vector<std::uint8_t>& out, const std::int32_t* values, std::size_t count) {
    if (count == 0U) {
        return;
    }
    const std::size_t oldSize = out.size();
    out.resize(oldSize + count * 4U);
    std::uint8_t* dest = out.data() + oldSize;
    for (std::size_t i = 0; i < count; ++i) {
        const std::uint32_t value = static_cast<std::uint32_t>(values[i]);
        dest[0] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
        dest[1] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
        dest[2] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
        dest[3] = static_cast<std::uint8_t>(value & 0xFFU);
        dest += 4;
    }
}

inline void appendI64BEArray(std::vector<std::uint8_t>& out, const std::int64_t* values, std::size_t count) {
    if (count == 0U) {
        return;
    }
    const std::size_t oldSize = out.size();
    out.resize(oldSize + count * 8U);
    std::uint8_t* dest = out.data() + oldSize;
    for (std::size_t i = 0; i < count; ++i) {
        const std::uint64_t value = static_cast<std::uint64_t>(values[i]);
        for (int shift = 56; shift >= 0; shift -= 8) {
            *dest++ = static_cast<std::uint8_t>((value >> shift) & 0xFFU);
        }
    }
}

inline std::uint8_t readU8(const std::vector<std::uint8_t>& data, std::size_t& pos) {
    if (pos >= data.size()) {
        throw std::runtime_error("Unexpected end of buffer");
    }
    return data[pos++];
}

inline std::uint16_t readU16BE(const std::vector<std::uint8_t>& data, std::size_t& pos) {
    if (pos > data.size() || data.size() - pos < 2U) {
        throw std::runtime_error("Unexpected end of buffer");
    }
    const std::uint16_t value =
        static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[pos]) << 8U) | data[pos + 1]);
    pos += 2;
    return value;
}

inline std::uint32_t readU32BE(const std::vector<std::uint8_t>& data, std::size_t& pos) {
    if (pos > data.size() || data.size() - pos < 4U) {
        throw std::runtime_error("Unexpected end of buffer");
    }
    const std::uint8_t* bytes = data.data() + pos;
    const std::uint32_t value = (static_cast<std::uint32_t>(bytes[0]) << 24U) |
                                (static_cast<std::uint32_t>(bytes[1]) << 16U) |
                                (static_cast<std::uint32_t>(bytes[2]) << 8U) | bytes[3];
    pos += 4;
    return value;
}

inline std::uint64_t readU64BE(const std::vector<std::uint8_t>& data, std::size_t& pos) {
    if (pos > data.size() || data.size() - pos < 8U) {
        throw std::runtime_error("Unexpected end of buffer");
    }
    std::uint64_t value = 0;
    const std::uint8_t* bytes = data.data() + pos;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8U) | bytes[i];
    }
    pos += 8;
    return value;
}

inline std::int16_t readI16BE(const std::vector<std::uint8_t>& data, std::size_t& pos) {
    return static_cast<std::int16_t>(readU16BE(data, pos));
}

inline std::int32_t readI32BE(const std::vector<std::uint8_t>& data, std::size_t& pos) {
    return static_cast<std::int32_t>(readU32BE(data, pos));
}

inline std::int64_t readI64BE(const std::vector<std::uint8_t>& data, std::size_t& pos) {
    return static_cast<std::int64_t>(readU64BE(data, pos));
}

inline void readBytes(const std::vector<std::uint8_t>& data, std::size_t& pos, std::uint8_t* dest, std::size_t size) {
    if (size == 0U) {
        return;
    }
    if (pos > data.size() || size > data.size() - pos) {
        throw std::runtime_error("Unexpected end of buffer");
    }
    std::memcpy(dest, data.data() + pos, size);
    pos += size;
}

inline void readI32BEArray(const std::vector<std::uint8_t>& data,
                           std::size_t& pos,
                           std::int32_t* dest,
                           std::size_t count) {
    const std::size_t byteCount = count * 4U;
    if (byteCount == 0U) {
        return;
    }
    if (pos > data.size() || byteCount > data.size() - pos) {
        throw std::runtime_error("Unexpected end of buffer");
    }
    const std::uint8_t* src = data.data() + pos;
    for (std::size_t i = 0; i < count; ++i) {
        dest[i] = static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(src[0]) << 24U) | (static_cast<std::uint32_t>(src[1]) << 16U) |
            (static_cast<std::uint32_t>(src[2]) << 8U) | static_cast<std::uint32_t>(src[3]));
        src += 4;
    }
    pos += byteCount;
}

inline void readI64BEArray(const std::vector<std::uint8_t>& data,
                           std::size_t& pos,
                           std::int64_t* dest,
                           std::size_t count) {
    const std::size_t byteCount = count * 8U;
    if (byteCount == 0U) {
        return;
    }
    if (pos > data.size() || byteCount > data.size() - pos) {
        throw std::runtime_error("Unexpected end of buffer");
    }
    const std::uint8_t* src = data.data() + pos;
    for (std::size_t i = 0; i < count; ++i) {
        std::uint64_t value = 0;
        for (int j = 0; j < 8; ++j) {
            value = (value << 8U) | static_cast<std::uint64_t>(src[j]);
        }
        dest[i] = static_cast<std::int64_t>(value);
        src += 8;
    }
    pos += byteCount;
}

inline std::vector<std::uint8_t> readAllBytes(std::istream& input) {
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

inline void writeAllBytes(std::ostream& output, const std::vector<std::uint8_t>& bytes) {
    if (!bytes.empty()) {
        output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
}

inline std::vector<std::uint8_t> encodeModifiedUtf8(std::string_view value) {
    if (isAsciiUtf8(value)) {
        return std::vector<std::uint8_t>(reinterpret_cast<const std::uint8_t*>(value.data()),
                                         reinterpret_cast<const std::uint8_t*>(value.data() + value.size()));
    }
    const std::u16string utf16 = utf8ToUtf16(value);
    std::vector<std::uint8_t> encoded;
    encoded.reserve(utf16.size() * 3U);
    for (const char16_t ch : utf16) {
        if (ch >= 0x0001 && ch <= 0x007F) {
            encoded.push_back(static_cast<std::uint8_t>(ch));
        } else if (ch == 0x0000) {
            encoded.push_back(0xC0U);
            encoded.push_back(0x80U);
        } else if (ch <= 0x07FF) {
            encoded.push_back(static_cast<std::uint8_t>(0xC0U | ((ch >> 6U) & 0x1FU)));
            encoded.push_back(static_cast<std::uint8_t>(0x80U | (ch & 0x3FU)));
        } else {
            encoded.push_back(static_cast<std::uint8_t>(0xE0U | ((ch >> 12U) & 0x0FU)));
            encoded.push_back(static_cast<std::uint8_t>(0x80U | ((ch >> 6U) & 0x3FU)));
            encoded.push_back(static_cast<std::uint8_t>(0x80U | (ch & 0x3FU)));
        }
    }
    return encoded;
}

inline std::string decodeModifiedUtf8(const std::vector<std::uint8_t>& data,
                                      std::size_t& pos,
                                      std::uint16_t byteLength) {
    if (pos > data.size() || byteLength > data.size() - pos) {
        throw std::runtime_error("Unexpected end of buffer while reading UTF string");
    }
    if (byteLength == 0U) {
        return {};
    }
    if (isAsciiModifiedUtf8(data.data() + pos, byteLength)) {
        std::string result(reinterpret_cast<const char*>(data.data() + pos), byteLength);
        pos += byteLength;
        return result;
    }
    std::u16string utf16;
    utf16.reserve(byteLength);
    const std::size_t end = pos + byteLength;
    while (pos < end) {
        const std::uint8_t c = data[pos++];
        if ((c & 0x80U) == 0U) {
            utf16.push_back(static_cast<char16_t>(c));
            continue;
        }
        if ((c & 0xE0U) == 0xC0U) {
            if (pos >= end) {
                throw std::runtime_error("Malformed modified UTF-8 sequence");
            }
            const std::uint8_t c2 = data[pos++];
            if ((c2 & 0xC0U) != 0x80U) {
                throw std::runtime_error("Malformed modified UTF-8 continuation byte");
            }
            utf16.push_back(static_cast<char16_t>(((c & 0x1FU) << 6U) | (c2 & 0x3FU)));
            continue;
        }
        if ((c & 0xF0U) == 0xE0U) {
            if (pos + 1 >= end) {
                throw std::runtime_error("Malformed modified UTF-8 sequence");
            }
            const std::uint8_t c2 = data[pos++];
            const std::uint8_t c3 = data[pos++];
            if ((c2 & 0xC0U) != 0x80U || (c3 & 0xC0U) != 0x80U) {
                throw std::runtime_error("Malformed modified UTF-8 continuation byte");
            }
            utf16.push_back(static_cast<char16_t>(((c & 0x0FU) << 12U) | ((c2 & 0x3FU) << 6U) | (c3 & 0x3FU)));
            continue;
        }
        throw std::runtime_error("Malformed modified UTF-8 sequence");
    }
    return utf16ToUtf8(utf16);
}

inline void writeModifiedUtf8(std::vector<std::uint8_t>& out, std::string_view value) {
    if (value.size() > 0xFFFFU) {
        throw std::runtime_error("NBT string is too long");
    }
    if (isAsciiUtf8(value)) {
        appendU16BE(out, static_cast<std::uint16_t>(value.size()));
        appendBytes(out, reinterpret_cast<const std::uint8_t*>(value.data()), value.size());
        return;
    }
    const std::vector<std::uint8_t> encoded = encodeModifiedUtf8(value);
    if (encoded.size() > 0xFFFFU) {
        throw std::runtime_error("NBT string is too long");
    }
    appendU16BE(out, static_cast<std::uint16_t>(encoded.size()));
    appendBytes(out, encoded.data(), encoded.size());
}

inline std::string readModifiedUtf8(const std::vector<std::uint8_t>& data, std::size_t& pos) {
    const std::uint16_t byteLength = readU16BE(data, pos);
    return decodeModifiedUtf8(data, pos, byteLength);
}
}  // namespace net::minecraft::binary
