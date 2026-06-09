#pragma once

#include "net/minecraft/nbt/NbtElement.hpp"

namespace net::minecraft {

class NbtByte : public NbtElement {
public:
    std::int8_t value = 0;

    NbtByte() = default;
    explicit NbtByte(std::int8_t value) : value(value) {}

    [[nodiscard]] std::uint8_t getType() const override { return 1; }

    void write(std::ostream& output) const override;
    void read(std::istream& input) override;

    [[nodiscard]] Nbt toStorage() const override { return Nbt(value); }
};

} // namespace net::minecraft
