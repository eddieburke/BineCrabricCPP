#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace net::minecraft {
namespace fs = std::filesystem;

class ResourcePack {
   public:
    explicit ResourcePack(fs::path root) : root_(std::move(root)) {
    }

    [[nodiscard]] const fs::path& root() const noexcept {
        return root_;
    }

    [[nodiscard]] bool exists(std::string_view relativePath) const {
        return fs::exists(resolve(relativePath));
    }

    [[nodiscard]] std::vector<std::uint8_t> readBinary(std::string_view relativePath) const {
        const fs::path path = resolve(relativePath);
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            return {};
        }
        in.seekg(0, std::ios::end);
        const std::streamsize size = in.tellg();
        in.seekg(0, std::ios::beg);
        std::vector<std::uint8_t> buffer(static_cast<std::size_t>(size));
        if (size > 0) {
            in.read(reinterpret_cast<char*>(buffer.data()), size);
        }
        return buffer;
    }

    [[nodiscard]] std::string readText(std::string_view relativePath) const {
        const std::vector<std::uint8_t> data = readBinary(relativePath);
        return std::string(data.begin(), data.end());
    }

    [[nodiscard]] std::vector<fs::path> listFiles() const {
        std::vector<fs::path> files;
        if (!fs::exists(root_)) {
            return files;
        }
        for (const auto& entry : fs::recursive_directory_iterator(root_)) {
            if (entry.is_regular_file()) {
                files.push_back(fs::relative(entry.path(), root_));
            }
        }
        return files;
    }

   private:
    [[nodiscard]] fs::path resolve(std::string_view relativePath) const {
        fs::path candidate = root_ / fs::path(relativePath);
        candidate = candidate.lexically_normal();
        return candidate;
    }

    fs::path root_;
};
}  // namespace net::minecraft
