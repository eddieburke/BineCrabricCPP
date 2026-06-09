#pragma once

#include "net/minecraft/nbt/NbtElement.hpp"

#include <vector>

namespace net::minecraft {

class NbtByteArray : public NbtElement {
public:
    std::vector<std::uint8_t> value;

    NbtByteArray() = default;
    explicit NbtByteArray(std::vector<std::uint8_t> value) : value(std::move(value)) {}

    [[nodiscard]] std::uint8_t getType() const override { return 7; }
    void write(std::ostream& output) const override;
    void read(std::istream& input) override;
    [[nodiscard]] Nbt toStorage() const override { return Nbt(value); }
};

} // namespace net::minecraft
