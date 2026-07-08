#include "net/minecraft/util/SeedText.hpp"

#include <stdexcept>

namespace net::minecraft::util {
std::int32_t javaStringHashCode(const std::string& text) {
    std::int32_t hash = 0;
    for (unsigned char ch : text) {
        hash = 31 * hash + static_cast<std::int32_t>(ch);
    }
    return hash;
}

std::uint64_t resolveSeedText(const std::string& text) {
    if (text.empty()) {
        return 0;
    }
    std::size_t digit = text[0] == '-' || text[0] == '+' ? 1 : 0;
    bool signedDecimal = digit < text.size();
    for (; digit < text.size(); ++digit) {
        signedDecimal = signedDecimal && text[digit] >= '0' && text[digit] <= '9';
    }
    if (!signedDecimal) {
        return static_cast<std::uint64_t>(static_cast<std::int64_t>(javaStringHashCode(text)));
    }
    try {
        return static_cast<std::uint64_t>(std::stoll(text));
    } catch (const std::invalid_argument&) {
        return static_cast<std::uint64_t>(static_cast<std::int64_t>(javaStringHashCode(text)));
    } catch (const std::out_of_range&) {
        return static_cast<std::uint64_t>(static_cast<std::int64_t>(javaStringHashCode(text)));
    }
}
}  // namespace net::minecraft::util
