#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace net::minecraft {
class Nbt;
namespace nbt::detail {
[[nodiscard]] Nbt readNamedTag(const std::vector<std::uint8_t>& data, std::size_t& pos);
[[nodiscard]] Nbt readPayload(std::uint8_t type, const std::vector<std::uint8_t>& data, std::size_t& pos);
void writeNamedTag(std::vector<std::uint8_t>& out, const std::string& name, const Nbt& tag);
void writePayload(std::vector<std::uint8_t>& out, const Nbt& tag);
} // namespace nbt::detail
} // namespace net::minecraft
