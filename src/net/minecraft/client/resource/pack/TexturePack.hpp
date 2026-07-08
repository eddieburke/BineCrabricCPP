#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace net::minecraft::client::texture {
class TextureManager;
}

namespace net::minecraft::client::resource::pack {
class TexturePack {
   public:
    virtual ~TexturePack() = default;

    virtual void open() {
    }

    virtual void close() {
    }

    virtual void load() {
    }

    virtual void unload(texture::TextureManager& /*textureManager*/) {
    }

    virtual void bindIcon(texture::TextureManager& /*textureManager*/) {
    }

    [[nodiscard]] virtual std::vector<std::uint8_t> getResource(std::string_view path) const = 0;
    std::string name;
    std::string descriptionLine1;
    std::string descriptionLine2;
    std::string key;
};
}  // namespace net::minecraft::client::resource::pack
