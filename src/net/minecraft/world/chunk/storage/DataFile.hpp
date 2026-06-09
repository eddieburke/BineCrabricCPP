#pragma once

#include "net/minecraft/util/math/Types.hpp"

#include <filesystem>
#include <string>

namespace net::minecraft {

namespace fs = std::filesystem;

class DataFile {
public:
    explicit DataFile(fs::path file)
        : file_(std::move(file))
    {
        const std::string name = file_.filename().string();
        // c.<x>.<z>.dat
        const std::size_t firstDot = name.find('.');
        const std::size_t secondDot = name.find('.', firstDot + 1);
        const std::size_t thirdDot = name.find('.', secondDot + 1);
        if (firstDot != std::string::npos && secondDot != std::string::npos && thirdDot != std::string::npos) {
            chunkX_ = parseBase36(name.substr(firstDot + 1, secondDot - firstDot - 1));
            chunkZ_ = parseBase36(name.substr(secondDot + 1, thirdDot - secondDot - 1));
        }
    }

    [[nodiscard]] int compareTo(const DataFile& other) const noexcept
    {
        if (chunkX_ != other.chunkX_) {
            return chunkX_ < other.chunkX_ ? -1 : 1;
        }
        if (chunkZ_ != other.chunkZ_) {
            return chunkZ_ < other.chunkZ_ ? -1 : 1;
        }
        return 0;
    }

    [[nodiscard]] const fs::path& getFile() const noexcept { return file_; }
    [[nodiscard]] int getChunkX() const noexcept { return chunkX_; }
    [[nodiscard]] int getChunkZ() const noexcept { return chunkZ_; }

private:
    static int parseBase36(const std::string& value)
    {
        int result = 0;
        bool negative = false;
        std::size_t start = 0;
        if (!value.empty() && value[0] == '-') {
            negative = true;
            start = 1;
        }
        for (std::size_t i = start; i < value.size(); ++i) {
            const char c = value[i];
            int digit = 0;
            if (c >= '0' && c <= '9') {
                digit = c - '0';
            } else if (c >= 'a' && c <= 'z') {
                digit = 10 + (c - 'a');
            } else {
                continue;
            }
            result = result * 36 + digit;
        }
        return negative ? -result : result;
    }

    fs::path file_;
    int chunkX_ = 0;
    int chunkZ_ = 0;
};

} // namespace net::minecraft
