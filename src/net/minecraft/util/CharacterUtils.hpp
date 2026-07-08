#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace net::minecraft {
// Faithful port of net.minecraft.util.CharacterUtils (beta 1.7.3).
class CharacterUtils {
   public:
    static const std::string& validCharacters() {
        static const std::string chars = loadValidCharacters();
        return chars;
    }

    static constexpr char invalidCharsWorldName[] = {
        '/', '\n', '\r', '\t', '\0', '\f', '`', '?', '*', '\\', '<', '>', '|', '"', ':'};

   private:
    static std::string loadValidCharacters() {
        std::string result;
        const std::filesystem::path path = std::filesystem::path(MINECRAFT_NATIVE_RESOURCE_DIR) / "font.txt";
        std::ifstream in(path);
        if (!in) {
            return result;
        }
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line[0] == '#') {
                continue;
            }
            result += line;
        }
        return result;
    }
};
}  // namespace net::minecraft
