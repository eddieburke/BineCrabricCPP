#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace net::minecraft::client::render::shaderpack {
struct DecodedTexture {
 int width = 0;
 int height = 0;
 std::vector<std::uint8_t> rgba;
};
DecodedTexture decodeTexture(const std::string& bytes);
} // namespace net::minecraft::client::render::shaderpack
