#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace net::minecraft::util {

// Faithful port of net.minecraft.util.MD5MessageDigest (beta 1.7.3).
// Java computes MessageDigest.getInstance("MD5") over (salt + s) and returns
// new BigInteger(1, digest).toString(16): a lowercase, big-endian hex string
// with leading zeros stripped. The MD5 primitive is implemented inline since
// the JVM's java.security.MessageDigest is unavailable in native.
class MD5MessageDigest {
public:
    explicit MD5MessageDigest(std::string salt) : salt(std::move(salt)) {}

    std::string digest(const std::string& s) {
        std::string string = this->salt + s;
        std::array<std::uint8_t, 16> bytes = md5(string);

        // BigInteger(1, bytes).toString(16): big-endian hex, leading zeros removed.
        static const char* hexDigits = "0123456789abcdef";
        std::string hex;
        hex.reserve(32);
        for (std::uint8_t b : bytes) {
            hex.push_back(hexDigits[b >> 4]);
            hex.push_back(hexDigits[b & 0x0f]);
        }
        std::size_t start = hex.find_first_not_of('0');
        if (start == std::string::npos) {
            return "0";
        }
        return hex.substr(start);
    }

private:
    std::string salt;

    static std::uint32_t leftRotate(std::uint32_t x, std::uint32_t c) {
        return (x << c) | (x >> (32 - c));
    }

    static std::array<std::uint8_t, 16> md5(const std::string& message) {
        static const std::uint32_t s[64] = {
            7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
            5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
            4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
            6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
        static const std::uint32_t K[64] = {
            0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a,
            0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
            0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340,
            0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
            0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
            0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
            0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
            0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
            0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
            0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
            0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

        std::vector<std::uint8_t> msg(message.begin(), message.end());
        std::uint64_t originalBitLen = static_cast<std::uint64_t>(msg.size()) * 8u;
        msg.push_back(0x80);
        while (msg.size() % 64 != 56) {
            msg.push_back(0x00);
        }
        for (int i = 0; i < 8; ++i) {
            msg.push_back(static_cast<std::uint8_t>((originalBitLen >> (8 * i)) & 0xff));
        }

        std::uint32_t a0 = 0x67452301, b0 = 0xefcdab89, c0 = 0x98badcfe, d0 = 0x10325476;

        for (std::size_t chunk = 0; chunk < msg.size(); chunk += 64) {
            std::uint32_t M[16];
            for (int i = 0; i < 16; ++i) {
                M[i] = static_cast<std::uint32_t>(msg[chunk + i * 4]) |
                       (static_cast<std::uint32_t>(msg[chunk + i * 4 + 1]) << 8) |
                       (static_cast<std::uint32_t>(msg[chunk + i * 4 + 2]) << 16) |
                       (static_cast<std::uint32_t>(msg[chunk + i * 4 + 3]) << 24);
            }

            std::uint32_t A = a0, B = b0, C = c0, D = d0;
            for (std::uint32_t i = 0; i < 64; ++i) {
                std::uint32_t F;
                std::uint32_t g;
                if (i < 16) {
                    F = (B & C) | (~B & D);
                    g = i;
                } else if (i < 32) {
                    F = (D & B) | (~D & C);
                    g = (5 * i + 1) % 16;
                } else if (i < 48) {
                    F = B ^ C ^ D;
                    g = (3 * i + 5) % 16;
                } else {
                    F = C ^ (B | ~D);
                    g = (7 * i) % 16;
                }
                F = F + A + K[i] + M[g];
                A = D;
                D = C;
                C = B;
                B = B + leftRotate(F, s[i]);
            }
            a0 += A;
            b0 += B;
            c0 += C;
            d0 += D;
        }

        std::array<std::uint8_t, 16> digestBytes{};
        std::uint32_t words[4] = {a0, b0, c0, d0};
        for (int i = 0; i < 4; ++i) {
            digestBytes[i * 4] = static_cast<std::uint8_t>(words[i] & 0xff);
            digestBytes[i * 4 + 1] = static_cast<std::uint8_t>((words[i] >> 8) & 0xff);
            digestBytes[i * 4 + 2] = static_cast<std::uint8_t>((words[i] >> 16) & 0xff);
            digestBytes[i * 4 + 3] = static_cast<std::uint8_t>((words[i] >> 24) & 0xff);
        }
        return digestBytes;
    }
};

} // namespace net::minecraft::util
