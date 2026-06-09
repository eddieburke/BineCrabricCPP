#pragma once

#include <cstdint>
#include <istream>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace net::minecraft::binary {

inline std::u16string utf8ToUtf16(std::string_view value)
{
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

        if ((width == 2 && codePoint < 0x80U) || (width == 3 && codePoint < 0x800U) || (width == 4 && codePoint < 0x10000U)
            || codePoint > 0x10FFFFU || (codePoint >= 0xD800U && codePoint <= 0xDFFFU)) {
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

inline std::string utf16ToUtf8(std::u16string_view value)
{
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

inline void appendU8(std::vector<std::uint8_t>& out, std::uint8_t value)
{
    out.push_back(value);
}

inline void appendU16BE(std::vector<std::uint8_t>& out, std::uint16_t value)
{
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

inline void appendU32BE(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    out.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

inline void appendU64BE(std::vector<std::uint8_t>& out, std::uint64_t value)
{
    for (int shift = 56; shift >= 0; shift -= 8) {
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
}

inline void appendI16BE(std::vector<std::uint8_t>& out, std::int16_t value)
{
    appendU16BE(out, static_cast<std::uint16_t>(value));
}

inline void appendI32BE(std::vector<std::uint8_t>& out, std::int32_t value)
{
    appendU32BE(out, static_cast<std::uint32_t>(value));
}

inline void appendI64BE(std::vector<std::uint8_t>& out, std::int64_t value)
{
    appendU64BE(out, static_cast<std::uint64_t>(value));
}

inline std::uint8_t readU8(const std::vector<std::uint8_t>& data, std::size_t& pos)
{
    if (pos >= data.size()) {
        throw std::runtime_error("Unexpected end of buffer");
    }
    return data[pos++];
}

inline std::uint16_t readU16BE(const std::vector<std::uint8_t>& data, std::size_t& pos)
{
    const std::uint16_t high = readU8(data, pos);
    const std::uint16_t low = readU8(data, pos);
    return static_cast<std::uint16_t>((high << 8U) | low);
}

inline std::uint32_t readU32BE(const std::vector<std::uint8_t>& data, std::size_t& pos)
{
    const std::uint32_t b0 = readU8(data, pos);
    const std::uint32_t b1 = readU8(data, pos);
    const std::uint32_t b2 = readU8(data, pos);
    const std::uint32_t b3 = readU8(data, pos);
    return (b0 << 24U) | (b1 << 16U) | (b2 << 8U) | b3;
}

inline std::uint64_t readU64BE(const std::vector<std::uint8_t>& data, std::size_t& pos)
{
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8U) | readU8(data, pos);
    }
    return value;
}

inline std::int16_t readI16BE(const std::vector<std::uint8_t>& data, std::size_t& pos)
{
    return static_cast<std::int16_t>(readU16BE(data, pos));
}

inline std::int32_t readI32BE(const std::vector<std::uint8_t>& data, std::size_t& pos)
{
    return static_cast<std::int32_t>(readU32BE(data, pos));
}

inline std::int64_t readI64BE(const std::vector<std::uint8_t>& data, std::size_t& pos)
{
    return static_cast<std::int64_t>(readU64BE(data, pos));
}

inline std::vector<std::uint8_t> readAllBytes(std::istream& input)
{
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

inline void writeAllBytes(std::ostream& output, const std::vector<std::uint8_t>& bytes)
{
    if (!bytes.empty()) {
        output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
}

inline std::vector<std::uint8_t> encodeModifiedUtf8(std::string_view value)
{
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

inline std::string decodeModifiedUtf8(const std::vector<std::uint8_t>& data, std::size_t& pos, std::uint16_t byteLength)
{
    if (pos + byteLength > data.size()) {
        throw std::runtime_error("Unexpected end of buffer while reading UTF string");
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
            utf16.push_back(static_cast<char16_t>(((c & 0x1FU) << 6U) | (c2 & 0x3FU)));
            continue;
        }

        if ((c & 0xF0U) == 0xE0U) {
            if (pos + 1 >= end) {
                throw std::runtime_error("Malformed modified UTF-8 sequence");
            }
            const std::uint8_t c2 = data[pos++];
            const std::uint8_t c3 = data[pos++];
            utf16.push_back(static_cast<char16_t>(((c & 0x0FU) << 12U) | ((c2 & 0x3FU) << 6U) | (c3 & 0x3FU)));
            continue;
        }

        throw std::runtime_error("Malformed modified UTF-8 sequence");
    }

    return utf16ToUtf8(utf16);
}

inline void writeModifiedUtf8(std::vector<std::uint8_t>& out, std::string_view value)
{
    const std::vector<std::uint8_t> encoded = encodeModifiedUtf8(value);
    if (encoded.size() > 0xFFFFU) {
        throw std::runtime_error("NBT string is too long");
    }
    appendU16BE(out, static_cast<std::uint16_t>(encoded.size()));
    out.insert(out.end(), encoded.begin(), encoded.end());
}

inline std::string readModifiedUtf8(const std::vector<std::uint8_t>& data, std::size_t& pos)
{
    const std::uint16_t byteLength = readU16BE(data, pos);
    return decodeModifiedUtf8(data, pos, byteLength);
}

} // namespace net::minecraft::binary
